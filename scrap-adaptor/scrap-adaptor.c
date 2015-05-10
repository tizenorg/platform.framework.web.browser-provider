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
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include <errno.h>

#include <browser-provider.h>
#include <common-adaptor.h>
#include <scrap-adaptor.h>
#include <browser-provider-socket.h>

static bp_adaptor_defs *g_adaptorinfo = NULL;
static pthread_mutex_t g_adaptor_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t g_adaptor_event_thread_pid = 0;
static bp_scrap_adaptor_data_changed_cb g_adaptor_noti_cb = NULL;
static void *g_adaptor_noti_user_data = NULL;
static bp_command_fmt g_bp_command;
static bp_error_defs errorcode = BP_ERROR_NONE;

/////// callback ///////////////

static void *__scrap_adaptor_event_manager(void *arg)
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
			sleep(8); // workaround . wait terminating provider
			bp_scrap_adaptor_set_data_changed_cb
				(g_adaptor_noti_cb, g_adaptor_noti_user_data);
		}
	}
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
		bp_client_type_defs client_type = BP_CLIENT_SCRAP;

#ifdef SUPPORT_CLOUD_SYSTEM
		if (bp_common_adaptor_is_sync_adaptor() == 0)
			client_type = BP_CLIENT_SCRAP_SYNC;
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
				__scrap_adaptor_event_manager, g_adaptorinfo) != 0) {
			TRACE_STRERROR("[CRITICAL] pthread_create");
			return -1;
		}
		pthread_detach(g_adaptor_event_thread_pid);
		TRACE_DEBUG("pthread:%0x", (int)g_adaptor_event_thread_pid);
	}
	return 0;
}

static int __bp_scrap_adaptor_get_ids_p(int id, int **ids, int *count,
	bp_command_defs cmd)
{
	if (ids == NULL || count == NULL)
		return -1;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	g_bp_command.cmd = cmd;
	g_bp_command.id = id;
	int ids_count = bp_common_adaptor_get_ids_p(sock, &g_bp_command, ids, &errorcode);
	if (ids_count < 0 && errorcode == BP_ERROR_IO_ERROR)
		__browser_adaptor_disconnect();
	pthread_mutex_unlock(&g_adaptor_mutex);
	if (ids_count < 0)
		return -1;
	*count = ids_count;
	return 0;
}

static int __bp_scrap_adaptor_get_string
	(const int id, bp_command_defs cmd, char **value)
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

static int __bp_scrap_adaptor_get_int
	(const int id, bp_command_defs cmd, int *value)
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

