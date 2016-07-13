/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <pthread.h>
#include <signal.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <browser-provider.h>
#include <common-adaptor.h>
#include <browser-provider-socket.h>

///////////// LOCAL APIs //////////////////

static int __adaptor_create_socket()
{
	int sockfd = -1;

	struct sockaddr_un clientaddr;
	if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		return -1;

	struct timeval tv_timeo = { 3, 500000 }; //3.5 second
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv_timeo,
			sizeof( tv_timeo ) ) < 0) {
		TRACE_STRERROR("[CRITICAL] setsockopt SO_RCVTIMEO");
		close(sockfd);
		return -1;
	}

	bzero(&clientaddr, sizeof(clientaddr));
	clientaddr.sun_family = AF_UNIX;
	memset(clientaddr.sun_path, 0x00, sizeof(clientaddr.sun_path));
	strncpy(clientaddr.sun_path, IPC_SOCKET, strlen(IPC_SOCKET));
	clientaddr.sun_path[strlen(IPC_SOCKET)] = '\0';
	if (connect(sockfd,
		(struct sockaddr *)&clientaddr, sizeof(clientaddr)) < 0) {
		close(sockfd);
		return -1;
	}
	TRACE_DEBUG("sockfd [%d]", sockfd);
	return sockfd;
}

///////////// COMMON APIs //////////////////

int bp_common_precheck_string(const char *str)
{
	if (str != NULL) {
		int length = strlen(str);
		if (length < 1)
			TRACE_ERROR("not null, but length is 0");
		else if(length > BP_MAX_STR_LEN)
			TRACE_ERROR("length is more long than %d", BP_MAX_STR_LEN);
		else
			return length;
	}
	return -1;
}

void bp_common_print_errorcode(const char *funcname, const int line,
	const long long int id, const bp_error_defs errorcode)
{
	switch (errorcode) {
	case BP_ERROR_IO_ERROR:
		TRACE_ERROR("%s(%d):%d response:IO_ERROR(%d)", funcname, id, line, errorcode);
		break;
	case BP_ERROR_IO_EAGAIN:
		TRACE_ERROR("%s(%d):%d response:IO_EAGAIN(%d)", funcname, id, line, errorcode);
		break;
	case BP_ERROR_IO_EINTR:
		TRACE_ERROR("%s(%d):%d response:IO_EINTR(%d)", funcname, id, line, errorcode);
		break;
	case BP_ERROR_INVALID_PARAMETER:
		TRACE_ERROR("%s(%d):%d response:INVALID_PARAMETER(%d)", funcname, id, line, errorcode);
		break;
	case BP_ERROR_OUT_OF_MEMORY:
		TRACE_ERROR("%s(%d):%d response:OUT_OF_MEMORY(%d)", funcname, id, line, errorcode);
		break;
	case BP_ERROR_NO_DATA:
		//TRACE_DEBUG("%s(%d):%d response:NO_DATA(%d)", funcname, id, line, errorcode);
		break;
	case BP_ERROR_ID_NOT_FOUND:
		TRACE_ERROR("%s(%d):%d response:ID_NOT_FOUND(%d)", funcname, id, line, errorcode);
		break;
	case BP_ERROR_DUPLICATED_ID:
		TRACE_ERROR("%s(%d):%d response:DUPLICATED_ID(%d)", funcname, id, line, errorcode);
		break;
	case BP_ERROR_PERMISSION_DENY:
		TRACE_ERROR("%s(%d):%d response:PERMISSION_DENY(%d)", funcname, id, line, errorcode);
		break;
	case BP_ERROR_DISK_BUSY:
		TRACE_ERROR("%s(%d):%d response:DISK_BUSY(%d)", funcname, id, line, errorcode);
		break;
	case BP_ERROR_DISK_FULL:
		TRACE_ERROR("%s(%d):%d response:DISK_FULL(%d)", funcname, id, line, errorcode);
		break;
	case BP_ERROR_TOO_BIG_DATA:
		TRACE_ERROR("%s(%d):%d response:TOO_BIG_DATA(%d)", funcname, id, line, errorcode);
		break;
	case BP_ERROR_NONE:
		TRACE_DEBUG("%s(%d):%d response:ERROR_NONE(%d)", funcname, id, line, errorcode);
		break;
	default:
		TRACE_ERROR("%s(%d):%d response:UNKNOWN(%d)", funcname, id, line, errorcode);
		break;
	}
}

