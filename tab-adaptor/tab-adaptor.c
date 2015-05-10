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
#include <common-adaptor-png.h>
#include <tab-adaptor.h>
#include <browser-provider-db-defs.h>
#include <browser-provider-socket.h>

static bp_adaptor_defs *g_adaptorinfo = NULL;
static pthread_mutex_t g_adaptor_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t g_adaptor_event_thread_pid = 0;
static bp_tab_adaptor_data_changed_cb g_adaptor_noti_cb = NULL;
static void *g_adaptor_noti_user_data = NULL;
static bp_command_fmt g_bp_command;
static bp_error_defs errorcode = BP_ERROR_NONE;

/////// callback ///////////////

static void *__tab_adaptor_event_manager(void *arg)
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
			sleep(6); // workaround . wait terminating provider
			bp_tab_adaptor_set_data_changed_cb
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
		bp_client_type_defs client_type = BP_CLIENT_TABS;

#ifdef SUPPORT_CLOUD_SYSTEM
		if (bp_common_adaptor_is_sync_adaptor() == 0)
			client_type = BP_CLIENT_TABS_SYNC;
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
				__tab_adaptor_event_manager, g_adaptorinfo) != 0) {
			TRACE_STRERROR("[CRITICAL] pthread_create");
			return -1;
		}
		pthread_detach(g_adaptor_event_thread_pid);
		TRACE_DEBUG("pthread:%0x", (int)g_adaptor_event_thread_pid);
	}
	return 0;
}

static int __bp_tab_adaptor_get_ids_p
	(int **ids, int *count, bp_command_defs cmd)
{
	if (ids == NULL || count == NULL)
		return -1;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	g_bp_command.cmd = cmd;
	g_bp_command.id = -1;
	int ids_count = bp_common_adaptor_get_ids_p(sock, &g_bp_command, ids, &errorcode);
	if (ids_count < 0 && errorcode == BP_ERROR_IO_ERROR)
		__browser_adaptor_disconnect();
	pthread_mutex_unlock(&g_adaptor_mutex);
	if (ids_count < 0)
		return -1;
	*count = ids_count;
	return 0;
}

static int __bp_tab_adaptor_get_string
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

static int __bp_tab_adaptor_get_int
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

static int __bp_tab_adaptor_set_string
	(const int id, bp_command_defs cmd, const char *value)
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

static int __bp_tab_adaptor_set_int
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

static int __bp_tab_adaptor_send_cmd(const int id, bp_command_defs cmd)
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

static int __bp_tab_adaptor_send_one_way(const int id, bp_command_defs cmd)
{
	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	g_bp_command.cmd = cmd;
	g_bp_command.id = id;
	int ret = bp_ipc_send_command(sock, &g_bp_command);
	pthread_mutex_unlock(&g_adaptor_mutex);
	return ret;
}

