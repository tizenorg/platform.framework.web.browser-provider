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

// rebuild dir
#include <sys/smack.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#include <ITapiModem.h>
#include <openssl/md5.h>

#include "browser-provider.h"
#include "browser-provider-log.h"
#include "browser-provider-slots.h"
#include "browser-provider-socket.h"
#include "browser-provider-db.h"
#include "browser-provider-requests.h"
#include "browser-provider-shm.h"

#define BP_PREFIX_URL_PROTOCOL "http\%://"
#define BP_PREFIX_URL_WWW "http\%://www.\%"

static int __is_same_noti_client(bp_client_type_defs src,
		bp_client_type_defs target)
{
	if (src == target)
		return 0;
	else if (src == BP_CLIENT_TABS && target == BP_CLIENT_TABS_SYNC)
		return 0;
	else if (src == BP_CLIENT_TABS_SYNC && target == BP_CLIENT_TABS)
		return 0;
	else if (src == BP_CLIENT_BOOKMARK &&
			target == BP_CLIENT_BOOKMARK_SYNC)
		return 0;
	else if (src == BP_CLIENT_BOOKMARK_SYNC &&
			target == BP_CLIENT_BOOKMARK)
		return 0;
	else if (src == BP_CLIENT_SCRAP && target == BP_CLIENT_SCRAP_SYNC)
		return 0;
	else if (src == BP_CLIENT_SCRAP_SYNC && target == BP_CLIENT_SCRAP)
		return 0;
	else if (src == BP_CLIENT_HISTORY &&
			target == BP_CLIENT_HISTORY_SYNC)
		return 0;
	else if (src == BP_CLIENT_HISTORY_SYNC &&
			target == BP_CLIENT_HISTORY)
		return 0;

	return -1;
}

void bp_common_send_noti_all(bp_client_slots_defs *slots,
		bp_client_type_defs ctype, int except_cid)
{
	TRACE_DEBUG("");
	int i = 0;
	bp_command_defs cmd = BP_CMD_COMMON_NOTI;
	for (i = 0; i < BP_MAX_CLIENT; i++) {
		if (slots[i].client != NULL &&
			__is_same_noti_client(slots[i].client->type, ctype) == 0 &&
			slots[i].client->notify > 0 &&
			slots[i].client->noti_enable == 1 &&
			slots[i].client->cid != except_cid) {
			// send noti
			bp_ipc_send_custom_type(slots[i].client->notify,
				&cmd, sizeof(bp_command_defs));
		}
	}
}

bp_error_defs bp_common_sql_errorcode(int errorcode)
{
	if (errorcode == SQLITE_FULL) {
		TRACE_ERROR("[ERROR] SQLITE_FULL");
		return BP_ERROR_DISK_FULL;
	} else if (errorcode == SQLITE_TOOBIG) {
		TRACE_ERROR("[ERROR] TOO_BIG_DATA");
		return BP_ERROR_TOO_BIG_DATA;
	} else if (errorcode == SQLITE_BUSY) {
		TRACE_ERROR("[ERROR] BUSY");
		return BP_ERROR_DISK_BUSY;
	} else if (errorcode == SQLITE_LOCKED) {
		TRACE_ERROR("[ERROR] LOCKED");
		return BP_ERROR_DISK_BUSY;
	}
	TRACE_ERROR("[ERROR] BUSY default errorcode");
	return BP_ERROR_DISK_BUSY;
}

int bp_common_is_connected_my_sync_adaptor(bp_client_slots_defs *slots,
	bp_client_type_defs type)
{
	bp_client_type_defs search_type = BP_CLIENT_NONE;

	if (type == BP_CLIENT_TABS)
		search_type = BP_CLIENT_TABS_SYNC;
	else if (type == BP_CLIENT_BOOKMARK )
		search_type = BP_CLIENT_BOOKMARK_SYNC;
	else if (type == BP_CLIENT_SCRAP)
		search_type = BP_CLIENT_SCRAP_SYNC;
	else if (type == BP_CLIENT_HISTORY)
		search_type = BP_CLIENT_HISTORY_SYNC;

	if (search_type != BP_CLIENT_NONE) {
		int i = 0;
		for (i = 0; i < BP_MAX_CLIENT; i++) {
			if (slots[i].client != NULL &&
					slots[i].client->type == search_type &&
					slots[i].client->notify > 0)
				return 0;
		}
	}

	return -1;
}

int bp_common_is_sync_adaptor(bp_client_type_defs type)
{
	if (type == BP_CLIENT_TABS_SYNC ||
			type == BP_CLIENT_BOOKMARK_SYNC ||
			type == BP_CLIENT_SCRAP_SYNC ||
			type == BP_CLIENT_HISTORY_SYNC)
		return 1;

	return 0;
}

bp_error_defs bp_common_set_dirty(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int id)
{
	int is_dirty = 1;
	bp_error_defs errorcode = BP_ERROR_NONE;
	pthread_mutex_lock(mutex);
	if (bp_db_set_column(handle, id, table, BP_DB_COMMON_COL_DIRTY,
			BP_DB_COL_TYPE_INT, &is_dirty, &errorcode) < 0) {
		TRACE_ERROR("[ERROR][SQL][%d] SET DIRTY", id);
	}
	pthread_mutex_unlock(mutex);
	return errorcode;
}

int bp_common_make_unique_id(sqlite3 *handle, pthread_mutex_t *mutex,
	char *table)
{
	int id = -1;
	int check_id = -1;
	bp_error_defs errorcode = BP_ERROR_NONE;
	unsigned check_count = 0;

	do {
		id = bp_create_unique_id();
		pthread_mutex_lock(mutex);
		errorcode = BP_ERROR_NONE;
		check_id = bp_db_get_int_column(handle, id, table,
				BP_DB_COMMON_COL_ID, &errorcode);
		pthread_mutex_unlock(mutex);
		if (errorcode == BP_ERROR_ID_NOT_FOUND)
			break;
		if (check_id < 0 && check_count < 5) { // workaround for client
			check_count++;
			TRACE_DEBUG("SQL LOCK, try again %d", check_count);
			continue;
		}
	} while(id == check_id); // means duplicated id
	return id;
}

bp_error_defs bp_common_create(sqlite3 *handle, pthread_mutex_t *mutex,
	char *table, int sock, int id)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	if (id < 0) { // create new id
		id = bp_common_make_unique_id(handle, mutex, table);
		if (id < 0) {
			TRACE_ERROR("[ERROR] failed to make unique timestamp");
			bp_ipc_send_errorcode(sock, BP_ERROR_DISK_BUSY);
			return BP_ERROR_DISK_BUSY;
		}
	} else { // use id setted by client, after checking.
		pthread_mutex_lock(mutex);
		int check_id = bp_db_get_int_column(handle, id, table,
				BP_DB_COMMON_COL_ID, &errorcode);
		pthread_mutex_unlock(mutex);
		if (check_id < 0 && errorcode != BP_ERROR_ID_NOT_FOUND) {
			bp_ipc_send_errorcode(sock, errorcode);
			return errorcode;
		}
		if (id == check_id) {
			TRACE_ERROR("[ERROR] Found duplicated ID [%d]", id);
			bp_ipc_send_errorcode(sock, BP_ERROR_DUPLICATED_ID);
			return BP_ERROR_DUPLICATED_ID;
		}
	}
	pthread_mutex_lock(mutex);
	errorcode = BP_ERROR_NONE;
	if (bp_db_insert_column(handle, id, table,
			BP_DB_COMMON_COL_DATE_CREATED, &errorcode) < 0) {
		TRACE_ERROR("[ERROR][SQL][%d] CREATE", id);
	}
	pthread_mutex_unlock(mutex);
	TRACE_SECURE_DEBUG("[CREATE] [%d]", id);
	bp_ipc_send_errorcode(sock, errorcode);
	if (errorcode == BP_ERROR_NONE)
		bp_ipc_send_custom_type(sock, &id, sizeof(int));
	return errorcode;
}

