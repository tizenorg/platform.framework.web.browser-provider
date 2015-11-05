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

#include <bookmark-adaptor.h>
#include <common-adaptor.h>
#include <browser-provider.h>
#include <browser-provider-socket.h>

#ifdef SUPPORT_CLOUD_SYNC
#include <sync-adaptor.h>
#include <sync-adaptor-bookmark.h>
#endif

bp_adaptor_defs *g_adaptorinfo = NULL;
pthread_mutex_t g_adaptor_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t g_adaptor_event_thread_pid = 0;
static bp_bookmark_adaptor_data_changed_cb g_adaptor_noti_cb = NULL;
static void *g_adaptor_noti_user_data = NULL;
bp_command_fmt g_bp_command;
bp_error_defs errorcode = BP_ERROR_NONE;

/////// callback ///////////////

static void *__bookmark_adaptor_event_manager(void *arg)
{
	int status = bp_common_adaptor_event_manager(arg,
		&g_adaptor_noti_cb, &g_adaptor_noti_user_data);

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
	if (status == 1) {
		// to receive noti, re-connect to provider
		if (g_adaptor_noti_cb != NULL) {
			sleep(4); // workaround . wait terminating provider
			bp_bookmark_adaptor_set_data_changed_cb
				(g_adaptor_noti_cb, g_adaptor_noti_user_data);
		}
	}
	TRACE_DEBUG("callback thread is end by deinit");
	return 0;
}

// disconnect
int __browser_adaptor_disconnect(void)
{
	TRACE_DEBUG("pthread:%0x", (int)g_adaptor_event_thread_pid);
	return bp_common_adaptor_disconnect(&g_adaptorinfo,
		&g_adaptor_event_thread_pid);
}

int __browser_adaptor_connect(int callback)
{
	if (g_adaptorinfo == NULL) {
		bp_client_type_defs client_type = BP_CLIENT_BOOKMARK;

#ifdef SUPPORT_CLOUD_SYSTEM
		if (bp_common_adaptor_is_sync_adaptor() == 0)
			client_type = BP_CLIENT_BOOKMARK_SYNC;
#endif
#ifdef SUPPORT_CLOUD_SYNC
		if (bp_sync_is_login()){
			client_type = BP_CLIENT_BOOKMARK_SYNC;
		}
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
				__bookmark_adaptor_event_manager, g_adaptorinfo) != 0) {
			TRACE_STRERROR("[CRITICAL] pthread_create");
			return -1;
		}
		pthread_detach(g_adaptor_event_thread_pid);
		TRACE_DEBUG("pthread:%0x", (int)g_adaptor_event_thread_pid);
	}
	return 0;
}

static int __bp_bookmark_adaptor_get_ids_p(int id, int **ids,
	int *count, bp_command_defs cmd)
{
	if (ids == NULL)
		return -1;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	g_bp_command.cmd = cmd;
	g_bp_command.id = id;
	int ids_count = bp_common_adaptor_get_ids_p(sock, &g_bp_command,
			ids, &errorcode);
	if (ids_count < 0 && errorcode == BP_ERROR_IO_ERROR)
		__browser_adaptor_disconnect();
	pthread_mutex_unlock(&g_adaptor_mutex);
	if (ids_count < 0)
		return -1;
	*count = ids_count;
	return 0;
}