static int __bp_scrap_adaptor_set_string(const int id,
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

static int __bp_scrap_adaptor_set_int
	(const int id, bp_command_defs cmd, const int value)
{
	if (id < 0) {
		TRACE_ERROR("[CHECK id][%d]", id);
		return -1;
	}

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

static int __bp_scrap_adaptor_send_cmd(const int id, bp_command_defs cmd)
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

static int __bp_scrap_adaptor_send_one_way(const int id, bp_command_defs cmd)
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
int bp_scrap_adaptor_initialize(void)
{
	pthread_mutex_lock(&g_adaptor_mutex);
	int ret = __browser_adaptor_connect(1);
	pthread_mutex_unlock(&g_adaptor_mutex);
	return ret;
}

// disconnect
int bp_scrap_adaptor_deinitialize(void)
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

// caller should free ids fully
int bp_scrap_adaptor_get_full_ids_p(int **ids, int *count)
{
	return __bp_scrap_adaptor_get_ids_p
		(-1, ids, count, BP_CMD_COMMON_GET_FULL_IDS);
}

int bp_scrap_adaptor_get_full_with_deleted_ids_p(int **ids, int *count)
{
	return __bp_scrap_adaptor_get_ids_p
		(-1, ids, count, BP_CMD_COMMON_GET_FULL_WITH_DELETED_IDS);
}
int bp_scrap_adaptor_get_dirty_ids_p(int **ids, int *count)
{
	return __bp_scrap_adaptor_get_ids_p
		(-1, ids, count, BP_CMD_COMMON_GET_DIRTY_IDS);
}

int bp_scrap_adaptor_get_deleted_ids_p(int **ids, int *count)
{
	return __bp_scrap_adaptor_get_ids_p
		(-1, ids, count, BP_CMD_COMMON_GET_DELETED_IDS);
}

int bp_scrap_adaptor_clear_dirty_ids(void)
{
	return __bp_scrap_adaptor_send_cmd
		(-1, BP_CMD_COMMON_CLEAR_DIRTY_IDS);
}

int bp_scrap_adaptor_clear_deleted_ids(void)
{
	return __bp_scrap_adaptor_send_cmd
		(-1, BP_CMD_COMMON_CLEAR_DELETED_IDS);
}

int bp_scrap_adaptor_set_data_changed_cb
	(bp_scrap_adaptor_data_changed_cb callback, void *user_data)
{
	if (callback == NULL) {
		TRACE_ERROR("check callback address:%p", callback);
		return -1;
	}
	g_adaptor_noti_user_data = NULL;
	g_adaptor_noti_cb = callback;
	g_adaptor_noti_user_data = user_data;
	return __bp_scrap_adaptor_send_cmd(-1, BP_CMD_SET_NOTI_CB);
}

int bp_scrap_adaptor_unset_data_changed_cb(bp_scrap_adaptor_data_changed_cb callback)
{
	if (callback != NULL && callback == g_adaptor_noti_cb) {
		g_adaptor_noti_cb = NULL;
		g_adaptor_noti_user_data = NULL;
		return __bp_scrap_adaptor_send_cmd(-1, BP_CMD_UNSET_NOTI_CB);
	}
	return -1;
}

int bp_scrap_adaptor_is_setted_data_changed_cb(void)
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

int bp_scrap_adaptor_set_dirty(const int id)
{
	if (id < 0)
		return -1;
	return __bp_scrap_adaptor_send_one_way(id, BP_CMD_COMMON_SET_DIRTY);
}

int bp_scrap_adaptor_publish_notification(void)
{
	return __bp_scrap_adaptor_send_one_way(-1, BP_CMD_COMMON_NOTI);
}

int bp_scrap_adaptor_get_is_read(const int id, int *value)
{
	return __bp_scrap_adaptor_get_int
		(id, BP_CMD_SCRAP_GET_IS_READ, value);
}

int bp_scrap_adaptor_get_is_reader(const int id, int *value)
{
	return __bp_scrap_adaptor_get_int
		(id, BP_CMD_SCRAP_GET_IS_READER, value);
}

int bp_scrap_adaptor_get_is_night_mode(const int id, int *night_mode_flag)
{
	return __bp_scrap_adaptor_get_int
		(id, BP_CMD_SCRAP_GET_IS_NIGHT_MODE, night_mode_flag);
}

int bp_scrap_adaptor_get_url(const int id, char **value)
{
	return __bp_scrap_adaptor_get_string
		(id, BP_CMD_COMMON_GET_URL, value);
}

int bp_scrap_adaptor_get_title(const int id, char **value)
{
	return __bp_scrap_adaptor_get_string
		(id, BP_CMD_COMMON_GET_TITLE, value);
}

int bp_scrap_adaptor_get_page_path(const int id, char **value)
{
	return __bp_scrap_adaptor_get_string
		(id, BP_CMD_SCRAP_GET_PAGE_PATH, value);
}

// blob
int bp_scrap_adaptor_get_icon(const int id, int *width,
	int *height, unsigned char **value, int *length)
{
	if (id < 0 || value == NULL || length == NULL)
		return -1;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	g_bp_command.cmd = BP_CMD_COMMON_GET_ICON;
	g_bp_command.id = id;
	int ret = bp_common_adaptor_get_blob_shm(sock, &g_bp_command, width,
			height, value, length, &errorcode, &g_adaptorinfo->shm);
	if (ret < 0 && errorcode == BP_ERROR_IO_ERROR)
		__browser_adaptor_disconnect();
	pthread_mutex_unlock(&g_adaptor_mutex);
	return ret;
}

int bp_scrap_adaptor_get_snapshot(const int id, int *width,
	int *height, unsigned char **value, int *length)
{
	if (id < 0 || value == NULL || length == NULL)
		return -1;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	g_bp_command.cmd = BP_CMD_COMMON_GET_SNAPSHOT;
	g_bp_command.id = id;
	int ret = bp_common_adaptor_get_blob_shm(sock, &g_bp_command, width,
			height, value, length, &errorcode, &g_adaptorinfo->shm);
	if (ret < 0 && errorcode == BP_ERROR_IO_ERROR)
		__browser_adaptor_disconnect();
	pthread_mutex_unlock(&g_adaptor_mutex);
	return ret;
}

int bp_scrap_adaptor_get_date_created(const int id, int *value)
{
	return __bp_scrap_adaptor_get_int
		(id, BP_CMD_COMMON_GET_DATE_CREATED, value);
}

int bp_scrap_adaptor_get_date_modified(const int id, int *value)
{
	return __bp_scrap_adaptor_get_int
		(id, BP_CMD_COMMON_GET_DATE_MODIFIED, value);
}

int bp_scrap_adaptor_get_account_name(const int id, char **value)
{
	return __bp_scrap_adaptor_get_string
		(id, BP_CMD_COMMON_GET_ACCOUNT_NAME, value);
}

int bp_scrap_adaptor_get_account_type(const int id, char **value)
{
	return __bp_scrap_adaptor_get_string
		(id, BP_CMD_COMMON_GET_ACCOUNT_TYPE, value);
}

int bp_scrap_adaptor_get_device_name(const int id, char **value)
{
	return __bp_scrap_adaptor_get_string
		(id, BP_CMD_COMMON_GET_DEVICE_NAME, value);
}

int bp_scrap_adaptor_get_device_id(const int id, char **value)
{
	return __bp_scrap_adaptor_get_string
		(id, BP_CMD_COMMON_GET_DEVICE_ID, value);
}

int bp_scrap_adaptor_get_main_content(const int id, char **value)
{
	return __bp_scrap_adaptor_get_string
		(id, BP_CMD_SCRAP_GET_MAIN_CONTENT, value);
}

int bp_scrap_adaptor_set_is_read(const int id, const int value)
{
	return __bp_scrap_adaptor_set_int
		(id, BP_CMD_SCRAP_SET_IS_READ, value);
}

int bp_scrap_adaptor_set_is_reader(const int id, const int value)
{
	return __bp_scrap_adaptor_set_int
		(id, BP_CMD_SCRAP_SET_IS_READER, value);
}

int bp_scrap_adaptor_set_is_night_mode(const int id, const int night_mode_flag)
{
	return __bp_scrap_adaptor_set_int
		(id, BP_CMD_SCRAP_SET_IS_NIGHT_MODE, night_mode_flag);
}

int bp_scrap_adaptor_set_url(const int id, const char *value)
{
	return __bp_scrap_adaptor_set_string
		(id, BP_CMD_COMMON_SET_URL, value);
}

int bp_scrap_adaptor_set_title(const int id, const char *value)
{
	return __bp_scrap_adaptor_set_string
		(id, BP_CMD_COMMON_SET_TITLE, value);
}

int bp_scrap_adaptor_set_page_path(const int id, const char *value)
{
	return __bp_scrap_adaptor_set_string
		(id, BP_CMD_SCRAP_SET_PAGE_PATH, value);
}

int bp_scrap_adaptor_set_icon(const int id, const int width,
	const int height, const unsigned char *value, const int length)
{
	if (id < 0)
		return -1;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	g_bp_command.cmd = BP_CMD_COMMON_SET_ICON;
	g_bp_command.id = id;
	int ret = bp_common_adaptor_set_blob_shm(sock, &g_bp_command, width,
			height, value, length, &errorcode, &g_adaptorinfo->shm);
	if (ret < 0 && errorcode == BP_ERROR_IO_ERROR)
		__browser_adaptor_disconnect();
	pthread_mutex_unlock(&g_adaptor_mutex);
	return ret;
}

int bp_scrap_adaptor_set_snapshot(const int id, const int width,
	const int height, const unsigned char *value, const int length)
{
	if (id < 0)
		return -1;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	g_bp_command.cmd = BP_CMD_COMMON_SET_SNAPSHOT;
	g_bp_command.id = id;
	int ret = bp_common_adaptor_set_blob_shm(sock, &g_bp_command, width,
			height, value, length, &errorcode, &g_adaptorinfo->shm);
	if (ret < 0 && errorcode == BP_ERROR_IO_ERROR)
		__browser_adaptor_disconnect();
	pthread_mutex_unlock(&g_adaptor_mutex);
	return ret;
}

int bp_scrap_adaptor_set_date_created(const int id, const int value)
{
	return __bp_scrap_adaptor_set_int
		(id, BP_CMD_COMMON_SET_DATE_CREATED, value);
}

int bp_scrap_adaptor_set_date_modified(const int id, const int value)
{
	return __bp_scrap_adaptor_set_int
		(id, BP_CMD_COMMON_SET_DATE_MODIFIED, value);
}

int bp_scrap_adaptor_set_account_name(const int id, const char *value)
{
	return __bp_scrap_adaptor_set_string
		(id, BP_CMD_COMMON_SET_ACCOUNT_NAME, value);
}

int bp_scrap_adaptor_set_account_type(const int id, const char *value)
{
	return __bp_scrap_adaptor_set_string
		(id, BP_CMD_COMMON_SET_ACCOUNT_TYPE, value);
}

int bp_scrap_adaptor_set_device_name(const int id, const char *value)
{
	return __bp_scrap_adaptor_set_string
		(id, BP_CMD_COMMON_SET_DEVICE_NAME, value);
}

int bp_scrap_adaptor_set_device_id(const int id, const char *value)
{
	return __bp_scrap_adaptor_set_string
		(id, BP_CMD_COMMON_SET_DEVICE_ID, value);
}

int bp_scrap_adaptor_set_main_content(const int id, const char *value)
{
	return __bp_scrap_adaptor_set_string
		(id, BP_CMD_SCRAP_SET_MAIN_CONTENT, value);
}

int bp_scrap_adaptor_create(int *id)
{
	int scrap_id = 0;

	if (id == NULL) {
		return -1;
	}

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
	scrap_id = bp_adaptor_ipc_read_int(sock);
	if (scrap_id < 0) {
		errorcode = bp_ipc_check_stderr(BP_ERROR_IO_ERROR);
		BP_PRINT_ERROR(*id, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	pthread_mutex_unlock(&g_adaptor_mutex);
	if (*id == scrap_id) {
		TRACE_INFO("created a scrap:%d", *id);
	} else {
		*id = scrap_id;
		TRACE_INFO("created new scrap:%d", *id);
	}
	return 0;
}

int bp_scrap_adaptor_delete(const int id)
{
	if (id < 0) {
		TRACE_ERROR("[CHECK id][%d]", id);
		return -1;
	}
	return __bp_scrap_adaptor_send_cmd(id, BP_CMD_COMMON_DELETE);
}

int bp_scrap_adaptor_get_errorcode(void)
{
	switch(errorcode) {
	case BP_ERROR_INVALID_PARAMETER:
		return BP_SCRAP_ERROR_INVALID_PARAMETER;
	case BP_ERROR_OUT_OF_MEMORY:
		return BP_SCRAP_ERROR_OUT_OF_MEMORY;
	case BP_ERROR_IO_EINTR:
	case BP_ERROR_IO_EAGAIN:
	case BP_ERROR_IO_ERROR:
		return BP_SCRAP_ERROR_IO_ERROR;
	case BP_ERROR_NO_DATA:
		return BP_SCRAP_ERROR_NO_DATA;
	case BP_ERROR_ID_NOT_FOUND:
		return BP_SCRAP_ERROR_ID_NOT_FOUND;
	case BP_ERROR_DUPLICATED_ID:
		return BP_SCRAP_ERROR_DUPLICATED_ID;
	case BP_ERROR_PERMISSION_DENY:
		return BP_SCRAP_ERROR_PERMISSION_DENY;
	case BP_ERROR_DISK_FULL:
		return BP_SCRAP_ERROR_DISK_FULL;
	case BP_ERROR_DISK_BUSY:
		return BP_SCRAP_ERROR_DISK_BUSY;
	case BP_ERROR_TOO_BIG_DATA:
		return BP_SCRAP_ERROR_TOO_BIG_DATA;
	case BP_ERROR_UNKNOWN:
		return BP_SCRAP_ERROR_UNKNOWN;
	default:
		break;
	}
	return BP_SCRAP_ERROR_NONE;
}

int bp_scrap_adaptor_easy_create(int *id, bp_scrap_info_fmt *info)
{
	if (id == NULL || info == NULL)
		return -1;

	if (*id <= 0) { // new scrap
		if (bp_scrap_adaptor_create(id) < 0) {
			TRACE_ERROR("[failed to create new scrap]");
			return -1;
		}
		if (*id <= 0) {
			TRACE_ERROR("[failed to create new scrap]");
			return -1;
		}
	}

	if (info->date_created > 0)
		bp_scrap_adaptor_set_date_created(*id, info->date_created);
	if (info->date_modified > 0)
		bp_scrap_adaptor_set_date_modified(*id, info->date_modified);
	if (info->is_read >= 0)
		bp_scrap_adaptor_set_is_read(*id, info->is_read);
	if (info->is_reader >= 0)
		bp_scrap_adaptor_set_is_reader(*id, info->is_reader);
	if (info->is_night_mode >= 0)
		bp_scrap_adaptor_set_is_night_mode(*id, info->is_night_mode);
	if (info->url != NULL)
		bp_scrap_adaptor_set_url(*id, info->url);
	if (info->title != NULL)
		bp_scrap_adaptor_set_title(*id, info->title);
	if (info->page_path != NULL)
		bp_scrap_adaptor_set_page_path(*id, info->page_path);
	if (info->account_name != NULL)
		bp_scrap_adaptor_set_account_name(*id, info->account_name);
	if (info->account_type != NULL)
		bp_scrap_adaptor_set_account_type(*id, info->account_type);
	if (info->device_name != NULL)
		bp_scrap_adaptor_set_device_name(*id, info->device_name);
	if (info->device_id != NULL)
		bp_scrap_adaptor_set_device_id(*id, info->device_id);
	if (info->main_content != NULL)
		bp_scrap_adaptor_set_main_content(*id, info->main_content);
	if (info->favicon_length > 0 && info->favicon != NULL) {
		bp_scrap_adaptor_set_icon(*id, info->favicon_width,
			info->favicon_height, info->favicon, info->favicon_length);
	}
	return 0;
}

int bp_scrap_adaptor_easy_free(bp_scrap_info_fmt *info)
{
	if (info != NULL) {
		free(info->url);
		free(info->title);
		free(info->page_path);
		free(info->account_name);
		free(info->account_type);
		free(info->device_name);
		free(info->device_id);
		free(info->main_content);
		free(info->favicon);
		memset(info, 0x00, sizeof(bp_scrap_info_fmt));
	}
	return 0;
}

int bp_scrap_adaptor_get_inquired_ids_p
	(int **ids, int *count, const int limit, const int offset,
	const bp_scrap_offset order_column_offset, const int ordering,
	const char *keyword, const int is_like)
{
	return bp_scrap_adaptor_get_duplicated_ids_p
		(ids, count, limit, offset,
		order_column_offset, ordering,
		(BP_SCRAP_O_TITLE | BP_SCRAP_O_URL),
		keyword, is_like);
}

int bp_scrap_adaptor_get_duplicated_ids_p
	(int **ids, int *count, const int limit, const int offset,
	const bp_scrap_offset order_column_offset, const int ordering,
	const bp_scrap_offset check_column_offset,
	const char *keyword, const int is_like)
{
	if (ids == NULL || count == NULL)
		return -1;
	if (keyword == NULL || bp_common_precheck_string(keyword) < 0)
		return -1;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	bp_command_fmt *cmd = &g_bp_command;

	cmd->cmd = BP_CMD_COMMON_GET_CONDS_ORDER_IDS;
	cmd->id = -1;

	bp_db_base_conds_fmt conds;
	memset(&conds, 0x00, sizeof(bp_db_base_conds_fmt));
	conds.limit = limit;
	conds.offset = offset;
	conds.order_column_offset = order_column_offset;
	conds.ordering = ordering;

	errorcode = bp_ipc_simple_response(sock, cmd);
	if (errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(-1, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}

	bp_scrap_offset oflags = check_column_offset;
	if (bp_ipc_send_custom_type(sock, &conds,
			sizeof(bp_db_base_conds_fmt)) < 0 ||
			bp_ipc_send_custom_type(sock, &oflags,
			sizeof(bp_scrap_offset)) < 0 ||
			bp_adaptor_ipc_send_int(sock, is_like) < 0 ||
			bp_ipc_send_string(sock, keyword) < 0) {
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

int bp_scrap_adaptor_get_cond_ids_p
	(int **ids, int *count,
	bp_scrap_rows_cond_fmt *conds,
	const bp_scrap_offset check_offset,
	const char *keyword, const int is_like)
{
	if (ids == NULL || count == NULL)
		return -1;
	bp_scrap_rows_cond_fmt t_conds;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	bp_command_fmt *cmd = &g_bp_command;

	cmd->cmd = BP_CMD_COMMON_GET_CONDS_DATE_IDS;
	cmd->id = -1;

	memset(&t_conds, 0x00, sizeof(bp_scrap_rows_cond_fmt));

	if (conds != NULL)
		memcpy(&t_conds, conds, sizeof(bp_scrap_rows_cond_fmt));
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
	bp_scrap_offset oflags = check_offset;
	if (keyword == NULL)
		oflags = 0;
	if (bp_ipc_send_custom_type(sock, &t_conds,
			sizeof(bp_scrap_rows_cond_fmt)) < 0 ||
			bp_ipc_send_custom_type(sock, &oflags,
			sizeof(bp_scrap_offset)) < 0) {
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

int bp_scrap_adaptor_get_info(const int id,
	const bp_scrap_offset offset, bp_scrap_info_fmt *info)
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
	bp_scrap_offset oflags = offset;
	if (bp_ipc_send_custom_type(sock, &oflags,
			sizeof(bp_scrap_offset)) < 0) {
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
	bp_scrap_info_fmt scraps;
	memset(&scraps, 0x00, sizeof(bp_scrap_info_fmt));
	// getting bp_scrap_info_fmt from provider.
	if (bp_ipc_read_custom_type(sock, &scraps,
			sizeof(bp_scrap_info_fmt)) < 0) {
		errorcode = bp_ipc_check_stderr(BP_ERROR_IO_ERROR);
		BP_PRINT_ERROR(id, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	memset(info, 0x00, sizeof(bp_scrap_info_fmt));

	// fill info
	if (offset & BP_SCRAP_O_DATE_CREATED) {
		info->date_created = scraps.date_created;
	}
	if (offset & BP_SCRAP_O_DATE_MODIFIED) {
		info->date_modified = scraps.date_modified;
	}
	if (offset & BP_SCRAP_O_IS_READ) {
		info->is_read = scraps.is_read;
	}
	if (offset & BP_SCRAP_O_IS_READER) {
		info->is_reader = scraps.is_reader;
	}
	if (offset & BP_SCRAP_O_IS_NIGHT_MODE) {
		info->is_night_mode = scraps.is_night_mode;
	}
	// get strings . keep the order with provider
	if (offset & BP_SCRAP_O_PAGE) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->page_path = bp_ipc_read_string(sock);
	}
	if (offset & BP_SCRAP_O_URL) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->url = bp_ipc_read_string(sock);
	}
	if (offset & BP_SCRAP_O_TITLE) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->title = bp_ipc_read_string(sock);
	}
	if (offset & BP_SCRAP_O_ACCOUNT_NAME) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->account_name = bp_ipc_read_string(sock);
	}
	if (offset & BP_SCRAP_O_ACCOUNT_TYPE) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->account_type = bp_ipc_read_string(sock);
	}
	if (offset & BP_SCRAP_O_DEVICE_NAME) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->device_name = bp_ipc_read_string(sock);
	}
	if (offset & BP_SCRAP_O_DEVICE_ID) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->device_id = bp_ipc_read_string(sock);
	}
	if (offset & BP_SCRAP_O_MAIN_CONTENT) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->main_content = bp_ipc_read_string(sock);
	}
	if (offset & BP_SCRAP_O_FAVICON) {
		info->favicon_length = 0;
		info->favicon_width = scraps.favicon_width;
		info->favicon_height = scraps.favicon_height;
		bp_error_defs ret = bp_ipc_read_errorcode(sock);
		if (ret == BP_ERROR_NONE) {
			unsigned char *blob_data = NULL;
			if ((info->favicon_length =
					bp_common_adaptor_get_info_blob(sock, &blob_data,
						&g_adaptorinfo->shm)) > 0 && blob_data != NULL)
				info->favicon = blob_data;
		}
	}
	if (offset & BP_SCRAP_O_SNAPSHOT) {
		info->snapshot_length = 0;
		info->snapshot_width = scraps.snapshot_width;
		info->snapshot_height = scraps.snapshot_height;
		bp_error_defs ret = bp_ipc_read_errorcode(sock);
		if (ret == BP_ERROR_NONE) {
			unsigned char *blob_data = NULL;
			if ((info->snapshot_length =
					bp_common_adaptor_get_info_blob(sock, &blob_data,
						&g_adaptorinfo->shm)) > 0 && blob_data != NULL)
				info->snapshot = blob_data;
		}
	}

	pthread_mutex_unlock(&g_adaptor_mutex);
	return 0;
}
