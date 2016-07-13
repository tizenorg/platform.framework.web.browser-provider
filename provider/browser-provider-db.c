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

#include <string.h>
#include <errno.h>

#include <stdlib.h> // alloc
#include <unistd.h> // unlink

#include "browser-provider.h"
#include "browser-provider-db.h"
#include "browser-provider-log.h"
#include "browser-provider-requests.h"

#define BP_DB_BASIC_NULL_CHECK do {\
	if (handle == 0) {\
		TRACE_ERROR("[HANDLE] null");\
		return -1;\
	}\
	if (table == NULL) {\
		TRACE_ERROR("[TABLE] null");\
		return -1;\
	}\
} while(0)

#define BP_DB_BASIC_PREPARE do {\
	errorcode = sqlite3_prepare_v2(handle, query, -1, &stmt, NULL);\
	sqlite3_free(query);\
	if (errorcode != SQLITE_OK) {\
		*error = __get_sql_errorcode(errorcode);\
		if (*error == BP_ERROR_NONE)\
			*error = BP_ERROR_INVALID_PARAMETER;\
		TRACE_ERROR("[PREPARE] %d:%s", errorcode,\
			sqlite3_errmsg(handle));\
		__bp_finalize(stmt);\
		return -1;\
	}\
} while(0)

#define BP_DB_BASIC_BIND_EXCEPTION do {\
	if (errorcode != SQLITE_OK) {\
		*error = __get_sql_errorcode(errorcode);\
		if (*error == BP_ERROR_NONE)\
			*error = BP_ERROR_INVALID_PARAMETER;\
		TRACE_ERROR("[BIND] %d:%s", errorcode,\
			sqlite3_errmsg(handle));\
		__bp_finalize(stmt);\
		return -1;\
	}\
} while(0)

static bp_error_defs __get_sql_errorcode(int errorcode)
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
	} else if (errorcode == SQLITE_MISUSE) {
		TRACE_ERROR("[ERROR] MISUSE");
		return BP_ERROR_DISK_BUSY;
	}
	return BP_ERROR_NONE;
}

static void __bp_finalize(sqlite3_stmt *stmt)
{
	if (stmt != 0) {
		if (sqlite3_finalize(stmt) != SQLITE_OK) {
			sqlite3 *handle = sqlite3_db_handle(stmt);
			TRACE_ERROR("failed sqlite3_finalize [%s]",
				sqlite3_errmsg(handle));
		}
	}
}

static int __check_table(sqlite3 *handle, char *table)
{
	//"SELECT name FROM sqlite_master WHERE type='table' AND name='" + table +"'";
	sqlite3_stmt *stmt = NULL;

	if (table == NULL) {
		TRACE_ERROR("[CHECK TABLE NAME]");
		return -1;
	}

	char *query = sqlite3_mprintf("SELECT name FROM sqlite_master WHERE type='table' AND name='%s'", table);
	if (query == NULL) {
		TRACE_ERROR("[CHECK COMBINE]");
		return -1;
	}
	int ret = sqlite3_prepare_v2(handle, query, -1, &stmt, NULL);
	sqlite3_free(query);
	int result = 0;
	if (ret != SQLITE_OK) {
		TRACE_ERROR("[ERROR] sql message :%s", sqlite3_errmsg(handle));
		result = -1;
	}
	if (result == 0 && sqlite3_step(stmt) != SQLITE_ROW) {
		TRACE_DEBUG("not found table:%s", table);
		result = -1;
	}
	__bp_finalize(stmt);
	return result;
}

static int __bp_dp_rebuild_tables(sqlite3 *handle, bp_client_type_defs adaptor_type)
{
	int ret = SQLITE_OK;

	if (adaptor_type == BP_CLIENT_BOOKMARK) {
		if (__check_table(handle, BP_DB_TABLE_BOOKMARK) < 0) {
			ret = sqlite3_exec(handle, BP_SCHEMA_BOOKMARKS, 0, 0, 0);
			if (ret == SQLITE_OK) {
				ret = sqlite3_exec(handle, BP_SCHEMA_BOOKMARKS_INDEX, 0, 0, 0);
			}
		}
		if (ret == SQLITE_OK && __check_table(handle, BP_DB_TABLE_FAVICONS) < 0) {
			ret = sqlite3_exec(handle, BP_SCHEMA_IMAGE(BP_DB_TABLE_FAVICONS, BP_DB_TABLE_BOOKMARK), 0, 0, 0);
		}
		if (ret == SQLITE_OK && __check_table(handle, BP_DB_TABLE_THUMBNAILS) < 0) {
			ret = sqlite3_exec(handle, BP_SCHEMA_IMAGE(BP_DB_TABLE_THUMBNAILS, BP_DB_TABLE_BOOKMARK), 0, 0, 0);
		}
		if (ret == SQLITE_OK && __check_table(handle, BP_DB_TABLE_WEBICONS) < 0) {
			ret = sqlite3_exec(handle, BP_SCHEMA_IMAGE(BP_DB_TABLE_WEBICONS, BP_DB_TABLE_BOOKMARK), 0, 0, 0);
		}
		if (ret == SQLITE_OK && __check_table(handle, BP_DB_TABLE_TAGS) < 0) {
			ret = sqlite3_exec(handle, BP_SCHEMA_TAGS(BP_DB_TABLE_TAGS, BP_DB_TABLE_BOOKMARK), 0, 0, 0);
		}
	} else if (adaptor_type == BP_CLIENT_HISTORY) {
		if (__check_table(handle, BP_DB_TABLE_HISTORY) < 0) {
			ret = sqlite3_exec(handle, BP_SCHEMA_HISTORY, 0, 0, 0);
			if (ret == SQLITE_OK) {
				ret = sqlite3_exec(handle, BP_SCHEMA_HISTORY_INDEX, 0, 0, 0);
			}
		}
		if (ret == SQLITE_OK && __check_table(handle, BP_DB_TABLE_FAVICONS) < 0) {
			ret = sqlite3_exec(handle, BP_SCHEMA_IMAGE(BP_DB_TABLE_FAVICONS, BP_DB_TABLE_HISTORY), 0, 0, 0);
		}
		if (ret == SQLITE_OK && __check_table(handle, BP_DB_TABLE_THUMBNAILS) < 0) {
			ret = sqlite3_exec(handle, BP_SCHEMA_IMAGE(BP_DB_TABLE_THUMBNAILS, BP_DB_TABLE_HISTORY), 0, 0, 0);
		}
		if (ret == SQLITE_OK && __check_table(handle, BP_DB_TABLE_WEBICONS) < 0) {
			ret = sqlite3_exec(handle, BP_SCHEMA_IMAGE(BP_DB_TABLE_WEBICONS, BP_DB_TABLE_HISTORY), 0, 0, 0);
		}
	} else if (adaptor_type == BP_CLIENT_TABS) {
		if (__check_table(handle, BP_DB_TABLE_TABS) < 0) {
			ret = sqlite3_exec(handle, BP_SCHEMA_TABS, 0, 0, 0);
		}
		if (ret == SQLITE_OK && __check_table(handle, BP_DB_TABLE_FAVICONS) < 0) {
			ret = sqlite3_exec(handle, BP_SCHEMA_IMAGE(BP_DB_TABLE_FAVICONS, BP_DB_TABLE_TABS), 0, 0, 0);
		}
		if (ret == SQLITE_OK && __check_table(handle, BP_DB_TABLE_THUMBNAILS) < 0) {
			ret = sqlite3_exec(handle, BP_SCHEMA_IMAGE(BP_DB_TABLE_THUMBNAILS, BP_DB_TABLE_TABS), 0, 0, 0);
		}
	}

	if (ret != SQLITE_OK) {
		TRACE_ERROR("create tables:%d error:%s", ret, sqlite3_errmsg(handle));
		return -1;
	}
	return 0;
}