void bp_common_adaptor_close_all(bp_adaptor_defs *adaptorinfo)
{
	if (adaptorinfo != NULL) {
		close(adaptorinfo->cmd_socket);
		adaptorinfo->cmd_socket= -1;
		if (adaptorinfo->notify > 0) {
			close(adaptorinfo->notify);
			adaptorinfo->notify= -1;
		}
	}
	bp_shm_free(&adaptorinfo->shm);
}

int bp_common_adaptor_connect_to_provider(bp_adaptor_defs **adaptorinfo,
	bp_client_type_defs client_type)
{
	bp_adaptor_defs *ipcinfo = NULL;

	ipcinfo = (bp_adaptor_defs *)calloc(1, sizeof(bp_adaptor_defs));
	if (ipcinfo != NULL) {
		int connect_retry = 10;
		ipcinfo->cmd_socket = -1;
		while(ipcinfo->cmd_socket < 0 && connect_retry-- > 0) {
			ipcinfo->cmd_socket = __adaptor_create_socket();
		}
		if (ipcinfo->cmd_socket < 0) {
			TRACE_STRERROR("[CRITICAL] connect system error");
			free(ipcinfo);
			ipcinfo = NULL;
			return -1;
		}
		// send a command
		long long int cid = -1;
		bp_error_defs errorcode = BP_ERROR_NONE;
		bp_command_defs cmd = BP_CMD_INITIALIZE;
		if (bp_ipc_send_custom_type(ipcinfo->cmd_socket, &cmd,
				sizeof(bp_command_defs)) < 0 ||
				bp_ipc_send_custom_type(ipcinfo->cmd_socket, &client_type,
				sizeof(bp_client_type_defs)) < 0 ||
				(errorcode = bp_ipc_read_errorcode(ipcinfo->cmd_socket)) != BP_ERROR_NONE ||
				(cid = bp_adaptor_ipc_read_int64(ipcinfo->cmd_socket)) <= 0) {
			TRACE_ERROR("[CRITICAL] failed to connect with provider");
			close(ipcinfo->cmd_socket);
			free(ipcinfo);
			ipcinfo = NULL;
			usleep(50000);
			return -1;
		}
		TRACE_INFO("bp_common_adaptor_connect_to_provider cid:%lld", cid);
		ipcinfo->cid = cid;
		ipcinfo->shm.key = cid;
		ipcinfo->shm.local = NULL;
		ipcinfo->notify = -1;
	}
	*adaptorinfo = ipcinfo;
	return 0;
}

int bp_adaptor_ipc_send_int(int fd, int value)
{
	if (fd < 0) {
		TRACE_ERROR("[CHECK FD] [%d]", fd);
		return -1;
	}

	if (fd >= 0 && write(fd, &value, sizeof(int)) < 0) {
		if (errno == EPIPE) {
			TRACE_ERROR("[EPIPE] Broken Pipe errno [%d]", errno);
		} else if (errno == EAGAIN) {
			TRACE_ERROR("[EAGAIN] Resource temporarily unavailable errno [%d]", errno);
		} else {
			TRACE_ERROR("errno [%d]", errno);
		}
		return -1;
	}
	return 0;
}

int bp_adaptor_ipc_read_int(int fd)
{
	if (fd < 0) {
		TRACE_ERROR("[CHECK SOCKET]");
		return -1;
	}

	int value = -1;
	ssize_t recv_bytes = read(fd, &value, sizeof(int));
	if (recv_bytes < 0) {
		TRACE_STRERROR("[CRITICAL] read");
		return -1;
	}
	return value;
}

long long int bp_adaptor_ipc_read_int64(int fd)
{
	if (fd < 0) {
		TRACE_ERROR("[CHECK SOCKET]");
		return -1;
	}

	long long int value = -1;
	ssize_t recv_bytes = read(fd, &value, sizeof(long long int));
	TRACE_INFO("bp_adaptor_ipc_read_int64, size of long long: %d", sizeof(long long int));
	TRACE_INFO("bp_adaptor_ipc_read_int64, bytes: %d", recv_bytes);
	if (recv_bytes < 0) {
		TRACE_STRERROR("[CRITICAL] read");
		return -1;
	}
	return value;
}

