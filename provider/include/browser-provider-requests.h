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

#ifndef BROWSER_PROVIDER_REQUESTS_H
#define BROWSER_PROVIDER_REQUESTS_H

#include <sqlite3.h>

#include "browser-provider.h"
#include "browser-provider-slots.h"


bp_error_defs bp_common_set_dirty(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int id);
void bp_common_send_noti_all(bp_client_slots_defs *slots,
	bp_client_type_defs ctype, int except_cid);
bp_error_defs bp_common_sql_errorcode(int errorcode);
int bp_common_is_connected_my_sync_adaptor(bp_client_slots_defs *slots,
	bp_client_type_defs type);
int bp_common_is_sync_adaptor(bp_client_type_defs type);
int bp_common_make_unique_id(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table);
bp_error_defs bp_common_create(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock, int id);
bp_error_defs bp_common_delete(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock, int id);
bp_error_defs bp_common_set_is_deleted(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock, int id);
bp_error_defs bp_common_get_full_ids(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock);
bp_error_defs bp_common_get_full_with_deleted_ids(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock);
bp_error_defs bp_common_get_dirty_ids(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock);
bp_error_defs bp_common_get_deleted_ids(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock);
bp_error_defs bp_common_clear_dirty_ids(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock);
bp_error_defs bp_common_clear_deleted_ids(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock);
bp_error_defs bp_common_set_tag(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock, int id);
bp_error_defs bp_common_unset_tag(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock, int id);
bp_error_defs bp_common_get_tag(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock, int id);
bp_error_defs bp_common_get_tag_ids(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock, int id);
bp_error_defs bp_common_set_blob(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, char *column, int sock, int id);
bp_error_defs bp_common_get_blob(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, char *column, int sock, int id);

int bp_common_shm_get_bytes(sqlite3 *handle, char *table, int sock,
	int id, bp_shm_defs *shm, bp_error_defs *errorcode);
void bp_common_get_info_send_blob(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock, int id,
	bp_shm_defs *shm);
bp_error_defs bp_common_get_blob_shm(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock, int id,
	bp_shm_defs *shm);
bp_error_defs bp_common_set_blob_shm(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock, int id,
	bp_shm_defs *shm);

bp_error_defs bp_common_get_blob_with_size(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock, int id);
bp_error_defs bp_common_set_blob_with_size(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int sock, int id);
bp_error_defs bp_common_get_blob_int(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, char *column, int sock, int id);
bp_error_defs bp_common_get_int(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, char *column, int sock, int id);
bp_error_defs bp_common_set_int(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, char *column, int sock, int id);
bp_error_defs bp_common_get_string(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, char *column, int sock, int id);
bp_error_defs bp_common_set_string(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, char *column, int sock, int id);
bp_error_defs bp_common_set_date(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, char *column, int sock, int id);
bp_error_defs bp_common_replace_int(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, char *column, int sock, int id);
void bp_common_convert_to_lowercase(char *src);
bp_error_defs bp_common_get_inquired_ids(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int **ids,
	int *ids_count, int limit, int offset, char *ordercolumn,
	unsigned ordering, int is_like, char *keyword, char *cond2, const int raw_search);
bp_error_defs bp_common_get_duplicated_url_ids(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int **ids,
	int *ids_count, int limit, int offset,
	char *ordercolumn, unsigned ordering, int is_like, char *keyword,
	char *cond2);
bp_error_defs bp_common_get_duplicated_ids(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int **ids,
	int *ids_count, int limit, int offset, char *checkcolumn,
	char *ordercolumn, unsigned ordering, int is_like, char *keyword,
	char *cond2);
bp_error_defs bp_common_get_ids(sqlite3 *handle,
	pthread_mutex_t *mutex, char *table, int **ids,
	int *ids_count, int limit, int offset,
	char *ordercolumn, unsigned ordering, char *extra_conds);
char *bp_get_my_deviceid();
char *bp_merge_strings(char *dest, const char *src);
void bp_rebuild_dir(const char *dirpath);

#endif