int bp_db_open(sqlite3 **handle, char *database)
{
	if (database == NULL) {
		TRACE_ERROR("[ERROR] database [%s]", database);
		return -1;
	}

	if (*handle == 0) {

#ifdef PROVIDER_DIR
		bp_rebuild_dir(PROVIDER_DIR);
#endif
#ifdef DATABASE_DIR
		bp_rebuild_dir(DATABASE_DIR);
#endif

		bp_client_type_defs adaptor_type = BP_CLIENT_NONE;
		char *check_table = NULL;
		int length = strlen(database);
		if (strncmp(DATABASE_BOOKMARK_FILE, database, length) == 0) {
			adaptor_type = BP_CLIENT_BOOKMARK;
			check_table = BP_DB_TABLE_BOOKMARK;
		} else if (strncmp(DATABASE_HISTORY_FILE, database, length) == 0) {
			adaptor_type = BP_CLIENT_HISTORY;
			check_table = BP_DB_TABLE_HISTORY;
		} else if (strncmp(DATABASE_TAB_FILE, database, length) == 0) {
			adaptor_type = BP_CLIENT_TABS;
			check_table = BP_DB_TABLE_TABS;
		}
		if (adaptor_type == BP_CLIENT_NONE || check_table == NULL) {
			TRACE_ERROR("[ERROR] can not recognize:%s", database);
			return -1;
		}

		if (sqlite3_open_v2(database, handle, SQLITE_OPEN_READWRITE,
				NULL) != SQLITE_OK) {
			TRACE_ERROR
				("[ERROR][%s][%s]", database, sqlite3_errmsg(*handle));
			int errorcode = sqlite3_errcode(*handle);
			*handle = 0;
			if (errorcode == SQLITE_CORRUPT) { // remove & re-create
				TRACE_SECURE_INFO("unlink [%s]", database);
				unlink(database);
				errorcode = SQLITE_CANTOPEN;
			}
			if (errorcode == SQLITE_CANTOPEN) {
				// create empty database
				if (sqlite3_open(database, handle) != SQLITE_OK ) {
					TRACE_SECURE_INFO("failed to connect:%s", database);
					unlink(database);
					return -1;
				}
			} else {
				TRACE_ERROR("[ERROR] can not handle this error:%d", errorcode);
				return -1;
			}
		}

		// whenever open new handle, check all tables. it's simple
		if (__bp_dp_rebuild_tables(*handle, adaptor_type) < 0) {
			TRACE_SECURE_INFO("failed to create:%s", database);
			bp_db_close(*handle);
			return -1;
		}

		if (sqlite3_exec(*handle, "PRAGMA journal_mode=PERSIST;", 0, 0, 0) != SQLITE_OK)
			TRACE_ERROR("check property journal_mode:PERSIST");
		if (sqlite3_exec(*handle, "PRAGMA foreign_keys=ON;", 0, 0, 0) != SQLITE_OK)
			TRACE_ERROR("check property foreign_keys:ON");
	}
	return *handle ? 0 : -1;
}

void bp_db_close(sqlite3 *handle)
{
	if (handle != 0) {
		// remove empty page of db
		//sqlite3_exec(handle, "VACUUM;", 0, 0, 0);
		if (sqlite3_close(handle) != SQLITE_OK)
			TRACE_ERROR("check sqlite close");
	}
}

void bp_db_reset(sqlite3_stmt *stmt)
{
	if (stmt != 0) {
		sqlite3_clear_bindings(stmt);
		if (sqlite3_reset(stmt) != SQLITE_OK) {
			sqlite3 *handle = sqlite3_db_handle(stmt);
			TRACE_ERROR("[ERROR] reset:%s",
				sqlite3_errmsg(handle));
		}
	}
}

void bp_db_finalize(sqlite3_stmt *stmt)
{
	__bp_finalize(stmt);
}

int bp_db_exec_stmt(sqlite3_stmt *stmt)
{
	if (stmt == NULL) {
		TRACE_ERROR("[CHECK sqlite3_stmt]");
		return SQLITE_ERROR;
	}
	sqlite3 *handle = sqlite3_db_handle(stmt);
	if (handle == 0) {
		TRACE_ERROR("[HANDLE]");
		return SQLITE_ERROR;
	}
	return sqlite3_step(stmt);
}

sqlite3_stmt *bp_db_prepare_basic_insert_stmt(sqlite3 *handle,
	char *table)
{
	int errorcode = SQLITE_OK;
	sqlite3_stmt *stmt = NULL;

	if (handle == 0) {
		TRACE_ERROR("[HANDLE] null");
		return NULL;
	}
	if (table == NULL) {
		TRACE_ERROR("[TABLE] null");
		return NULL;
	}

	char *query = NULL;
	int table_length = strlen(table);
	if (strncmp(BP_DB_TABLE_TABS, table, table_length) == 0) {
		query = sqlite3_mprintf
			("INSERT INTO %s (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s) VALUES (?, DATETIME('NOW'), DATETIME('NOW'), 1, ?, ?, ?, ?, ?, ?, ?)",
			table, BP_DB_COMMON_COL_ID, BP_DB_COMMON_COL_DATE_CREATED,
			BP_DB_COMMON_COL_DATE_MODIFIED,
			BP_DB_COMMON_COL_DIRTY, BP_DB_TABS_COL_INDEX,
			BP_DB_TABS_COL_IS_INCOGNITO, BP_DB_TABS_COL_BROWSER_INSTANCE,
			BP_DB_COMMON_COL_TITLE, BP_DB_COMMON_COL_URL,
			BP_DB_COMMON_COL_DEVICE_NAME, BP_DB_COMMON_COL_DEVICE_ID);
	} else {
		query = sqlite3_mprintf
			("INSERT INTO %s (%s, %s, %s, %s, %s, %s) VALUES (?, DATETIME('NOW'), DATETIME('NOW'), 1, ?, ?)",
			table, BP_DB_COMMON_COL_ID, BP_DB_COMMON_COL_DATE_CREATED,
			BP_DB_COMMON_COL_DIRTY,
			BP_DB_COMMON_COL_DATE_MODIFIED, BP_DB_COMMON_COL_TITLE,
			BP_DB_COMMON_COL_URL);
	}
	if (query == NULL) {
		TRACE_ERROR("[CHECK COMBINE]");
		return NULL;
	}
	errorcode = sqlite3_prepare_v2(handle, query, -1, &stmt, NULL);
	sqlite3_free(query);
	if (errorcode != SQLITE_OK) {
		TRACE_ERROR("[ERROR] prepare:%s", sqlite3_errmsg(handle));
		return NULL;
	}
	return stmt;
}

sqlite3_stmt *bp_db_prepare_basic_get_info_stmt(sqlite3 *handle,
	char *table, char *cond_column)
{
	int errorcode = SQLITE_OK;
	sqlite3_stmt *stmt = NULL;

	if (handle == 0) {
		TRACE_ERROR("[HANDLE] null");
		return NULL;
	}
	if (table == NULL) {
		TRACE_ERROR("[TABLE] null");
		return NULL;
	}

	char *query = NULL;
	int table_length = strlen(table);
	if (strncmp(BP_DB_TABLE_BOOKMARK, table, table_length) == 0) {
		query = sqlite3_mprintf
			("SELECT %s, %s, %s, %s, %s, %s, %s, %s, %s FROM %s WHERE %s = ?",
			BP_DB_BOOKMARK_COL_TYPE, BP_DB_BOOKMARK_COL_PARENT,
			BP_DB_BOOKMARK_COL_SEQUENCE, BP_DB_BOOKMARK_COL_IS_EDITABLE,
			BP_DB_COMMON_COL_URL, BP_DB_COMMON_COL_TITLE,
			BP_DB_COMMON_COL_SYNC, BP_DB_COMMON_COL_DATETIME_CREATED,
			BP_DB_COMMON_COL_DATETIME_MODIFIED, table, cond_column);
	} else if (strncmp(BP_DB_TABLE_HISTORY, table, table_length) == 0) {
		query = sqlite3_mprintf
			("SELECT %s, %s, %s, %s, %s, %s FROM %s WHERE %s = ?",
			BP_DB_HISTORY_COL_FREQUENCY,
			BP_DB_COMMON_COL_DATETIME_CREATED,
			BP_DB_COMMON_COL_DATETIME_MODIFIED,
			BP_DB_COMMON_COL_DATETIME_VISITED,
			BP_DB_COMMON_COL_URL, BP_DB_COMMON_COL_TITLE,
			table, cond_column);
	} else {
		query = sqlite3_mprintf
			("SELECT %s, %s, %s, %s FROM %s WHERE %s = ?",
			BP_DB_COMMON_COL_TITLE, BP_DB_COMMON_COL_URL,
			BP_DB_COMMON_COL_DATETIME_CREATED,
			BP_DB_COMMON_COL_DATETIME_MODIFIED, table, cond_column);
	}
	if (query == NULL) {
		TRACE_ERROR("[CHECK COMBINE]");
		return NULL;
	}
	errorcode = sqlite3_prepare_v2(handle, query, -1, &stmt, NULL);
	sqlite3_free(query);
	if (errorcode != SQLITE_OK) {
		TRACE_ERROR("[ERROR] prepare:%s", sqlite3_errmsg(handle));
		return NULL;
	}
	return stmt;
}