int bp_common_adaptor_get_ids_p(const int sock, bp_command_fmt *cmd,
	int **ids, bp_error_defs *errorcode)
{
	int *idlist = NULL;

	*errorcode = BP_ERROR_NONE;
	if (sock < 0 || cmd == NULL || ids == NULL) {
		*errorcode = BP_ERROR_INVALID_PARAMETER;
		return -1;
	}

	*errorcode = bp_ipc_simple_response(sock, cmd);
	if (*errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(cmd->id, *errorcode);
		if (*errorcode == BP_ERROR_NO_DATA) // success
			return 0;
		return -1;
	}

	// int count.
	int ids_count = bp_adaptor_ipc_read_int(sock);
	TRACE_DEBUG("response ids count:%d", ids_count);
	if (ids_count > 0) {
		idlist = (int *)calloc(ids_count, sizeof(int));
		if (idlist == NULL) {
			TRACE_STRERROR("[CRITICAL] allocation");
			*errorcode = BP_ERROR_OUT_OF_MEMORY;
			BP_PRINT_ERROR(cmd->id, *errorcode);
			return -1;
		}
		// getting ids array from provider
		if (bp_ipc_read_blob(sock, idlist,
				(sizeof(int) * ids_count)) < 0) {
			free(idlist);
			*errorcode = bp_ipc_check_stderr(BP_ERROR_IO_ERROR);
			BP_PRINT_ERROR(cmd->id, *errorcode);
			return -1;
		}
		*ids = idlist;
	}
	return ids_count;
}

int bp_common_adaptor_get_string(const int sock, bp_command_fmt *cmd,
	char **value, bp_error_defs *errorcode)
{
	char *recv_str = NULL;

	*errorcode = BP_ERROR_NONE;
	if (sock < 0 || cmd == NULL || value == NULL) {
		*errorcode = BP_ERROR_INVALID_PARAMETER;
		return -1;
	}

	*errorcode = bp_ipc_simple_response(sock, cmd);
	if (*errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(cmd->id, *errorcode);
		if (*errorcode == BP_ERROR_NO_DATA) // success
			return 0;
		return -1;
	}

	// getting string from provider.
	recv_str = bp_ipc_read_string(sock);
	if (recv_str == NULL) {
		*errorcode = bp_ipc_check_stderr(BP_ERROR_IO_ERROR);
		BP_PRINT_ERROR(cmd->id, *errorcode);
		return -1;
	}
	*value = recv_str;
	return 0;
}

int bp_common_adaptor_get_int(const int sock, bp_command_fmt *cmd,
	int *value, bp_error_defs *errorcode)
{
	int recv_value = 0;

	*errorcode = BP_ERROR_NONE;
	if (sock < 0 || cmd == NULL || value == NULL) {
		*errorcode = BP_ERROR_INVALID_PARAMETER;
		return -1;
	}

	*errorcode = bp_ipc_simple_response(sock, cmd);
	if (*errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(cmd->id, *errorcode);
		if (*errorcode == BP_ERROR_NO_DATA) // success
			return 0;
		return -1;
	}

	// getting int from provider.
	recv_value = bp_adaptor_ipc_read_int(sock);
	if (recv_value < 0) {
		*errorcode = bp_ipc_check_stderr(BP_ERROR_IO_ERROR);
		BP_PRINT_ERROR(cmd->id, *errorcode);
		return -1;
	}
	*value = recv_value;
	return 0;
}

int bp_common_adaptor_set_string(const int sock, bp_command_fmt *cmd,
	const char *value, bp_error_defs *errorcode)
{
	*errorcode = BP_ERROR_NONE;
	if (sock < 0 || cmd == NULL ||
			bp_common_precheck_string(value) < 0) {
		*errorcode = BP_ERROR_INVALID_PARAMETER;
		return -1;
	}

	*errorcode = bp_ipc_simple_response(sock, cmd);
	if (*errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(cmd->id, *errorcode);
		return -1;
	}

	if (bp_ipc_send_string(sock, value) < 0) {
		*errorcode = BP_ERROR_IO_ERROR;
		BP_PRINT_ERROR(cmd->id, *errorcode);
		return -1;
	}

	*errorcode = bp_ipc_read_errorcode(sock);
	if (*errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(cmd->id, *errorcode);
		return -1;
	}
	return 0;
}

