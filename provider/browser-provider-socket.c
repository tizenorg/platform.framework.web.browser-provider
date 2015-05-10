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

#include <pthread.h>

#include <time.h>
#include <sys/time.h>

#include <sys/socket.h>

#include <signal.h>

#include "browser-provider.h"
#include "browser-provider-log.h"
#include "browser-provider-socket.h"

ssize_t bp_ipc_recv(int sock, void *value, size_t type_size,
	const char *func)
{
	bp_error_defs errorcode = BP_ERROR_NONE;
	ssize_t recv_bytes = 0;

	if (sock < 0) {
		TRACE_ERROR("[ERROR] %s check sock:%d", func, sock);
		return -1;
	}
	if (value == NULL) {
		TRACE_ERROR("[ERROR] %s check buffer sock:%d", func, sock);
		return -1;
	}

	int tryagain = 3;
	do {
		errorcode = BP_ERROR_NONE;
		recv_bytes = read(sock, value, type_size);
		if (recv_bytes < 0) {
			TRACE_ERROR("[IPC.Read] %s exception sock:%d", func, sock);
			errorcode = bp_ipc_check_stderr(BP_ERROR_IO_ERROR);
		} else if (recv_bytes == 0) {
			TRACE_ERROR("[ERROR] %s closed peer sock:%d", func, sock);
			errorcode = BP_ERROR_IO_ERROR;
		}
	} while (sock >= 0 && (errorcode == BP_ERROR_IO_EAGAIN ||
		errorcode == BP_ERROR_IO_EINTR) && (--tryagain > 0));
	return recv_bytes;
}

//////////////////////////////////////////////////////////////////////////
/// @brief write the error to socket
/// @return if success, return 0
int bp_ipc_send_errorcode(int fd, bp_error_defs errorcode)
{
	if (fd < 0) {
		TRACE_ERROR("[ERROR] check sock:%d", fd);
		return -1;
	}

	if (fd < 0 || write(fd, &errorcode, sizeof(bp_error_defs)) <= 0) {
		TRACE_STRERROR("[IPC.Write] exception sock:%d", fd);
		return -1;
	}
	return 0;
}

bp_error_defs bp_ipc_read_errorcode(int fd)
{
	bp_error_defs errorcode = BP_ERROR_NONE;
	if (bp_ipc_recv(fd, &errorcode, sizeof(bp_error_defs),
			__FUNCTION__) <= 0)
		errorcode = BP_ERROR_IO_ERROR;
	return errorcode;
}

// send command and wait return value.
bp_error_defs bp_ipc_simple_response(int fd, bp_command_fmt *cmd)
{
	if (fd < 0) {
		TRACE_ERROR("[ERROR] check sock:%d", fd);
		return BP_ERROR_INVALID_PARAMETER;
	}
	if (cmd->cmd <= BP_CMD_NONE) {
		TRACE_ERROR("[ERROR] check command(%d) sock:%d", cmd, fd);
		return BP_ERROR_INVALID_PARAMETER;
	}
	if (bp_ipc_send_command(fd, cmd) != 0)
		return BP_ERROR_IO_ERROR;
	// return from provider.
	return bp_ipc_read_errorcode(fd);
}

int bp_ipc_send_command(int fd, bp_command_fmt *cmd)
{
	if (fd < 0) {
		TRACE_ERROR("[ERROR] check sock:%d", fd);
		return -1;
	}

	if (fd < 0 || write(fd, cmd, sizeof(bp_command_fmt)) < 0) {
		TRACE_STRERROR("[IPC.Write] exception sock:%d", fd);
		return -1;
	}
	return 0;
}

char *bp_ipc_read_string(int fd)
{
	//ssize_t : signed int size_t : unsigned int
	unsigned length = 0;
	ssize_t recv_size = 0;
	unsigned remain_size = 0;
	size_t buffer_size = 0;
	char *str = NULL;

	if (fd < 0) {
		TRACE_ERROR("[ERROR] check sock:%d", fd);
		return NULL;
	}
	if (bp_ipc_recv(fd, &length, sizeof(unsigned), __FUNCTION__) <= 0) {
		TRACE_ERROR("[ERROR] read length:%d sock:%d", length, fd);
		return NULL;
	}
	if (length < 1 || length > BP_MAX_STR_LEN) {
		TRACE_ERROR("[ERROR] check length:%d sock:%d", length, fd);
		return NULL;
	}
	str = (char *)calloc((length + 1), sizeof(char));
	if (str == NULL) {
		TRACE_STRERROR("[ERROR] calloc length:%d sock:%d", length, fd);
		return NULL;
	}
	remain_size = length;
	do {
		if (remain_size > BP_DEFAULT_BUFFER_SIZE)
			buffer_size = BP_DEFAULT_BUFFER_SIZE;
		else
			buffer_size = remain_size;
		recv_size = bp_ipc_recv(fd, str + (int)(length - remain_size),
				buffer_size * sizeof(char), __FUNCTION__);
		if (recv_size > 0) {
			if (recv_size <= BP_DEFAULT_BUFFER_SIZE)
				remain_size = remain_size - (unsigned)recv_size;
			else
				recv_size = -1;
		}
	} while (fd >= 0 && recv_size > 0 && remain_size > 0);

	if (recv_size == 0) {
		TRACE_ERROR("[ERROR] closed peer sock:%d", fd);
		free(str);
		return NULL;
	} else if (recv_size < 0) {
		TRACE_STRERROR("[IPC.Read] exception sock:%d", fd);
		free(str);
		return NULL;
	}
	str[length] = '\0';
	return str;
}