static int __bp_sql_bind_value(sqlite3_stmt *stmt,
	bp_column_data_defs condtype,  void *value, int index)
{
	int errorcode = SQLITE_ERROR;
	int *cast_value = 0;

	if (stmt == NULL)
		return SQLITE_ERROR;

	switch (condtype) {
	case BP_DB_COL_TYPE_INT:
		cast_value = value;
		errorcode = sqlite3_bind_int(stmt, index, *cast_value);
		break;
	case BP_DB_COL_TYPE_INT64:
#ifdef SQLITE_INT64_TYPE
		sqlite3_int64 *cast_value = value;
		errorcode = sqlite3_bind_int64(stmt, index, *cast_value);
#else
		cast_value = value;
		errorcode = sqlite3_bind_int(stmt, index, *cast_value);
#endif
		break;
	case BP_DB_COL_TYPE_TEXT:
		errorcode = sqlite3_bind_text(stmt, index, (char *)value, -1,
			SQLITE_STATIC);
		break;
	case BP_DB_COL_TYPE_DATETIME:
		cast_value = value;
		errorcode = sqlite3_bind_int(stmt, index, *cast_value);
		break;
	default:
		errorcode = SQLITE_ERROR;
		break;
	}
	return errorcode;
}

int bp_db_bind_value(sqlite3_stmt *stmt,
	bp_column_data_defs condtype,  void *value, int index)
{
	return __bp_sql_bind_value(stmt, condtype, value, index);
}

static char *__merge_strings(char *dest, const char *src, char sep)
{
	char *merged = NULL;

	if (dest == NULL)
		return NULL;

	if (src == NULL)
		merged = sqlite3_mprintf("%s%c", dest, sep);
	else
		merged = sqlite3_mprintf("%s%c%s", dest, sep, src);

	if (merged == NULL)
		return dest;
	sqlite3_free(dest);
	return merged;
}

int bp_db_set_datetime(sqlite3 *handle, long long int id, char *table,
	char *column, int timestamp, bp_error_defs *error)
{
	int errorcode = SQLITE_OK;
	sqlite3_stmt *stmt = NULL;
	char *query = NULL;

	BP_DB_BASIC_NULL_CHECK;

	if (id < 0) {
		TRACE_ERROR("[CHECK ID]");
		return -1;
	}
	if (column == NULL) {
		TRACE_ERROR("[CHECK COLUMN NAME]");
		return -1;
	}

	if (timestamp <= 0) {
		query = sqlite3_mprintf(
			"UPDATE %s SET %s = DATETIME('NOW') WHERE id = ?",
			table, column);
	} else {
		query = sqlite3_mprintf(
			"UPDATE %s SET %s = DATETIME(%d, 'unixepoch') WHERE id = ?",
			table, column, timestamp);
	}
	if (query == NULL) {
		TRACE_ERROR("[CHECK COMBINE]");
		return -1;
	}

	BP_DB_BASIC_PREPARE;

	errorcode = sqlite3_bind_int(stmt, 1, id);
	BP_DB_BASIC_BIND_EXCEPTION;

	errorcode = sqlite3_step(stmt);
	__bp_finalize(stmt);
	if (errorcode != SQLITE_OK && errorcode != SQLITE_DONE) {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_INVALID_PARAMETER;
		TRACE_ERROR("[STEP] %d:%s", errorcode, sqlite3_errmsg(handle));
		return -1;
	}
	return 0;
}

int bp_db_set_increase_int(sqlite3 *handle, long long int id, char *table,
	char *column, bp_error_defs *error)
{
	int errorcode = SQLITE_OK;
	sqlite3_stmt *stmt = NULL;
	char *query = NULL;

	BP_DB_BASIC_NULL_CHECK;

	if (id < 0) {
		TRACE_ERROR("[CHECK ID]");
		return -1;
	}
	if (column == NULL) {
		TRACE_ERROR("[CHECK COLUMN NAME]");
		return -1;
	}

	query = sqlite3_mprintf("UPDATE %s SET %s = %s + 1 WHERE id = ?",
			table, column, column);
	if (query == NULL) {
		TRACE_ERROR("[CHECK COMBINE]");
		return -1;
	}

	BP_DB_BASIC_PREPARE;

	errorcode = sqlite3_bind_int(stmt, 1, id);
	BP_DB_BASIC_BIND_EXCEPTION;

	errorcode = sqlite3_step(stmt);
	__bp_finalize(stmt);
	if (errorcode != SQLITE_OK && errorcode != SQLITE_DONE) {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_INVALID_PARAMETER;
		TRACE_ERROR("[STEP] %d:%s", errorcode, sqlite3_errmsg(handle));
		return -1;
	}
	return 0;
}

int bp_db_limit_rows(sqlite3 *handle, char *table, int limit_size,
	char *ordercolumn, char *ordering, bp_error_defs *error)
{
	int errorcode = SQLITE_OK;
	sqlite3_stmt *stmt = NULL;

	BP_DB_BASIC_NULL_CHECK;

	if (limit_size <= 0) {
		TRACE_ERROR("[CHECK LIMIT] %d", limit_size);
		return -1;
	}

	if (ordercolumn == NULL)
		ordercolumn = BP_DB_COMMON_COL_DATE_CREATED;
	if (ordering == NULL)
		ordering = "ASC";

	char *query = sqlite3_mprintf
			("DELETE FROM %s WHERE %s NOT IN (SELECT %s FROM %s ORDER BY %s %s LIMIT %d)",
			table, BP_DB_COMMON_COL_ID, BP_DB_COMMON_COL_ID,
			table, ordercolumn, ordering, limit_size);

	BP_DB_BASIC_PREPARE;

	errorcode = sqlite3_step(stmt);
	__bp_finalize(stmt);
	if (errorcode != SQLITE_OK && errorcode != SQLITE_DONE) {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_INVALID_PARAMETER;
		TRACE_ERROR("[STEP] %d:%s", errorcode, sqlite3_errmsg(handle));
		return -1;
	}
	return 0;
}

static char *__make_conds_count_query(char *table, char *getcolumn,
	char *condition)
{
	char *query = NULL;

	if (getcolumn == NULL)
		getcolumn = BP_DB_COMMON_COL_ID;

	if (condition != NULL) {
		query = sqlite3_mprintf("SELECT count(%s) FROM %s WHERE %s",
				getcolumn, table, condition);
	} else {
		query = sqlite3_mprintf("SELECT count(%s) FROM %s",
				getcolumn, table);
	}
	if (query == NULL) {
		TRACE_ERROR("[CHECK COMBINE]");
		return NULL;
	}
	return query;
}

static int __get_conds_rows_count(sqlite3_stmt *stmt,
	bp_error_defs *error)
{
	int errorcode = SQLITE_OK;
	int count = 0;

	errorcode = sqlite3_step(stmt);
	if (errorcode == SQLITE_ROW) {
		count = sqlite3_column_int(stmt, 0);
	} else if (errorcode == SQLITE_DONE) {
		count = 0;
	} else {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_NO_DATA;
		count = 0;
		sqlite3 *handle = sqlite3_db_handle(stmt);
		TRACE_ERROR("[STEP] %d:%s", errorcode, sqlite3_errmsg(handle));
	}
	return count;
}

static char *__make_conds_ids_query(char *table, char *getcolumn,
	int rowslimit, int rowsoffset, char *ordercolumn, char *ordering,
	char *condition)
{
	char *limit = NULL;
	char *order = NULL;
	char *query = NULL;
	if (ordering == NULL)
		ordering = "ASC";
	if (getcolumn == NULL)
		getcolumn = BP_DB_COMMON_COL_ID;
	if (condition == NULL) {
		query = sqlite3_mprintf("SELECT %s FROM %s", getcolumn, table);
	} else  {
		query = sqlite3_mprintf("SELECT %s FROM %s WHERE %s", getcolumn,
				table, condition);
	}
	if (ordercolumn != NULL) {
		order =
			sqlite3_mprintf("ORDER BY %s %s", ordercolumn, ordering);
		if (order != NULL) {
			query = __merge_strings(query, order, ' ');
			sqlite3_free(order);
		}
	}
	if (rowslimit > 0) { // 0 or negative : no limitation
		if (rowsoffset >= 0) {
			limit = sqlite3_mprintf("LIMIT %d OFFSET %d", rowslimit,
				rowsoffset);
		} else {
			limit = sqlite3_mprintf("LIMIT %d", rowslimit);
		}
		if (limit != NULL) {
			query = __merge_strings(query, limit, ' ');
			sqlite3_free(limit);
		}
	}
	if (query == NULL) {
		TRACE_ERROR("[CHECK COMBINE]");
		return NULL;
	}
	return query;
}

