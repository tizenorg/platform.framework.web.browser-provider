/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd All Rights Reserved
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

#include <history-adaptor.h>
#include <common-adaptor.h>
#include <browser-provider.h>
#include <browser-provider-db-defs.h>
#include <browser-provider-socket.h>

static bp_adaptor_defs *g_adaptorinfo = NULL;
static pthread_mutex_t g_adaptor_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t g_adaptor_event_thread_pid = 0;
static bp_history_adaptor_data_changed_cb g_adaptor_noti_cb = NULL;
static void *g_adaptor_noti_user_data = NULL;
static bp_command_fmt g_bp_command;
static bp_error_defs errorcode = BP_ERROR_NONE;

static void *__history_adaptor_event_manager(void *arg)
{
	int status = bp_common_adaptor_event_manager(arg, NULL, NULL);

	pthread_mutex_lock(&g_adaptor_mutex);
	g_adaptor_event_thread_pid = 0; // set 0 to not call pthread_cancel
	if (status >= 0) {
		if (g_adaptorinfo != NULL) {
			bp_common_adaptor_close_all(g_adaptorinfo);
			free(g_adaptorinfo);
			g_adaptorinfo = NULL;
		}
	}
	pthread_mutex_unlock(&g_adaptor_mutex);

	// Todo : the codes for cloud

	TRACE_DEBUG("callback thread is end by deinit");
	return 0;
}

// disconnect
static int __browser_adaptor_disconnect(void)
{
	TRACE_DEBUG("pthread:%0x", (int)g_adaptor_event_thread_pid);
	return bp_common_adaptor_disconnect(&g_adaptorinfo,
		&g_adaptor_event_thread_pid);
}

static int __browser_adaptor_connect(int callback)
{
	if (g_adaptorinfo == NULL) {
		bp_client_type_defs client_type = BP_CLIENT_HISTORY;

#ifdef SUPPORT_CLOUD_SYSTEM
		if (bp_common_adaptor_is_sync_adaptor() == 0)
			client_type = BP_CLIENT_HISTORY_SYNC;
#endif

		if (bp_common_adaptor_connect_to_provider(&g_adaptorinfo,
				client_type) < 0) {
			TRACE_ERROR("[CHECK connection]");
			return -1;
		}
	}
	g_bp_command.cmd = BP_CMD_NONE;
	g_bp_command.id = -1;
	g_bp_command.cid = g_adaptorinfo->cid;
	if (callback == 1 && g_adaptor_event_thread_pid <= 0) {
		// create thread here ( getting event_socket )
		if (pthread_create(&g_adaptor_event_thread_pid, NULL,
				__history_adaptor_event_manager, g_adaptorinfo) != 0) {
			TRACE_STRERROR("[CRITICAL] pthread_create");
			return -1;
		}
		pthread_detach(g_adaptor_event_thread_pid);
		TRACE_DEBUG("pthread:%0x", (int)g_adaptor_event_thread_pid);
	}
	return 0;
}

static int __bp_history_adaptor_get_string(const int id,
	bp_command_defs cmd, char **value)
{
	if (id < 0 || value == NULL)
		return -1;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	g_bp_command.cmd = cmd;
	g_bp_command.id = id;
	int ret = bp_common_adaptor_get_string(sock, &g_bp_command, value, &errorcode);
	if (ret < 0 && errorcode == BP_ERROR_IO_ERROR)
		__browser_adaptor_disconnect();
	pthread_mutex_unlock(&g_adaptor_mutex);
	return ret;
}

static int __bp_history_adaptor_get_int(const int id,
	bp_command_defs cmd, int *value)
{
	if (id < 0 || value == NULL)
		return -1;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	g_bp_command.cmd = cmd;
	g_bp_command.id = id;
	int ret = bp_common_adaptor_get_int(sock, &g_bp_command, value, &errorcode);
	if (ret < 0 && errorcode == BP_ERROR_IO_ERROR)
		__browser_adaptor_disconnect();
	pthread_mutex_unlock(&g_adaptor_mutex);
	return ret;
}