static int __bp_bookmark_adaptor_get_string(const int id,
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

static int __bp_bookmark_adaptor_get_int(const int id,
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

static int __bp_bookmark_adaptor_set_string(const int id,
	bp_command_defs cmd, const char *value)
{
	// allow negative for backup.restore
	if (value == NULL || bp_common_precheck_string(value) < 0)
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

static int __bp_bookmark_adaptor_set_int(const int id,
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

static int __bp_bookmark_adaptor_send_cmd(const int id, bp_command_defs cmd)
{
	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
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

static int __bp_bookmark_adaptor_send_one_way(const int id, bp_command_defs cmd)
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
int bp_bookmark_adaptor_initialize(void)
{
	pthread_mutex_lock(&g_adaptor_mutex);
	int ret = __browser_adaptor_connect(1);
	pthread_mutex_unlock(&g_adaptor_mutex);
	return ret;
}

// disconnect
int bp_bookmark_adaptor_deinitialize(void)
{
	pthread_mutex_lock(&g_adaptor_mutex);
	if (__browser_adaptor_connect(0) < 0) {
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	int sock = BP_CHECK_IPC_SOCK;

	g_adaptor_noti_cb = NULL;
	g_adaptor_noti_user_data = NULL;

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

int bp_bookmark_adaptor_get_root(int *id)
{
	if (id == NULL)
		return -1;
	*id = WEB_BOOKMARK_ROOT_ID;
	return 0;
}

// caller should free ids fully
int bp_bookmark_adaptor_get_full_ids_p(int **ids, int *count)
{
	return __bp_bookmark_adaptor_get_ids_p
		(-1, ids, count, BP_CMD_COMMON_GET_FULL_IDS);
}

int bp_bookmark_adaptor_get_full_with_deleted_ids_p(int **ids,
	int *count)
{
	return __bp_bookmark_adaptor_get_ids_p
		(-1, ids, count, BP_CMD_COMMON_GET_FULL_WITH_DELETED_IDS);
}

int bp_bookmark_adaptor_get_dirty_ids_p(int **ids, int *count)
{
	return __bp_bookmark_adaptor_get_ids_p
		(-1, ids, count, BP_CMD_COMMON_GET_DIRTY_IDS);
}

int bp_bookmark_adaptor_get_deleted_ids_p(int **ids, int *count)
{
	return __bp_bookmark_adaptor_get_ids_p
		(-1, ids, count, BP_CMD_COMMON_GET_DELETED_IDS);
}

int bp_bookmark_adaptor_clear_dirty_ids(void)
{
	return __bp_bookmark_adaptor_send_cmd
		(-1, BP_CMD_COMMON_CLEAR_DIRTY_IDS);
}

int bp_bookmark_adaptor_clear_deleted_ids(void)
{
	return __bp_bookmark_adaptor_send_cmd
		(-1, BP_CMD_COMMON_CLEAR_DELETED_IDS);
}

int bp_bookmark_adaptor_set_data_changed_cb
	(bp_bookmark_adaptor_data_changed_cb callback, void *user_data)
{
	if (callback == NULL) {
		TRACE_ERROR("check callback address:%p", callback);
		return -1;
	}
	g_adaptor_noti_user_data = NULL;
	g_adaptor_noti_cb = callback;
	g_adaptor_noti_user_data = user_data;
	return __bp_bookmark_adaptor_send_cmd(-1, BP_CMD_SET_NOTI_CB);
}

int bp_bookmark_adaptor_unset_data_changed_cb
	(bp_bookmark_adaptor_data_changed_cb callback)
{
	if (callback != NULL && callback == g_adaptor_noti_cb) {
		g_adaptor_noti_cb = NULL;
		g_adaptor_noti_user_data = NULL;
		return __bp_bookmark_adaptor_send_cmd(-1, BP_CMD_UNSET_NOTI_CB);
	}
	return -1;
}

int bp_bookmark_adaptor_is_setted_data_changed_cb(void)
{
	pthread_mutex_lock(&g_adaptor_mutex);
	if (g_adaptorinfo == NULL || g_adaptor_noti_cb == NULL) {
		TRACE_ERROR("callback is not setted");
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	if (g_adaptor_event_thread_pid <= 0) {
		TRACE_ERROR("[CRITICAL] callback is setted, but event manager not work");
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	pthread_mutex_unlock(&g_adaptor_mutex);
	return 0;
}

int bp_bookmark_adaptor_set_dirty(const int id)
{
	if (id < 0)
		return -1;
	return __bp_bookmark_adaptor_send_one_way
		(id, BP_CMD_COMMON_SET_DIRTY);
}

int bp_bookmark_adaptor_publish_notification(void)
{
	return __bp_bookmark_adaptor_send_one_way(-1, BP_CMD_COMMON_NOTI);
}

int bp_bookmark_adaptor_create(int *id)
{
	int bookmark_id = 0;

	if (id == NULL)
		return -1;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	int root_id = -1;
	if (*id >= 0 && bp_bookmark_adaptor_get_root(&root_id) == 0) {
		if (*id == root_id) {
			TRACE_ERROR("Deny to access Root");
			errorcode = BP_ERROR_INVALID_PARAMETER;
			pthread_mutex_unlock(&g_adaptor_mutex);
			return -1;
		}
	}

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
	bookmark_id = bp_adaptor_ipc_read_int(sock);
	if (bookmark_id < 0) {
		errorcode = bp_ipc_check_stderr(BP_ERROR_IO_ERROR);
		BP_PRINT_ERROR(*id, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	if (*id == bookmark_id) {
		TRACE_INFO("created a bookmark:%d", *id);
	} else {
		*id = bookmark_id;
		TRACE_INFO("created new bookmark:%d", *id);
	}
	pthread_mutex_unlock(&g_adaptor_mutex);
	return 0;
}

int bp_bookmark_adaptor_delete(const int id)
{
	if (id < 0) {
		TRACE_ERROR("[CHECK id][%d]", id);
		return -1;
	}
	return __bp_bookmark_adaptor_send_cmd(id, BP_CMD_COMMON_DELETE);
}

int bp_bookmark_adaptor_get_errorcode(void)
{
	switch(errorcode) {
	case BP_ERROR_INVALID_PARAMETER:
		return BP_BOOKMARK_ERROR_INVALID_PARAMETER;
	case BP_ERROR_OUT_OF_MEMORY:
		return BP_BOOKMARK_ERROR_OUT_OF_MEMORY;
	case BP_ERROR_IO_EINTR:
	case BP_ERROR_IO_EAGAIN:
	case BP_ERROR_IO_ERROR:
		return BP_BOOKMARK_ERROR_IO_ERROR;
	case BP_ERROR_NO_DATA:
		return BP_BOOKMARK_ERROR_NO_DATA;
	case BP_ERROR_ID_NOT_FOUND:
		return BP_BOOKMARK_ERROR_ID_NOT_FOUND;
	case BP_ERROR_DUPLICATED_ID:
		return BP_BOOKMARK_ERROR_DUPLICATED_ID;
	case BP_ERROR_PERMISSION_DENY:
		return BP_BOOKMARK_ERROR_PERMISSION_DENY;
	case BP_ERROR_DISK_FULL:
		return BP_BOOKMARK_ERROR_DISK_FULL;
	case BP_ERROR_DISK_BUSY:
		return BP_BOOKMARK_ERROR_DISK_BUSY;
	case BP_ERROR_TOO_BIG_DATA:
		return BP_BOOKMARK_ERROR_TOO_BIG_DATA;
	case BP_ERROR_UNKNOWN:
		return BP_BOOKMARK_ERROR_UNKNOWN;
	default:
		break;
	}
	return BP_BOOKMARK_ERROR_NONE;
}

int bp_bookmark_adaptor_get_type(const int id, int *value)
{
	return __bp_bookmark_adaptor_get_int
		(id, BP_CMD_BOOKMARK_GET_TYPE, value);
}

int bp_bookmark_adaptor_get_parent_id(const int id, int *value)
{
	return __bp_bookmark_adaptor_get_int
		(id, BP_CMD_BOOKMARK_GET_PARENT, value);
}

int bp_bookmark_adaptor_get_url(const int id, char **value)
{
	return __bp_bookmark_adaptor_get_string
		(id, BP_CMD_COMMON_GET_URL, value);
}

int bp_bookmark_adaptor_get_title(const int id, char **value)
{
	return __bp_bookmark_adaptor_get_string
		(id, BP_CMD_COMMON_GET_TITLE, value);
}

int bp_bookmark_adaptor_get_sequence(const int id, int *value)
{
	return __bp_bookmark_adaptor_get_int
		(id, BP_CMD_BOOKMARK_GET_SEQUENCE, value);
}

int bp_bookmark_adaptor_get_is_editable(const int id, int *value)
{
	return __bp_bookmark_adaptor_get_int
		(id, BP_CMD_BOOKMARK_GET_IS_EDITABLE, value);
}

int bp_bookmark_adaptor_get_is_operator(const int id, int *value)
{
	return __bp_bookmark_adaptor_get_int
		(id, BP_CMD_BOOKMARK_GET_IS_OPERATOR, value);
}

int bp_bookmark_adaptor_get_access_count(const int id, int *value)
{
	return __bp_bookmark_adaptor_get_int
		(id, BP_CMD_BOOKMARK_GET_ACCESS_COUNT, value);
}

int bp_bookmark_adaptor_get_date_created(const int id, int *value)
{
	return __bp_bookmark_adaptor_get_int
		(id, BP_CMD_COMMON_GET_DATE_CREATED, value);
}

int bp_bookmark_adaptor_get_date_modified(const int id, int *value)
{
	return __bp_bookmark_adaptor_get_int
		(id, BP_CMD_COMMON_GET_DATE_MODIFIED, value);
}

int bp_bookmark_adaptor_get_date_visited(const int id, int *value)
{
	return __bp_bookmark_adaptor_get_int
		(id, BP_CMD_COMMON_GET_DATE_VISITED, value);
}

int bp_bookmark_adaptor_get_account_name(const int id, char **value)
{
	return __bp_bookmark_adaptor_get_string
		(id, BP_CMD_COMMON_GET_ACCOUNT_NAME, value);
}

int bp_bookmark_adaptor_get_account_type(const int id, char **value)
{
	return __bp_bookmark_adaptor_get_string
		(id, BP_CMD_COMMON_GET_ACCOUNT_TYPE, value);
}

int bp_bookmark_adaptor_get_device_name(const int id, char **value)
{
	return __bp_bookmark_adaptor_get_string
		(id, BP_CMD_COMMON_GET_DEVICE_NAME, value);
}

int bp_bookmark_adaptor_get_device_id(const int id, char **value)
{
	return __bp_bookmark_adaptor_get_string
		(id, BP_CMD_COMMON_GET_DEVICE_ID, value);
}

int bp_bookmark_adaptor_get_sync(const int id, char **value)
{
	return __bp_bookmark_adaptor_get_string
		(id, BP_CMD_COMMON_GET_SYNC, value);
}

int bp_bookmark_adaptor_set_type(const int id, const int value)
{
	return __bp_bookmark_adaptor_set_int
		(id, BP_CMD_BOOKMARK_SET_TYPE, value);
}

int bp_bookmark_adaptor_set_parent_id(const int id, const int value)
{
	return __bp_bookmark_adaptor_set_int
		(id, BP_CMD_BOOKMARK_SET_PARENT, value);
}

int bp_bookmark_adaptor_set_url(const int id, const char *value)
{
	return __bp_bookmark_adaptor_set_string
		(id, BP_CMD_COMMON_SET_URL, value);
}

int bp_bookmark_adaptor_set_title(const int id, const char *value)
{
	return __bp_bookmark_adaptor_set_string
		(id, BP_CMD_COMMON_SET_TITLE, value);
}

int bp_bookmark_adaptor_set_sequence(const int id, const int value)
{
	return __bp_bookmark_adaptor_set_int
		(id, BP_CMD_BOOKMARK_SET_SEQUENCE, value);
}

int bp_bookmark_adaptor_set_access_count(const int id, const int value)
{
	return __bp_bookmark_adaptor_set_int
		(id, BP_CMD_BOOKMARK_SET_ACCESS_COUNT, value);
}

int bp_bookmark_adaptor_set_date_created(const int id, const int value)
{
	return __bp_bookmark_adaptor_set_int
		(id, BP_CMD_COMMON_SET_DATE_CREATED, value);
}

int bp_bookmark_adaptor_set_date_modified(const int id, const int value)
{
	return __bp_bookmark_adaptor_set_int
		(id, BP_CMD_COMMON_SET_DATE_MODIFIED, value);
}

int bp_bookmark_adaptor_set_date_visited(const int id, const int value)
{
	return __bp_bookmark_adaptor_set_int
		(id, BP_CMD_COMMON_SET_DATE_VISITED, value);
}

int bp_bookmark_adaptor_set_account_name(const int id,
	const char *value)
{
	return __bp_bookmark_adaptor_set_string
		(id, BP_CMD_COMMON_SET_ACCOUNT_NAME, value);
}

int bp_bookmark_adaptor_set_account_type(const int id,
	const char *value)
{
	return __bp_bookmark_adaptor_set_string
		(id, BP_CMD_COMMON_SET_ACCOUNT_TYPE, value);
}

int bp_bookmark_adaptor_set_device_name(const int id, const char *value)
{
	return __bp_bookmark_adaptor_set_string
		(id, BP_CMD_COMMON_SET_DEVICE_NAME, value);
}

int bp_bookmark_adaptor_set_device_id(const int id, const char *value)
{
	return __bp_bookmark_adaptor_set_string
		(id, BP_CMD_COMMON_SET_DEVICE_ID, value);
}

int bp_bookmark_adaptor_set_sync(const int id, const char *value)
{
	return __bp_bookmark_adaptor_set_string
		(id, BP_CMD_COMMON_SET_SYNC, value);
}

int bp_bookmark_adaptor_easy_create(int *id, bp_bookmark_info_fmt *info)
{
	if (id == NULL || info == NULL)
		return -1;

	if (*id <= 0) { // new bookmark
		if (bp_bookmark_adaptor_create(id) < 0) {
			TRACE_ERROR("[failed to create new bookmark]");
			return -1;
		}
		if (*id <= 0) {
			TRACE_ERROR("[failed to create new bookmark]");
			return -1;
		}
	}

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	errorcode = BP_ERROR_NONE;
	bp_bookmark_base_fmt new_base_info;
	memset(&new_base_info, 0x00, sizeof(bp_bookmark_base_fmt));
	new_base_info.type = info->type;
	new_base_info.parent = info->parent;
	new_base_info.sequence = info->sequence;
	new_base_info.editable = info->editable;
	new_base_info.is_operator = info->is_operator;
	new_base_info.access_count = info->access_count;
	new_base_info.date_created = info->date_created;
	new_base_info.date_modified = info->date_modified;
	new_base_info.date_visited = info->date_visited;

	g_bp_command.cmd = BP_CMD_COMMON_SET_EASY_ALL;
	g_bp_command.id = *id;
	// send command without waiting return value
	errorcode = bp_ipc_simple_response(sock, &g_bp_command);
	if (errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(*id, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	// 1. send bp_bookmark_base_fmt structure
	if (bp_ipc_send_custom_type(sock, &new_base_info,
			sizeof(bp_bookmark_base_fmt)) < 0) {
		errorcode = BP_ERROR_IO_ERROR;
		BP_PRINT_ERROR(*id, errorcode);
		__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	// 2. wait id from provider.
	errorcode = bp_ipc_read_errorcode(sock);
	if (errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(*id, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	pthread_mutex_unlock(&g_adaptor_mutex);

	// 3. set strings
	if (info->url != NULL)
		bp_bookmark_adaptor_set_url(*id, info->url);
	if (info->title != NULL)
		bp_bookmark_adaptor_set_title(*id, info->title);
	if (info->account_name != NULL)
		bp_bookmark_adaptor_set_account_name(*id, info->account_name);
	if (info->account_type != NULL)
		bp_bookmark_adaptor_set_account_type(*id, info->account_type);
	if (info->device_name != NULL)
		bp_bookmark_adaptor_set_device_name(*id, info->device_name);
	if (info->device_id != NULL)
		bp_bookmark_adaptor_set_device_id(*id, info->device_id);
	if (info->sync != NULL)
		bp_bookmark_adaptor_set_sync(*id, info->sync);

	if (info->favicon_length > 0 && info->favicon != NULL) {
		bp_bookmark_adaptor_set_icon(*id, info->favicon_width,
			info->favicon_height, info->favicon, info->favicon_length);
	}
	if (info->thumbnail_length > 0 && info->thumbnail != NULL) {
		bp_bookmark_adaptor_set_snapshot(*id, info->thumbnail_width,
			info->thumbnail_height, info->thumbnail, info->thumbnail_length);
	}
	if (info->webicon_length > 0 && info->webicon != NULL) {
		bp_bookmark_adaptor_set_webicon(*id, info->webicon_width,
			info->webicon_height, info->webicon, info->webicon_length);
	}

	return 0;
}

int bp_bookmark_adaptor_get_easy_all(int id, bp_bookmark_info_fmt *info)
{
	return bp_bookmark_adaptor_get_info(id, BP_BOOKMARK_O_ALL, info);
}

int bp_bookmark_adaptor_get_info(const int id,
	const bp_bookmark_offset offset, bp_bookmark_info_fmt *info)
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
	bp_bookmark_offset oflags = offset;
	if (bp_ipc_send_custom_type(sock, &oflags,
			sizeof(bp_bookmark_offset)) < 0) {
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
	bp_bookmark_base_fmt bookmark;
	memset(&bookmark, 0x00, sizeof(bp_bookmark_base_fmt));
	// getting bp_tab_base_fmt from provider.
	if (bp_ipc_read_custom_type(sock, &bookmark,
			sizeof(bp_bookmark_base_fmt)) < 0) {
		errorcode = bp_ipc_check_stderr(BP_ERROR_IO_ERROR);
		BP_PRINT_ERROR(id, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	memset(info, 0x00, sizeof(bp_bookmark_info_fmt));

	// fill info
	if (offset & BP_BOOKMARK_O_TYPE) {
		info->type = bookmark.type;
	}
	if (offset & BP_BOOKMARK_O_PARENT) {
		info->parent = bookmark.parent;
	}
	if (offset & BP_BOOKMARK_O_SEQUENCE) {
		info->sequence = bookmark.sequence;
	}
	if (offset & BP_BOOKMARK_O_IS_EDITABLE) {
		info->editable = bookmark.editable;
	}
	if (offset & BP_BOOKMARK_O_IS_OPERATOR) {
		info->is_operator = bookmark.is_operator;
	}
	if (offset & BP_BOOKMARK_O_ACCESS_COUNT) {
		info->access_count = bookmark.access_count;
	}
	if (offset & BP_BOOKMARK_O_DATE_CREATED) {
		info->date_created = bookmark.date_created;
	}
	if (offset & BP_BOOKMARK_O_DATE_MODIFIED) {
		info->date_modified = bookmark.date_modified;
	}
	if (offset & BP_BOOKMARK_O_DATE_VISITED) {
		info->date_visited = bookmark.date_visited;
	}
	// get strings . keep the order with provider
	if (offset & BP_BOOKMARK_O_URL) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->url = bp_ipc_read_string(sock);
	}
	if (offset & BP_BOOKMARK_O_TITLE) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->title = bp_ipc_read_string(sock);
	}
	if (offset & BP_BOOKMARK_O_ACCOUNT_NAME) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->account_name = bp_ipc_read_string(sock);
	}
	if (offset & BP_BOOKMARK_O_ACCOUNT_TYPE) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->account_type = bp_ipc_read_string(sock);
	}
	if (offset & BP_BOOKMARK_O_DEVICE_NAME) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->device_name = bp_ipc_read_string(sock);
	}
	if (offset & BP_BOOKMARK_O_DEVICE_ID) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->device_id = bp_ipc_read_string(sock);
	}
	if (offset & BP_BOOKMARK_O_SYNC) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->sync = bp_ipc_read_string(sock);
	}
	if (offset & BP_BOOKMARK_O_ICON) {
		info->favicon_length = 0;
		info->favicon_width = bookmark.favicon_width;
		info->favicon_height = bookmark.favicon_height;
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE) {
			unsigned char *blob_data = NULL;
			if ((info->favicon_length =
					bp_common_adaptor_get_info_blob(sock, &blob_data,
						&g_adaptorinfo->shm)) > 0 && blob_data != NULL)
				info->favicon = blob_data;
		}
	}
	if (offset & BP_BOOKMARK_O_SNAPSHOT) {
		info->thumbnail_length = 0;
		info->thumbnail_width = bookmark.thumbnail_width;
		info->thumbnail_height = bookmark.thumbnail_height;
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE) {
			unsigned char *blob_data = NULL;
			if ((info->thumbnail_length =
					bp_common_adaptor_get_info_blob(sock, &blob_data,
						&g_adaptorinfo->shm)) > 0 && blob_data != NULL)
				info->thumbnail = blob_data;
		}
	}
	if (offset & BP_BOOKMARK_O_WEBICON) {
		info->webicon_length = 0;
		info->webicon_width = bookmark.webicon_width;
		info->webicon_height = bookmark.webicon_height;
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE) {
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

int bp_bookmark_adaptor_easy_free(bp_bookmark_info_fmt *info)
{
	if (info != NULL) {
		free(info->url);
		free(info->title);
		free(info->account_name);
		free(info->account_type);
		free(info->device_name);
		free(info->device_id);
		free(info->favicon);
		free(info->thumbnail);
		free(info->webicon);
		free(info->sync);
		memset(info, 0x00, sizeof(bp_bookmark_info_fmt));
	}
	return 0;
}

static int __bp_bookmark_adaptor_get_cond_ids_p
	(bp_command_defs pcommand, int **ids, int *count,
	bp_bookmark_property_cond_fmt *properties,
	bp_bookmark_rows_cond_fmt *conds,
	const bp_bookmark_offset check_offset,
	const char *keyword, const int is_like)
{
	if (ids == NULL || count == NULL)
		return -1;
	bp_bookmark_property_cond_fmt t_properties;
	bp_bookmark_rows_cond_fmt t_conds;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	bp_command_fmt *cmd = &g_bp_command;

	cmd->cmd = pcommand;
	cmd->id = -1;

	memset(&t_properties, 0x00, sizeof(bp_bookmark_property_cond_fmt));
	memset(&t_conds, 0x00, sizeof(bp_bookmark_rows_cond_fmt));

	if (properties != NULL) {
		memcpy(&t_properties, properties,
				sizeof(bp_bookmark_property_cond_fmt));
	} else {
		t_properties.parent = -1;
		t_properties.type = -1;
		t_properties.is_operator = -1;
		t_properties.is_editable = -1;
	}
	if (conds != NULL)
		memcpy(&t_conds, conds, sizeof(bp_bookmark_rows_cond_fmt));
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

	bp_bookmark_offset oflags = check_offset;
	if (keyword == NULL)
		oflags = 0;
	if (bp_ipc_send_custom_type(sock, &t_properties,
			sizeof(bp_bookmark_property_cond_fmt)) < 0 ||
			bp_ipc_send_custom_type(sock, &t_conds,
			sizeof(bp_bookmark_rows_cond_fmt)) < 0 ||
			bp_ipc_send_custom_type(sock, &oflags,
			sizeof(bp_bookmark_offset)) < 0) {
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

int bp_bookmark_adaptor_get_timestamp_ids_p
	(int **ids, int *count,
	const bp_bookmark_property_cond_fmt *properties, //if NULL, ignore
	const bp_bookmark_rows_fmt *limits,//if NULL, ignore
	const bp_bookmark_timestamp_fmt times[], const int times_count,
	const bp_bookmark_offset check_offset, //if zero, ignore keyword search
	const char *keyword, // if NULL, does not search
	const int is_like)
{
	if (ids == NULL || count == NULL)
		return -1;
	bp_bookmark_property_cond_fmt t_properties;
	bp_bookmark_rows_fmt t_limits;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	bp_command_fmt *cmd = &g_bp_command;

	cmd->cmd = BP_CMD_COMMON_GET_CONDS_TIMESTAMP_IDS;
	cmd->id = -1;

	memset(&t_properties, 0x00, sizeof(bp_bookmark_property_cond_fmt));
	memset(&t_limits, 0x00, sizeof(bp_bookmark_rows_fmt));

	if (properties != NULL) {
		memcpy(&t_properties, properties,
				sizeof(bp_bookmark_property_cond_fmt));
	} else {
		t_properties.parent = -1;
		t_properties.type = -1;
		t_properties.is_operator = -1;
		t_properties.is_editable = -1;
	}
	if (limits != NULL)
		memcpy(&t_limits, limits, sizeof(bp_bookmark_rows_fmt));
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

	if (bp_ipc_send_custom_type(sock, &t_properties,
			sizeof(bp_bookmark_property_cond_fmt)) < 0 ||
			bp_ipc_send_custom_type(sock, &t_limits,
			sizeof(bp_bookmark_rows_fmt)) < 0 ||
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
			bp_bookmark_timestamp_fmt *timestamps =
				(bp_bookmark_timestamp_fmt *)(times + i);
			if (bp_ipc_send_custom_type(sock, timestamps,
					sizeof(bp_bookmark_timestamp_fmt)) < 0) {
				errorcode = BP_ERROR_IO_ERROR;
				BP_PRINT_ERROR(-1, errorcode);
				__browser_adaptor_disconnect();
				pthread_mutex_unlock(&g_adaptor_mutex);
				return -1;
			}
		}
	}

	bp_bookmark_offset oflags = check_offset;
	if (keyword == NULL)
		oflags = 0;
	if (bp_ipc_send_custom_type(sock, &oflags,
			sizeof(bp_bookmark_offset)) < 0) {
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

int bp_bookmark_adaptor_backup(char *value)
{
	return __bp_bookmark_adaptor_set_string
		(-1, BP_CMD_BOOKMARK_BACKUP, value);
}

int bp_bookmark_adaptor_restore(char *value)
{
	return __bp_bookmark_adaptor_set_string
		(-1, BP_CMD_BOOKMARK_RESTORE, value);
}

int bp_bookmark_adaptor_get_ids_p
	(int **ids, int *count, const int limit, const int offset,
	const int parent, const int type,
	const int is_operator, const int is_editable,
	const bp_bookmark_offset order_offset, const int ordering)
{
	if (ids == NULL || count == NULL)
		return -1;
	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	bp_command_fmt *cmd = &g_bp_command;

	cmd->cmd = BP_CMD_COMMON_GET_CONDS_INT_IDS;
	cmd->id = -1;

	bp_db_base_conds_fmt conds;
	memset(&conds, 0x00, sizeof(bp_db_base_conds_fmt));
	conds.limit = limit;
	conds.offset = offset;
	conds.order_column_offset = order_offset;
	conds.ordering = ordering;

	errorcode = bp_ipc_simple_response(sock, cmd);
	if (errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(-1, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}

	if (bp_ipc_send_custom_type(sock, &conds,
			sizeof(bp_db_base_conds_fmt)) < 0 ||
			bp_adaptor_ipc_send_int(sock, parent) < 0 ||
			bp_adaptor_ipc_send_int(sock, type) < 0 ||
			bp_adaptor_ipc_send_int(sock, is_operator) < 0 ||
			bp_adaptor_ipc_send_int(sock, is_editable) < 0) {
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

int bp_bookmark_adaptor_get_cond_ids_p
	(int **ids, int *count,
	bp_bookmark_property_cond_fmt *properties,
	bp_bookmark_rows_cond_fmt *conds,
	const bp_bookmark_offset check_offset,
	const char *keyword, const int is_like)
{
	return __bp_bookmark_adaptor_get_cond_ids_p
		(BP_CMD_COMMON_GET_CONDS_DATE_IDS, ids, count, properties,
		conds, check_offset, keyword, is_like);
}

int bp_bookmark_adaptor_get_raw_retrieved_ids_p
	(int **ids, int *count,
	bp_bookmark_property_cond_fmt *properties,
	bp_bookmark_rows_cond_fmt *conds,
	const bp_bookmark_offset check_offset,
	const char *keyword, const int is_like)
{
	return __bp_bookmark_adaptor_get_cond_ids_p
		(BP_CMD_COMMON_GET_CONDS_RAW_IDS, ids, count, properties,
		conds, check_offset, keyword, is_like);
}

//////////////// internet_bookmark_xxx APIs

// removing an item without corresponding sub items. remain this code just for reference.
/*
int bp_bookmark_adaptor_remove(const int id, const int remove_flag)
{
	// workaround API for WEBAPI TC(favorite api)
	if (remove_flag < 0) {
		return __bp_bookmark_adaptor_send_cmd
			(id, BP_CMD_BOOKMARK_DELETE_NO_CARE_CHILD);
	}

	if (remove_flag == 1) {
		int type = 0;
		if (bp_bookmark_adaptor_get_type(id, &type) < 0)
			return -1;

		if (type == 1) {
			// move all child to root folder
			int *ids = NULL;
			int ids_count = 0;
			int i = 0;
			bp_bookmark_adaptor_get_ids_p(&ids, &ids_count, -1, 0, id,
				-1, -1, -1, BP_BOOKMARK_O_SEQUENCE, 0);
			for (i = 0; i < ids_count; i++) {
				bp_bookmark_adaptor_set_parent_id(ids[i],
					bp_bookmark_adaptor_get_root_id());
			}
			free(ids);
		}
	}
	return bp_bookmark_adaptor_delete(id);
}
*/

int bp_bookmark_adaptor_reset(void)
{
	return __bp_bookmark_adaptor_send_cmd(-1, BP_CMD_COMMON_RESET);
}