static int __get_conds_ids_p(sqlite3_stmt *stmt, int *ids,
	int rowslimit, bp_error_defs *error)
{
	int errorcode = 0;
	int rows_count = 0;
	while ((errorcode = sqlite3_step(stmt)) == SQLITE_ROW) {
		if (sqlite3_column_type(stmt, 0) == SQLITE_INTEGER) {
			int columnvalue = sqlite3_column_int(stmt, 0);
			ids[rows_count++] = columnvalue;
		}
		if (rowslimit > 0 && rows_count >= rowslimit) {
			errorcode = SQLITE_DONE;
			break;
		}
	}
	if (errorcode != SQLITE_DONE) {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE) {
			*error = BP_ERROR_NO_DATA;
			rows_count = 0;
		}
		sqlite3 *handle = sqlite3_db_handle(stmt);
		TRACE_ERROR("[STEP] %d:%s", errorcode, sqlite3_errmsg(handle));
		rows_count = -1;
	} else {
		if (rows_count == 0)
			*error = BP_ERROR_NO_DATA;
	}
	return rows_count;
}

int bp_db_get_custom_bind_conds_rows_count(sqlite3 *handle, char *table,
	char *getcolumn, char *conditions, char *keyword, int bind_count,
	bp_error_defs *error)
{
	sqlite3_stmt *stmt = NULL;
	int rows_count = 0;
	int errorcode = SQLITE_OK;
	char *query = __make_conds_count_query(table, BP_DB_COMMON_COL_ID,
			conditions);
	if (query == NULL) {
		TRACE_ERROR("[MAKE QUERY]");
		*error = BP_ERROR_OUT_OF_MEMORY;
	} else {
		BP_DB_BASIC_PREPARE;
		int i = 0;
		for (;i < bind_count; i++) {
			if (sqlite3_bind_text(stmt, (i+1), keyword, -1,
					SQLITE_STATIC) != SQLITE_OK) {
				TRACE_ERROR("[BIND] %s", sqlite3_errmsg(handle));
				*error = BP_ERROR_INVALID_PARAMETER;
				break;
			}
		}
		if (*error == BP_ERROR_NONE)
			rows_count = __get_conds_rows_count(stmt, error);
		__bp_finalize(stmt);
	}
	return rows_count;
}

int bp_db_get_custom_bind_conds_ids(sqlite3 *handle, char *table,
	int *ids, char *getcolumn, int rowslimit, int rowsoffset,
	char *ordercolumn, char *ordering, char *conditions, char *keyword,
	int bind_count, bp_error_defs *error)
{
	sqlite3_stmt *stmt = NULL;
	int rows_count = 0;
	int errorcode = SQLITE_OK;
	char *query = __make_conds_ids_query(table, BP_DB_COMMON_COL_ID,
			rowslimit, rowsoffset, ordercolumn, ordering, conditions);
	if (query == NULL) {
		TRACE_ERROR("[MAKE QUERY]");
		*error = BP_ERROR_OUT_OF_MEMORY;
	} else {
		BP_DB_BASIC_PREPARE;
		int i = 0;
		for (;i < bind_count; i++) {
			if (sqlite3_bind_text(stmt, (i+1), keyword, -1,
					SQLITE_STATIC) != SQLITE_OK) {
				TRACE_ERROR("[BIND] %s", sqlite3_errmsg(handle));
				*error = BP_ERROR_INVALID_PARAMETER;
				break;
			}
		}
		if (*error == BP_ERROR_NONE)
			rows_count = __get_conds_ids_p(stmt, ids, rowslimit, error);
		__bp_finalize(stmt);
	}
	return rows_count;
}

int bp_db_get_custom_conds_rows_count(sqlite3 *handle, char *table,
	char *getcolumn, char *condition, bp_error_defs *error)
{
	int errorcode = SQLITE_OK;
	int count = 0;
	sqlite3_stmt *stmt = NULL;
	char *query = NULL;

	BP_DB_BASIC_NULL_CHECK;

	if (getcolumn == NULL)
		getcolumn = BP_DB_COMMON_COL_ID;

	if (condition != NULL) {
		query = sqlite3_mprintf("SELECT count(%s) FROM %s WHERE %s",
				getcolumn, table, condition);
	} else {
		query = sqlite3_mprintf("SELECT count(%s) FROM %s",
				getcolumn, table);
	}
	if (query == NULL) {
		TRACE_ERROR("[CHECK COMBINE]");
		return -1;
	}

	BP_DB_BASIC_PREPARE;

	errorcode = sqlite3_step(stmt);
	if (errorcode == SQLITE_ROW) {
		count = sqlite3_column_int(stmt, 0);
	} else if (errorcode == SQLITE_DONE) {
		count = 0;
	} else {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_NO_DATA;
		count = 0;
		TRACE_ERROR("[STEP] %d:%s", errorcode, sqlite3_errmsg(handle));
	}
	__bp_finalize(stmt);
	return count;
}

int bp_db_get_custom_conds_ids(sqlite3 *handle, char *table, int *ids,
	char *getcolumn, int rowslimit, int rowsoffset, char *ordercolumn,
	char *ordering, char *condition, bp_error_defs *error)
{
	int errorcode = SQLITE_OK;
	int rows_count = 0;
	sqlite3_stmt *stmt = NULL;

	BP_DB_BASIC_NULL_CHECK;

	char *limit = NULL;
	char *order = NULL;
	char *query = NULL;
	if (ordering == NULL)
		ordering = "ASC";
	if (getcolumn == NULL)
		getcolumn = BP_DB_COMMON_COL_ID;
	if (condition == NULL) {
		query = sqlite3_mprintf("SELECT %s FROM %s", getcolumn, table);
	} else  {
		query = sqlite3_mprintf("SELECT %s FROM %s WHERE %s", getcolumn,
				table, condition);
	}
	if (ordercolumn == NULL) {
		order = sqlite3_mprintf("ORDER BY %s %s",
				BP_DB_COMMON_COL_DATE_CREATED, ordering);
	} else {
		order = sqlite3_mprintf("ORDER BY %s %s, ROWID %s", ordercolumn,
				ordering, ordering);
	}
	if (order != NULL) {
		query = __merge_strings(query, order, ' ');
		sqlite3_free(order);
	}
	if (rowslimit > 0) { // 0 or negative : no limitation
		if (rowsoffset >= 0) {
			limit = sqlite3_mprintf("LIMIT %d OFFSET %d", rowslimit,
				rowsoffset);
		} else {
			limit = sqlite3_mprintf("LIMIT %d", rowslimit);
		}
		if (limit != NULL) {
			query = __merge_strings(query, limit, ' ');
			sqlite3_free(limit);
		}
	}
	if (query == NULL) {
		TRACE_ERROR("[CHECK COMBINE]");
		return -1;
	}

	BP_DB_BASIC_PREPARE;

	while ((errorcode = sqlite3_step(stmt)) == SQLITE_ROW) {
		if (sqlite3_column_type(stmt, 0) == SQLITE_INTEGER) {
			int columnvalue = sqlite3_column_int(stmt, 0);
			ids[rows_count++] = columnvalue;
		}
		if (rowslimit > 0 && rows_count >= rowslimit) {
			errorcode = SQLITE_DONE;
			break;
		}
	}
	if (errorcode != SQLITE_DONE) {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE) {
			*error = BP_ERROR_NO_DATA;
			rows_count = 0;
		}
		TRACE_ERROR("[STEP] %d:%s", errorcode, sqlite3_errmsg(handle));
		rows_count = -1;
	} else {
		if (rows_count == 0)
			*error = BP_ERROR_NO_DATA;
	}
	__bp_finalize(stmt);
	return rows_count;
}

int bp_db_insert_column(sqlite3 *handle, long long int id, char *table,
	char *date_column, bp_error_defs *error)
{
	int errorcode = SQLITE_OK;
	sqlite3_stmt *stmt = NULL;
	char *query = NULL;

	BP_DB_BASIC_NULL_CHECK;

	if (id < 0) {
		TRACE_ERROR("[CHECK ID]");
		return -1;
	}
	if (date_column == NULL) {
		TRACE_ERROR("[CHECK COLUMN NAME]");
		return -1;
	}

	query = sqlite3_mprintf
			("INSERT INTO %s (id, %s) VALUES (?, DATETIME('NOW'))",
			table, date_column);
	if (query == NULL) {
		TRACE_ERROR("[CHECK COMBINE]");
		return -1;
	}

	BP_DB_BASIC_PREPARE;

	errorcode = sqlite3_bind_int(stmt, 1, id);
	BP_DB_BASIC_BIND_EXCEPTION;

	errorcode = sqlite3_step(stmt);
	__bp_finalize(stmt);
	if (errorcode != SQLITE_OK && errorcode != SQLITE_DONE) {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_INVALID_PARAMETER;
		TRACE_ERROR("[STEP] %d:%s", errorcode, sqlite3_errmsg(handle));
		return -1;
	}
	return 0;
}