static int __bp_tab_adaptor_get_blob_shm(const int id,
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

static int __bp_tab_adaptor_set_blob_shm(const int id,
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

/////////////////////// APIs /////////////////////////////////
// return
// 0 : Success
// -1 : Failed

// launch browser-provider, connect to browser-provider
int bp_tab_adaptor_initialize(void)
{
	pthread_mutex_lock(&g_adaptor_mutex);
	int ret = __browser_adaptor_connect(1);
	pthread_mutex_unlock(&g_adaptor_mutex);
	return ret;
}

// disconnect
int bp_tab_adaptor_deinitialize(void)
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
int bp_tab_adaptor_get_full_ids_p(int **ids, int *count)
{
	return __bp_tab_adaptor_get_ids_p
		(ids, count, BP_CMD_COMMON_GET_FULL_IDS);
}

int bp_tab_adaptor_get_full_with_deleted_ids_p(int **ids, int *count)
{
	return __bp_tab_adaptor_get_ids_p
		(ids, count, BP_CMD_COMMON_GET_FULL_WITH_DELETED_IDS);
}
int bp_tab_adaptor_get_dirty_ids_p(int **ids, int *count)
{
	return __bp_tab_adaptor_get_ids_p
		(ids, count, BP_CMD_COMMON_GET_DIRTY_IDS);
}

int bp_tab_adaptor_get_deleted_ids_p(int **ids, int *count)
{
	return __bp_tab_adaptor_get_ids_p
		(ids, count, BP_CMD_COMMON_GET_DELETED_IDS);
}

int bp_tab_adaptor_clear_dirty_ids(void)
{
	return __bp_tab_adaptor_send_cmd(-1, BP_CMD_COMMON_CLEAR_DIRTY_IDS);
}

int bp_tab_adaptor_clear_deleted_ids(void)
{
	return __bp_tab_adaptor_send_cmd(-1, BP_CMD_COMMON_CLEAR_DELETED_IDS);
}

int bp_tab_adaptor_set_data_changed_cb(bp_tab_adaptor_data_changed_cb callback, void *user_data)
{
	if (callback == NULL) {
		TRACE_ERROR("check callback address:%p", callback);
		return -1;
	}
	g_adaptor_noti_user_data = NULL;
	g_adaptor_noti_cb = callback;
	g_adaptor_noti_user_data = user_data;
	return __bp_tab_adaptor_send_cmd(-1, BP_CMD_SET_NOTI_CB);
}

int bp_tab_adaptor_unset_data_changed_cb(bp_tab_adaptor_data_changed_cb callback)
{
	if (callback != NULL && callback == g_adaptor_noti_cb) {
		g_adaptor_noti_cb = NULL;
		g_adaptor_noti_user_data = NULL;
		return __bp_tab_adaptor_send_cmd(-1, BP_CMD_UNSET_NOTI_CB);
	}
	return -1;
}

int bp_tab_adaptor_is_setted_data_changed_cb(void)
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

int bp_tab_adaptor_set_dirty(const int id)
{
	if (id < 0)
		return -1;
	return __bp_tab_adaptor_send_one_way(id, BP_CMD_COMMON_SET_DIRTY);
}

int bp_tab_adaptor_set_deleted(const int id)
{
	if (id < 0)
		return -1;
	return __bp_tab_adaptor_send_cmd(id, BP_CMD_COMMON_SET_IS_DELETED);
}

int bp_tab_adaptor_publish_notification(void)
{
	return __bp_tab_adaptor_send_one_way(-1, BP_CMD_COMMON_NOTI);
}

int bp_tab_adaptor_get_index(const int id, int *value)
{
	return __bp_tab_adaptor_get_int(id, BP_CMD_TABS_GET_INDEX, value);
}

int bp_tab_adaptor_get_url(const int id, char **value)
{
	return __bp_tab_adaptor_get_string(id, BP_CMD_COMMON_GET_URL, value);
}

int bp_tab_adaptor_get_title(const int id, char **value)
{
	return __bp_tab_adaptor_get_string(id, BP_CMD_COMMON_GET_TITLE, value);
}

// blob
int bp_tab_adaptor_get_icon(const int id, int *width,
	int *height, unsigned char **value, int *length)
{
	return __bp_tab_adaptor_get_blob_shm
		(id, BP_CMD_COMMON_GET_ICON, width, height, value, length);
}

int bp_tab_adaptor_get_icon_png(const int id,
	unsigned char **value, int *length)
{
	unsigned char *raw_data = NULL;
	int raw_length = 0;
	int width = 0;
	int height = 0;
	int ret = bp_tab_adaptor_get_icon(id, &width, &height, &raw_data,
			&raw_length);
	if (ret == 0 && width > 0 && height > 0 && raw_length > 0 &&
			raw_data != NULL)
		ret = bp_common_raw_to_png(raw_data, width, height, value,
				length);
	return ret;
}

int bp_tab_adaptor_get_snapshot(const int id, int *width,
	int *height, unsigned char **value, int *length)
{
	return __bp_tab_adaptor_get_blob_shm
		(id, BP_CMD_COMMON_GET_SNAPSHOT, width, height, value, length);
}

int bp_tab_adaptor_get_activated(const int id, int *value)
{
	return __bp_tab_adaptor_get_int(id, BP_CMD_TABS_GET_ACTIVATED, value);
}

int bp_tab_adaptor_get_incognito(const int id, int *value)
{
	return __bp_tab_adaptor_get_int(id, BP_CMD_TABS_GET_INCOGNITO, value);
}

int bp_tab_adaptor_get_browser_instance(const int id, int *value)
{
	return __bp_tab_adaptor_get_int(id, BP_CMD_TABS_GET_INCOGNITO, value);
}

int bp_tab_adaptor_get_date_created(const int id, int *value)
{
	return __bp_tab_adaptor_get_int
		(id, BP_CMD_COMMON_GET_DATE_CREATED, value);
}

int bp_tab_adaptor_get_date_modified(const int id, int *value)
{
	return __bp_tab_adaptor_get_int
		(id, BP_CMD_COMMON_GET_DATE_MODIFIED, value);
}

int bp_tab_adaptor_get_account_name(const int id, char **value)
{
	return __bp_tab_adaptor_get_string
		(id, BP_CMD_COMMON_GET_ACCOUNT_NAME, value);
}

int bp_tab_adaptor_get_account_type(const int id, char **value)
{
	return __bp_tab_adaptor_get_string
		(id, BP_CMD_COMMON_GET_ACCOUNT_TYPE, value);
}

int bp_tab_adaptor_get_device_name(const int id, char **value)
{
	return __bp_tab_adaptor_get_string
		(id, BP_CMD_COMMON_GET_DEVICE_NAME, value);
}

int bp_tab_adaptor_get_device_id(const int id, char **value)
{
	return __bp_tab_adaptor_get_string
		(id, BP_CMD_COMMON_GET_DEVICE_ID, value);
}

int bp_tab_adaptor_get_usage(const int id, char **value)
{
	return __bp_tab_adaptor_get_string
		(id, BP_CMD_TABS_GET_USAGE, value);
}

int bp_tab_adaptor_get_sync(const int id, char **value)
{
	return __bp_tab_adaptor_get_string
		(id, BP_CMD_COMMON_GET_SYNC, value);
}

int bp_tab_adaptor_set_index(const int id, const int value)
{
	return __bp_tab_adaptor_set_int(id, BP_CMD_TABS_SET_INDEX, value);
}

int bp_tab_adaptor_set_url(const int id, const char *value)
{
	return __bp_tab_adaptor_set_string(id, BP_CMD_COMMON_SET_URL, value);
}

int bp_tab_adaptor_set_title(const int id, const char *value)
{
	return __bp_tab_adaptor_set_string(id, BP_CMD_COMMON_SET_TITLE, value);
}

int bp_tab_adaptor_set_icon(const int id, const int width,
	const int height, const unsigned char *value, const int length)
{
	return __bp_tab_adaptor_set_blob_shm
		(id, BP_CMD_COMMON_SET_ICON, width, height, value, length);
}

int bp_tab_adaptor_set_icon_png(const int id,
	const unsigned char *value)
{
	if (value == NULL)
		return -1;
	unsigned char *raws_buffer = NULL;
	int raws_length = 0;
	int width = 0;
	int height = 0;
	int ret = bp_common_png_to_raw(value, &raws_buffer, &width, &height,
			&raws_length);
	if (ret == 0)
		ret = bp_tab_adaptor_set_icon(id, width, height, raws_buffer,
				raws_length);
	free(raws_buffer);
	return ret;
}

int bp_tab_adaptor_set_snapshot(const int id, const int width,
	const int height, const unsigned char *value, const int length)
{
	return __bp_tab_adaptor_set_blob_shm
		(id, BP_CMD_COMMON_SET_SNAPSHOT, width, height, value, length);
}

int bp_tab_adaptor_set_activated(const int id, const int value)
{
	return __bp_tab_adaptor_set_int
		(id, BP_CMD_TABS_SET_ACTIVATED, value);
}

int bp_tab_adaptor_set_incognito(const int id, const int value)
{
	return __bp_tab_adaptor_set_int
		(id, BP_CMD_TABS_SET_INCOGNITO, value);
}

int bp_tab_adaptor_set_browser_instance(const int id, const int value)
{
	return __bp_tab_adaptor_set_int
		(id, BP_CMD_TABS_SET_BROWSER_INSTANCE, value);
}

int bp_tab_adaptor_set_date_created(const int id, const int value)
{
	return __bp_tab_adaptor_set_int
		(id, BP_CMD_COMMON_SET_DATE_CREATED, value);
}

int bp_tab_adaptor_set_date_modified(const int id, const int value)
{
	return __bp_tab_adaptor_set_int
		(id, BP_CMD_COMMON_SET_DATE_MODIFIED, value);
}

int bp_tab_adaptor_set_account_name(const int id, const char *value)
{
	return __bp_tab_adaptor_set_string
		(id, BP_CMD_COMMON_SET_ACCOUNT_NAME, value);
}

int bp_tab_adaptor_set_account_type(const int id, const char *value)
{
	return __bp_tab_adaptor_set_string
		(id, BP_CMD_COMMON_SET_ACCOUNT_TYPE, value);
}

int bp_tab_adaptor_set_device_name(const int id, const char *value)
{
	return __bp_tab_adaptor_set_string
		(id, BP_CMD_COMMON_SET_DEVICE_NAME, value);
}

int bp_tab_adaptor_set_device_id(const int id, const char *value)
{
	return __bp_tab_adaptor_set_string
		(id, BP_CMD_COMMON_SET_DEVICE_ID, value);
}

int bp_tab_adaptor_set_usage(const int id, const char *value)
{
	return __bp_tab_adaptor_set_string(id, BP_CMD_TABS_SET_USAGE, value);
}

int bp_tab_adaptor_set_sync(const int id, const char *value)
{
	return __bp_tab_adaptor_set_string(id, BP_CMD_COMMON_SET_SYNC, value);
}

int bp_tab_adaptor_create(int *id)
{
	int tab_id = 0;

	if (id == NULL)
		return -1;

	if (*id < 0)
		*id = -1; // create new tab

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
	tab_id = bp_adaptor_ipc_read_int(sock);
	if (tab_id < 0) {
		errorcode = bp_ipc_check_stderr(BP_ERROR_IO_ERROR);
		BP_PRINT_ERROR(*id, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	pthread_mutex_unlock(&g_adaptor_mutex);
	if (*id == tab_id) {
		TRACE_INFO("created a tab:%d", *id);
	} else {
		*id = tab_id;
		TRACE_INFO("created new tab:%d", *id);
	}
	return 0;
}

int bp_tab_adaptor_delete(const int id)
{
	if (id < 0) {
		TRACE_ERROR("[CHECK id][%d]", id);
		return -1;
	}
	return __bp_tab_adaptor_send_cmd(id, BP_CMD_COMMON_DELETE);
}

int bp_tab_adaptor_get_errorcode(void)
{
	switch(errorcode) {
	case BP_ERROR_INVALID_PARAMETER:
		return BP_TAB_ERROR_INVALID_PARAMETER;
	case BP_ERROR_OUT_OF_MEMORY:
		return BP_TAB_ERROR_OUT_OF_MEMORY;
	case BP_ERROR_IO_EINTR:
	case BP_ERROR_IO_EAGAIN:
	case BP_ERROR_IO_ERROR:
		return BP_TAB_ERROR_IO_ERROR;
	case BP_ERROR_NO_DATA:
		return BP_TAB_ERROR_NO_DATA;
	case BP_ERROR_ID_NOT_FOUND:
		return BP_TAB_ERROR_ID_NOT_FOUND;
	case BP_ERROR_DUPLICATED_ID:
		return BP_TAB_ERROR_DUPLICATED_ID;
	case BP_ERROR_PERMISSION_DENY:
		return BP_TAB_ERROR_PERMISSION_DENY;
	case BP_ERROR_DISK_FULL:
		return BP_TAB_ERROR_DISK_FULL;
	case BP_ERROR_DISK_BUSY:
		return BP_TAB_ERROR_DISK_BUSY;
	case BP_ERROR_TOO_BIG_DATA:
		return BP_TAB_ERROR_TOO_BIG_DATA;
	case BP_ERROR_UNKNOWN:
		return BP_TAB_ERROR_UNKNOWN;
	default:
		break;
	}
	return BP_TAB_ERROR_NONE;
}

int bp_tab_adaptor_easy_create(int *id, bp_tab_info_fmt *info)
{
	if (id == NULL || info == NULL)
		return -1;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	errorcode = BP_ERROR_NONE;
	bp_tab_base_fmt new_base_tab;
	memset(&new_base_tab, 0x00, sizeof(bp_tab_base_fmt));
	new_base_tab.index = info->index;
	new_base_tab.is_activated = info->is_activated;
	new_base_tab.is_incognito = info->is_incognito;
	new_base_tab.browser_instance = info->browser_instance;
	new_base_tab.date_created = info->date_created;
	new_base_tab.date_modified = info->date_modified;

	// make offset for strings
	bp_tab_offset offset = 0;

	if (bp_common_precheck_string(info->url) > 0)
		offset = offset | BP_TAB_O_URL;
	if (bp_common_precheck_string(info->title) > 0)
		offset = offset | BP_TAB_O_TITLE;
	if (bp_common_precheck_string(info->account_name) > 0)
		offset = offset | BP_TAB_O_ACCOUNT_NAME;
	if (bp_common_precheck_string(info->account_type) > 0)
		offset = offset | BP_TAB_O_ACCOUNT_TYPE;
	if (bp_common_precheck_string(info->device_name) > 0)
		offset = offset | BP_TAB_O_DEVICE_NAME;
	if (bp_common_precheck_string(info->device_id) > 0)
		offset = offset | BP_TAB_O_DEVICE_ID;
	if (bp_common_precheck_string(info->usage) > 0)
		offset = offset | BP_TAB_O_USAGE;
	if (bp_common_precheck_string(info->sync) > 0)
		offset = offset | BP_TAB_O_SYNC;

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
	if (bp_ipc_send_custom_type(sock, &new_base_tab,
			sizeof(bp_tab_base_fmt)) < 0 ||
			bp_ipc_send_custom_type(sock, &offset,
			sizeof(bp_tab_offset)) < 0) {
		errorcode = BP_ERROR_IO_ERROR;
		BP_PRINT_ERROR(*id, errorcode);
		__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	if (offset & BP_TAB_O_URL) {
		if (bp_ipc_send_string(sock, info->url) < 0) {
			errorcode = BP_ERROR_IO_ERROR;
			BP_PRINT_ERROR(*id, errorcode);
			__browser_adaptor_disconnect();
			pthread_mutex_unlock(&g_adaptor_mutex);
			return -1;
		}
	}
	if ((offset & BP_TAB_O_TITLE)) {
		if (bp_ipc_send_string(sock, info->title) < 0) {
			errorcode = BP_ERROR_IO_ERROR;
			BP_PRINT_ERROR(*id, errorcode);
			__browser_adaptor_disconnect();
			pthread_mutex_unlock(&g_adaptor_mutex);
			return -1;
		}
	}
	if (offset & BP_TAB_O_ACCOUNT_NAME) {
		if (bp_ipc_send_string(sock, info->account_name) < 0) {
			errorcode = BP_ERROR_IO_ERROR;
			BP_PRINT_ERROR(*id, errorcode);
			__browser_adaptor_disconnect();
			pthread_mutex_unlock(&g_adaptor_mutex);
			return -1;
		}
	}
	if (offset & BP_TAB_O_ACCOUNT_TYPE) {
		if (bp_ipc_send_string(sock, info->account_type) < 0) {
			errorcode = BP_ERROR_IO_ERROR;
			BP_PRINT_ERROR(*id, errorcode);
			__browser_adaptor_disconnect();
			pthread_mutex_unlock(&g_adaptor_mutex);
			return -1;
		}
	}
	if (offset & BP_TAB_O_DEVICE_NAME) {
		if (bp_ipc_send_string(sock, info->device_name) < 0) {
			errorcode = BP_ERROR_IO_ERROR;
			BP_PRINT_ERROR(*id, errorcode);
			__browser_adaptor_disconnect();
			pthread_mutex_unlock(&g_adaptor_mutex);
			return -1;
		}
	}
	if (offset & BP_TAB_O_DEVICE_ID) {
		if (bp_ipc_send_string(sock, info->device_id) < 0) {
			errorcode = BP_ERROR_IO_ERROR;
			BP_PRINT_ERROR(*id, errorcode);
			__browser_adaptor_disconnect();
			pthread_mutex_unlock(&g_adaptor_mutex);
			return -1;
		}
	}
	if (offset & BP_TAB_O_USAGE) {
		if (bp_ipc_send_string(sock, info->usage) < 0) {
			errorcode = BP_ERROR_IO_ERROR;
			BP_PRINT_ERROR(*id, errorcode);
			__browser_adaptor_disconnect();
			pthread_mutex_unlock(&g_adaptor_mutex);
			return -1;
		}
	}
	if (offset & BP_TAB_O_SYNC) {
		if (bp_ipc_send_string(sock, info->sync) < 0) {
			errorcode = BP_ERROR_IO_ERROR;
			BP_PRINT_ERROR(*id, errorcode);
			__browser_adaptor_disconnect();
			pthread_mutex_unlock(&g_adaptor_mutex);
			return -1;
		}
	}

	// 4. wait id from provider.
	errorcode = bp_ipc_read_errorcode(sock);
	if (errorcode != BP_ERROR_NONE) {
		BP_PRINT_ERROR(*id, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	int tab_id = bp_adaptor_ipc_read_int(sock);
	if (tab_id < 0) {
		errorcode = bp_ipc_check_stderr(BP_ERROR_IO_ERROR);
		BP_PRINT_ERROR(*id, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	pthread_mutex_unlock(&g_adaptor_mutex);
	if (*id == tab_id) {
		TRACE_INFO("updated a tab:%d", *id);
	} else {
		*id = tab_id;
		TRACE_INFO("created new tab:%d", *id);
	}

	if (info->favicon_length > 0 && info->favicon != NULL) {
		bp_tab_adaptor_set_icon(*id, info->favicon_width,
			info->favicon_height, info->favicon, info->favicon_length);
	}
	if (info->thumbnail_length > 0 && info->thumbnail != NULL) {
		bp_tab_adaptor_set_snapshot(*id, info->thumbnail_width,
			info->thumbnail_height, info->thumbnail, info->thumbnail_length);
	}
	return 0;
}

int bp_tab_adaptor_get_easy_all(const int id, bp_tab_info_fmt *info)
{
	return bp_tab_adaptor_get_info(id, BP_TAB_O_ALL, info);
}

int bp_tab_adaptor_get_info(const int id, const bp_tab_offset offset,
	bp_tab_info_fmt *info)
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
	// 1. send bp_tab_base_fmt structure
	bp_tab_offset oflags = offset;
	if (bp_ipc_send_custom_type(sock, &oflags,
			sizeof(bp_tab_offset)) < 0) {
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
	bp_tab_base_fmt tabinfo;
	memset(&tabinfo, 0x00, sizeof(bp_tab_base_fmt));
	// getting bp_tab_base_fmt from provider.
	if (bp_ipc_read_custom_type(sock, &tabinfo,
			sizeof(bp_tab_base_fmt)) < 0) {
		errorcode = bp_ipc_check_stderr(BP_ERROR_IO_ERROR);
		BP_PRINT_ERROR(id, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	memset(info, 0x00, sizeof(bp_tab_info_fmt));

	// fill info
	if (offset & BP_TAB_O_INDEX) {
		info->index = tabinfo.index;
	}
	if (offset & BP_TAB_O_IS_ACTIVATED) {
		info->is_activated = tabinfo.is_activated;
	}
	if (offset & BP_TAB_O_IS_INCOGNITO) {
		info->is_incognito = tabinfo.is_incognito;
	}
	if (offset & BP_TAB_O_BROWSER_INSTANCE) {
		info->browser_instance = tabinfo.browser_instance;
	}
	if (offset & BP_TAB_O_DATE_CREATED) {
		info->date_created = tabinfo.date_created;
	}
	if (offset & BP_TAB_O_DATE_MODIFIED) {
		info->date_modified = tabinfo.date_modified;
	}
	// get strings . keep the order with provider
	if (offset & BP_TAB_O_URL) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->url = bp_ipc_read_string(sock);
	}
	if (offset & BP_TAB_O_TITLE) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->title = bp_ipc_read_string(sock);
	}
	if (offset & BP_TAB_O_ACCOUNT_NAME) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->account_name = bp_ipc_read_string(sock);
	}
	if (offset & BP_TAB_O_ACCOUNT_TYPE) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->account_type = bp_ipc_read_string(sock);
	}
	if (offset & BP_TAB_O_DEVICE_NAME) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->device_name = bp_ipc_read_string(sock);
	}
	if (offset & BP_TAB_O_DEVICE_ID) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->device_id = bp_ipc_read_string(sock);
	}
	if (offset & BP_TAB_O_USAGE) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->usage = bp_ipc_read_string(sock);
	}
	if (offset & BP_TAB_O_SYNC) {
		if (bp_ipc_read_errorcode(sock) == BP_ERROR_NONE)
			info->sync = bp_ipc_read_string(sock);
	}
	if (offset & BP_TAB_O_ICON) {
		info->favicon_length = 0;
		info->favicon_width = tabinfo.favicon_width;
		info->favicon_height = tabinfo.favicon_height;
		bp_error_defs ret = bp_ipc_read_errorcode(sock);
		if (ret == BP_ERROR_NONE) {
			unsigned char *blob_data = NULL;
			if ((info->favicon_length =
					bp_common_adaptor_get_info_blob(sock, &blob_data,
						&g_adaptorinfo->shm)) > 0 && blob_data != NULL)
				info->favicon = blob_data;
		}
	}
	if (offset & BP_TAB_O_SNAPSHOT) {
		info->thumbnail_length = 0;
		info->thumbnail_width = tabinfo.thumbnail_width;
		info->thumbnail_height = tabinfo.thumbnail_height;
		bp_error_defs ret = bp_ipc_read_errorcode(sock);
		if (ret == BP_ERROR_NONE) {
			unsigned char *blob_data = NULL;
			if ((info->thumbnail_length =
					bp_common_adaptor_get_info_blob(sock, &blob_data,
						&g_adaptorinfo->shm)) > 0 && blob_data != NULL)
				info->thumbnail = blob_data;
		}
	}

	pthread_mutex_unlock(&g_adaptor_mutex);
	return 0;
}

int bp_tab_adaptor_easy_free(bp_tab_info_fmt *info)
{
	if (info != NULL) {
		free(info->url);
		free(info->title);
		free(info->account_name);
		free(info->account_type);
		free(info->device_name);
		free(info->device_id);
		free(info->usage);
		free(info->sync);
		free(info->favicon);
		memset(info, 0x00, sizeof(bp_tab_info_fmt));
	}
	return 0;
}

EXPORT_API int bp_tab_adaptor_activate(const int id)
{
	if (id < 0) {
		TRACE_ERROR("[CHECK id][%d]", id);
		return -1;
	}
	return __bp_tab_adaptor_send_cmd(id, BP_CMD_TABS_ACTIVATE);
}

int bp_tab_adaptor_get_duplicated_ids_p
	(int **ids, int *count, const int limit, const int offset,
	const bp_tab_offset order_column_offset, const int ordering,
	const bp_tab_offset check_column_offset,
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
	bp_tab_offset oflags = check_column_offset;
	if (bp_ipc_send_custom_type(sock, &conds,
			sizeof(bp_db_base_conds_fmt)) < 0 ||
			bp_ipc_send_custom_type(sock, &oflags,
			sizeof(bp_tab_offset)) < 0 ||
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
