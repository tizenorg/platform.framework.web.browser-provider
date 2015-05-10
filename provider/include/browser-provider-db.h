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

#ifndef BROWSER_PROVIDER_DB_H
#define BROWSER_PROVIDER_DB_H

#include <sqlite3.h>

#include "browser-provider-db-defs.h"

typedef struct {
	char *column;
	bp_column_data_defs type;
	int is_like;
	void *value;
} bp_db_conds_list_fmt;

typedef struct {
	char *column;
	size_t length;
	void *value;
} bp_db_get_columns_fmt;

// caller should free this structure cause of void pointer

int bp_db_open(sqlite3 **handle, char *database);
void bp_db_close(sqlite3 *handle);
void bp_db_reset(sqlite3_stmt *stmt);
void bp_db_finalize(sqlite3_stmt *stmt);
int bp_db_exec_stmt(sqlite3_stmt *stmt);
sqlite3_stmt *bp_db_prepare_basic_insert_stmt(sqlite3 *handle,
	char *table);
sqlite3_stmt *bp_db_prepare_basic_get_info_stmt(sqlite3 *handle,
	char *table, char *cond_column);
int bp_db_bind_value(sqlite3_stmt *stmt, bp_column_data_defs condtype,
	void *value, int index);
// if timestamp <= 0, basically apply now
int bp_db_set_datetime(sqlite3 *handle, int id, char *table,
	char *column, int timestamp, bp_error_defs *error);
int bp_db_set_increase_int(sqlite3 *handle, int id, char *table,
	char *column, bp_error_defs *error);
int bp_db_limit_rows(sqlite3 *handle, char *table, int limit_size,
	char *ordercolumn, char *ordering, bp_error_defs *error);
int bp_db_get_custom_bind_conds_rows_count(sqlite3 *handle, char *table,
	char *getcolumn, char *conditions, char *keyword, int bind_count,
	bp_error_defs *error);
int bp_db_get_custom_bind_conds_ids(sqlite3 *handle, char *table,
	int *ids, char *getcolumn, int rowslimit, int rowsoffset,
	char *ordercolumn, char *ordering, char *conditions, char *keyword,
	 int bind_count, bp_error_defs *error);
int bp_db_get_custom_conds_rows_count(sqlite3 *handle, char *table,
	char *getcolumn, char *condition, bp_error_defs *error);
int bp_db_get_custom_conds_ids(sqlite3 *handle,
	char *table, int *ids, char *getcolumn, int rowslimit, int rowsoffset,
	char *ordercolumn, char *ordering,
	char *condition, bp_error_defs *error);
int bp_db_insert_column(sqlite3 *handle, int id, char *table,
	char *date_column, bp_error_defs *error);
int bp_db_insert3_column(sqlite3 *handle, char *table, char *column,
	bp_column_data_defs datatype, void *value, char *column2,
	bp_column_data_defs datatype2, void *value2, char *column3,
	bp_column_data_defs datatype3, void *value3, bp_error_defs *error);
int bp_db_set_column(sqlite3 *handle, int id, char *table, char *column,
	bp_column_data_defs datatype, void *value, bp_error_defs *error);
int bp_db_set_cond2_column(sqlite3 *handle, char *table, char *column,
	bp_column_data_defs datatype, void *value,
	char *condcolumn, bp_column_data_defs condtype, void *condvalue,
	char *condcolumn2, bp_column_data_defs condtype2,
	void *condvalue2, bp_error_defs *error);
int bp_db_set_columns(sqlite3 *handle, int id, char *table,
	int columns_count, bp_db_conds_list_fmt *columns,
	bp_error_defs *error);
char *bp_db_get_text_column(sqlite3 *handle, int id, char *table,
	char *column, bp_error_defs *error);
char *bp_db_get_cond2_text_column(sqlite3 *handle, char *table,
	char *column, char *condcolumn, bp_column_data_defs condtype,
	void *condvalue, char *condcolumn2, bp_column_data_defs condtype2,
	void *condvalue2, bp_error_defs *error);
int bp_db_set_blob_column(sqlite3 *handle, int id, char *table,
	char *column, int length, unsigned char *value,
	bp_error_defs *error);
sqlite3_stmt *bp_db_get_blob_stmt(sqlite3 *handle, int id, char *table,
	char *column, bp_error_defs *error);
int bp_db_get_blob_column(sqlite3 *handle, int id, char *table,
	char *column, unsigned char **data, bp_error_defs *error);
int bp_db_get_int_column(sqlite3 *handle, int id, char *table,
	char *column, bp_error_defs *error);
int bp_db_get_cond2_int_column(sqlite3 *handle, char *table,
	char *column, char *condcolumn, bp_column_data_defs condtype,
	void *condvalue, char *condcolumn2, bp_column_data_defs condtype2,
	void *condvalue2, bp_error_defs *error);
int bp_db_get_columns(sqlite3 *handle, int id, char *table,
	int columns_count, bp_db_get_columns_fmt *columns,
	bp_error_defs *error);
void bp_db_free_columns_fmt_values(int columns_count,
	bp_db_get_columns_fmt *columns);
int bp_db_remove_cond(sqlite3 *handle, char *table, char *condcolumn,
	int is_op, bp_column_data_defs condtype, void *condvalue,
	bp_error_defs *error);
int bp_db_remove_cond2(sqlite3 *handle, char *table, char *condcolumn,
	bp_column_data_defs condtype, void *condvalue,
	char *condcolumn2, bp_column_data_defs condtype2,
	void *condvalue2, bp_error_defs *error);

int bp_db_backup(sqlite3 *handle, char *path);
int bp_db_restore(sqlite3 *handle, char *path);

char *bp_db_get_text_stmt(sqlite3_stmt* stmt, int index);
#endif