int bp_db_insert3_column(sqlite3 *handle, char *table, char *column,
	bp_column_data_defs datatype, void *value, char *column2,
	bp_column_data_defs datatype2, void *value2, char *column3,
	bp_column_data_defs datatype3, void *value3, bp_error_defs *error)
{
	int errorcode = SQLITE_OK;
	sqlite3_stmt *stmt = NULL;
	char *query = NULL;

	BP_DB_BASIC_NULL_CHECK;

	if (column == NULL) {
		TRACE_ERROR("[CHECK COLUMN NAME]");
		return -1;
	}

	if (column2 == NULL) {
		query = sqlite3_mprintf
				("INSERT INTO %s (%s) VALUES (?)", table, column);
	} else {
		if (column3 == NULL) {
			query = sqlite3_mprintf
					("INSERT INTO %s (%s, %s) VALUES (?, ?)",
					table, column, column2);
		} else {
			query = sqlite3_mprintf
					("INSERT INTO %s (%s, %s, %s) VALUES (?, ?, ?)",
					table, column, column2, column3);
		}
	}

	BP_DB_BASIC_PREPARE;

	errorcode = __bp_sql_bind_value(stmt, datatype, value, 1);
	BP_DB_BASIC_BIND_EXCEPTION;

	if (column2 != NULL) {
		errorcode = __bp_sql_bind_value(stmt, datatype2, value2, 2);
		BP_DB_BASIC_BIND_EXCEPTION;
		if (column3 != NULL) {
			errorcode = __bp_sql_bind_value(stmt, datatype3, value3, 3);
			BP_DB_BASIC_BIND_EXCEPTION;
		}
	}
	errorcode = sqlite3_step(stmt);
	__bp_finalize(stmt);
	if (errorcode != SQLITE_OK && errorcode != SQLITE_DONE) {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_INVALID_PARAMETER;
		TRACE_ERROR("[STEP] %d:%s", errorcode, sqlite3_errmsg(handle));
		return -1;
	}
	return 0;
}

int bp_db_set_column(sqlite3 *handle, long long int id, char *table, char *column,
	bp_column_data_defs datatype, void *value, bp_error_defs *error)
{
	int errorcode = SQLITE_OK;
	sqlite3_stmt *stmt = NULL;
	char *query = NULL;

	BP_DB_BASIC_NULL_CHECK;

	if (column == NULL) {
		TRACE_ERROR("[CHECK COLUMN NAME]");
		return -1;
	}

	if (id < 0) {
		query = sqlite3_mprintf("UPDATE %s SET %s = ?", table, column);
	} else {
		query =
			sqlite3_mprintf
				("UPDATE %s SET %s = ? WHERE id = ?", table, column);
	}
	if (query == NULL) {
		TRACE_ERROR("[CHECK COMBINE]");
		return -1;
	}

	BP_DB_BASIC_PREPARE;

	errorcode = __bp_sql_bind_value(stmt, datatype, value, 1);
	BP_DB_BASIC_BIND_EXCEPTION;

	if (id >= 0) {
		errorcode = sqlite3_bind_int(stmt, 2, id);
		BP_DB_BASIC_BIND_EXCEPTION;
	}
	errorcode = sqlite3_step(stmt);
	__bp_finalize(stmt);
	if (errorcode != SQLITE_OK && errorcode != SQLITE_DONE) {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_INVALID_PARAMETER;
		TRACE_ERROR("[STEP] %d:%s", errorcode, sqlite3_errmsg(handle));
		return -1;
	}
	return 0;
}

int bp_db_set_cond2_column(sqlite3 *handle, char *table, char *column,
	bp_column_data_defs datatype, void *value,
	char *condcolumn, bp_column_data_defs condtype, void *condvalue,
	char *condcolumn2, bp_column_data_defs condtype2,
	void *condvalue2, bp_error_defs *error)
{
	int errorcode = SQLITE_OK;
	sqlite3_stmt *stmt = NULL;
	char *query = NULL;

	BP_DB_BASIC_NULL_CHECK;

	if (column == NULL) {
		TRACE_ERROR("[CHECK COLUMN NAME]");
		return -1;
	}

	if (condcolumn == NULL) {
		query = sqlite3_mprintf("UPDATE %s SET %s = ?", table, column);
	} else {
		if (condcolumn2 == NULL) {
			query = sqlite3_mprintf
					("UPDATE %s SET %s = ? WHERE %s is ?",
					table, column, condcolumn);
		} else {
			query = sqlite3_mprintf
					("UPDATE %s SET %s = ? WHERE %s is ? AND %s is ?",
					table, column, condcolumn, condcolumn2);
		}
	}
	if (query == NULL) {
		TRACE_ERROR("[CHECK COMBINE]");
		return -1;
	}

	BP_DB_BASIC_PREPARE;

	errorcode = __bp_sql_bind_value(stmt, datatype, value, 1);
	BP_DB_BASIC_BIND_EXCEPTION;

	if (condcolumn != NULL) {
		errorcode = __bp_sql_bind_value(stmt, condtype, condvalue, 2);
		BP_DB_BASIC_BIND_EXCEPTION;
		if (condcolumn2 != NULL) {
			errorcode =
				__bp_sql_bind_value(stmt, condtype2, condvalue2, 3);
			BP_DB_BASIC_BIND_EXCEPTION;
		}
	}
	errorcode = sqlite3_step(stmt);
	__bp_finalize(stmt);
	if (errorcode != SQLITE_OK && errorcode != SQLITE_DONE) {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_INVALID_PARAMETER;
		TRACE_ERROR("[STEP] %d:%s", errorcode, sqlite3_errmsg(handle));
		return -1;
	}
	return 0;
}

static char *__get_updates_query(int count,
	bp_db_conds_list_fmt *columns)
{
	char *conditions = NULL;
	int i = 0;

	if (count > 0 && columns != NULL) {
		conditions = sqlite3_mprintf("SET");
		for (i = 0; i < count; i++) {
			char *token = NULL;
			if (i < count - 1) {
				if (columns[i].type == BP_DB_COL_TYPE_DATETIME_NOW) {
					token = sqlite3_mprintf("%s = DATETIME('NOW'),",
							columns[i].column);
				} else if (columns[i].type == BP_DB_COL_TYPE_DATETIME) {
					token = sqlite3_mprintf
							("%s = DATETIME(?, 'unixepoch'),",
							columns[i].column);
				} else {
					token = sqlite3_mprintf("%s = ?,",
							columns[i].column);
				}
			} else {
				if (columns[i].type == BP_DB_COL_TYPE_DATETIME_NOW) {
					token = sqlite3_mprintf("%s = DATETIME('NOW')",
							columns[i].column);
				} else if (columns[i].type == BP_DB_COL_TYPE_DATETIME) {
					token = sqlite3_mprintf
							("%s = DATETIME(?, 'unixepoch')",
							columns[i].column);
				} else {
					token = sqlite3_mprintf("%s = ?",
							columns[i].column);
				}
			}
			if (token != NULL) {
				conditions = __merge_strings(conditions, token, ' ');
				sqlite3_free(token);
				token = NULL;
			}
		}
	}
	return conditions;
}

int bp_db_set_columns(sqlite3 *handle, long long int id, char *table,
	int columns_count, bp_db_conds_list_fmt *columns,
	bp_error_defs *error)
{
	int errorcode = SQLITE_OK;
	sqlite3_stmt *stmt = NULL;

	BP_DB_BASIC_NULL_CHECK;

	if (id < 0) {
		TRACE_ERROR("[CHECK ID]");
		return -1;
	}
	if (columns_count <= 0 || columns == NULL) {
		TRACE_ERROR("[CHECK COLUMNS]");
		return -1;
	}

	char *query = NULL;
	char *conditions = __get_updates_query(columns_count, columns);
	if (conditions != NULL) {
		query = sqlite3_mprintf("UPDATE %s %s WHERE id = ?", table,
				conditions);
		sqlite3_free(conditions);
	}
	if (query == NULL) {
		TRACE_ERROR("[CHECK COMBINE]");
		return -1;
	}

	BP_DB_BASIC_PREPARE;

	int i = 0;
	int bind_index = 0;
	for (i = 0; i < columns_count; i++) {
		if (columns[i].type == BP_DB_COL_TYPE_DATETIME_NOW)
			continue;
		errorcode = __bp_sql_bind_value(stmt, columns[i].type,
				columns[i].value, (++bind_index));
		BP_DB_BASIC_BIND_EXCEPTION;
	}
	errorcode = __bp_sql_bind_value(stmt, BP_DB_COL_TYPE_INT, &id,
			(++bind_index));
	BP_DB_BASIC_BIND_EXCEPTION;

	errorcode = sqlite3_step(stmt);
	__bp_finalize(stmt);
	if (errorcode != SQLITE_OK && errorcode != SQLITE_DONE) {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_INVALID_PARAMETER;
		TRACE_ERROR("[STEP] %d:%s", errorcode, sqlite3_errmsg(handle));
		return -1;
	}
	return 0;
}