// keep the order/ unsigned , str
int bp_ipc_send_string(int fd, const char *str)
{
	unsigned length = 0;

	if (fd < 0) {
		TRACE_ERROR("[ERROR] check sock:%d", fd);
		return -1;
	}
	if (str == NULL) {
		TRACE_ERROR("[ERROR] check buffer sock:%d", fd);
		return -1;
	}

	length = strlen(str);
	if (length < 1 || length > BP_MAX_STR_LEN) {
		TRACE_ERROR("[ERROR] check length:%d sock:%d", length, fd);
		return -1;
	}
	if (fd < 0 || write(fd, &length, sizeof(unsigned)) <= 0 ||
			write(fd, str, length * sizeof(char)) <= 0) {
		TRACE_STRERROR("[IPC.Write] exception sock:%d", fd);
		return -1;
	}
	return 0;
}

int bp_ipc_send_custom_type(int fd, void *value, size_t type_size)
{
	if (fd < 0) {
		TRACE_ERROR("[ERROR] check sock:%d", fd);
		return -1;
	}
	if (value == NULL) {
		TRACE_ERROR("[ERROR] check buffer sock:%d", fd);
		return -1;
	}
	if (fd < 0 || write(fd, value, type_size) <= 0) {
		TRACE_STRERROR("[IPC.Write] exception sock:%d", fd);
		return -1;
	}
	return 0;
}

int bp_ipc_read_custom_type(int fd, void *value, size_t type_size)
{
	ssize_t ret = bp_ipc_recv(fd, value, type_size, __FUNCTION__);
	if (ret <= 0)
		return -1;
	return 0;
}

int bp_ipc_read_blob(int fd, void *value, size_t type_size)
{
	ssize_t recv_size = 0;
	size_t remain_size = 0;
	size_t buffer_size = 0;

	if (fd < 0) {
		TRACE_ERROR("[ERROR] check sock:%d", fd);
		return -1;
	}
	if (value == NULL) {
		TRACE_ERROR("[ERROR] check buffer sock:%d", fd);
		return -1;
	}
	if (type_size == 0) {
		TRACE_ERROR("[ERROR] check size:%d sock:%d", type_size, fd);
		return -1;
	}
	remain_size = type_size;
	do {
		if (remain_size > BP_DEFAULT_BUFFER_SIZE)
			buffer_size = BP_DEFAULT_BUFFER_SIZE;
		else
			buffer_size = remain_size;
		recv_size = bp_ipc_recv(fd, value + (type_size - remain_size),
				buffer_size, __FUNCTION__);
		if (recv_size > 0)
			remain_size = remain_size - recv_size;
	} while (recv_size > 0 && remain_size > 0);

	if (recv_size == 0) {
		TRACE_ERROR("[ERROR] closed peer sock:%d", fd);
		return -1;
	} else if (recv_size < 0) {
		TRACE_STRERROR("[IPC.Read] exception sock:%d", fd);
		return -1;
	}
	return 0;
}

int bp_socket_free(int sockfd)
{
	if (sockfd < 0)
		return -1;
	close(sockfd);
	return 0;
}

bp_error_defs bp_ipc_check_stderr(bp_error_defs basecode)
{
	bp_error_defs errorcode = basecode;
	if (errno == EPIPE) {
		TRACE_STRERROR("[EPIPE:%d] Broken Pipe", errno);
		errorcode = BP_ERROR_IO_ERROR;
	} else if (errno == EAGAIN) {
		TRACE_STRERROR("[EAGAIN:%d]", errno);
		errorcode = BP_ERROR_IO_EAGAIN;
	} else if (errno == EINTR) {
		TRACE_STRERROR("[EINTR:%d]", errno);
		errorcode = BP_ERROR_IO_EINTR;
	} else {
		TRACE_STRERROR("[errno:%d]", errno);
	}
	return errorcode;
}
