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

#include <bookmark-csc-adaptor.h>
#include <common-adaptor.h>
#include <browser-provider.h>
#include <browser-provider-db-defs.h>
#include <browser-provider-socket.h>

#include <bookmark-adaptor.h>

#define BP_CSC_CHECK_PROVIDER_STATUS do {\
	if (bookmark_csc_initialize() < 0) {\
		TRACE_ERROR("[CHECK connection]");\
		return -1;\
	}\
} while(0)

static bp_adaptor_defs *g_adaptorinfo = NULL;
static pthread_mutex_t g_adaptor_mutex = PTHREAD_MUTEX_INITIALIZER;
static bp_command_fmt g_bp_command;
static bp_error_defs errorcode = BP_ERROR_NONE;

// disconnect
static int __browser_adaptor_disconnect(void)
{
	return bp_common_adaptor_disconnect(&g_adaptorinfo, NULL);
}

static int __bookmark_csc_get_string(const long long int id,
	bp_command_defs cmd, char **value)
{
	if (id < 0) {
		TRACE_ERROR("[CHECK id][%d]", id);
		return -1;
	}
	if (value == NULL) {
		TRACE_ERROR("[CHECK value]");
		return -1;
	}

	BP_CSC_CHECK_PROVIDER_STATUS;

	pthread_mutex_lock(&g_adaptor_mutex);
	int sock = BP_CHECK_IPC_SOCK;
	g_bp_command.cmd = cmd;
	g_bp_command.id = id;
	int ret = bp_common_adaptor_get_string(sock, &g_bp_command, value, &errorcode);
	if (ret < 0 && errorcode == BP_ERROR_IO_ERROR)
		__browser_adaptor_disconnect();
	pthread_mutex_unlock(&g_adaptor_mutex);
	return ret;
}

static int __bookmark_csc_set_string(const long long int id,
	bp_command_defs cmd, const char *value)
{
	if (value == NULL || strlen(value) <= 0) {
		TRACE_ERROR("[CHECK value]");
		return -1;
	}

	BP_CSC_CHECK_PROVIDER_STATUS;

	pthread_mutex_lock(&g_adaptor_mutex);
	int sock = BP_CHECK_IPC_SOCK;
	g_bp_command.cmd = cmd;
	g_bp_command.id = id;
	int ret = bp_common_adaptor_set_string(sock, &g_bp_command, value, &errorcode);
	if (ret < 0 && errorcode == BP_ERROR_IO_ERROR)
		__browser_adaptor_disconnect();
	pthread_mutex_unlock(&g_adaptor_mutex);
	return ret;
}

static int __bp_bookmark_csc_get_ids_p
	(int **ids, int *count, const int limit, const int offset,
	const int parent, const int type,
	const int is_operator, const int is_editable,
	const bp_bookmark_offset order_offset, const int ordering)
{
	if (ids == NULL || count == NULL)
		return -1;

	BP_CSC_CHECK_PROVIDER_STATUS;

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
	}
	pthread_mutex_unlock(&g_adaptor_mutex);
	return 0;
}