// success : 0
// error   : -1
char *bp_db_get_text_column(sqlite3 *handle, long long int id, char *table,
	char *column, bp_error_defs *error)
{
	int errorcode = SQLITE_OK;
	sqlite3_stmt *stmt = NULL;

	if (table == NULL) {
		TRACE_ERROR("[TABLE]");
		return NULL;
	}
	if (handle == 0) {
		TRACE_ERROR("[HANDLE]");
		return NULL;
	}
	if (id < 0) {
		TRACE_ERROR("[CHECK ID]");
		return NULL;
	}
	if (column == NULL) {
		TRACE_ERROR("[CHECK COLUMN NAME]");
		return NULL;
	}

	char *query = sqlite3_mprintf("SELECT %s FROM %s WHERE id = ?",
			column, table);
	if (query == NULL) {
		TRACE_ERROR("[CHECK COMBINE]");
		return NULL;
	}
	errorcode = sqlite3_prepare_v2(handle, query, -1, &stmt, NULL);
	sqlite3_free(query);
	if (errorcode != SQLITE_OK) {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_INVALID_PARAMETER;
		TRACE_ERROR("[PREPARE] %d:%s", errorcode,
			sqlite3_errmsg(handle));
		__bp_finalize(stmt);
		return NULL;
	}
	errorcode = sqlite3_bind_int(stmt, 1, id);
	if (errorcode != SQLITE_OK) {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_INVALID_PARAMETER;
		TRACE_ERROR("[BIND] %d:%s", errorcode,
			sqlite3_errmsg(handle));
		__bp_finalize(stmt);
		return NULL;
	}

	char *getstr = NULL;
	errorcode = sqlite3_step(stmt);
	if (errorcode == SQLITE_ROW) {
		int getbytes = sqlite3_column_bytes(stmt, 0);
		if (getbytes > 0) {
			getstr = (char *)calloc(getbytes + 1, sizeof(char));
			if (getstr != NULL) {
				memcpy(getstr, sqlite3_column_text(stmt, 0),
					getbytes * sizeof(char));
				getstr[getbytes] = '\0';
			} else {
				TRACE_ERROR("[MEM] allocating");
				*error = BP_ERROR_OUT_OF_MEMORY;
			}
		} else {
			TRACE_DEBUG("NO_DATA");
			*error = BP_ERROR_NO_DATA;
		}
	} else {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_ID_NOT_FOUND;
		TRACE_ERROR("[STEP] %d:%s", errorcode, sqlite3_errmsg(handle));
	}
	__bp_finalize(stmt);
	return getstr;
}

char *bp_db_get_cond2_text_column(sqlite3 *handle, char *table,
	char *column, char *condcolumn, bp_column_data_defs condtype,
	void *condvalue, char *condcolumn2, bp_column_data_defs condtype2,
	void *condvalue2, bp_error_defs *error)
{
	int errorcode = SQLITE_OK;
	sqlite3_stmt *stmt = NULL;
	char *query = NULL;

	if (table == NULL) {
		TRACE_ERROR("[CHECK TABLE NAME]");
		return NULL;
	}
	if (column == NULL) {
		TRACE_ERROR("[CHECK COLUMN NAME]");
		return NULL;
	}
	if (condcolumn == NULL) {
		TRACE_ERROR("[CHECK Condition]");
		return NULL;
	}
	if (handle == 0) {
		TRACE_ERROR("[HANDLE]");
		return NULL;
	}

	if (condcolumn2 == NULL) {
		query = sqlite3_mprintf("SELECT %s FROM %s WHERE %s is ?",
				column, table, condcolumn);
	} else {
		query = sqlite3_mprintf
				("SELECT %s FROM %s WHERE %s is ? AND %s is ?",
				column, table, condcolumn, condcolumn2);
	}
	if (query == NULL) {
		TRACE_ERROR("[CHECK COMBINE]");
		return NULL;
	}
	errorcode = sqlite3_prepare_v2(handle, query, -1, &stmt, NULL);
	sqlite3_free(query);
	if (errorcode != SQLITE_OK) {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_INVALID_PARAMETER;
		TRACE_ERROR("[PREPARE] %d:%s", errorcode,
			sqlite3_errmsg(handle));
		__bp_finalize(stmt);
		return NULL;
	}
	errorcode = __bp_sql_bind_value(stmt, condtype, condvalue, 1);
	if (errorcode != SQLITE_OK) {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_INVALID_PARAMETER;
		TRACE_ERROR("[BIND] %d:%s", errorcode, sqlite3_errmsg(handle));
		__bp_finalize(stmt);
		return NULL;
	}
	if (condcolumn2 != NULL) {
		errorcode = __bp_sql_bind_value(stmt, condtype2, condvalue2, 2);
		if (errorcode != SQLITE_OK) {
			*error = __get_sql_errorcode(errorcode);
			if (*error == BP_ERROR_NONE)
				*error = BP_ERROR_INVALID_PARAMETER;
			TRACE_ERROR("[BIND] %d:%s", errorcode,
				sqlite3_errmsg(handle));
			__bp_finalize(stmt);
			return NULL;
		}
	}

	char *getstr = NULL;
	errorcode = sqlite3_step(stmt);
	if (errorcode == SQLITE_ROW) {
		int getbytes = sqlite3_column_bytes(stmt, 0);
		if (getbytes > 0) {
			getstr = (char *)calloc(getbytes + 1, sizeof(char));
			if (getstr != NULL) {
				memcpy(getstr, sqlite3_column_text(stmt, 0),
					getbytes * sizeof(char));
				getstr[getbytes] = '\0';
			} else {
				TRACE_ERROR("[MEM] allocating");
				*error = BP_ERROR_OUT_OF_MEMORY;
			}
		} else {
			TRACE_DEBUG("NO_DATA");
			*error = BP_ERROR_NO_DATA;
		}
	} else {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_ID_NOT_FOUND;
		TRACE_ERROR("[STEP] %d:%s", errorcode, sqlite3_errmsg(handle));
	}
	__bp_finalize(stmt);
	return getstr;
}

int bp_db_set_blob_column(sqlite3 *handle, long long int id, char *table,
	char *column, int length, unsigned char *value,
	bp_error_defs *error)
{
	int errorcode = SQLITE_OK;
	sqlite3_stmt *stmt = NULL;
	char *query = NULL;

	BP_DB_BASIC_NULL_CHECK;

	if (column == NULL) {
		TRACE_ERROR("[CHECK COLUMN NAME]");
		return -1;
	}

	if (id < 0) {
		query = sqlite3_mprintf("UPDATE %s SET %s = ?", table, column);
	} else {
		query =
			sqlite3_mprintf
				("UPDATE %s SET %s = ? WHERE id = ?", table, column);
	}
	if (query == NULL) {
		TRACE_ERROR("[CHECK COMBINE]");
		return -1;
	}

	BP_DB_BASIC_PREPARE;

	errorcode = sqlite3_bind_blob(stmt, 1, value, length, NULL);
	BP_DB_BASIC_BIND_EXCEPTION;

	if (id >= 0) {
		errorcode = sqlite3_bind_int(stmt, 2, id);
		BP_DB_BASIC_BIND_EXCEPTION;
	}

	errorcode = sqlite3_step(stmt);
	__bp_finalize(stmt);
	if (errorcode != SQLITE_OK && errorcode != SQLITE_DONE) {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_INVALID_PARAMETER;
		TRACE_ERROR("[STEP] %d:%s", errorcode, sqlite3_errmsg(handle));
		return -1;
	}
	return 0;
}