int bp_common_adaptor_set_int(const int sock, bp_command_fmt *cmd,
	const int value, bp_error_defs *errorcode)
{
	*errorcode = BP_ERROR_NONE;
	if (sock < 0 || cmd == NULL) {
		*errorcode = BP_ERROR_INVALID_PARAMETER;
		return -1;
	}

	*errorcode = bp_ipc_simple_response(sock, cmd);
	if (*errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(cmd->id, *errorcode);
		return -1;
	}

	if (bp_adaptor_ipc_send_int(sock, value) < 0) {
		*errorcode = BP_ERROR_IO_ERROR;
		BP_PRINT_ERROR(cmd->id, *errorcode);
		return -1;
	}

	*errorcode = bp_ipc_read_errorcode(sock);
	if (*errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(cmd->id, *errorcode);
		return -1;
	}
	return 0;
}

int bp_common_adaptor_get_blob(const int sock, bp_command_fmt *cmd,
	unsigned char **value, int *length, bp_error_defs *errorcode)
{
	unsigned char *blob_data = NULL;
	int blob_length = 0;

	*errorcode = BP_ERROR_NONE;
	if (sock < 0 || cmd == NULL || value == NULL || length == NULL) {
		*errorcode = BP_ERROR_INVALID_PARAMETER;
		return -1;
	}

	*errorcode = bp_ipc_simple_response(sock, cmd);
	if (*errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(cmd->id, *errorcode);
		if (*errorcode == BP_ERROR_NO_DATA) // success
			return 0;
		return -1;
	}
	// getting length of blob
	if (bp_ipc_read_custom_type(sock, &blob_length, sizeof(int)) < 0) {
		*errorcode = bp_ipc_check_stderr(BP_ERROR_IO_ERROR);
		BP_PRINT_ERROR(cmd->id, *errorcode);
		return -1;
	}
	if (blob_length > 0) {
		blob_data =
			(unsigned char *)calloc(blob_length, sizeof(unsigned char));
		if (blob_data == NULL) {
			*errorcode = BP_ERROR_OUT_OF_MEMORY;
			BP_PRINT_ERROR(cmd->id, *errorcode);
			return -1;
		}
		// getting blob from provider.
		if (bp_ipc_read_blob(sock, blob_data,
				sizeof(unsigned char) * blob_length) < 0) {
			free(blob_data);
			*errorcode = bp_ipc_check_stderr(BP_ERROR_IO_ERROR);
			BP_PRINT_ERROR(cmd->id, *errorcode);
			return -1;
		}
		*value = blob_data;
	} else {
		blob_length = 0;
	}
	*length = blob_length;
	return 0;
}

int bp_common_adaptor_set_blob(const int sock, bp_command_fmt *cmd,
	const unsigned char *value, const int length,
	bp_error_defs *errorcode)
{
	*errorcode = BP_ERROR_NONE;
	if (sock < 0 || cmd == NULL) {
		*errorcode = BP_ERROR_INVALID_PARAMETER;
		return -1;
	}

	unsigned char *buffer = NULL;
	int byte_length = length;
	if (value == NULL)
		byte_length = 0;

	if (byte_length > 0) {
		// check buffer, this code will make crash by overflow
		buffer =
			(unsigned char *)calloc(byte_length, sizeof(unsigned char));
		char *check_buffer = memcpy(buffer, value, byte_length);
		if (check_buffer == NULL) {
			TRACE_ERROR("[CHECK][%d] buffer:%d", cmd->id, length);
			*errorcode = BP_ERROR_INVALID_PARAMETER;
			BP_PRINT_ERROR(cmd->id, *errorcode);
			free(buffer);
			return -1;
		}
	}

	*errorcode = bp_ipc_simple_response(sock, cmd);
	if (*errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(cmd->id, *errorcode);
		free(buffer);
		return -1;
	}

	if (bp_adaptor_ipc_send_int(sock, byte_length) < 0) {
		*errorcode = BP_ERROR_IO_ERROR;
		BP_PRINT_ERROR(cmd->id, *errorcode);
		free(buffer);
		return -1;
	}

	if (byte_length > 0) {
		if (bp_ipc_send_custom_type(sock, (void *)buffer,
				sizeof(unsigned char) * byte_length) < 0) {
			*errorcode = BP_ERROR_IO_ERROR;
			BP_PRINT_ERROR(cmd->id, *errorcode);
			free(buffer);
			return -1;
		}
	} // else case mean deleting blob

	free(buffer);

	*errorcode = bp_ipc_read_errorcode(sock);
	if (*errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(cmd->id, *errorcode);
		return -1;
	}
	return 0;
}