static int __bp_history_adaptor_set_string(const int id,
	bp_command_defs cmd, const char *value)
{
	if (id < 0 || value == NULL || bp_common_precheck_string(value) < 0)
		return -1;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	g_bp_command.cmd = cmd;
	g_bp_command.id = id;
	int ret = bp_common_adaptor_set_string(sock, &g_bp_command, value, &errorcode);
	if (ret < 0 && errorcode == BP_ERROR_IO_ERROR)
		__browser_adaptor_disconnect();
	pthread_mutex_unlock(&g_adaptor_mutex);
	return ret;
}

static int __bp_history_adaptor_set_int(const int id,
	bp_command_defs cmd, const int value)
{
	if (id < 0)
		return -1;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	g_bp_command.cmd = cmd;
	g_bp_command.id = id;
	int ret = bp_common_adaptor_set_int(sock, &g_bp_command, value, &errorcode);
	if (ret < 0 && errorcode == BP_ERROR_IO_ERROR)
		__browser_adaptor_disconnect();
	pthread_mutex_unlock(&g_adaptor_mutex);
	return ret;
}

static int __bp_history_adaptor_send_cmd(const int id, bp_command_defs cmd)
{
	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	errorcode = BP_ERROR_NONE;
	g_bp_command.cmd = cmd;
	g_bp_command.id = id;
	errorcode = bp_ipc_simple_response(sock, &g_bp_command);
	if (errorcode == BP_ERROR_IO_ERROR)
		__browser_adaptor_disconnect();
	pthread_mutex_unlock(&g_adaptor_mutex);
	if (errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(id, errorcode);
		return -1;
	}
	return 0;
}

static int __bp_history_adaptor_get_blob(const int id,
	bp_command_defs cmd, unsigned char **value, int *length)
{
	if (id < 0 || value == NULL || length == NULL)
		return -1;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	errorcode = BP_ERROR_NONE;
	g_bp_command.cmd = cmd;
	g_bp_command.id = id;
	int ret = bp_common_adaptor_get_blob(sock, &g_bp_command, value, length, &errorcode);
	if (ret < 0 && errorcode == BP_ERROR_IO_ERROR)
		__browser_adaptor_disconnect();
	pthread_mutex_unlock(&g_adaptor_mutex);
	return ret;
}

static int __bp_history_adaptor_get_blob_shm(const int id,
	bp_command_defs cmd, int *width, int *height, unsigned char **value,
	int *length)
{
	if (id < 0 || value == NULL || length == NULL)
		return -1;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	g_bp_command.cmd = cmd;
	g_bp_command.id = id;
	int ret = bp_common_adaptor_get_blob_shm(sock, &g_bp_command, width,
			height, value, length, &errorcode, &g_adaptorinfo->shm);
	if (ret < 0 && errorcode == BP_ERROR_IO_ERROR)
		__browser_adaptor_disconnect();
	pthread_mutex_unlock(&g_adaptor_mutex);
	return ret;
}

static int __bp_history_adaptor_set_blob_shm(const int id,
	bp_command_defs cmd, const int width, const int height,
	const unsigned char *value, const int length)
{
	if (id < 0)
		return -1;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	g_bp_command.cmd = cmd;
	g_bp_command.id = id;
	int ret = bp_common_adaptor_set_blob_shm(sock, &g_bp_command, width,
			height, value, length, &errorcode, &g_adaptorinfo->shm);
	if (ret < 0 && errorcode == BP_ERROR_IO_ERROR)
		__browser_adaptor_disconnect();
	pthread_mutex_unlock(&g_adaptor_mutex);
	return ret;
}

static int __bp_history_adaptor_send_one_way(const int id, bp_command_defs cmd)
{
	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	g_bp_command.cmd = cmd;
	g_bp_command.id = id;
	int ret = bp_ipc_send_command(sock, &g_bp_command);
	pthread_mutex_unlock(&g_adaptor_mutex);
	return ret;
}

/////////////////////// APIs /////////////////////////////////
// return
// 0 : Success
// -1 : Failed

// launch browser-provider, connect to browser-provider
int bp_history_adaptor_initialize(void)
{
	pthread_mutex_lock(&g_adaptor_mutex);
	int ret = __browser_adaptor_connect(1);
	pthread_mutex_unlock(&g_adaptor_mutex);
	return ret;
}