sqlite3_stmt *bp_db_get_blob_stmt(sqlite3 *handle, long long int id, char *table,
	char *column, bp_error_defs *error)
{
	int errorcode = SQLITE_OK;
	sqlite3_stmt *stmt = NULL;

	if (handle == 0) {
		TRACE_ERROR("[HANDLE] null");
		return NULL;
	}
	if (table == NULL) {
		TRACE_ERROR("[TABLE] null");
		return NULL;
	}

	if (id < 0) {
		TRACE_ERROR("[CHECK ID]");
		return NULL;
	}
	if (column == NULL) {
		TRACE_ERROR("[CHECK COLUMN NAME]");
		return NULL;
	}

	char *query = sqlite3_mprintf("SELECT %s FROM %s WHERE id = ?",
			column, table);
	if (query == NULL) {
		TRACE_ERROR("[CHECK COMBINE]");
		return NULL;
	}

	errorcode = sqlite3_prepare_v2(handle, query, -1, &stmt, NULL);
	sqlite3_free(query);
	if (errorcode != SQLITE_OK) {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_INVALID_PARAMETER;
		TRACE_ERROR("[PREPARE] %d:%s", errorcode,
			sqlite3_errmsg(handle));
		__bp_finalize(stmt);
		return NULL;
	}

	errorcode = sqlite3_bind_int(stmt, 1, id);
	if (errorcode != SQLITE_OK) {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_INVALID_PARAMETER;
		TRACE_ERROR("[BIND] %d:%s", errorcode, sqlite3_errmsg(handle));
		__bp_finalize(stmt);
		return NULL;
	}

	errorcode = sqlite3_step(stmt);
	return stmt;
}

int bp_db_get_blob_column(sqlite3 *handle, long long int id, char *table,
	char *column, unsigned char **data, bp_error_defs *error)
{
	int errorcode = SQLITE_OK;
	sqlite3_stmt *stmt = NULL;

	BP_DB_BASIC_NULL_CHECK;

	if (id < 0) {
		TRACE_ERROR("[CHECK ID]");
		return -1;
	}
	if (column == NULL) {
		TRACE_ERROR("[CHECK COLUMN NAME]");
		return -1;
	}

	char *query = sqlite3_mprintf("SELECT %s FROM %s WHERE id = ?",
			column, table);
	if (query == NULL) {
		TRACE_ERROR("[CHECK COMBINE]");
		return -1;
	}

	BP_DB_BASIC_PREPARE;

	errorcode = sqlite3_bind_int(stmt, 1, id);
	BP_DB_BASIC_BIND_EXCEPTION;

	int blob_length = -1;
	errorcode = sqlite3_step(stmt);
	if (errorcode == SQLITE_ROW) {
		blob_length = sqlite3_column_bytes(stmt, 0);
		if (blob_length > 0) {
			*data = (unsigned char *)calloc
					(blob_length, sizeof(unsigned char));
			if (*data != NULL) {
				memcpy(*data, sqlite3_column_blob(stmt, 0),
				sizeof(unsigned char) * blob_length);
			} else {
				TRACE_ERROR("[MEM] allocating");
				*error = BP_ERROR_OUT_OF_MEMORY;
				blob_length = -1;
			}
		} else {
			TRACE_DEBUG("NO_DATA");
			*error = BP_ERROR_NO_DATA;
			blob_length = -1;
		}
	} else if (errorcode == SQLITE_DONE) {
		*error = BP_ERROR_NO_DATA;
	} else {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_ID_NOT_FOUND;
		TRACE_ERROR("[STEP] %d:%s", errorcode, sqlite3_errmsg(handle));
	}
	__bp_finalize(stmt);
	return blob_length;
}

int bp_db_get_int_column(sqlite3 *handle, long long int id, char *table,
	char *column, bp_error_defs *error)
{
	int errorcode = SQLITE_OK;
	sqlite3_stmt *stmt = NULL;

	BP_DB_BASIC_NULL_CHECK;

	if (id < 0) {
		TRACE_ERROR("[CHECK ID]");
		return -1;
	}
	if (column == NULL) {
		TRACE_ERROR("[CHECK COLUMN NAME]");
		return -1;
	}

	char *query = sqlite3_mprintf("SELECT %s FROM %s WHERE id = ?",
			column, table);
	if (query == NULL) {
		TRACE_ERROR("[CHECK COMBINE]");
		return -1;
	}

	BP_DB_BASIC_PREPARE;

	errorcode = sqlite3_bind_int(stmt, 1, id);
	BP_DB_BASIC_BIND_EXCEPTION;

	int recv_int = -1;
	errorcode = sqlite3_step(stmt);
	if (errorcode == SQLITE_ROW) {
		recv_int = sqlite3_column_int(stmt, 0);
	} else if (errorcode == SQLITE_DONE) {
		*error = BP_ERROR_ID_NOT_FOUND;
	} else {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_ID_NOT_FOUND;
		TRACE_ERROR("[STEP] %d:%s", errorcode, sqlite3_errmsg(handle));
	}
	__bp_finalize(stmt);
	return recv_int;
}

int bp_db_get_cond2_int_column(sqlite3 *handle, char *table,
	char *column, char *condcolumn, bp_column_data_defs condtype,
	void *condvalue, char *condcolumn2, bp_column_data_defs condtype2,
	void *condvalue2, bp_error_defs *error)
{
	int errorcode = SQLITE_OK;
	sqlite3_stmt *stmt = NULL;
	char *query = NULL;

	BP_DB_BASIC_NULL_CHECK;

	if (column == NULL) {
		TRACE_ERROR("[CHECK COLUMN NAME]");
		return -1;
	}
	if (condcolumn == NULL) {
		TRACE_ERROR("[CHECK Condition]");
		return -1;
	}

	if (condcolumn2 == NULL) {
		query = sqlite3_mprintf("SELECT %s FROM %s WHERE %s is ?",
				column, table, condcolumn);
	} else {
		query = sqlite3_mprintf
				("SELECT %s FROM %s WHERE %s is ? AND %s is ?",
				column, table, condcolumn, condcolumn2);
	}
	if (query == NULL) {
		TRACE_ERROR("[CHECK COMBINE]");
		return -1;
	}

	BP_DB_BASIC_PREPARE;

	errorcode = __bp_sql_bind_value(stmt, condtype, condvalue, 1);
	BP_DB_BASIC_BIND_EXCEPTION;

	if (condcolumn2 != NULL) {
		errorcode = __bp_sql_bind_value(stmt, condtype2, condvalue2, 2);
		BP_DB_BASIC_BIND_EXCEPTION;
	}

	int recv_int = -1;
	errorcode = sqlite3_step(stmt);
	if (errorcode == SQLITE_ROW) {
		recv_int = sqlite3_column_int(stmt, 0);
	} else if (errorcode == SQLITE_DONE) {
		*error = BP_ERROR_NO_DATA;
	} else {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_NO_DATA;
		TRACE_ERROR("[STEP] %d:%s", errorcode, sqlite3_errmsg(handle));
	}
	__bp_finalize(stmt);
	return recv_int;
}

static char *__get_selects_query(int count,
	bp_db_get_columns_fmt *columns)
{
	char *conditions = NULL;
	int i = 0;

	if (count > 0 && columns != NULL) {
		conditions = sqlite3_mprintf("%s", columns[i].column);
		for (i = 1; i < count; i++)
			conditions =
				__merge_strings(conditions, columns[i].column, ',');
	}
	return conditions;
}

int bp_db_get_columns(sqlite3 *handle, long long int id, char *table,
	int columns_count, bp_db_get_columns_fmt *columns,
	bp_error_defs *error)
{
	int errorcode = SQLITE_OK;
	sqlite3_stmt *stmt = NULL;

	BP_DB_BASIC_NULL_CHECK;

	if (id < 0) {
		TRACE_ERROR("[CHECK ID]");
		return -1;
	}
	if (columns_count <= 0 || columns == NULL) {
		TRACE_ERROR("[CHECK COLUMNs]");
		return -1;
	}

	char *query = NULL;
	char *conditions = __get_selects_query(columns_count, columns);
	if (conditions != NULL) {
		query = sqlite3_mprintf("SELECT %s FROM %s WHERE id = ?",
					conditions, table);
		sqlite3_free(conditions);
	}
	if (query == NULL) {
		TRACE_ERROR("[CHECK COMBINE]");
		return -1;
	}

	BP_DB_BASIC_PREPARE;

	errorcode = __bp_sql_bind_value(stmt, BP_DB_COL_TYPE_INT, &id, 1);
	BP_DB_BASIC_BIND_EXCEPTION;

	int ret = -1;
	errorcode = sqlite3_step(stmt);
	if (errorcode == SQLITE_ROW) {
		int i = 0;
		for (i = 0; i < sqlite3_column_count(stmt); i++) {
			if (i >= columns_count)
				break;
			if (sqlite3_column_type(stmt, i) == SQLITE_INTEGER) {
				int *getint_p = (int *)calloc(1, sizeof(int));
				if (getint_p != NULL) {
					*getint_p = sqlite3_column_int(stmt, i);
					columns[i].value = getint_p;
				}
			} else if (sqlite3_column_type(stmt, i) == SQLITE_TEXT){
				int getbytes = sqlite3_column_bytes(stmt, i);
				columns[i].length = 0;
				if (getbytes > 0) {
					char *getstr =
						(char *)calloc(getbytes + 1, sizeof(char));
					if (getstr != NULL) {
						memcpy(getstr, sqlite3_column_text(stmt, i),
							getbytes * sizeof(char));
						getstr[getbytes] = '\0';
						columns[i].value = getstr;
						columns[i].length = getbytes;
					}
				}
			} else if (sqlite3_column_type(stmt, i) == SQLITE_BLOB){
				int getbytes = sqlite3_column_bytes(stmt, i);
				columns[i].length = 0;
				if (getbytes > 0) {
					columns[i].value = (unsigned char *)calloc
							(getbytes, sizeof(unsigned char));
					if (columns[i].value != NULL) {
						memcpy(columns[i].value,
							sqlite3_column_blob(stmt, i),
							sizeof(unsigned char) * getbytes);
						columns[i].length = getbytes;
					}
				}
			}
		}
		ret = 0;
	} else if (errorcode == SQLITE_DONE) {
		*error = BP_ERROR_ID_NOT_FOUND;
	} else {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_ID_NOT_FOUND;
		TRACE_ERROR("[STEP] %d:%s", errorcode, sqlite3_errmsg(handle));
	}
	__bp_finalize(stmt);
	return ret;
}