int __bp_bookmark_csc_get_retrieved_ids_p
	(int **ids, int *count, const int limit, const int offset,
	const int parent, const int type,
	const int is_operator, const int is_editable,
	const bp_bookmark_offset order_column_offset, const int ordering,
	const bp_bookmark_offset check_column_offset,
	const char *keyword, const int is_like)
{
	if (keyword == NULL || bp_common_precheck_string(keyword) < 0)
		return -1;

	BP_CSC_CHECK_PROVIDER_STATUS;

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

	bp_bookmark_offset oflags = check_column_offset;
	if (bp_ipc_send_custom_type(sock, &conds,
			sizeof(bp_db_base_conds_fmt)) < 0 ||
			bp_adaptor_ipc_send_int(sock, parent) < 0 ||
			bp_adaptor_ipc_send_int(sock, type) < 0 ||
			bp_adaptor_ipc_send_int(sock, is_operator) < 0 ||
			bp_adaptor_ipc_send_int(sock, is_editable) < 0 ||
			bp_ipc_send_custom_type(sock, &oflags,
				sizeof(bp_bookmark_offset)) < 0 ||
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

/////////////////////// APIs /////////////////////////////////
// return
// 0 : Success
// -1 : Failed

// launch browser-provider, connect to browser-provider
int bookmark_csc_initialize(void)
{
	pthread_mutex_lock(&g_adaptor_mutex);

	if (g_adaptorinfo == NULL) {

		if (bp_common_adaptor_connect_to_provider
				(&g_adaptorinfo, BP_CLIENT_BOOKMARK_CSC) != 0) {
			pthread_mutex_unlock(&g_adaptor_mutex);
			return -1;
		}
		g_bp_command.cmd = BP_CMD_NONE;
		g_bp_command.id = -1;
		g_bp_command.cid = g_adaptorinfo->cid;
	}
	pthread_mutex_unlock(&g_adaptor_mutex);
	return 0;
}

// disconnect
int bookmark_csc_deinitialize(void)
{
	BP_CSC_CHECK_PROVIDER_STATUS;

	pthread_mutex_lock(&g_adaptor_mutex);

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

int bookmark_csc_get_root(int *id)
{
	if (id == NULL)
		return -1;
	*id = WEB_BOOKMARK_ROOT_ID;
	return 0;
}

int bookmark_csc_delete(const long long int id)
{
	if (id < 0) {
		TRACE_ERROR("[CHECK id][%d]", id);
		return -1;
	}

	BP_CSC_CHECK_PROVIDER_STATUS;

	pthread_mutex_lock(&g_adaptor_mutex);
	int sock = BP_CHECK_IPC_SOCK;
	errorcode = BP_ERROR_NONE;
	g_bp_command.cmd = BP_CMD_COMMON_DELETE;
	g_bp_command.id = id;
	errorcode = bp_ipc_simple_response(sock, &g_bp_command);
	if (errorcode == BP_ERROR_IO_ERROR)
		__browser_adaptor_disconnect();
	pthread_mutex_unlock(&g_adaptor_mutex);
	if (errorcode != BP_ERROR_NONE)
		return -1;
	return 0;
}

int bookmark_csc_get_errorcode(void)
{
	switch(errorcode) {
	case BP_ERROR_INVALID_PARAMETER:
		return BOOKMARK_CSC_ERROR_INVALID_PARAMETER;
	case BP_ERROR_OUT_OF_MEMORY:
		return BOOKMARK_CSC_ERROR_OUT_OF_MEMORY;
	case BP_ERROR_IO_EINTR:
	case BP_ERROR_IO_ERROR:
		return BOOKMARK_CSC_ERROR_IO_ERROR;
	case BP_ERROR_NO_DATA:
		return BOOKMARK_CSC_ERROR_NO_DATA;
	case BP_ERROR_ID_NOT_FOUND:
		return BOOKMARK_CSC_ERROR_ID_NOT_FOUND;
	case BP_ERROR_DUPLICATED_ID:
		return BOOKMARK_CSC_ERROR_DUPLICATED_ID;
	case BP_ERROR_PERMISSION_DENY:
		return BOOKMARK_CSC_ERROR_PERMISSION_DENY;
	case BP_ERROR_DISK_FULL:
		return BOOKMARK_CSC_ERROR_DISK_FULL;
	case BP_ERROR_DISK_BUSY:
		return BOOKMARK_CSC_ERROR_DISK_BUSY;
	case BP_ERROR_TOO_BIG_DATA:
		return BOOKMARK_CSC_ERROR_TOO_BIG_DATA;
	case BP_ERROR_UNKNOWN:
		return BOOKMARK_CSC_ERROR_UNKNOWN;
	default:
		break;
	}
	return BOOKMARK_CSC_ERROR_NONE;
}

int bookmark_csc_create(int *id, bookmark_csc_info_fmt *info)
{
	int ret = 0;
	int rootid = -1;

	if (id == NULL || info == NULL) {
		TRACE_ERROR("[CHECK params]");
		return -1;
	}

	if (info->type != BOOKMARK_CSC_TYPE_BOOKMARK &&
			info->type != BOOKMARK_CSC_TYPE_FOLDER) {
		TRACE_ERROR("[CHECK type] %d", info->type);
		return -1;
	}
	if (bookmark_csc_get_root(&rootid) < 0)
		return -1;

	if (rootid == *id) {
		TRACE_ERROR("invalid ID refer bookmark_csc_get_root");
		return -1;
	}

	BP_CSC_CHECK_PROVIDER_STATUS;

	pthread_mutex_lock(&g_adaptor_mutex);
	int sock = BP_CHECK_IPC_SOCK;
	errorcode = BP_ERROR_NONE;
	bp_bookmark_csc_base_fmt base_info;
	memset(&base_info, 0x00, sizeof(bp_bookmark_csc_base_fmt));
	base_info.type = info->type;
	base_info.parent = info->parent;
	base_info.editable = info->editable;

	g_bp_command.cmd = BP_CMD_COMMON_SET_EASY_ALL;
	g_bp_command.id = *id;

	// send command without waiting return value
	errorcode = bp_ipc_simple_response(sock, &g_bp_command);
	if (errorcode != BP_ERROR_NONE) {
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	// 1. send bp_bookmark_csc_base_fmt structure
	if (bp_ipc_send_custom_type(sock, &base_info,
			sizeof(bp_bookmark_csc_base_fmt)) < 0) {
		__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	// 2. wait id from provider.
	errorcode = bp_ipc_read_errorcode(sock);
	if (errorcode != BP_ERROR_NONE) {
		TRACE_ERROR("[CHECK ERROR] (%d) [%d]", id, errorcode);
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	int bookmark_id = bp_adaptor_ipc_read_int64(sock);
	if (bookmark_id < 0) {
		if (bp_ipc_check_stderr(BP_ERROR_IO_ERROR) == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}
	pthread_mutex_unlock(&g_adaptor_mutex);
	if (*id == bookmark_id) {
		TRACE_INFO("created a csc bookmark:%d", *id);
	} else {
		*id = bookmark_id;
		TRACE_INFO("created new csc bookmark:%d", *id);
	}

	// 3. set strings
	if (info->title != NULL)
		ret = __bookmark_csc_set_string(*id, BP_CMD_COMMON_SET_TITLE, info->title);
	if (info->url != NULL && info->type == BOOKMARK_CSC_TYPE_BOOKMARK)
		ret = __bookmark_csc_set_string(*id, BP_CMD_COMMON_SET_URL, info->url);
	return ret;
}

int bookmark_csc_get_info(const long long int id, bookmark_csc_info_fmt *info)
{
	// get bp_bookmark_base_fmt from provider
	// need each call for string
	bp_bookmark_csc_base_fmt base_info;

	if (id < 0) {
		TRACE_ERROR("[CHECK id][%d]", id);
		return -1;
	}
	if (info == NULL) {
		TRACE_ERROR("[CHECK bookmark_csc_info_fmt][%d]", id);
		return -1;
	}

	BP_CSC_CHECK_PROVIDER_STATUS;

	pthread_mutex_lock(&g_adaptor_mutex);
	int sock = BP_CHECK_IPC_SOCK;
	errorcode = BP_ERROR_NONE;
	g_bp_command.cmd = BP_CMD_CSC_BOOKMARK_GET_ALL;
	g_bp_command.id = id;
	errorcode = bp_ipc_simple_response(sock, &g_bp_command);
	if (errorcode != BP_ERROR_NONE) {
		if (errorcode == BP_ERROR_IO_ERROR)
			__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}

	memset(&base_info, 0x00, sizeof(bp_bookmark_csc_base_fmt));

	// getting bp_bookmark_base_fmt from provider.
	if (bp_ipc_read_custom_type(sock, &base_info,
			sizeof(bp_bookmark_csc_base_fmt)) < 0) {
		TRACE_ERROR("[CHECK IO] (%d)", id);
		__browser_adaptor_disconnect();
		pthread_mutex_unlock(&g_adaptor_mutex);
		return -1;
	}

	memset(info, 0x00, sizeof(bookmark_csc_info_fmt));

	// fill info
	info->type = base_info.type;
	info->parent = base_info.parent;
	info->editable = base_info.editable;

	pthread_mutex_unlock(&g_adaptor_mutex);
	__bookmark_csc_get_string(id, BP_CMD_COMMON_GET_TITLE,
		&info->title);
	if(info->type == BOOKMARK_CSC_TYPE_BOOKMARK)
		__bookmark_csc_get_string(id, BP_CMD_COMMON_GET_URL,
			&info->url);
	return 0;
}

int bookmark_csc_get_full_ids_p(bookmark_csc_type_defs type, int **ids, int *count)
{
	return __bp_bookmark_csc_get_ids_p
		(ids, count, -1, 0, -1, type, 1, -1,
		BP_BOOKMARK_O_DATE_CREATED, 0);
}

int bookmark_csc_get_ids_p(int parent, int **ids, int *count)
{
	if (parent < 0)
		return -1;
	return __bp_bookmark_csc_get_ids_p
		(ids, count, -1, 0, parent, -1, 1, -1,
		BP_BOOKMARK_O_DATE_CREATED, 0);
}

int bookmark_csc_free(bookmark_csc_info_fmt *info)
{
	if (info != NULL) {
		free(info->url);
		free(info->title);
		memset(info, 0x00, sizeof(bookmark_csc_info_fmt));
	}
	return 0;
}

int bookmark_csc_reset(void)
{
	BP_CSC_CHECK_PROVIDER_STATUS;

	pthread_mutex_lock(&g_adaptor_mutex);
	int sock = BP_CHECK_IPC_SOCK;
	errorcode = BP_ERROR_NONE;
	g_bp_command.cmd = BP_CMD_COMMON_RESET;
	g_bp_command.id = -1;
	errorcode = bp_ipc_simple_response(sock, &g_bp_command);
	if (errorcode == BP_ERROR_IO_ERROR)
		__browser_adaptor_disconnect();
	pthread_mutex_unlock(&g_adaptor_mutex);
	if (errorcode != BP_ERROR_NONE)
		return -1;
	return 0;
}

int bookmark_csc_get_duplicated_title_ids_p
	(int **ids, int *count, const int limit, const int offset,
	const int parent, const int type,
	const int is_operator, const int is_editable,
	const int ordering,
	const char *title, const int is_like)
{
	return __bp_bookmark_csc_get_retrieved_ids_p
		(ids, count, limit, offset, parent, type, -1, -1,
		BP_BOOKMARK_O_DATE_CREATED, ordering,
		BP_BOOKMARK_O_TITLE,
		title, is_like);
}