// used in get_info API
int bp_common_adaptor_get_info_blob(int sock, unsigned char **value,
	bp_shm_defs *shm)
{
	int length = 0;
	if (bp_ipc_read_custom_type(sock, &length,
			sizeof(int)) == 0 && length > 0) {

		int trans_way = 0; // 0:socket 1:shm
		if (bp_ipc_read_custom_type(sock, &trans_way,
				sizeof(int)) < 0) {
			return -1;
		}
		if (trans_way == 0) {
			unsigned char *buffer = (unsigned char *)calloc(length,
					sizeof(unsigned char));
			if (buffer == NULL) {
				TRACE_ERROR("[ERROR] alloc");
				return -1;
			}
			if (bp_ipc_read_blob(sock, buffer,
					sizeof(unsigned char) * length) < 0) {
				TRACE_ERROR("[ERROR] read blob");
				free(buffer);
				return -1;
			}
			*value = buffer;
		} else {
			if (bp_shm_read_copy(shm, value, length) < 0) {
				return -1;
			}
		}
	}
	return length;
}

int bp_common_adaptor_get_blob_shm(const int sock,
	bp_command_fmt *cmd, int *width, int *height, unsigned char **value,
	int *length, bp_error_defs *errorcode, bp_shm_defs *shm)
{
	int blob_length = 0;
	int blob_width = 0;
	int blob_height = 0;

	*errorcode = BP_ERROR_NONE;
	if (sock < 0 || cmd == NULL || value == NULL || length == NULL) {
		*errorcode = BP_ERROR_INVALID_PARAMETER;
		return -1;
	}

	*errorcode = bp_ipc_simple_response(sock, cmd);
	if (*errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(cmd->id, *errorcode);
		if (*errorcode == BP_ERROR_NO_DATA) // success
			return 0;
		return -1;
	}
	blob_length = bp_adaptor_ipc_read_int(sock);
	if (blob_length < 0) {
		*errorcode = bp_ipc_check_stderr(BP_ERROR_IO_ERROR);
		BP_PRINT_ERROR(cmd->id, *errorcode);
		return -1;
	}

	if (blob_length > 0) {

		// read here what IPC should be used below from provider.
		int trans_way = 0; // 0:socket 1:shm
		trans_way = bp_adaptor_ipc_read_int(sock);
		if (trans_way < 0) {
			*errorcode = bp_ipc_check_stderr(BP_ERROR_IO_ERROR);
			BP_PRINT_ERROR(cmd->id, *errorcode);
			return -1;
		}
		if (trans_way == 0) {
			free(shm->local);
			shm->local = (unsigned char *)calloc(blob_length,
					sizeof(unsigned char));
			if (shm->local == NULL) {
				*errorcode = BP_ERROR_OUT_OF_MEMORY;
				BP_PRINT_ERROR(cmd->id, *errorcode);
				return -1;
			}
			if (bp_ipc_read_blob(sock, shm->local,
					sizeof(unsigned char) * blob_length) < 0) {
				free(shm->local);
				*errorcode = BP_ERROR_IO_ERROR;
				BP_PRINT_ERROR(cmd->id, *errorcode);
				return -1;
			}
			*value = shm->local;
		} else {
			if (bp_shm_is_ready(shm, blob_length) == 0)
				*value = shm->mem;
		}
	}

	if ((blob_width = bp_adaptor_ipc_read_int(sock)) < 0 ||
			(blob_height = bp_adaptor_ipc_read_int(sock)) < 0) {
		*errorcode = bp_ipc_check_stderr(BP_ERROR_IO_ERROR);
		BP_PRINT_ERROR(cmd->id, *errorcode);
		return -1;
	}
	*length = blob_length;
	*width = blob_width;
	*height = blob_height;
	return 0;
}