bp_error_defs bp_common_delete(sqlite3 *handle, pthread_mutex_t *mutex,
	char *table, int sock, int id)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	pthread_mutex_lock(mutex);
	if (bp_db_remove_cond(handle, table, BP_DB_COMMON_COL_ID, 0,
			BP_DB_COL_TYPE_INT, &id, &errorcode) < 0) {
		TRACE_ERROR("[ERROR][SQL][%d] DELETE", id);
	}
	pthread_mutex_unlock(mutex);
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

bp_error_defs bp_common_set_is_deleted(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock, int id)
{
	BP_PRE_CHECK;

	int is_deleted = 1;
	bp_error_defs errorcode = BP_ERROR_NONE;
	pthread_mutex_lock(mutex);
	if (bp_db_set_column(handle, id, table, BP_DB_COMMON_COL_IS_DELETED,
			BP_DB_COL_TYPE_INT, &is_deleted, &errorcode) < 0) {
		TRACE_ERROR("[ERROR][SQL][%d] SET DELETED", id);
	}
	pthread_mutex_unlock(mutex);
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

// without deleted
bp_error_defs bp_common_get_full_ids(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock)
{
	BP_PRE_CHECK;

	int *ids = NULL;
	int ids_count = 0;

	BP_PRE_CHECK;

	char *conditions =
			sqlite3_mprintf("(%s IS 0)", BP_DB_COMMON_COL_IS_DELETED);

	bp_error_defs errorcode =
		bp_common_get_ids(handle, mutex, table, &ids, &ids_count, -1, 0,
			BP_DB_COMMON_COL_DATE_CREATED, 0, conditions);

	bp_ipc_send_errorcode(sock, errorcode);
	if (errorcode == BP_ERROR_NONE) {
		bp_ipc_send_custom_type(sock, &ids_count, sizeof(int));
		bp_ipc_send_custom_type(sock, ids, (sizeof(int) * ids_count));
	}
	free(ids);
	if (conditions != NULL)
		sqlite3_free(conditions);
	return errorcode;
}

bp_error_defs bp_common_get_full_with_deleted_ids
	(sqlite3 *handle, pthread_mutex_t *mutex, char *table, int sock)
{
	BP_PRE_CHECK;

	int *ids = NULL;
	int ids_count = 0;

	BP_PRE_CHECK;

	bp_error_defs errorcode =
		bp_common_get_ids(handle, mutex, table, &ids, &ids_count, -1, 0,
			BP_DB_COMMON_COL_DATE_CREATED, 0, NULL);

	bp_ipc_send_errorcode(sock, errorcode);
	if (errorcode == BP_ERROR_NONE) {
		bp_ipc_send_custom_type(sock, &ids_count, sizeof(int));
		bp_ipc_send_custom_type(sock, ids, (sizeof(int) * ids_count));
	}
	free(ids);
	return errorcode;
}

// only dirty(is_deleted is 0)
bp_error_defs bp_common_get_dirty_ids(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock)
{
	int *ids = NULL;
	int ids_count = 0;

	BP_PRE_CHECK;

	char *conditions = sqlite3_mprintf("(%s IS 0) AND (%s IS 1)",
				BP_DB_COMMON_COL_IS_DELETED, BP_DB_COMMON_COL_DIRTY);

	bp_error_defs errorcode =
		bp_common_get_ids(handle, mutex, table, &ids, &ids_count, -1, 0,
			BP_DB_COMMON_COL_DATE_CREATED, 0, conditions);

	bp_ipc_send_errorcode(sock, errorcode);
	if (errorcode == BP_ERROR_NONE) {
		bp_ipc_send_custom_type(sock, &ids_count, sizeof(int));
		bp_ipc_send_custom_type(sock, ids, (sizeof(int) * ids_count));
	}
	free(ids);
	if (conditions != NULL)
		sqlite3_free(conditions);
	return errorcode;
}

bp_error_defs bp_common_get_deleted_ids
	(sqlite3 *handle, pthread_mutex_t *mutex, char *table, int sock)
{
	BP_PRE_CHECK;

	int *ids = NULL;
	int ids_count = 0;

	BP_PRE_CHECK;

	char *conditions =
		sqlite3_mprintf("(%s IS 1)", BP_DB_COMMON_COL_IS_DELETED);

	bp_error_defs errorcode =
		bp_common_get_ids(handle, mutex, table, &ids, &ids_count, -1, 0,
			BP_DB_COMMON_COL_DATE_CREATED, 0, conditions);

	bp_ipc_send_errorcode(sock, errorcode);
	if (errorcode == BP_ERROR_NONE) {
		bp_ipc_send_custom_type(sock, &ids_count, sizeof(int));
		bp_ipc_send_custom_type(sock, ids, (sizeof(int) * ids_count));
	}
	free(ids);
	if (conditions != NULL)
		sqlite3_free(conditions);
	return errorcode;
}

bp_error_defs bp_common_clear_dirty_ids(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	int is_dirty = 0;
	pthread_mutex_lock(mutex);
	if (bp_db_set_column(handle, -1, table, BP_DB_COMMON_COL_DIRTY,
			BP_DB_COL_TYPE_INT, &is_dirty, &errorcode) < 0) {
		TRACE_ERROR("[ERROR][SQL] CLEAR_DIRTY_IDS");
	}
	pthread_mutex_unlock(mutex);
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

bp_error_defs bp_common_clear_deleted_ids(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	int is_deleted = 1;
	pthread_mutex_lock(mutex);
	if (bp_db_remove_cond(handle, table, BP_DB_COMMON_COL_IS_DELETED, 0,
			BP_DB_COL_TYPE_INT, &is_deleted, &errorcode) < 0) {
		TRACE_ERROR("[ERROR][SQL] CLEAR_DELETED_IDS");
	}
	pthread_mutex_unlock(mutex);
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

bp_error_defs bp_common_set_tag(sqlite3 *handle, pthread_mutex_t *mutex,
	char *table, int sock, int id)
{
	BP_PRE_CHECK;

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	// get index of tag
	int recv_int = -1;
	if (bp_ipc_read_custom_type(sock, &recv_int, sizeof(int)) < 0) {
		TRACE_ERROR("[ERROR][%d] SET_TAG [IO_ERROR]", id);
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	// get the content of tag
	char *recv_str = bp_ipc_read_string(sock);
	if (recv_str == NULL) {
		TRACE_ERROR("[ERROR][%d] SET_TAG [IO_ERROR]", id);
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	if (recv_int < 0) {
		TRACE_ERROR("[ERROR][%d] SET_TAG [INDEX %d]", id, recv_int);
		bp_ipc_send_errorcode(sock, BP_ERROR_INVALID_PARAMETER);
		free(recv_str);
		return BP_ERROR_INVALID_PARAMETER;
	}

	bp_error_defs errorcode = BP_ERROR_NONE;

	pthread_mutex_lock(mutex);
	int check_id = bp_db_get_cond2_int_column(handle, table,
			BP_DB_COMMON_COL_ID, BP_DB_COMMON_COL_ID,
			BP_DB_COL_TYPE_INT, &id, BP_DB_COMMON_TAGS_COL_TAG_ID,
			BP_DB_COL_TYPE_INT, &recv_int, &errorcode);
	if (id == check_id) { // UPDATE
		// SET TAG INFO
		if (bp_db_set_cond2_column(handle, table,
				BP_DB_COMMON_TAGS_COL_TAG, BP_DB_COL_TYPE_TEXT,
				recv_str,
				BP_DB_COMMON_COL_ID, BP_DB_COL_TYPE_INT, &id,
				BP_DB_COMMON_TAGS_COL_TAG_ID, BP_DB_COL_TYPE_INT,
				&recv_int, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] SET_TAG%d", id, recv_int);
		}
	} else { // INSERT
		if (bp_db_insert3_column(handle, table, BP_DB_COMMON_COL_ID,
					BP_DB_COL_TYPE_INT, &id,
					BP_DB_COMMON_TAGS_COL_TAG_ID,
					BP_DB_COL_TYPE_INT, &recv_int,
					BP_DB_COMMON_TAGS_COL_TAG,
					BP_DB_COL_TYPE_TEXT, recv_str, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] INSERT_TAG%d", id, recv_int);
		}
	}
	pthread_mutex_unlock(mutex);
	free(recv_str);
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

bp_error_defs bp_common_unset_tag(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock, int id)
{
	BP_PRE_CHECK;

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	int recv_int = -1;
	if (bp_ipc_read_custom_type(sock, &recv_int, sizeof(int)) < 0) {
		TRACE_ERROR("[ERROR][%d] UNSET_TAG [IO_ERROR]", id);
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	if (recv_int < 0) {
		TRACE_ERROR("[ERROR][%d] UNSET_TAG [INVALID_PARAMETER]", id);
		bp_ipc_send_errorcode(sock, BP_ERROR_INVALID_PARAMETER);
		return BP_ERROR_INVALID_PARAMETER;
	}
	bp_error_defs errorcode = BP_ERROR_NONE;
	pthread_mutex_lock(mutex);
	if (bp_db_remove_cond2(handle, table, BP_DB_COMMON_COL_ID,
					BP_DB_COL_TYPE_INT, &id,
					BP_DB_COMMON_TAGS_COL_TAG_ID,
					BP_DB_COL_TYPE_INT, &recv_int, &errorcode) < 0) {
		TRACE_ERROR("[ERROR][SQL][%d] UNSET_TAG%d", id, recv_int);
	}
	pthread_mutex_unlock(mutex);
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

bp_error_defs bp_common_get_tag(sqlite3 *handle, pthread_mutex_t *mutex,
	char *table, int sock, int id)
{
	BP_PRE_CHECK;

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	// get index of tag
	int recv_int = 0;
	if (bp_ipc_read_custom_type(sock, &recv_int, sizeof(int)) < 0) {
		TRACE_ERROR("[ERROR][%d] SET_TYPE [BP_ERROR_IO_ERROR]", id);
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	bp_error_defs errorcode = BP_ERROR_NONE;
	char *recv_str = NULL;
	pthread_mutex_lock(mutex);
	recv_str = bp_db_get_cond2_text_column
			(handle, table, BP_DB_COMMON_TAGS_COL_TAG,
			BP_DB_COMMON_COL_ID, BP_DB_COL_TYPE_INT, &id,
			BP_DB_COMMON_TAGS_COL_TAG_ID, BP_DB_COL_TYPE_INT,
			&recv_int, &errorcode);
	pthread_mutex_unlock(mutex);
	if (recv_str == NULL) {
		TRACE_DEBUG("[ERROR][%d] GET_TAG%d [NO_DATA]", id, recv_int);
		bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
		return BP_ERROR_NO_DATA;
	}
	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	bp_ipc_send_string(sock, recv_str);
	free(recv_str);
	return BP_ERROR_NONE;
}


bp_error_defs bp_common_get_tag_ids(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock, int id)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	int *ids = NULL;
	int ids_count = 0;
	pthread_mutex_lock(mutex);
	char *conditions =
		sqlite3_mprintf("%s IS %d", BP_DB_COMMON_COL_ID, id);
	int check_ids_count = bp_db_get_custom_conds_rows_count(handle,
			table, BP_DB_COMMON_TAGS_COL_TAG_ID, conditions,
			&errorcode);
	if (check_ids_count > 0) {
		if (check_ids_count > MAX_LIMIT_ROWS_COUNT)
			check_ids_count = MAX_LIMIT_ROWS_COUNT;
		ids = (int *)calloc(check_ids_count, sizeof(int));
		if (ids == NULL) {
			pthread_mutex_unlock(mutex);
			bp_ipc_send_errorcode(sock, BP_ERROR_OUT_OF_MEMORY);
			if (conditions != NULL)
				sqlite3_free(conditions);
			return BP_ERROR_OUT_OF_MEMORY;
		}
		ids_count = bp_db_get_custom_conds_ids(handle, table, ids,
				BP_DB_COMMON_TAGS_COL_TAG_ID, check_ids_count, 0,
				NULL, "ASC", conditions, &errorcode);
	} else if (check_ids_count == 0) {
		errorcode = BP_ERROR_NO_DATA;
	}
	pthread_mutex_unlock(mutex);
	bp_ipc_send_errorcode(sock, errorcode);
	if (ids_count > 0 && errorcode == BP_ERROR_NONE) {
		bp_ipc_send_custom_type(sock, &ids_count, sizeof(int));
		bp_ipc_send_custom_type(sock, ids, (sizeof(int) * ids_count));
	}

	free(ids);
	if (conditions != NULL)
		sqlite3_free(conditions);
	return errorcode;
}

bp_error_defs bp_common_set_blob(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, char *column, int sock, int id)
{
	BP_PRE_CHECK;

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	bp_error_defs errorcode = BP_ERROR_NONE;
	int blob_length = -1;
	unsigned char *blob_data = NULL;
	if (bp_ipc_read_custom_type(sock, &blob_length, sizeof(int)) < 0) {
		TRACE_ERROR("[ERROR][%d] SET_BLOB [IO_ERROR]", id);
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	if (blob_length > 0) {
		blob_data =
			(unsigned char *)calloc(blob_length, sizeof(unsigned char));
		if (blob_data == NULL) {
			TRACE_ERROR("[ERROR][%d] SET_BLOB [OUT_OF_MEMORY]", id);
			bp_ipc_send_errorcode(sock, BP_ERROR_OUT_OF_MEMORY);
			return BP_ERROR_OUT_OF_MEMORY;
		}
		if (bp_ipc_read_blob(sock, blob_data,
				sizeof(unsigned char) * blob_length) < 0) {
			TRACE_ERROR("[ERROR][%d] SET_BLOB [IO_ERROR]", id);
			bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
			free(blob_data);
			return BP_ERROR_IO_ERROR;
		}

		pthread_mutex_lock(mutex);
		if (bp_db_get_int_column(handle, id, table, BP_DB_COMMON_COL_ID,
				&errorcode) < 0) { // insert
			if (errorcode == BP_ERROR_ID_NOT_FOUND) {
				errorcode = BP_ERROR_NONE;
				if (bp_db_insert3_column(handle, table,
						BP_DB_COMMON_COL_ID, BP_DB_COL_TYPE_INT, &id,
						NULL, 0, NULL, NULL, 0, NULL, &errorcode) < 0) {
					TRACE_ERROR("[ERROR][SQL][%d] new blob", id);
				}
			}
		}
		if (errorcode == BP_ERROR_NONE) {
			if (bp_db_set_blob_column(handle, id, table, column,
					blob_length, blob_data, &errorcode) < 0) {
				TRACE_ERROR("[ERROR][SQL][%d] update blob", id);
			}
		}
		pthread_mutex_unlock(mutex);
	} else { // delete blob
		pthread_mutex_lock(mutex);
		if (bp_db_remove_cond(handle, table, BP_DB_COMMON_COL_ID, 0,
				BP_DB_COL_TYPE_INT, &id, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] delete blob", id);
		}
		pthread_mutex_unlock(mutex);
	}
	bp_ipc_send_errorcode(sock, errorcode);
	free(blob_data);
	return errorcode;
}

bp_error_defs bp_common_set_blob_with_size(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock, int id)
{
	BP_PRE_CHECK;

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	bp_error_defs errorcode = BP_ERROR_NONE;
	int blob_length = -1;
	int width = 0;
	int height = 0;
	unsigned char *blob_data = NULL;
	if (bp_ipc_read_custom_type(sock, &blob_length, sizeof(int)) < 0) {
		TRACE_ERROR("[ERROR][%d] SET_BLOB [IO_ERROR]", id);
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	if (blob_length > 0) {
		if (bp_ipc_read_custom_type(sock, &width, sizeof(int)) < 0) {
			TRACE_ERROR("[ERROR][%d] SET_BLOB [IO_ERROR]", id);
			bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
			return BP_ERROR_IO_ERROR;
		}
		if (bp_ipc_read_custom_type(sock, &height, sizeof(int)) < 0) {
			TRACE_ERROR("[ERROR][%d] SET_BLOB [IO_ERROR]", id);
			bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
			return BP_ERROR_IO_ERROR;
		}

		blob_data =
			(unsigned char *)calloc(blob_length, sizeof(unsigned char));
		if (blob_data == NULL) {
			TRACE_ERROR("[ERROR][%d] SET_BLOB [OUT_OF_MEMORY]", id);
			bp_ipc_send_errorcode(sock, BP_ERROR_OUT_OF_MEMORY);
			return BP_ERROR_OUT_OF_MEMORY;
		}
		if (bp_ipc_read_blob(sock, blob_data,
				sizeof(unsigned char) * blob_length) < 0) {
			TRACE_ERROR("[ERROR][%d] SET_BLOB [IO_ERROR]", id);
			bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
			free(blob_data);
			return BP_ERROR_IO_ERROR;
		}

		pthread_mutex_lock(mutex);
		if (bp_db_get_int_column(handle, id, table, BP_DB_COMMON_COL_ID,
				&errorcode) < 0) { // insert
			if (errorcode == BP_ERROR_ID_NOT_FOUND) {
				errorcode = BP_ERROR_NONE;
				if (bp_db_insert3_column(handle, table,
						BP_DB_COMMON_COL_ID, BP_DB_COL_TYPE_INT, &id,
						NULL, 0, NULL, NULL, 0, NULL, &errorcode) < 0) {
					TRACE_ERROR("[ERROR][SQL][%d] new blob", id);
				}
			}
		}
		if (errorcode == BP_ERROR_NONE) {
			if (bp_db_set_blob_column(handle, id, table,
					BP_DB_COMMON_COL_BLOB, blob_length, blob_data,
					&errorcode) < 0) {
				TRACE_ERROR("[ERROR][SQL][%d] update blob", id);
			}
			if (bp_db_set_column(handle, id, table,
					BP_DB_COMMON_COL_BLOB_WIDTH, BP_DB_COL_TYPE_INT,
					&width, &errorcode) < 0) {
				TRACE_ERROR("[ERROR][SQL] update width");
			}
			if (bp_db_set_column(handle, id, table,
					BP_DB_COMMON_COL_BLOB_HEIGHT, BP_DB_COL_TYPE_INT,
					&height, &errorcode) < 0) {
				TRACE_ERROR("[ERROR][SQL] update height");
			}
		}
		pthread_mutex_unlock(mutex);
	} else { // delete blob
		pthread_mutex_lock(mutex);
		if (bp_db_remove_cond(handle, table, BP_DB_COMMON_COL_ID, 0,
				BP_DB_COL_TYPE_INT, &id, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] delete blob", id);
		}
		pthread_mutex_unlock(mutex);
	}
	bp_ipc_send_errorcode(sock, errorcode);
	free(blob_data);
	return errorcode;
}

bp_error_defs bp_common_get_blob(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, char *column, int sock, int id)
{
	BP_PRE_CHECK;

	unsigned char *blob_data = NULL;
	bp_error_defs errorcode = BP_ERROR_NONE;
	pthread_mutex_lock(mutex);
	int blob_length = bp_db_get_blob_column(handle, id, table, column,
			&blob_data, &errorcode);
	pthread_mutex_unlock(mutex);
	if (blob_length <= 0) {
		if (errorcode == BP_ERROR_ID_NOT_FOUND)
			errorcode = BP_ERROR_NO_DATA;
	}

	bp_ipc_send_errorcode(sock, errorcode);
	if (errorcode == BP_ERROR_NONE) {
		bp_ipc_send_custom_type(sock, &blob_length, sizeof(int));
		bp_ipc_send_custom_type(sock, blob_data,
			sizeof(unsigned char) * blob_length);
	}
	free(blob_data);
	return errorcode;
}


int bp_common_shm_get_bytes(sqlite3 *handle, char *table, int sock,
	int id, bp_shm_defs *shm, bp_error_defs *errorcode)
{
	if (shm == NULL)
		return -1;
	int blob_length = 0;
	*errorcode = BP_ERROR_NONE;
	sqlite3_stmt *stmt = bp_db_get_blob_stmt(handle, id, table,
			BP_DB_COMMON_COL_BLOB, errorcode);
	if (stmt == NULL) {
		*errorcode = BP_ERROR_NO_DATA;
	} else {
		blob_length = sqlite3_column_bytes(stmt, 0);
		if (blob_length <= 0) {
			*errorcode = BP_ERROR_NO_DATA;
		} else {
			if (bp_shm_is_ready(shm, blob_length) == 0) {
				memcpy(shm->mem, sqlite3_column_blob(stmt, 0),
						sizeof(unsigned char) * blob_length);
			} else {
				free(shm->local);
				shm->local = NULL;
				shm->local = (unsigned char *)calloc(blob_length,
						sizeof(unsigned char));
				if (shm->local != NULL) {
					memcpy(shm->local, sqlite3_column_blob(stmt, 0),
						sizeof(unsigned char) * blob_length);
				} else {
					TRACE_ERROR("[MEM] allocating");
					*errorcode = BP_ERROR_OUT_OF_MEMORY;
					blob_length = 0;
				}
			}
		}
		bp_db_finalize(stmt);
	}
	return blob_length;
}

void bp_common_get_info_send_blob(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock, int id,
	bp_shm_defs *shm)
{
	bp_error_defs errorcode = BP_ERROR_NONE;
	int blob_length = bp_common_shm_get_bytes(handle, table, sock, id,
		shm, &errorcode);
	if (blob_length <= 0) {
		bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
	} else {
		bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
		bp_ipc_send_custom_type(sock, &blob_length, sizeof(int));
		int trans_way = 0; // 0:socket 1:shm
		// read here what IPC should be used below from provider.
		if (bp_shm_is_ready(shm, blob_length) == 0)
			trans_way = 1;
		// send .. the way of IPC
		bp_ipc_send_custom_type(sock, &trans_way, sizeof(int));
		if (trans_way == 0) {
			bp_ipc_send_custom_type(sock, shm->local,
				sizeof(unsigned char) * blob_length);
			free(shm->local);
			shm->local = NULL;
		}
	}
}

bp_error_defs bp_common_get_blob_shm(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock, int id, bp_shm_defs *shm)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	int width = 0;
	int height = 0;

	pthread_mutex_lock(mutex);

	width = bp_db_get_int_column(handle, id, table,
			BP_DB_COMMON_COL_BLOB_WIDTH, &errorcode);
	height = bp_db_get_int_column(handle, id, table,
			BP_DB_COMMON_COL_BLOB_HEIGHT, &errorcode);

	int blob_length = bp_common_shm_get_bytes(handle, table, sock, id,
			shm, &errorcode);
	pthread_mutex_unlock(mutex);

	bp_ipc_send_errorcode(sock, errorcode);

	if (errorcode == BP_ERROR_NONE) {
		bp_ipc_send_custom_type(sock, &blob_length, sizeof(int));

		if (blob_length > 0) {

			int trans_way = 0; // 0:socket 1:shm
			// read here what IPC should be used below from provider.
			if (bp_shm_is_ready(shm, blob_length) == 0)
				trans_way = 1;
			// send .. the way of IPC
			bp_ipc_send_custom_type(sock, &trans_way, sizeof(int));

			if (trans_way == 0) {
				bp_ipc_send_custom_type(sock, shm->local,
					sizeof(unsigned char) * blob_length);
				free(shm->local);
				shm->local = NULL;
			}
		}
		bp_ipc_send_custom_type(sock, &width, sizeof(int));
		bp_ipc_send_custom_type(sock, &height, sizeof(int));
	}
	return errorcode;
}

bp_error_defs bp_common_set_blob_shm(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock, int id,
	bp_shm_defs *shm)
{
	BP_PRE_CHECK;

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	bp_error_defs errorcode = BP_ERROR_NONE;
	int blob_length = -1;
	int width = 0;
	int height = 0;
	if (bp_ipc_read_custom_type(sock, &blob_length, sizeof(int)) < 0) {
		TRACE_ERROR("[ERROR][%d] SET_BLOB [IO_ERROR]", id);
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	if (blob_length > 0) {

		unsigned char *blob_buffer = NULL;
		unsigned char *blob_data = NULL;
		// read here what IPC should be used below from provider.
		int trans_way = 0; // 0:socket 1:shm
		if (bp_ipc_read_custom_type
				(sock, &trans_way, sizeof(int)) < 0) {
			bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
			return BP_ERROR_IO_ERROR;
		}
		if (trans_way == 0) {
			blob_data = (unsigned char *)calloc(blob_length,
					sizeof(unsigned char));
			if (blob_data == NULL) {
				TRACE_ERROR("[ERROR][%d] SET_BLOB [OUT_OF_MEMORY]", id);
				bp_ipc_send_errorcode(sock, BP_ERROR_OUT_OF_MEMORY);
				return BP_ERROR_OUT_OF_MEMORY;
			}
			if (bp_ipc_read_blob(sock, blob_data,
					sizeof(unsigned char) * blob_length) < 0) {
				TRACE_ERROR("[ERROR][%d] SET_BLOB [IO_ERROR]", id);
				bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
				free(blob_data);
				return BP_ERROR_IO_ERROR;
			}
			blob_buffer = blob_data;
		} else {
			if (bp_shm_is_ready(shm, blob_length) == 0)
				blob_buffer = shm->mem;
		}

		if (bp_ipc_read_custom_type(sock, &width, sizeof(int)) < 0) {
			TRACE_ERROR("[ERROR][%d] SET_BLOB [IO_ERROR]", id);
			bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
			free(blob_data);
			return BP_ERROR_IO_ERROR;
		}
		if (bp_ipc_read_custom_type(sock, &height, sizeof(int)) < 0) {
			TRACE_ERROR("[ERROR][%d] SET_BLOB [IO_ERROR]", id);
			bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
			free(blob_data);
			return BP_ERROR_IO_ERROR;
		}

		pthread_mutex_lock(mutex);
		if (bp_db_get_int_column(handle, id, table, BP_DB_COMMON_COL_ID,
				&errorcode) < 0) { // insert
			if (errorcode == BP_ERROR_ID_NOT_FOUND) {
				errorcode = BP_ERROR_NONE;
				if (bp_db_insert3_column(handle, table,
						BP_DB_COMMON_COL_ID, BP_DB_COL_TYPE_INT, &id,
						NULL, 0, NULL, NULL, 0, NULL, &errorcode) < 0) {
					TRACE_ERROR("[ERROR][SQL][%d] new blob", id);
				}
			}
		}
		if (errorcode == BP_ERROR_NONE) {
			if (bp_db_set_blob_column(handle, id, table,
					BP_DB_COMMON_COL_BLOB, blob_length, blob_buffer,
					&errorcode) < 0) {
				TRACE_ERROR("[ERROR][SQL][%d] update blob", id);
			}
			if (bp_db_set_column(handle, id, table,
					BP_DB_COMMON_COL_BLOB_WIDTH, BP_DB_COL_TYPE_INT,
					&width, &errorcode) < 0) {
				TRACE_ERROR("[ERROR][SQL] update width");
			}
			if (bp_db_set_column(handle, id, table,
					BP_DB_COMMON_COL_BLOB_HEIGHT, BP_DB_COL_TYPE_INT,
					&height, &errorcode) < 0) {
				TRACE_ERROR("[ERROR][SQL] update height");
			}
		}
		pthread_mutex_unlock(mutex);
		free(blob_data);
	} else { // delete blob
		pthread_mutex_lock(mutex);
		if (bp_db_remove_cond(handle, table, BP_DB_COMMON_COL_ID, 0,
				BP_DB_COL_TYPE_INT, &id, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] delete blob", id);
		}
		pthread_mutex_unlock(mutex);
	}
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

bp_error_defs bp_common_get_blob_with_size(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock, int id)
{
	BP_PRE_CHECK;

	unsigned char *blob_data = NULL;
	bp_error_defs errorcode = BP_ERROR_NONE;
	int width = 0;
	int height = 0;
	pthread_mutex_lock(mutex);
	int blob_length = bp_db_get_blob_column(handle, id, table,
			BP_DB_COMMON_COL_BLOB, &blob_data, &errorcode);
	if (blob_length <= 0) {
		if (errorcode == BP_ERROR_ID_NOT_FOUND)
			errorcode = BP_ERROR_NO_DATA;
	}
	if (errorcode == BP_ERROR_NONE && blob_length > 0) {
		width = bp_db_get_int_column(handle, id, table,
				BP_DB_COMMON_COL_BLOB_WIDTH, &errorcode);
		height = bp_db_get_int_column(handle, id, table,
				BP_DB_COMMON_COL_BLOB_HEIGHT, &errorcode);
	}

	pthread_mutex_unlock(mutex);

	bp_ipc_send_errorcode(sock, errorcode);
	if (errorcode == BP_ERROR_NONE) {
		bp_ipc_send_custom_type(sock, &blob_length, sizeof(int));
		bp_ipc_send_custom_type(sock, &width, sizeof(int));
		bp_ipc_send_custom_type(sock, &height, sizeof(int));
		bp_ipc_send_custom_type(sock, blob_data,
			sizeof(unsigned char) * blob_length);
	}
	free(blob_data);
	return errorcode;
}

bp_error_defs bp_common_get_blob_int(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, char *column, int sock, int id)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	pthread_mutex_lock(mutex);
	int recv_int =
		bp_db_get_int_column(handle, id, table, column, &errorcode);
	pthread_mutex_unlock(mutex);
	if (errorcode == BP_ERROR_ID_NOT_FOUND)
		errorcode = BP_ERROR_NO_DATA;
	bp_ipc_send_errorcode(sock, errorcode);
	if (errorcode == BP_ERROR_NONE)
		bp_ipc_send_custom_type(sock, &recv_int, sizeof(int));
	return errorcode;
}

bp_error_defs bp_common_get_int(sqlite3 *handle, pthread_mutex_t *mutex,
	char *table, char *column, int sock, int id)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	pthread_mutex_lock(mutex);
	int recv_int =
		bp_db_get_int_column(handle, id, table, column, &errorcode);
	pthread_mutex_unlock(mutex);
	bp_ipc_send_errorcode(sock, errorcode);
	if (errorcode == BP_ERROR_NONE)
		bp_ipc_send_custom_type(sock, &recv_int, sizeof(int));
	return errorcode;
}

bp_error_defs bp_common_set_int(sqlite3 *handle, pthread_mutex_t *mutex,
	char *table, char *column, int sock, int id)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	int recv_int = 0;
	if (bp_ipc_read_custom_type(sock, &recv_int, sizeof(int)) < 0) {
		TRACE_ERROR("[ERROR][%d] %s [IO_ERROR]", id, column);
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	pthread_mutex_lock(mutex);
	if (bp_db_get_int_column(handle, id, table, BP_DB_COMMON_COL_ID,
			&errorcode) >= 0) {
		if (bp_db_set_column(handle, id, table, column,
				BP_DB_COL_TYPE_INT, &recv_int, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] %s", id, column);
		}
	}
	pthread_mutex_unlock(mutex);
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

bp_error_defs bp_common_get_string(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, char *column, int sock, int id)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	char *recv_str = NULL;
	pthread_mutex_lock(mutex);
	recv_str =
		bp_db_get_text_column(handle, id, table, column, &errorcode);
	pthread_mutex_unlock(mutex);
	bp_ipc_send_errorcode(sock, errorcode);
	if (errorcode == BP_ERROR_NONE)
		bp_ipc_send_string(sock, recv_str);
	free(recv_str);
	return errorcode;
}

bp_error_defs bp_common_set_string(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, char *column, int sock, int id)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	char *recv_str = bp_ipc_read_string(sock);
	if (recv_str == NULL) {
		TRACE_ERROR("[ERROR][%d] SET_STRING [IO_ERROR]", id);
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}

	pthread_mutex_lock(mutex);
	if (bp_db_get_int_column(handle, id, table, BP_DB_COMMON_COL_ID,
			&errorcode) >= 0) {
		if (bp_db_set_column(handle, id, table, column,
				BP_DB_COL_TYPE_TEXT, recv_str, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] SET_STRING", id);
		}
	}
	pthread_mutex_unlock(mutex);
	free(recv_str);
	bp_ipc_send_errorcode(sock, errorcode);
	return BP_ERROR_NONE;
}

bp_error_defs bp_common_set_date(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, char *column, int sock, int id)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	int recv_int = 0;
	if (bp_ipc_read_custom_type(sock, &recv_int, sizeof(int)) < 0) {
		TRACE_ERROR("[ERROR][%d] SET_DATE [IO_ERROR]", id);
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	pthread_mutex_lock(mutex);
	if (bp_db_set_datetime(handle, id, table, column, recv_int,
			&errorcode) < 0) {
		TRACE_ERROR("[ERROR][SQL][%d] SET_DATE", id);
	}
	pthread_mutex_unlock(mutex);
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

bp_error_defs bp_common_replace_int(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, char *column, int sock, int id)
{
	BP_PRE_CHECK;

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	int recv_int = 0;
	bp_error_defs errorcode = BP_ERROR_NONE;
	if (bp_ipc_read_custom_type(sock, &recv_int, sizeof(int)) < 0) {
		TRACE_ERROR("[ERROR][%d] %s [IO_ERROR]", id, column);
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	pthread_mutex_lock(mutex);
	if (bp_db_get_int_column(handle, id, table, BP_DB_COMMON_COL_ID,
			&errorcode) < 0) { // insert
		if (errorcode == BP_ERROR_ID_NOT_FOUND) {
			errorcode = BP_ERROR_NONE;
			if (bp_db_insert3_column(handle, table,
					BP_DB_COMMON_COL_ID, BP_DB_COL_TYPE_INT, &id,
					column, BP_DB_COL_TYPE_INT, &recv_int, NULL, 0,
					NULL, &errorcode) < 0) {
				TRACE_ERROR("[ERROR][SQL][%d] New Row", id);
			}
		}
	} else {
		if (bp_db_set_column(handle, id, table, column,
				BP_DB_COL_TYPE_INT, &recv_int, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] %s", id, column);
		}
	}
	pthread_mutex_unlock(mutex);
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

void bp_common_convert_to_lowercase(char *src)
{
	int i = 0;
	if (src == NULL)
		return ;
	for (;i < strlen(src); i++) {
		char c = *(src + i);
		if (c >= 'A' && c <= 'Z')
			*(src + i) = c - 'A' + 'a';
	}
}

static int __is_url_www_prefix(char *keyword)
{
	if (keyword == NULL || strlen(keyword) <= 0)
		return -1;
	// check keyword... ( w, ww, www, www.)
	char *target = NULL;
	if (keyword[0] == '%' || keyword[0] == '*')
		target = keyword + 1;
	else
		target = keyword;
	int is_w_keyword = 0;
	int keyword_length = strlen(target);
	if (keyword_length <= 5) {  // www. + % in tail
		int i = 0;
		int w_count = 0;
		is_w_keyword = 1;
		for (; i < keyword_length; i++) {
			if (target[i] == '%' || target[i] == '*')
				break;
			if (target[i] != 'w') {
				is_w_keyword = 0;
				break;
			}
			w_count++;
		}
		if (is_w_keyword == 1 && w_count > 3) {
			is_w_keyword = 0;
		} else {
			if (is_w_keyword == 0 && w_count == 3) {
				if (keyword_length >= 4 &&
						strncmp("www.", target, 4) == 0) {
					is_w_keyword = 1;
				}
			}
		}
	}
	return (is_w_keyword == 1 ? 0 : -1);
}

bp_error_defs bp_common_get_inquired_ids(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int **ids,
	int *ids_count, int limit, int offset, char *ordercolumn,
	unsigned ordering, int is_like, char *keyword, char *cond2, const int raw_search)
{
	int *inquired_ids = NULL;
	char *conditions = NULL;
	char *basic_cond = NULL;
	bp_error_defs errorcode = BP_ERROR_NONE;
	int inquired_ids_count = 0;
	int bind_count = 0;

	if (limit > MAX_LIMIT_ROWS_COUNT) {
		TRACE_ERROR("[ERROR] limit overflow");
		return BP_ERROR_INVALID_PARAMETER;
	}

	// make condition.
	if (cond2 == NULL) {
		basic_cond = sqlite3_mprintf("%s IS 0",
			BP_DB_COMMON_COL_IS_DELETED);
	} else {
		basic_cond = sqlite3_mprintf("%s IS 0 AND %s",
			BP_DB_COMMON_COL_IS_DELETED, cond2);
	}
	if (basic_cond == NULL) {
		TRACE_STRERROR("[ERROR] make query");
		return BP_ERROR_OUT_OF_MEMORY;
	}
	if (is_like > 0) {
		bp_common_convert_to_lowercase(keyword);
		// LIKE
		if (raw_search == 0) {
			if (__is_url_www_prefix(keyword) == 0) {
				conditions = sqlite3_mprintf(
					"%s AND (%s LIKE ? OR (CASE WHEN (%s LIKE '%s') THEN (%s LIKE '%swww.' || ?) ELSE (%s LIKE '%s' || ?) END))",
					basic_cond, BP_DB_COMMON_COL_LOWER_TITLE,
					BP_DB_COMMON_COL_LOWER_URL, BP_PREFIX_URL_WWW,
					BP_DB_COMMON_COL_LOWER_URL, BP_PREFIX_URL_PROTOCOL,
					BP_DB_COMMON_COL_LOWER_URL, BP_PREFIX_URL_PROTOCOL);
					bind_count = 3;
			} else {
				conditions = sqlite3_mprintf(
					"%s AND (%s LIKE ? OR (%s LIKE '%s' || ?))",
					basic_cond, BP_DB_COMMON_COL_LOWER_TITLE,
					BP_DB_COMMON_COL_LOWER_URL,
					BP_PREFIX_URL_PROTOCOL);
					bind_count = 2;
			}
		} else { // raw search without excluding protocol or prefix
			conditions = sqlite3_mprintf(
					"%s AND (%s LIKE ? OR %s LIKE ?)",
					basic_cond, BP_DB_COMMON_COL_LOWER_TITLE,
					BP_DB_COMMON_COL_LOWER_URL);
					bind_count = 2;
		}
	} else {
		char *is_like_str = NULL;
		if (is_like == 0)
			is_like_str = "IS";
		else
			is_like_str = "IS NOT";
		conditions = sqlite3_mprintf("%s AND (%s %s ? OR (%s %s ?))",
				basic_cond, BP_DB_COMMON_COL_TITLE, is_like_str,
				BP_DB_COMMON_COL_URL, is_like_str);
		bind_count = 2;
	}
	sqlite3_free(basic_cond);
	if (conditions == NULL) {
		TRACE_STRERROR("[ERROR] make query");
		return BP_ERROR_OUT_OF_MEMORY;
	}

	pthread_mutex_lock(mutex);

	if (limit <= 0) {
		// get total size.
		int total_rows = bp_db_get_custom_bind_conds_rows_count(handle,
			table, BP_DB_COMMON_COL_ID, conditions, keyword, bind_count,
			&errorcode);
		if (total_rows <= 0) {
			errorcode = BP_ERROR_NO_DATA;
		} else {
			if (total_rows > MAX_LIMIT_ROWS_COUNT)
				limit = MAX_LIMIT_ROWS_COUNT;
			else
				limit = total_rows;
		}
	}

	if (limit > 0 && errorcode == BP_ERROR_NONE) {
		inquired_ids = (int *)calloc(limit, sizeof(int));
		if (inquired_ids == NULL) {
			TRACE_STRERROR("[ERROR] allocation:%d", limit);
			errorcode = BP_ERROR_OUT_OF_MEMORY;
		} else {
			inquired_ids_count = bp_db_get_custom_bind_conds_ids(handle,
				table, inquired_ids, BP_DB_COMMON_COL_ID, limit, offset,
				ordercolumn, (ordering <= 0 ? "ASC" : "DESC"),
				conditions, keyword, bind_count, &errorcode);
		}
	}

	pthread_mutex_unlock(mutex);

	sqlite3_free(conditions);
	if (errorcode == BP_ERROR_NONE) {
		*ids = inquired_ids;
		*ids_count = inquired_ids_count;
	} else {
		free(inquired_ids);
	}
	return errorcode;
}

bp_error_defs bp_common_get_duplicated_url_ids(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int **ids,
	int *ids_count, int limit, int offset,
	char *ordercolumn, unsigned ordering, int is_like, char *keyword,
	char *cond2)
{
	int *inquired_ids = NULL;
	char *conditions = NULL;
	char *basic_cond = NULL;
	bp_error_defs errorcode = BP_ERROR_NONE;
	int inquired_ids_count = 0;
	int bind_count = 0;

	if (limit > MAX_LIMIT_ROWS_COUNT) {
		TRACE_ERROR("[ERROR] limit overflow");
		return BP_ERROR_INVALID_PARAMETER;
	}

	if (cond2 == NULL) {
		basic_cond = sqlite3_mprintf("%s IS 0",
			BP_DB_COMMON_COL_IS_DELETED);
	} else {
		basic_cond = sqlite3_mprintf("%s IS 0 AND %s",
			BP_DB_COMMON_COL_IS_DELETED, cond2);
	}
	if (basic_cond == NULL) {
		TRACE_STRERROR("[ERROR] make query");
		return BP_ERROR_OUT_OF_MEMORY;
	}
	if (is_like > 0) {
		bp_common_convert_to_lowercase(keyword);
		// LIKE
		if (__is_url_www_prefix(keyword) == 0) {
			conditions = sqlite3_mprintf(
				"%s AND (CASE WHEN (%s LIKE '%s') THEN (%s LIKE '%swww.' || ?) ELSE (%s LIKE '%s' || ?) END)",
				basic_cond,
				BP_DB_COMMON_COL_LOWER_URL, BP_PREFIX_URL_WWW,
				BP_DB_COMMON_COL_LOWER_URL, BP_PREFIX_URL_PROTOCOL,
				BP_DB_COMMON_COL_LOWER_URL, BP_PREFIX_URL_PROTOCOL);
			bind_count = 2;
		} else {
			conditions = sqlite3_mprintf("%s AND (%s LIKE '%s' || ?)",
				basic_cond,
				BP_DB_COMMON_COL_LOWER_URL, BP_PREFIX_URL_PROTOCOL,
				keyword);
			bind_count = 1;
		}
	} else {
		conditions = sqlite3_mprintf("%s AND %s %s ?", basic_cond,
				BP_DB_COMMON_COL_URL, (is_like == 0 ? "IS" : "IS NOT"));
		bind_count = 1;
	}
	sqlite3_free(basic_cond);
	if (conditions == NULL) {
		TRACE_STRERROR("[ERROR] make query");
		return BP_ERROR_OUT_OF_MEMORY;
	}

	pthread_mutex_lock(mutex);

	if (limit <= 0) {
		// get total size.

		int total_rows = bp_db_get_custom_bind_conds_rows_count(handle,
				table, BP_DB_COMMON_COL_ID, conditions, keyword,
				bind_count, &errorcode);
		if (total_rows <= 0) {
			errorcode = BP_ERROR_NO_DATA;
		} else {
			if (total_rows > MAX_LIMIT_ROWS_COUNT)
				limit = MAX_LIMIT_ROWS_COUNT;
			else
				limit = total_rows;
		}
	}

	if (limit > 0 && errorcode == BP_ERROR_NONE) {
		inquired_ids = (int *)calloc(limit, sizeof(int));
		if (inquired_ids == NULL) {
			TRACE_STRERROR("[ERROR] allocation:%d", limit);
			errorcode = BP_ERROR_OUT_OF_MEMORY;
		} else {
			inquired_ids_count = bp_db_get_custom_bind_conds_ids(handle,
					table, inquired_ids, BP_DB_COMMON_COL_ID, limit,
					offset, ordercolumn,
					(ordering <= 0 ? "ASC" : "DESC"), conditions,
					keyword, bind_count, &errorcode);
		}
	}

	pthread_mutex_unlock(mutex);

	sqlite3_free(conditions);
	if (errorcode == BP_ERROR_NONE) {
		*ids = inquired_ids;
		*ids_count = inquired_ids_count;
	} else {
		free(inquired_ids);
	}
	return errorcode;
}

bp_error_defs bp_common_get_duplicated_ids(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int **ids,
	int *ids_count, int limit, int offset, char *checkcolumn,
	char *ordercolumn, unsigned ordering, int is_like, char *keyword,
	char *cond2)
{
	int *inquired_ids = NULL;
	char *conditions = NULL;
	char *basic_cond = NULL;
	bp_error_defs errorcode = BP_ERROR_NONE;
	int inquired_ids_count = 0;

	if (limit > MAX_LIMIT_ROWS_COUNT) {
		TRACE_ERROR("[ERROR] limit overflow");
		return BP_ERROR_INVALID_PARAMETER;
	}

	if (cond2 == NULL) {
		basic_cond = sqlite3_mprintf("%s IS 0",
			BP_DB_COMMON_COL_IS_DELETED);
	} else {
		basic_cond = sqlite3_mprintf("%s IS 0 AND %s",
			BP_DB_COMMON_COL_IS_DELETED, cond2);
	}
	if (basic_cond == NULL) {
		TRACE_STRERROR("[ERROR] make query");
		return BP_ERROR_OUT_OF_MEMORY;
	}
	if (is_like > 0) {
		bp_common_convert_to_lowercase(keyword);
		conditions = sqlite3_mprintf("%s AND LOWER(%s) LIKE ?",
			basic_cond, checkcolumn);
	} else {
		conditions = sqlite3_mprintf("%s AND %s %s ?",
			basic_cond, checkcolumn, (is_like == 0 ? "IS" : "IS NOT"));
	}
	sqlite3_free(basic_cond);
	if (conditions == NULL) {
		TRACE_STRERROR("[ERROR] make query");
		return BP_ERROR_OUT_OF_MEMORY;
	}

	pthread_mutex_lock(mutex); // start to use sqlite, lock

	if (limit <= 0) {
		// get total size.
		int total_rows =
			bp_db_get_custom_bind_conds_rows_count(handle, table,
				BP_DB_COMMON_COL_ID, conditions, keyword, 1, &errorcode);
		if (total_rows <= 0) {
			errorcode = BP_ERROR_NO_DATA;
		} else {
			if (total_rows > MAX_LIMIT_ROWS_COUNT)
				limit = MAX_LIMIT_ROWS_COUNT;
			else
				limit = total_rows;
		}
	}

	if (limit > 0 && errorcode == BP_ERROR_NONE) {
		inquired_ids = (int *)calloc(limit, sizeof(int));
		if (inquired_ids == NULL) {
			TRACE_STRERROR("[ERROR] allocation:%d", limit);
			errorcode = BP_ERROR_OUT_OF_MEMORY;
		} else {
			inquired_ids_count = bp_db_get_custom_bind_conds_ids(handle,
				table, inquired_ids, BP_DB_COMMON_COL_ID, limit, offset,
				ordercolumn, (ordering <= 0 ? "ASC" : "DESC"),
				conditions, keyword, 1, &errorcode);
		}
	}
	pthread_mutex_unlock(mutex);

	sqlite3_free(conditions);
	if (errorcode == BP_ERROR_NONE) {
		*ids = inquired_ids;
		*ids_count = inquired_ids_count;
	} else {
		free(inquired_ids);
	}
	return errorcode;
}

bp_error_defs bp_common_get_ids(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int **ids,
	int *ids_count, int limit, int offset,
	char *ordercolumn, unsigned ordering, char *conditions)
{
	int *inquired_ids = NULL;
	bp_error_defs errorcode = BP_ERROR_NONE;

	if (limit > MAX_LIMIT_ROWS_COUNT) {
		TRACE_ERROR("[ERROR] limit overflow");
		return BP_ERROR_INVALID_PARAMETER;
	}

	if (limit <= 0) {
		// get total size.
		int total_rows = bp_db_get_custom_conds_rows_count(handle,
			table, BP_DB_COMMON_COL_ID, conditions, &errorcode);
		if (total_rows <= 0) {
			return BP_ERROR_NO_DATA;
		} else {
			if (total_rows > MAX_LIMIT_ROWS_COUNT)
				limit = MAX_LIMIT_ROWS_COUNT;
			else
				limit = total_rows;
		}
	}

	inquired_ids = (int *)calloc(limit, sizeof(int));
	if (inquired_ids == NULL) {
		TRACE_STRERROR("[ERROR] allocation:%d", limit);
		return BP_ERROR_OUT_OF_MEMORY;
	}

	pthread_mutex_lock(mutex);
	int inquired_ids_count = bp_db_get_custom_conds_ids(handle, table,
		inquired_ids, BP_DB_COMMON_COL_ID, limit, offset, ordercolumn,
			(ordering <= 0 ? "ASC" : "DESC"), conditions, &errorcode);
	pthread_mutex_unlock(mutex);

	if (errorcode == BP_ERROR_NONE) {
		*ids = inquired_ids;
		*ids_count = inquired_ids_count;
	} else {
		free(inquired_ids);
	}
	return errorcode;
}

char *bp_get_my_deviceid()
{
	static char *device_id = NULL;

	if (device_id)
		return device_id;

	// Check modem is available.
	TapiHandle *tapi_handle = tel_init(NULL);
	if (tapi_handle != NULL) {
		int status = 0;
		char *imei = NULL;
		tel_check_modem_power_status(tapi_handle, &status);
		if (status == 0)
			imei = tel_get_misc_me_imei_sync(tapi_handle);
		tel_deinit(tapi_handle);

		if (imei != NULL) {
			// Make hash key with IMEI.
			MD5_CTX context;
			unsigned char digest[17] = { 0, };

			MD5_Init(&context);
			MD5_Update(&context, imei, strlen(imei));
			MD5_Final(digest, &context);

			int i;
			for (i = 0; i < 16; i++)
				digest[i] = '0' + (digest[i] % 10);

			device_id = strdup(digest);
			free(imei);
		}
	}

	TRACE_SECURE_DEBUG("*** device id: %s", device_id);

	return device_id;
}

char *bp_merge_strings(char *dest, const char *src)
{
	if (src == NULL)
		return dest;

	if (dest == NULL)
		return sqlite3_mprintf("%s", src);
	char *merged = sqlite3_mprintf("%s%s", dest, src);

	if (merged == NULL)
		return dest;
	sqlite3_free(dest);
	return merged;
}

static int __bp_is_valid_dir(const char *dirpath)
{
	struct stat dir_state;
	int stat_ret;
	if (dirpath == NULL) {
		TRACE_ERROR("check path");
		return -1;
	}
	stat_ret = stat(dirpath, &dir_state);
	if (stat_ret == 0 && S_ISDIR(dir_state.st_mode)) {
		return 0;
	}
	return -1;
}

void bp_rebuild_dir(const char *dirpath)
{
	if (__bp_is_valid_dir(dirpath) < 0) {
		if (mkdir(dirpath, S_IRWXU | S_IRWXG | S_IRWXO) == 0) {
			TRACE_INFO("check directory:%s", dirpath);
			if (smack_setlabel(dirpath, "_", SMACK_LABEL_ACCESS) != 0) {
				TRACE_SECURE_ERROR("failed to set smack label:%s", dirpath);
			}
		} else {
			TRACE_STRERROR("failed to create directory:%s", dirpath);
		}
	}
}