// disconnect
int bp_history_adaptor_deinitialize(void)
{
	pthread_mutex_lock(&g_adaptor_mutex);
	if (__browser_adaptor_connect(0) < 0) {
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}

	int sock = BP_CHECK_IPC_SOCK;
	g_bp_command.cmd = BP_CMD_DEINITIALIZE;
	g_bp_command.id = -1;
	bp_ipc_simple_response(sock, &g_bp_command);

	__browser_adaptor_disconnect();

	g_bp_command.cmd = BP_CMD_NONE;
	g_bp_command.id = -1;
	g_bp_command.cid = 0;
	pthread_mutex_unlock(&g_adaptor_mutex);
	return 0;
}

int bp_history_adaptor_create(int *id)
{
	int history_id = 0;

	if (id == NULL)
		return -1;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	errorcode = BP_ERROR_NONE;
	g_bp_command.cmd = BP_CMD_COMMON_CREATE;
	g_bp_command.id = *id;

	errorcode = bp_ipc_simple_response(sock, &g_bp_command);
	if (errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(*id, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	history_id = bp_adaptor_ipc_read_int(sock);
	if (history_id < 0) {
		errorcode = bp_ipc_check_stderr(BP_ERROR_IO_ERROR);
		BP_PRINT_ERROR(*id, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	if (*id == history_id) {
		TRACE_INFO("created a history:%d", *id);
	} else {
		*id = history_id;
		TRACE_INFO("created new history:%d", *id);
	}
	pthread_mutex_unlock(&g_adaptor_mutex);
	return 0;
}

int bp_history_adaptor_delete(const int id)
{
	if (id < 0)
		return -1;
	return __bp_history_adaptor_send_cmd(id, BP_CMD_COMMON_DELETE);
}

int bp_history_adaptor_get_errorcode(void)
{
	switch(errorcode) {
	case BP_ERROR_INVALID_PARAMETER:
		return BP_HISTORY_ERROR_INVALID_PARAMETER;
	case BP_ERROR_OUT_OF_MEMORY:
		return BP_HISTORY_ERROR_OUT_OF_MEMORY;
	case BP_ERROR_IO_EINTR:
	case BP_ERROR_IO_EAGAIN:
	case BP_ERROR_IO_ERROR:
		return BP_HISTORY_ERROR_IO_ERROR;
	case BP_ERROR_NO_DATA:
		return BP_HISTORY_ERROR_NO_DATA;
	case BP_ERROR_ID_NOT_FOUND:
		return BP_HISTORY_ERROR_ID_NOT_FOUND;
	case BP_ERROR_DUPLICATED_ID:
		return BP_HISTORY_ERROR_DUPLICATED_ID;
	case BP_ERROR_PERMISSION_DENY:
		return BP_HISTORY_ERROR_PERMISSION_DENY;
	case BP_ERROR_DISK_FULL:
		return BP_HISTORY_ERROR_DISK_FULL;
	case BP_ERROR_DISK_BUSY:
		return BP_HISTORY_ERROR_DISK_BUSY;
	case BP_ERROR_TOO_BIG_DATA:
		return BP_HISTORY_ERROR_TOO_BIG_DATA;
	case BP_ERROR_UNKNOWN:
		return BP_HISTORY_ERROR_UNKNOWN;
	default:
		break;
	}
	return BP_HISTORY_ERROR_NONE;
}

int bp_history_adaptor_get_frequency(const int id, int *value)
{
	return __bp_history_adaptor_get_int
		(id, BP_CMD_HISTORY_GET_FREQUENCY, value);
}

int bp_history_adaptor_get_url(const int id, char **value)
{
	return __bp_history_adaptor_get_string
		(id, BP_CMD_COMMON_GET_URL, value);
}

int bp_history_adaptor_get_title(const int id, char **value)
{
	return __bp_history_adaptor_get_string
		(id, BP_CMD_COMMON_GET_TITLE, value);
}

int bp_history_adaptor_get_icon(const int id, int *width,
	int *height, unsigned char **value, int *length)
{
	return __bp_history_adaptor_get_blob_shm
		(id, BP_CMD_COMMON_GET_ICON, width, height, value, length);
}

int bp_history_adaptor_get_snapshot(const int id, int *width,
	int *height, unsigned char **value, int *length)
{
	return __bp_history_adaptor_get_blob_shm
		(id, BP_CMD_COMMON_GET_SNAPSHOT, width, height, value, length);
}

int bp_history_adaptor_get_webicon(const int id, int *width,
	int *height, unsigned char **value, int *length)
{
	return __bp_history_adaptor_get_blob_shm
		(id, BP_CMD_COMMON_GET_WEBICON, width, height, value, length);
}

int bp_history_adaptor_get_date_created(const int id, int *value)
{
	return __bp_history_adaptor_get_int
		(id, BP_CMD_COMMON_GET_DATE_CREATED, value);
}

int bp_history_adaptor_get_date_modified(const int id, int *value)
{
	return __bp_history_adaptor_get_int
		(id, BP_CMD_COMMON_GET_DATE_MODIFIED, value);
}

int bp_history_adaptor_get_date_visited(const int id, int *value)
{
	return __bp_history_adaptor_get_int
		(id, BP_CMD_COMMON_GET_DATE_VISITED, value);
}

int bp_history_adaptor_set_frequency(const int id, const int value)
{
	return __bp_history_adaptor_set_int
		(id, BP_CMD_HISTORY_SET_FREQUENCY, value);
}

int bp_history_adaptor_set_url(const int id, const char *value)
{
	return __bp_history_adaptor_set_string
		(id, BP_CMD_COMMON_SET_URL, value);
}

int bp_history_adaptor_set_title(const int id, const char *value)
{
	return __bp_history_adaptor_set_string
		(id, BP_CMD_COMMON_SET_TITLE, value);
}

int bp_history_adaptor_set_icon(const int id, const int width,
	const int height, const unsigned char *value, const int length)
{
	return __bp_history_adaptor_set_blob_shm
		(id, BP_CMD_COMMON_SET_ICON, width, height, value, length);
}

int bp_history_adaptor_set_snapshot(const int id, const int width,
	const int height, const unsigned char *value, const int length)
{
	return __bp_history_adaptor_set_blob_shm
		(id, BP_CMD_COMMON_SET_SNAPSHOT, width, height, value, length);
}

int bp_history_adaptor_set_webicon(const int id, const int width,
	const int height, const unsigned char *value, const int length)
{
	return __bp_history_adaptor_set_blob_shm
		(id, BP_CMD_COMMON_SET_WEBICON, width, height, value, length);
}

int bp_history_adaptor_set_date_created(const int id, const int value)
{
	return __bp_history_adaptor_set_int
		(id, BP_CMD_COMMON_SET_DATE_CREATED, value);
}

int bp_history_adaptor_set_date_modified(const int id, const int value)
{
	return __bp_history_adaptor_set_int
		(id, BP_CMD_COMMON_SET_DATE_MODIFIED, value);
}

int bp_history_adaptor_set_date_visited(const int id, const int value)
{
	return __bp_history_adaptor_set_int
		(id, BP_CMD_COMMON_SET_DATE_VISITED, value);
}

int bp_history_adaptor_visit(const int id)
{
	if (id < 0)
		return -1;
	return __bp_history_adaptor_send_cmd(id, BP_CMD_HISTORY_SET_VISIT);
}

int bp_history_adaptor_limit_size
	(const int size, const bp_history_offset order_column_offset, const int ordering)
{
	if (size < 0)
		return -1;
	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	errorcode = BP_ERROR_NONE;
	g_bp_command.cmd = BP_CMD_HISTORY_SET_LIMIT_SIZE;
	g_bp_command.id = -1;

	bp_db_base_conds_fmt conds;
	memset(&conds, 0x00, sizeof(bp_db_base_conds_fmt));
	conds.limit = -1;
	conds.offset = -1;
	conds.order_column_offset = order_column_offset;
	conds.ordering = ordering;

	errorcode = bp_ipc_simple_response(sock, &g_bp_command);
	if (errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(-1, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	if (bp_ipc_send_custom_type(sock, &conds,
			sizeof(bp_db_base_conds_fmt)) < 0 ||
			bp_adaptor_ipc_send_int(sock, size) < 0) {
		errorcode = BP_ERROR_IO_ERROR;
		BP_PRINT_ERROR(-1, errorcode);
		__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	// wait id from provider.
	errorcode = bp_ipc_read_errorcode(sock);
	if (errorcode == BP_ERROR_IO_ERROR)
		__browser_adaptor_disconnect();
	pthread_mutex_unlock(&g_adaptor_mutex);
	if (errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(-1, errorcode);
		return -1;
	}
	return 0;
}

int bp_history_adaptor_easy_create(int *id, bp_history_info_fmt *info)
{
	if (id == NULL || info == NULL)
		return -1;

	if (*id <= 0) { // new history
		if (bp_history_adaptor_create(id) < 0) {
			TRACE_ERROR("[failed to create new history]");
			return -1;
		}
		if (*id <= 0) {
			TRACE_ERROR("[failed to create new history]");
			return -1;
		}
	}

	if (info->date_created != 0)
		bp_history_adaptor_set_date_created(*id, info->date_created);
	if (info->date_modified != 0)
		bp_history_adaptor_set_date_modified(*id, info->date_modified);
	if (info->date_visited != 0)
		bp_history_adaptor_set_date_visited(*id, info->date_visited);
	if (info->frequency >= 0)
		bp_history_adaptor_set_frequency(*id, info->frequency);
	if (info->url != NULL)
		bp_history_adaptor_set_url(*id, info->url);
	if (info->title != NULL)
		bp_history_adaptor_set_title(*id, info->title);

	if (info->favicon_length > 0 && info->favicon != NULL) {
		bp_history_adaptor_set_icon(*id, info->favicon_width,
			info->favicon_height, info->favicon, info->favicon_length);
	}
	if (info->thumbnail_length > 0 && info->thumbnail != NULL) {
		bp_history_adaptor_set_snapshot(*id, info->thumbnail_width,
			info->thumbnail_height, info->thumbnail, info->thumbnail_length);
	}
	if (info->webicon_length > 0 && info->webicon != NULL) {
		bp_history_adaptor_set_webicon(*id, info->webicon_width,
			info->webicon_height, info->webicon, info->webicon_length);
	}

	return 0;
}

int bp_history_adaptor_get_info(const int id,
	const bp_history_offset offset, bp_history_info_fmt *info)
{
	if (id < 0 || offset <= 0 || info == NULL)
		return -1;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	g_bp_command.cmd = BP_CMD_COMMON_GET_INFO_OFFSET;
	g_bp_command.id = id;

	// send command without waiting return value
	errorcode = bp_ipc_simple_response(sock, &g_bp_command);
	if (errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(id, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	// 1. oflags
	unsigned int oflags = offset;
	if (bp_ipc_send_custom_type(sock, &oflags,
			sizeof(bp_history_offset)) < 0) {
		errorcode = BP_ERROR_IO_ERROR;
		BP_PRINT_ERROR(id, errorcode);
		__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	// 2. wait id from provider.
	errorcode = bp_ipc_read_errorcode(sock);
	if (errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(id, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	bp_history_info_fmt history;
	memset(&history, 0x00, sizeof(bp_history_info_fmt));
	// getting bp_tab_base_fmt from provider.
	if (bp_ipc_read_custom_type(sock, &history,
			sizeof(bp_history_info_fmt)) < 0) {
		TRACE_ERROR("[CHECK IO] (%d)", id);
		errorcode = bp_ipc_check_stderr(BP_ERROR_IO_ERROR);
		BP_PRINT_ERROR(id, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	memset(info, 0x00, sizeof(bp_history_info_fmt));

	// fill info
	if (offset & BP_HISTORY_O_FREQUENCY) {
		info->frequency = history.frequency;
	}
	if (offset & BP_HISTORY_O_DATE_CREATED) {
		info->date_created = history.date_created;
	}
	if (offset & BP_HISTORY_O_DATE_MODIFIED) {
		info->date_modified = history.date_modified;
	}
	if (offset & BP_HISTORY_O_DATE_VISITED) {
		info->date_visited = history.date_visited;
	}
	// get strings . keep the order with provider
	if (offset & BP_HISTORY_O_URL) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->url =  bp_ipc_read_string(sock);
	}
	if (offset & BP_HISTORY_O_TITLE) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->title =  bp_ipc_read_string(sock);
	}
	if (offset & BP_HISTORY_O_ICON) {
		info->favicon_length = 0;
		info->favicon_width = history.favicon_width;
		info->favicon_height = history.favicon_height;
		bp_error_defs ret = bp_ipc_read_errorcode(sock);
		if (ret == BP_ERROR_NONE) {
			unsigned char *blob_data = NULL;
			if ((info->favicon_length =
					bp_common_adaptor_get_info_blob(sock, &blob_data,
						&g_adaptorinfo->shm)) > 0 && blob_data != NULL)
				info->favicon = blob_data;
		}
	}
	if (offset & BP_HISTORY_O_SNAPSHOT) {
		info->thumbnail_length = 0;
		info->thumbnail_width = history.thumbnail_width;
		info->thumbnail_height = history.thumbnail_height;
		bp_error_defs ret = bp_ipc_read_errorcode(sock);
		if (ret == BP_ERROR_NONE) {
			unsigned char *blob_data = NULL;
			if ((info->thumbnail_length =
					bp_common_adaptor_get_info_blob(sock, &blob_data,
						&g_adaptorinfo->shm)) > 0 && blob_data != NULL)
				info->thumbnail = blob_data;
		}
	}
	if (offset & BP_HISTORY_O_WEBICON) {
		info->webicon_length = 0;
		info->webicon_width = history.webicon_width;
		info->webicon_height = history.webicon_height;
		bp_error_defs ret = bp_ipc_read_errorcode(sock);
		if (ret == BP_ERROR_NONE) {
			unsigned char *blob_data = NULL;
			if ((info->webicon_length =
					bp_common_adaptor_get_info_blob(sock, &blob_data,
						&g_adaptorinfo->shm)) > 0 && blob_data != NULL)
				info->webicon = blob_data;
		}
	}

	pthread_mutex_unlock(&g_adaptor_mutex);
	return 0;
}

int bp_history_adaptor_easy_free(bp_history_info_fmt *info)
{
	if (info != NULL) {
		free(info->url);
		free(info->title);
		free(info->favicon);
		free(info->thumbnail);
		free(info->webicon);
		free(info->sync);
		memset(info, 0x00, sizeof(bp_history_info_fmt));
	}
	return 0;
}

int bp_history_adaptor_get_date_count
	(int *count, const bp_history_offset date_column_offset,
	const bp_history_date_defs date_type)
{
	if (count == NULL)
		return -1;
	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	bp_command_fmt *cmd = &g_bp_command;

	cmd->cmd = BP_CMD_COMMON_GET_CONDS_DATE_COUNT;
	cmd->id = -1;

	errorcode = bp_ipc_simple_response(sock, cmd);
	if (errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(-1, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	bp_history_offset oflags = date_column_offset;
	if (bp_ipc_send_custom_type(sock, &oflags,
			sizeof(bp_history_offset)) < 0 ||
			bp_adaptor_ipc_send_int(sock, date_type) < 0) {
		errorcode = BP_ERROR_IO_ERROR;
		BP_PRINT_ERROR(-1, errorcode);
		__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	// wait id from provider.
	errorcode = bp_ipc_read_errorcode(sock);
	if (errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(-1, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		if (errorcode == BP_ERROR_NO_DATA) { // success
			*count = 0;
			return 0;
		}
		return -1;
	}
	int recv_int = bp_adaptor_ipc_read_int(sock);
	TRACE_DEBUG("response ids count:%d", recv_int);
	if (recv_int < 0) {
		errorcode = bp_ipc_check_stderr(BP_ERROR_IO_ERROR);
		BP_PRINT_ERROR(-1, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	pthread_mutex_unlock(&g_adaptor_mutex);
	*count = recv_int;
	return 0;
}

static int __bp_history_adaptor_get_cond_ids_p
	(bp_command_defs pcommand, int **ids, int *count,
	bp_history_rows_cond_fmt *conds,
	const bp_history_offset check_offset,
	const char *keyword, const int is_like)
{
	if (ids == NULL || count == NULL)
		return -1;
	bp_history_rows_cond_fmt t_conds;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	bp_command_fmt *cmd = &g_bp_command;

	cmd->cmd = pcommand;
	cmd->id = -1;

	memset(&t_conds, 0x00, sizeof(bp_history_rows_cond_fmt));

	if (conds != NULL)
		memcpy(&t_conds, conds, sizeof(bp_history_rows_cond_fmt));
	else
		t_conds.limit = -1;

	errorcode = bp_ipc_simple_response(sock, cmd);
	if (errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(-1, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	bp_history_offset oflags = check_offset;
	if (keyword == NULL)
		oflags = 0;
	if (bp_ipc_send_custom_type(sock, &t_conds,
			sizeof(bp_history_rows_cond_fmt)) < 0 ||
			bp_ipc_send_custom_type(sock, &oflags,
			sizeof(bp_history_offset)) < 0) {
		errorcode = BP_ERROR_IO_ERROR;
		BP_PRINT_ERROR(-1, errorcode);
		__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	if (oflags > 0) {
		if (bp_adaptor_ipc_send_int(sock, is_like) < 0 ||
				bp_ipc_send_string(sock, keyword) < 0) {
			errorcode = BP_ERROR_IO_ERROR;
			BP_PRINT_ERROR(-1, errorcode);
			__browser_adaptor_disconnect();
			pthread_mutex_unlock(&g_adaptor_mutex);
			return -1;
		}
	}
	// wait id from provider.
	errorcode = bp_ipc_read_errorcode(sock);
	if (errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(-1, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		if (errorcode == BP_ERROR_NO_DATA) { // success
			*count = 0;
			return 0;
		}
		return -1;
	}
	// int count.
	int ids_count = bp_adaptor_ipc_read_int(sock);
	TRACE_DEBUG("response ids count:%d", ids_count);
	if (ids_count > 0) {
		int *idlist = (int *)calloc(ids_count, sizeof(int));
		if (idlist == NULL) {
			errorcode = BP_ERROR_OUT_OF_MEMORY;
			BP_PRINT_ERROR(-1, errorcode);
			if (bp_common_adaptor_clear_read_buffer(sock,
					(sizeof(int) * ids_count)) < 0) {
				__browser_adaptor_disconnect();
			}
			pthread_mutex_unlock(&g_adaptor_mutex);
			return -1;
		}
		// getting ids array from provider
		if (bp_ipc_read_blob(sock, idlist,
				(sizeof(int) * ids_count)) < 0) {
			free(idlist);
			errorcode = bp_ipc_check_stderr(BP_ERROR_IO_ERROR);
			BP_PRINT_ERROR(-1, errorcode);
			if (errorcode == BP_ERROR_IO_ERROR)
				__browser_adaptor_disconnect();
			pthread_mutex_unlock(&g_adaptor_mutex);
			return -1;
		}
		*ids = idlist;
		*count = ids_count;
	} else {
		*ids = NULL;
		*count = 0;
	}
	pthread_mutex_unlock(&g_adaptor_mutex);
	return 0;
}

int bp_history_adaptor_get_timestamp_ids_p
	(int **ids, int *count,
	const bp_history_rows_fmt *limits,//if NULL, ignore
	const bp_history_timestamp_fmt times[], const int times_count,
	const bp_history_offset check_offset, //if zero, ignore keyword search
	const char *keyword, // if NULL, does not search
	const int is_like)
{
	if (ids == NULL || count == NULL)
		return -1;
	bp_history_rows_fmt t_limits;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	bp_command_fmt *cmd = &g_bp_command;

	cmd->cmd = BP_CMD_COMMON_GET_CONDS_TIMESTAMP_IDS;
	cmd->id = -1;

	memset(&t_limits, 0x00, sizeof(bp_history_rows_fmt));

	if (limits != NULL)
		memcpy(&t_limits, limits, sizeof(bp_history_rows_fmt));
	else
		t_limits.limit = -1;

	errorcode = bp_ipc_simple_response(sock, cmd);
	if (errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(-1, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}

	if (bp_ipc_send_custom_type(sock, &t_limits,
			sizeof(bp_history_rows_fmt)) < 0 ||
			bp_adaptor_ipc_send_int(sock, times_count) < 0) {
		errorcode = BP_ERROR_IO_ERROR;
		BP_PRINT_ERROR(-1, errorcode);
		__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	if (times_count > 0) {
		int i = 0;
		for (; i < times_count; i++) {
			bp_history_timestamp_fmt *timestamps =
				(bp_history_timestamp_fmt *)(times + i);
			if (bp_ipc_send_custom_type(sock, timestamps,
					sizeof(bp_history_timestamp_fmt)) < 0) {
				errorcode = BP_ERROR_IO_ERROR;
				BP_PRINT_ERROR(-1, errorcode);
				__browser_adaptor_disconnect();
				pthread_mutex_unlock(&g_adaptor_mutex);
				return -1;
			}
		}
	}

	unsigned int oflags = check_offset;
	if (keyword == NULL)
		oflags = 0;
	if (bp_ipc_send_custom_type(sock, &oflags,
			sizeof(unsigned int)) < 0) {
		errorcode = BP_ERROR_IO_ERROR;
		BP_PRINT_ERROR(-1, errorcode);
		__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	if (oflags > 0) {
		if (bp_adaptor_ipc_send_int(sock, is_like) < 0 ||
				bp_ipc_send_string(sock, keyword) < 0) {
			errorcode = BP_ERROR_IO_ERROR;
			BP_PRINT_ERROR(-1, errorcode);
			__browser_adaptor_disconnect();
			pthread_mutex_unlock(&g_adaptor_mutex);
			return -1;
		}
	}
	// wait id from provider.
	errorcode = bp_ipc_read_errorcode(sock);
	if (errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(-1, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		if (errorcode == BP_ERROR_NO_DATA) { // success
			*count = 0;
			return 0;
		}
		return -1;
	}
	// int count.
	int ids_count = bp_adaptor_ipc_read_int(sock);
	TRACE_DEBUG("response ids count:%d", ids_count);
	if (ids_count > 0) {
		int *idlist = (int *)calloc(ids_count, sizeof(int));
		if (idlist == NULL) {
			errorcode = BP_ERROR_OUT_OF_MEMORY;
			BP_PRINT_ERROR(-1, errorcode);
			if (bp_common_adaptor_clear_read_buffer(sock,
					(sizeof(int) * ids_count)) < 0) {
				__browser_adaptor_disconnect();
			}
			pthread_mutex_unlock(&g_adaptor_mutex);
			return -1;
		}
		// getting ids array from provider
		if (bp_ipc_read_blob(sock, idlist,
				(sizeof(int) * ids_count)) < 0) {
			free(idlist);
			errorcode = bp_ipc_check_stderr(BP_ERROR_IO_ERROR);
			BP_PRINT_ERROR(-1, errorcode);
			if (errorcode == BP_ERROR_IO_ERROR)
				__browser_adaptor_disconnect();
			pthread_mutex_unlock(&g_adaptor_mutex);
			return -1;
		}
		*ids = idlist;
		*count = ids_count;
	} else {
		*ids = NULL;
		*count = 0;
	}
	pthread_mutex_unlock(&g_adaptor_mutex);
	return 0;
}

int bp_history_adaptor_get_cond_ids_p
	(int **ids, int *count,
	bp_history_rows_cond_fmt *conds,
	const bp_history_offset check_offset,
	const char *keyword, const int is_like)
{
	return __bp_history_adaptor_get_cond_ids_p(
		BP_CMD_COMMON_GET_CONDS_DATE_IDS, ids, count, conds,
		check_offset, keyword, is_like);
}

int bp_history_adaptor_get_raw_retrieved_ids_p
	(int **ids, int *count,
	bp_history_rows_cond_fmt *conds,
	const bp_history_offset check_offset,
	const char *keyword,
	const int is_like)
{
	return __bp_history_adaptor_get_cond_ids_p(
		BP_CMD_COMMON_GET_CONDS_RAW_IDS, ids, count, conds,
		check_offset, keyword, is_like);
}

int bp_history_adaptor_reset(void)
{
	return __bp_history_adaptor_send_cmd(-1, BP_CMD_COMMON_RESET);
}

int bp_history_adaptor_set_data_changed_cb
	(bp_history_adaptor_data_changed_cb callback, void *user_data)
{
	if (callback == NULL) {
		TRACE_ERROR("check callback address:%p", callback);
		return -1;
	}
	g_adaptor_noti_user_data = NULL;
	g_adaptor_noti_cb = callback;
	g_adaptor_noti_user_data = user_data;
	return __bp_history_adaptor_send_cmd(-1, BP_CMD_SET_NOTI_CB);
}

int bp_history_adaptor_unset_data_changed_cb
	(bp_history_adaptor_data_changed_cb callback)
{
	if (callback != NULL && callback == g_adaptor_noti_cb) {
		g_adaptor_noti_cb = NULL;
		g_adaptor_noti_user_data = NULL;
		return __bp_history_adaptor_send_cmd(-1, BP_CMD_UNSET_NOTI_CB);
	}
	return -1;
}

int bp_history_adaptor_publish_notification(void)
{
	return __bp_history_adaptor_send_one_way(-1, BP_CMD_COMMON_NOTI);
}