int bp_common_adaptor_set_blob_shm(const int sock,
	bp_command_fmt *cmd, const int width, const int height,
	const unsigned char *value, const int length,
	bp_error_defs *errorcode, bp_shm_defs *shm)
{
	*errorcode = BP_ERROR_NONE;
	if (sock < 0 || cmd == NULL) {
		*errorcode = BP_ERROR_INVALID_PARAMETER;
		return -1;
	}

	int byte_length = length;
	if (value == NULL)
		byte_length = 0;

	if (byte_length > 0) {
		// check buffer, this code will make crash by overflow
		unsigned char *buffer =
			(unsigned char *)calloc(byte_length, sizeof(unsigned char));
		char *check_buffer = memcpy(buffer, value, byte_length);
		if (check_buffer == NULL) {
			TRACE_ERROR("[CHECK][%d] buffer:%d", cmd->id, length);
			*errorcode = BP_ERROR_INVALID_PARAMETER;
			free(buffer);
			return -1;
		}
		free(buffer);
	}
	*errorcode = bp_ipc_simple_response(sock, cmd);
	if (*errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(cmd->id, *errorcode);
		return -1;
	}

	if (bp_adaptor_ipc_send_int(sock, byte_length) < 0) {
		*errorcode = BP_ERROR_IO_ERROR;
		BP_PRINT_ERROR(cmd->id, *errorcode);
		return -1;
	}

	if (byte_length > 0) {
		int trans_way = 0; // 0:socket 1:shm
		// send here what IPC should be used below from provider.
		if (bp_shm_is_ready(shm, byte_length) == 0)
			trans_way = 1;
		// send .. the way of IPC
		if (bp_adaptor_ipc_send_int(sock, trans_way) < 0) {
			*errorcode = BP_ERROR_IO_ERROR;
			BP_PRINT_ERROR(cmd->id, *errorcode);
			return -1;
		}

		if (trans_way == 0) {
			if (bp_ipc_send_custom_type(sock, (void *)value,
					sizeof(unsigned char) * byte_length) < 0) {
				*errorcode = BP_ERROR_IO_ERROR;
				BP_PRINT_ERROR(cmd->id, *errorcode);
				return -1;
			}
		} else {
			if (bp_shm_write(shm, (unsigned char *)value,
					byte_length) < 0) {
				*errorcode = BP_ERROR_OUT_OF_MEMORY;
				BP_PRINT_ERROR(cmd->id, *errorcode);
				return -1;
			}
		}
		if (bp_adaptor_ipc_send_int(sock, width) < 0 ||
				bp_adaptor_ipc_send_int(sock, height) < 0) {
			*errorcode = BP_ERROR_IO_ERROR;
			BP_PRINT_ERROR(cmd->id, *errorcode);
			return -1;
		}
	} // else case mean deleting blob

	*errorcode = bp_ipc_read_errorcode(sock);
	if (*errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(cmd->id, *errorcode);
		return -1;
	}
	return 0;
}

#ifdef SUPPORT_CLOUD_SYSTEM
#include <sys/types.h>
#include <unistd.h>

#define BP_BINSH_NAME "/bin/sh"
#define BP_BINSH_SIZE 7
static char *__get_cmdline()
{
	int fd;
	int ret = -1;
	const int buf_size = 256;
	char proc_path[buf_size];
	char *pkgname = NULL;

	ret = snprintf(proc_path, buf_size, "/proc/%d/cmdline", getpid());
	if (ret < 0) {
		TRACE_STRERROR("failed to make cmdline path");
		return NULL;
	}

	fd = open(proc_path, O_RDONLY, 0600);
	if (fd < 0) {
		TRACE_STRERROR("open cmdline:%s", proc_path);
		return NULL;
	}
	ret = read(fd, proc_path, buf_size - 1);
	close(fd);
	if (ret <= 0) {
		TRACE_STRERROR("read cmdline:%s", proc_path);
		return NULL;
	}
	proc_path[ret] = 0;
	if (strncmp(proc_path, BP_BINSH_NAME, BP_BINSH_SIZE) == 0)
		pkgname = strdup(&proc_path[BP_BINSH_SIZE + 1]);
	else
		pkgname = strdup(proc_path);
	return pkgname;
}