void bp_db_free_columns_fmt_values(int columns_count,
	bp_db_get_columns_fmt *columns)
{
	if (columns != NULL) {
		int i = 0;
		for (i = 0; i < columns_count; i++) {
			free(columns[i].value);
			columns[i].length = 0;
		}
	}
}

int bp_db_remove_cond(sqlite3 *handle, char *table, char *condcolumn,
	int is_op, bp_column_data_defs condtype, void *condvalue,
	bp_error_defs *error)
{
	int errorcode = SQLITE_OK;
	sqlite3_stmt *stmt = NULL;
	char *query = NULL;

	if (table == NULL) {
		TRACE_ERROR("[CHECK TABLE NAME]");
		return -1;
	}

	if (handle == 0) {
		TRACE_ERROR("[HANDLE]");
		return -1;
	}

	if (condcolumn != NULL) {
		char *is_op_str = NULL;
		if (is_op > 0) {
			is_op_str = "LIKE";
		} else {
			if (is_op == 0)
				is_op_str = "IS";
			else
				is_op_str = "IS NOT";
		}
		query = sqlite3_mprintf("DELETE FROM %s WHERE %s %s ?",
				table, condcolumn, is_op_str);
	} else {
		query = sqlite3_mprintf("DELETE FROM %s", table);
	}
	if (query == NULL) {
		TRACE_ERROR("[CHECK COMBINE]");
		return -1;
	}

	BP_DB_BASIC_PREPARE;

	if (condcolumn != NULL) {
		errorcode = __bp_sql_bind_value(stmt, condtype, condvalue, 1);
		BP_DB_BASIC_BIND_EXCEPTION;
	}
	errorcode = sqlite3_step(stmt);
	__bp_finalize(stmt);
	if (errorcode != SQLITE_OK && errorcode != SQLITE_DONE) {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_INVALID_PARAMETER;
		TRACE_ERROR("[STEP] %d:%s", errorcode, sqlite3_errmsg(handle));
		return -1;
	}
	return 0;
}

int bp_db_remove_cond2(sqlite3 *handle, char *table, char *condcolumn,
	bp_column_data_defs condtype, void *condvalue,
	char *condcolumn2, bp_column_data_defs condtype2,
	void *condvalue2, bp_error_defs *error)
{
	int errorcode = SQLITE_OK;
	sqlite3_stmt *stmt = NULL;
	char *query = NULL;

	BP_DB_BASIC_NULL_CHECK;

	if (condcolumn == NULL) {
		query = sqlite3_mprintf("DELETE FROM %s", table);
	} else {
		if (condcolumn2 == NULL) {
			query = sqlite3_mprintf
					("DELETE FROM %s WHERE %s is ?", table, condcolumn);
		} else {
			query = sqlite3_mprintf
					("DELETE FROM %s WHERE %s is ? AND %s is ?",
					table, condcolumn, condcolumn2);
		}
	}
	if (query == NULL) {
		TRACE_ERROR("[CHECK COMBINE]");
		return -1;
	}

	BP_DB_BASIC_PREPARE;

	if (condcolumn != NULL) {
		errorcode = __bp_sql_bind_value(stmt, condtype, condvalue, 1);
		BP_DB_BASIC_BIND_EXCEPTION;

		if (condcolumn2 != NULL) {
			errorcode =
				__bp_sql_bind_value(stmt, condtype2, condvalue2, 2);
			BP_DB_BASIC_BIND_EXCEPTION;
		}
	}
	errorcode = sqlite3_step(stmt);
	__bp_finalize(stmt);
	if (errorcode != SQLITE_OK && errorcode != SQLITE_DONE) {
		*error = __get_sql_errorcode(errorcode);
		if (*error == BP_ERROR_NONE)
			*error = BP_ERROR_INVALID_PARAMETER;
		TRACE_ERROR("[STEP] %d:%s", errorcode, sqlite3_errmsg(handle));
		return -1;
	}
	return 0;
}

int bp_db_backup(sqlite3 *handle, char *path)
{
	int errorcode = SQLITE_OK;
	sqlite3_backup *pbackup = 0;
	sqlite3 *backup_handle = 0;

	if (handle == 0) {
		TRACE_ERROR("[HANDLE]");
		return -1;
	}
	if (path == NULL) {
		TRACE_ERROR("[CHECK] destination");
		return -1;
	}

	errorcode = sqlite3_open(path, &backup_handle);
	if (errorcode == SQLITE_OK) {
		pbackup =
			sqlite3_backup_init(backup_handle, "main", handle, "main");
		do {
			errorcode = sqlite3_backup_step(pbackup, -1);
			TRACE_SECURE_INFO("progress (%d)",
				(100 * (sqlite3_backup_pagecount(pbackup) -
				sqlite3_backup_remaining(pbackup)) /
				sqlite3_backup_pagecount(pbackup)));
		} while (errorcode == SQLITE_OK); // more pages to be copied
		sqlite3_backup_finish(pbackup);
		if (errorcode == SQLITE_DONE) { // finished
			sqlite3_close(backup_handle);
			return 0;
		}
	}
	TRACE_ERROR("[ERROR] [%s]", sqlite3_errmsg(backup_handle));
	(void)sqlite3_close(backup_handle);
	return -1;
}

int bp_db_restore(sqlite3 *handle, char *path)
{
	int errorcode = SQLITE_OK;
	sqlite3_backup *pbackup = 0;
	sqlite3 *backup_handle = 0;

	if (handle == 0) {
		TRACE_ERROR("[HANDLE]");
		return -1;
	}
	if (path == NULL) {
		TRACE_ERROR("[CHECK] destination");
		return -1;
	}

	errorcode = sqlite3_open(path, &backup_handle);
	if (errorcode == SQLITE_OK) {
		pbackup =
			sqlite3_backup_init(handle, "main", backup_handle, "main");
		do {
			errorcode = sqlite3_backup_step(pbackup, -1);
			TRACE_SECURE_INFO("progress (%d)",
				(100 * (sqlite3_backup_pagecount(pbackup) -
				sqlite3_backup_remaining(pbackup)) /
				sqlite3_backup_pagecount(pbackup)));
		} while (errorcode == SQLITE_OK); // more pages to be copied
		sqlite3_backup_finish(pbackup);
		if (errorcode == SQLITE_DONE) { // finished
			sqlite3_close(backup_handle);
			return 0;
		}
	}
	TRACE_ERROR("[ERROR] [%s]", sqlite3_errmsg(backup_handle));
	(void)sqlite3_close(backup_handle);
	return -1;
}

char *bp_db_get_text_stmt(sqlite3_stmt* stmt, int index)
{
	if (stmt == NULL)
		return NULL;

	char *getstr = NULL;
	int getbytes = sqlite3_column_bytes(stmt, index);
	if (getbytes > 0) {
		getstr = (char *)calloc(getbytes + 1, sizeof(char));
		if (getstr != NULL) {
			memcpy(getstr, sqlite3_column_text(stmt, index),
				getbytes * sizeof(char));
			getstr[getbytes] = '\0';
		} else {
			TRACE_STRERROR("[CHECK] alloc for string");
		}
	}
	return getstr;
}