int bp_common_adaptor_is_sync_adaptor()
{
	int ret = -1;
	char *pkgname = NULL;
	pkgname = __get_cmdline();

	if (pkgname == NULL) {
		TRACE_ERROR("no process info, it's adaptor");
		ret = 1;
	} else {
		int pkgname_len = strlen(pkgname);
#ifdef CLOUD_PDM_SERVER
		if (pkgname_len == strlen(CLOUD_PDM_SERVER) &&
				strncmp(pkgname, CLOUD_PDM_SERVER, pkgname_len) == 0) {
			ret = 0;
			TRACE_SECURE_DEBUG("cloud sync adaptor:%s", pkgname);
		}
#endif
		if (ret < 0) {
			TRACE_SECURE_DEBUG("just adaptor process:%s", pkgname);
			ret = 1;
		}
		free(pkgname);
	}
	return ret;
}

#endif

// disconnect
int bp_common_adaptor_disconnect(bp_adaptor_defs **info, pthread_t *tid)
{
	if (tid != NULL && (*tid > 0 && pthread_kill(*tid, 0) != ESRCH)) {
		if (pthread_cancel(*tid) != 0) {
			TRACE_STRERROR("pthread:%0x", (int)*tid);
		}
		*tid = 0;
	}
	bp_common_adaptor_close_all(*info);
	free(*info);
	*info = NULL;
	return 0;
}

// clear read buffer. call in head of API before calling IPC_SEND
int bp_common_adaptor_clear_read_buffer(int sock, size_t length)
{
	long i;
	char tmp_char;

	TRACE_DEBUG("[CLEAN] garbage packet[%ld]", length);
	for (i = 0; i < length; i++) {
		if (read(sock, &tmp_char, sizeof(char)) < 0) {
			TRACE_STRERROR("[CHECK] read");
			return -1;
		}
	}
	return 0;
}

// callback for client
int bp_common_adaptor_event_manager(bp_adaptor_defs *adaptorinfo,
	bp_common_adaptor_data_changed_cb *notify_cb, void **data)
{
	if (adaptorinfo == NULL) {
		TRACE_ERROR("check adaptor info");
		return -1;
	}

	size_t path_size = sizeof(NOTIFY_DIR) + 11;
	char notify_fifo[path_size];
	snprintf((char *)&notify_fifo, path_size, "%s/%d", NOTIFY_DIR, adaptorinfo->cid);
	TRACE_DEBUG("IPC ESTABILISH %s", notify_fifo);
	adaptorinfo->notify = open(notify_fifo, O_RDONLY, 0600);
	if (adaptorinfo->notify < 0) {
		TRACE_STRERROR("failed to ESTABILISH IPC %s", notify_fifo);
		return -1;
	}

	int sock = adaptorinfo->notify;

	// deferred wait to cencal until next function called.
	// ex) function : select, read in this thread
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
	TRACE_DEBUG("Noti Listening [%d]", sock);

	while(adaptorinfo != NULL && adaptorinfo->notify >= 0) {

		// blocking fifo.
		bp_command_defs provider_cmd = BP_CMD_NONE;
		TRACE_INFO(" at %s, line %d.", __FILE__, __LINE__);
		if (bp_ipc_recv(sock, &provider_cmd, sizeof(bp_command_defs),
				__FUNCTION__) <= 0 ||
					provider_cmd != BP_CMD_COMMON_NOTI) {
			TRACE_ERROR("closed by provider :%d", provider_cmd);
			return 1;
		}

		// begin protect callback sections
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		if (adaptorinfo != NULL && notify_cb != NULL && *notify_cb != NULL) {
			TRACE_DEBUG("notify callback:%p/%p", notify_cb, *notify_cb);
			// progress event
			if (data == NULL)
				(*notify_cb)(NULL);
			else
				(*notify_cb)(*data);
		}
		// end protect callback sections
		pthread_setcancelstate (PTHREAD_CANCEL_ENABLE,  NULL);

	} // while

	TRACE_DEBUG("callback thread is end by deinit");
	return 0;
}
