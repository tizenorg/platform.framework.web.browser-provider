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

#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#include "browser-provider.h"
#include "browser-provider-log.h"
#include "browser-provider-slots.h"
#include "browser-provider-socket.h"
#include "browser-provider-db.h"
#include "browser-provider-requests.h"

#include "history-adaptor.h"

static sqlite3 *g_db_handle = 0;
static sqlite3_stmt *g_db_basic_get_info_stmt = NULL;
static pthread_mutex_t g_db_mutex = PTHREAD_MUTEX_INITIALIZER;

static char *__bp_history_get_date_query(int is_deleted, bp_history_date_defs date_type, char *checkcolumn)
{
	char *conditions = NULL;
	char *date_cond = NULL;
	char *delete_cond = NULL;
	if (is_deleted >= 0)
		delete_cond = sqlite3_mprintf("%s IS %d", BP_DB_COMMON_COL_IS_DELETED,
			is_deleted);
	if (date_type == BP_HISTORY_DATE_TODAY) {
		date_cond =
			sqlite3_mprintf("DATE(%s) = DATE('now')", checkcolumn);
	} else if (date_type == BP_HISTORY_DATE_YESTERDAY) {
		date_cond = sqlite3_mprintf("DATE(%s) = DATE('now', '-1 day')",
				checkcolumn);
	} else if (date_type == BP_HISTORY_DATE_LAST_7_DAYS) {
		date_cond =
			sqlite3_mprintf("DATE(%s) <= DATE('now', '-2 days') AND DATE(%s) > DATE('now','-7 days')",
				checkcolumn, checkcolumn);
	} else if (date_type == BP_HISTORY_DATE_LAST_MONTH) {
		date_cond =
			sqlite3_mprintf("DATE(%s) <= DATE('now','-1 months') AND DATE(%s) > DATE('now', '-2 months')",
				checkcolumn, checkcolumn);
	} else if (date_type == BP_HISTORY_DATE_OLDER) {
		date_cond =
			sqlite3_mprintf("DATE(%s) <= DATE('now', '-2 months')",
				checkcolumn);
	}

	if (delete_cond != NULL && date_cond != NULL)
		conditions = sqlite3_mprintf("%s AND %s", delete_cond, date_cond);
	else if (delete_cond != NULL && date_cond == NULL)
		conditions = sqlite3_mprintf("%s", delete_cond);
	else if (delete_cond == NULL && date_cond != NULL)
		conditions = sqlite3_mprintf("%s", date_cond);

	if (date_cond != NULL)
		sqlite3_free(date_cond);
	if (delete_cond != NULL)
		sqlite3_free(delete_cond);
	return conditions;
}

static bp_error_defs __bp_history_get_date_ids_count(int sock)
{
	BP_PRE_CHECK;

	bp_history_offset offset = 0;
	bp_history_date_defs date_type = 0;
	char *checkcolumn = NULL;
	char *conditions = NULL;

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	// date_column_offset
	if (bp_ipc_read_custom_type(sock, &offset, sizeof(bp_history_offset)) < 0) {
		TRACE_ERROR("[ERROR]] GET_OFFSET [BP_ERROR_IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	if (bp_ipc_read_custom_type(sock, &date_type, sizeof(int)) < 0) {
		TRACE_ERROR("[ERROR] COND IS_LIKE [IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	if (offset & BP_HISTORY_O_DATE_VISITED)
		checkcolumn = BP_DB_COMMON_COL_DATE_VISITED;
	else if (offset & BP_HISTORY_O_DATE_MODIFIED)
		checkcolumn = BP_DB_COMMON_COL_DATE_MODIFIED;
	else
		checkcolumn = BP_DB_COMMON_COL_DATE_CREATED;

	conditions = __bp_history_get_date_query(0, date_type, checkcolumn);

	bp_error_defs errorcode = BP_ERROR_NONE;
	pthread_mutex_lock(&g_db_mutex);
	int recv_int = bp_db_get_custom_conds_rows_count(g_db_handle,
			BP_DB_TABLE_HISTORY, BP_DB_COMMON_COL_ID, conditions,
			&errorcode);
	pthread_mutex_unlock(&g_db_mutex);

	if (recv_int <= 0 && errorcode == BP_ERROR_NONE)
		errorcode = BP_ERROR_NO_DATA;
	bp_ipc_send_errorcode(sock, errorcode);
	if (errorcode == BP_ERROR_NONE && recv_int > 0)
		bp_ipc_send_custom_type(sock, &recv_int, sizeof(int));

	if (conditions != NULL)
		sqlite3_free(conditions);
	return errorcode;
}

static char *__get_column_by_offset(bp_history_offset offset)
{
	if (offset & BP_HISTORY_O_URL)
		return BP_DB_COMMON_COL_URL;
	else if (offset & BP_HISTORY_O_TITLE)
		return BP_DB_COMMON_COL_TITLE;
	else if (offset & BP_HISTORY_O_DATE_VISITED)
		return BP_DB_COMMON_COL_DATE_VISITED;
	else if (offset & BP_HISTORY_O_DATE_MODIFIED)
		return BP_DB_COMMON_COL_DATE_MODIFIED;
	else if (offset & BP_HISTORY_O_FREQUENCY)
		return BP_DB_HISTORY_COL_FREQUENCY;
	return BP_DB_COMMON_COL_DATE_CREATED;
}

static char *__get_operator(bp_history_timestamp_op_defs op)
{
	switch(op) {
	case BP_HISTORY_OP_NOT_EQUAL: // !=
		return "IS NOT";
	case BP_HISTORY_OP_GREATER: // >
		return ">";
	case BP_HISTORY_OP_LESS: // <
		return "<";
	case BP_HISTORY_OP_GREATER_QUEAL: // >=
		return ">=";
	case BP_HISTORY_OP_LESS_QUEAL: // <=
		return "<=";
	default:
		break;
	}
	return "IS";
}

static bp_error_defs __bp_history_get_cond_ids(int sock, const int raw_search)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	int ids_count = 0;
	int *ids = NULL;
	int is_like = 0;
	bp_history_offset check_offset = 0;
	char *order_column = NULL;
	char *period_column = NULL;
	char *keyword = NULL;
	char *conditions = NULL;
	bp_history_rows_cond_fmt conds;

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	memset(&conds, 0x00, sizeof(bp_history_rows_cond_fmt));
	if (bp_ipc_read_custom_type(sock, &conds,
			sizeof(bp_history_rows_cond_fmt)) < 0) {
		TRACE_ERROR("[ERROR] CONDs [BP_ERROR_IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	// check_column_offset
	if (bp_ipc_read_custom_type
			(sock, &check_offset, sizeof(bp_history_offset)) < 0) {
		TRACE_ERROR("[ERROR]] GET_OFFSET [BP_ERROR_IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	if (check_offset > 0) {
		// is_like
		if (bp_ipc_read_custom_type(sock, &is_like, sizeof(int)) < 0) {
			TRACE_ERROR("[ERROR] COND IS_LIKE [IO_ERROR]");
			bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
			return BP_ERROR_IO_ERROR;
		}
		// keyword
		keyword = bp_ipc_read_string(sock);
		if (keyword == NULL) {
			TRACE_ERROR("[ERROR] KEYWORD [IO_ERROR]");
			bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
			return BP_ERROR_IO_ERROR;
		}
	}

	if (conds.order_offset & BP_HISTORY_O_URL)
		order_column = BP_DB_COMMON_COL_URL;
	else if (conds.order_offset & BP_HISTORY_O_TITLE)
		order_column = BP_DB_COMMON_COL_TITLE;
	else if (conds.order_offset & BP_HISTORY_O_DATE_VISITED)
		order_column = BP_DB_COMMON_COL_DATE_VISITED;
	else if (conds.order_offset & BP_HISTORY_O_DATE_MODIFIED)
		order_column = BP_DB_COMMON_COL_DATE_MODIFIED;
	else if (conds.order_offset & BP_HISTORY_O_FREQUENCY)
		order_column = BP_DB_HISTORY_COL_FREQUENCY;
	else
		order_column = BP_DB_COMMON_COL_DATE_CREATED;

	if (conds.period_offset & BP_HISTORY_O_DATE_VISITED)
		period_column = BP_DB_COMMON_COL_DATE_VISITED;
	else if (conds.period_offset & BP_HISTORY_O_DATE_MODIFIED)
		period_column = BP_DB_COMMON_COL_DATE_MODIFIED;
	else
		period_column = BP_DB_COMMON_COL_DATE_CREATED;

	conditions =
		__bp_history_get_date_query(0, conds.period_type, period_column);

	if (check_offset > 0) {

		if ((check_offset & BP_HISTORY_O_TITLE) &&
				(check_offset & BP_HISTORY_O_URL)) {
			// inquired
			errorcode = bp_common_get_inquired_ids(g_db_handle,
				&g_db_mutex, BP_DB_TABLE_HISTORY, &ids,
				&ids_count, conds.limit, conds.offset, order_column,
				conds.ordering, is_like, keyword, conditions, raw_search);
		} else {

			if (raw_search == 0 && check_offset & BP_HISTORY_O_URL) {
				errorcode = bp_common_get_duplicated_url_ids(g_db_handle,
				&g_db_mutex, BP_DB_TABLE_HISTORY, &ids, &ids_count,
				conds.limit, conds.offset,
				order_column, conds.ordering, is_like, keyword, conditions);
			} else if (check_offset & BP_HISTORY_O_TITLE) {
				char *checkcolumn = NULL;
				if (check_offset & BP_HISTORY_O_TITLE) {
					checkcolumn = BP_DB_COMMON_COL_TITLE;
				} else if (check_offset & BP_HISTORY_O_URL) {
					checkcolumn = BP_DB_COMMON_COL_URL;
				} else {
					errorcode = BP_ERROR_INVALID_PARAMETER;
				}
				if (checkcolumn != NULL) {
					errorcode = bp_common_get_duplicated_ids(g_db_handle,
					&g_db_mutex, BP_DB_TABLE_HISTORY, &ids, &ids_count,
					conds.limit, conds.offset, BP_DB_COMMON_COL_TITLE,
					order_column, conds.ordering, is_like, keyword, conditions);
				}
			}
		}

	} else {
		errorcode = bp_common_get_ids(g_db_handle, &g_db_mutex,
			BP_DB_TABLE_HISTORY, &ids, &ids_count,
			conds.limit, conds.offset,
			order_column, conds.ordering, conditions);
	}

	bp_ipc_send_errorcode(sock, errorcode);
	if (errorcode == BP_ERROR_NONE) {
		bp_ipc_send_custom_type(sock, &ids_count, sizeof(int));
		bp_ipc_send_custom_type(sock, ids, (sizeof(int) * ids_count));
	}
	free(ids);
	free(keyword);
	if (conditions != NULL)
		sqlite3_free(conditions);
	return errorcode;
}

static bp_error_defs __bp_history_get_cond_timestamp_ids(int sock)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	int ids_count = 0;
	int *ids = NULL;
	int is_like = 0;
	int timestamp_count = 0;
	unsigned int check_offset = 0;
	char *order_column = NULL;
	char *keyword = NULL;
	char *conditions = NULL;
	bp_history_rows_fmt limits;

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	memset(&limits, 0x00, sizeof(bp_history_rows_fmt));
	if (bp_ipc_read_custom_type(sock, &limits,
			sizeof(bp_history_rows_fmt)) < 0) {
		TRACE_ERROR("[ERROR] LIMITs [BP_ERROR_IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	if (bp_ipc_read_custom_type(sock, &timestamp_count, sizeof(int)) < 0) {
		TRACE_ERROR("[ERROR] TIMESTAMP COUNT [IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	if (timestamp_count > 0) {
		bp_history_timestamp_fmt t_timestamps[timestamp_count];
		int i = 0;
		for (; i < timestamp_count; i++) {
			if (bp_ipc_read_custom_type(sock, &t_timestamps[i],
				sizeof(bp_history_timestamp_fmt)) < 0) {
				TRACE_ERROR("[ERROR] TIMESTAMPs [BP_ERROR_IO_ERROR]");
				bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
				return BP_ERROR_IO_ERROR;
			}
			if (i == 0) {
				conditions = sqlite3_mprintf("%s %s datetime(%ld, 'unixepoch')",
				__get_column_by_offset(t_timestamps[i].offset),
				__get_operator(t_timestamps[i].cmp_operator),
				t_timestamps[i].timestamp);
			} else {
				char *tmp_cond = sqlite3_mprintf
						(" %s (%s %s datetime(%ld, 'unixepoch'))",
						(t_timestamps[i].conds_operator == 0 ? "AND":"OR"),
						__get_column_by_offset(t_timestamps[i].offset),
						__get_operator(t_timestamps[i].cmp_operator),
						t_timestamps[i].timestamp);
				if (tmp_cond != NULL) {
					conditions = bp_merge_strings(conditions, tmp_cond);
					sqlite3_free(tmp_cond);
				}
			}
		}
	}
	// check_column_offset
	if (bp_ipc_read_custom_type
			(sock, &check_offset, sizeof(unsigned int)) < 0) {
		TRACE_ERROR("[ERROR]] GET_OFFSET [BP_ERROR_IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		if (conditions != NULL)
			sqlite3_free(conditions);
		return BP_ERROR_IO_ERROR;
	}
	if (check_offset > 0) {
		// is_like
		if (bp_ipc_read_custom_type(sock, &is_like, sizeof(int)) < 0) {
			TRACE_ERROR("[ERROR] COND IS_LIKE [IO_ERROR]");
			bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
			if (conditions != NULL)
				sqlite3_free(conditions);
			return BP_ERROR_IO_ERROR;
		}
		// keyword
		keyword = bp_ipc_read_string(sock);
		if (keyword == NULL) {
			TRACE_ERROR("[ERROR] KEYWORD [IO_ERROR]");
			bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
			if (conditions != NULL)
				sqlite3_free(conditions);
			return BP_ERROR_IO_ERROR;
		}
	}

	order_column = __get_column_by_offset(limits.order_offset);

	if (check_offset > 0) {

		if ((check_offset & BP_HISTORY_O_TITLE) &&
				(check_offset & BP_HISTORY_O_URL)) {
			// inquired
			errorcode = bp_common_get_inquired_ids(g_db_handle,
				&g_db_mutex, BP_DB_TABLE_HISTORY, &ids,
				&ids_count, limits.limit, limits.offset, order_column,
				limits.ordering, is_like, keyword, conditions, 0);
		} else {
			if (check_offset & BP_HISTORY_O_URL) {
				errorcode =
					bp_common_get_duplicated_url_ids(g_db_handle,
						&g_db_mutex, BP_DB_TABLE_HISTORY, &ids,
						&ids_count, limits.limit, limits.offset,
						order_column, limits.ordering, is_like, keyword,
						conditions);
			} else if (check_offset & BP_HISTORY_O_TITLE) {
				errorcode = bp_common_get_duplicated_ids(g_db_handle,
						&g_db_mutex, BP_DB_TABLE_HISTORY, &ids,
						&ids_count, limits.limit, limits.offset,
						BP_DB_COMMON_COL_TITLE, order_column,
						limits.ordering, is_like, keyword, conditions);
			} else {
				errorcode = BP_ERROR_INVALID_PARAMETER;
			}
		}

	} else {
		errorcode = bp_common_get_ids(g_db_handle, &g_db_mutex,
			BP_DB_TABLE_HISTORY, &ids, &ids_count,
			limits.limit, limits.offset,
			order_column, limits.ordering, conditions);
	}

	bp_ipc_send_errorcode(sock, errorcode);
	if (errorcode == BP_ERROR_NONE) {
		bp_ipc_send_custom_type(sock, &ids_count, sizeof(int));
		bp_ipc_send_custom_type(sock, ids, (sizeof(int) * ids_count));
	}
	free(ids);
	free(keyword);
	if (conditions != NULL)
		sqlite3_free(conditions);
	return errorcode;
}

static bp_error_defs __bp_history_get_duplicated_ids(int sock)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	bp_history_offset offset = 0;
	int ids_count = 0;
	int *ids = NULL;
	char *ordercolumn = NULL;
	char *keyword = NULL;
	bp_db_base_conds_fmt conds;

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	memset(&conds, 0x00, sizeof(bp_db_base_conds_fmt));
	if (bp_ipc_read_custom_type
		(sock, &conds, sizeof(bp_db_base_conds_fmt)) < 0) {
		TRACE_ERROR("[ERROR] CONDs [BP_ERROR_IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	// check_column_offset
	if (bp_ipc_read_custom_type(sock, &offset, sizeof(bp_history_offset)) < 0) {
		TRACE_ERROR("[ERROR]] GET_OFFSET [BP_ERROR_IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	// is_like
	int is_like = 0;
	if (bp_ipc_read_custom_type(sock, &is_like, sizeof(int)) < 0) {
		TRACE_ERROR("[ERROR] COND IS_LIKE [IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	// keyword
	keyword = bp_ipc_read_string(sock);
	if (keyword == NULL) {
		TRACE_ERROR("[ERROR] KEYWORD [IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}

	if (conds.order_column_offset & BP_HISTORY_O_URL)
		ordercolumn = BP_DB_COMMON_COL_URL;
	else if (conds.order_column_offset & BP_HISTORY_O_TITLE)
		ordercolumn = BP_DB_COMMON_COL_TITLE;
	else if (conds.order_column_offset & BP_HISTORY_O_DATE_VISITED)
		ordercolumn = BP_DB_COMMON_COL_DATE_VISITED;
	else if (conds.order_column_offset & BP_HISTORY_O_DATE_MODIFIED)
		ordercolumn = BP_DB_COMMON_COL_DATE_MODIFIED;
	else if (conds.order_column_offset & BP_HISTORY_O_FREQUENCY)
		ordercolumn = BP_DB_HISTORY_COL_FREQUENCY;
	else
		ordercolumn = BP_DB_COMMON_COL_DATE_CREATED;

	if ((offset & BP_HISTORY_O_TITLE) && (offset & BP_HISTORY_O_URL)) {
		// inquired
		errorcode = bp_common_get_inquired_ids(g_db_handle,
			&g_db_mutex, BP_DB_TABLE_HISTORY, &ids,
			&ids_count, conds.limit, conds.offset, ordercolumn,
			conds.ordering, is_like, keyword, NULL, 0);
	} else {

		if (offset & BP_HISTORY_O_URL) {
			errorcode = bp_common_get_duplicated_url_ids(g_db_handle,
			&g_db_mutex, BP_DB_TABLE_HISTORY, &ids, &ids_count,
			conds.limit, conds.offset,
			ordercolumn, conds.ordering, is_like, keyword, NULL);
		} else if (offset & BP_HISTORY_O_TITLE) {
			errorcode = bp_common_get_duplicated_ids(g_db_handle,
			&g_db_mutex, BP_DB_TABLE_HISTORY, &ids, &ids_count,
			conds.limit, conds.offset, BP_DB_COMMON_COL_TITLE,
			ordercolumn, conds.ordering, is_like, keyword, NULL);
		} else {
			errorcode = BP_ERROR_INVALID_PARAMETER;
		}

	}

	bp_ipc_send_errorcode(sock, errorcode);
	if (errorcode == BP_ERROR_NONE) {
		bp_ipc_send_custom_type(sock, &ids_count, sizeof(int));
		bp_ipc_send_custom_type(sock, ids, (sizeof(int) * ids_count));
	}
	free(ids);
	free(keyword);
	return errorcode;
}

static bp_error_defs __bp_history_set_visit(int sock, int id)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	pthread_mutex_lock(&g_db_mutex);
	if (bp_db_set_increase_int(g_db_handle, id, BP_DB_TABLE_HISTORY,
			BP_DB_HISTORY_COL_FREQUENCY, &errorcode) < 0) {
		TRACE_ERROR("[ERROR][SQL][%d] SET_VISIT INCREASE", id);
	} else {
		if (bp_db_set_datetime(g_db_handle, id, BP_DB_TABLE_HISTORY,
				BP_DB_COMMON_COL_DATE_VISITED, -1, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] SET_VISIT DATE_VISITED", id);
		}
	}
	pthread_mutex_unlock(&g_db_mutex);
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

static bp_error_defs __bp_history_set_limit_size(int sock)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	int table_size = 0;
	char *ordercolumn = NULL;
	bp_db_base_conds_fmt conds; // get from adaptor

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	memset(&conds, 0x00, sizeof(bp_db_base_conds_fmt));
	if (bp_ipc_read_custom_type
		(sock, &conds, sizeof(bp_db_base_conds_fmt)) < 0) {
		TRACE_ERROR("[ERROR] CONDs [BP_ERROR_IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	if (bp_ipc_read_custom_type(sock, &table_size, sizeof(int)) < 0) {
		TRACE_ERROR("[ERROR][%d] COND SIZE [IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}

	if (conds.order_column_offset & BP_HISTORY_O_URL)
		ordercolumn = BP_DB_COMMON_COL_URL;
	else if (conds.order_column_offset & BP_HISTORY_O_TITLE)
		ordercolumn = BP_DB_COMMON_COL_TITLE;
	else if (conds.order_column_offset & BP_HISTORY_O_FREQUENCY)
		ordercolumn = BP_DB_HISTORY_COL_FREQUENCY;
	else if (conds.order_column_offset & BP_HISTORY_O_DATE_VISITED)
		ordercolumn = BP_DB_COMMON_COL_DATE_VISITED;
	else if (conds.order_column_offset & BP_HISTORY_O_DATE_MODIFIED)
		ordercolumn = BP_DB_COMMON_COL_DATE_MODIFIED;
	else
		ordercolumn = BP_DB_COMMON_COL_DATE_CREATED;

	// order, clear
	pthread_mutex_lock(&g_db_mutex);
	if (bp_db_limit_rows(g_db_handle, BP_DB_TABLE_HISTORY, table_size,
			ordercolumn, (conds.ordering <= 0 ? "ASC" : "DESC"), &errorcode) < 0) {
		TRACE_ERROR("[ERROR][SQL] LIMIT SIZE:%d", table_size);
	}
	pthread_mutex_unlock(&g_db_mutex);
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

static bp_error_defs __bp_history_get_info_offset(int sock, int id, bp_shm_defs *shm)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	bp_history_offset offset = 0;
	bp_history_info_fmt info;

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	memset(&info, 0x00, sizeof(bp_history_info_fmt));
	if (bp_ipc_read_custom_type(sock, &offset, sizeof(bp_history_offset)) < 0) {
		TRACE_ERROR("[ERROR][%d] GET_OFFSET [BP_ERROR_IO_ERROR]", id);
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}

	pthread_mutex_lock(&g_db_mutex);
	if (g_db_handle != 0 && g_db_basic_get_info_stmt == NULL) {
		g_db_basic_get_info_stmt =
			bp_db_prepare_basic_get_info_stmt(g_db_handle, BP_DB_TABLE_HISTORY, BP_DB_COMMON_COL_ID);
	}
	if (g_db_basic_get_info_stmt == NULL) {
		bp_ipc_send_errorcode(sock, BP_ERROR_DISK_BUSY);
		pthread_mutex_unlock(&g_db_mutex);
		return BP_ERROR_DISK_BUSY;
	}
	bp_db_bind_value(g_db_basic_get_info_stmt, BP_DB_COL_TYPE_INT, &id, 1);
	int ret = bp_db_exec_stmt(g_db_basic_get_info_stmt);
	if (ret != SQLITE_ROW) {
		TRACE_ERROR("[ERROR][%d] GET_OFFSET [BP_ERROR_ID_NOT_FOUND]", id);
		bp_ipc_send_errorcode(sock, BP_ERROR_ID_NOT_FOUND);
		bp_db_reset(g_db_basic_get_info_stmt);
		pthread_mutex_unlock(&g_db_mutex);
		return BP_ERROR_ID_NOT_FOUND;
	}

	if (offset & BP_HISTORY_O_FREQUENCY)
		info.frequency = sqlite3_column_int(g_db_basic_get_info_stmt, 0);
	if (offset & BP_HISTORY_O_DATE_CREATED)
		info.date_created = sqlite3_column_int(g_db_basic_get_info_stmt, 1);
	if (offset & BP_HISTORY_O_DATE_MODIFIED)
		info.date_modified = sqlite3_column_int(g_db_basic_get_info_stmt, 2);
	if (offset & BP_HISTORY_O_DATE_VISITED)
		info.date_visited = sqlite3_column_int(g_db_basic_get_info_stmt, 3);

	// bob
	if (offset & BP_HISTORY_O_ICON) {
		int recvint = bp_db_get_int_column
				(g_db_handle, id, BP_DB_TABLE_FAVICONS,
				BP_DB_COMMON_COL_BLOB_WIDTH, &errorcode);
		if (recvint > 0)
			info.favicon_width = recvint;
		recvint = bp_db_get_int_column
				(g_db_handle, id, BP_DB_TABLE_FAVICONS,
				BP_DB_COMMON_COL_BLOB_HEIGHT, &errorcode);
		if (recvint > 0)
			info.favicon_height = recvint;
	}
	if (offset & BP_HISTORY_O_SNAPSHOT) {
		int recvint = bp_db_get_int_column
				(g_db_handle, id, BP_DB_TABLE_THUMBNAILS,
				BP_DB_COMMON_COL_BLOB_WIDTH, &errorcode);
		if (recvint > 0)
			info.thumbnail_width = recvint;
		recvint = bp_db_get_int_column
				(g_db_handle, id, BP_DB_TABLE_THUMBNAILS,
				BP_DB_COMMON_COL_BLOB_HEIGHT, &errorcode);
		if (recvint > 0)
			info.thumbnail_height = recvint;
	}
	if (offset & BP_HISTORY_O_WEBICON) {
		int recvint = bp_db_get_int_column
				(g_db_handle, id, BP_DB_TABLE_WEBICONS,
				BP_DB_COMMON_COL_BLOB_WIDTH, &errorcode);
		if (recvint > 0)
			info.webicon_width = recvint;
		recvint = bp_db_get_int_column
				(g_db_handle, id, BP_DB_TABLE_WEBICONS,
				BP_DB_COMMON_COL_BLOB_HEIGHT, &errorcode);
		if (recvint > 0)
			info.webicon_height = recvint;
	}

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	bp_ipc_send_custom_type(sock, &info, sizeof(bp_history_info_fmt));

	// send strings . keep the order with adaptor
	if (offset & BP_HISTORY_O_URL) {
		char *getstr = bp_db_get_text_stmt(g_db_basic_get_info_stmt, 4);
		if (getstr != NULL) {
			bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
			bp_ipc_send_string(sock, getstr);
		} else {
			bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
		}
		free(getstr);
	}
	if (offset & BP_HISTORY_O_TITLE) {
		char *getstr = bp_db_get_text_stmt(g_db_basic_get_info_stmt, 5);
		if (getstr != NULL) {
			bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
			bp_ipc_send_string(sock, getstr);
		} else {
			bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
		}
		free(getstr);
	}

	if (offset & BP_HISTORY_O_ICON) {
		bp_common_get_info_send_blob(g_db_handle, &g_db_mutex,
			BP_DB_TABLE_FAVICONS, sock, id, shm);
	}
	if (offset & BP_HISTORY_O_SNAPSHOT) {
		bp_common_get_info_send_blob(g_db_handle, &g_db_mutex,
			BP_DB_TABLE_THUMBNAILS, sock, id, shm);
	}
	if (offset & BP_HISTORY_O_WEBICON) {
		bp_common_get_info_send_blob(g_db_handle, &g_db_mutex,
			BP_DB_TABLE_WEBICONS, sock, id, shm);
	}
	bp_db_reset(g_db_basic_get_info_stmt);
	pthread_mutex_unlock(&g_db_mutex);
	return BP_ERROR_NONE;
}
/*
static bp_error_defs __bp_history_delete_all_childs(int sock, int remove)
{
	int is_deleted = 1;
	bp_error_defs errorcode = BP_ERROR_NONE;

	// if remove == 1, remove . or set deleted flag
	if (remove == 1) {
		if (bp_db_remove_cond(g_db_handle, BP_DB_TABLE_HISTORY,
				NULL, 0, NULL) < 0) {
			TRACE_ERROR("[ERROR][SQL] DELETE childs all");
			errorcode = BP_ERROR_OUT_OF_MEMORY;
		}
	} else {
		if (bp_db_set_cond2_column(g_db_handle, BP_DB_TABLE_HISTORY,
				BP_DB_COMMON_COL_IS_DELETED, BP_DB_COL_TYPE_INT,
				&is_deleted, NULL, 0, NULL, NULL, 0, NULL) < 0) {
			TRACE_ERROR("[ERROR][SQL] IS_DELETED childs all");
			errorcode = BP_ERROR_OUT_OF_MEMORY;
		}
	}
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}
*/

static bp_error_defs __bp_history_reset(int sock)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	pthread_mutex_lock(&g_db_mutex);
	if (bp_db_remove_cond(g_db_handle, BP_DB_TABLE_HISTORY,
			NULL, 0, 0, NULL, &errorcode) < 0) {
		TRACE_ERROR("[ERROR][SQL][%d] RESET");
	}
	pthread_mutex_unlock(&g_db_mutex);
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

/*
static bp_error_defs __bp_history_set_is_deleted(int sock, int id)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	int is_deleted = 1;
	if (bp_db_set_column(g_db_handle, id, BP_DB_TABLE_HISTORY,
			BP_DB_COMMON_COL_IS_DELETED,
			BP_DB_COL_TYPE_INT, &is_deleted) < 0) {
		TRACE_ERROR("[ERROR][SQL][%d] SET DELETED", id);
		errorcode = BP_ERROR_ID_NOT_FOUND;
	}
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}
*/

static bp_error_defs __bp_history_delete(int sock, int id)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	pthread_mutex_lock(&g_db_mutex);
	if (bp_db_remove_cond(g_db_handle, BP_DB_TABLE_HISTORY,
			BP_DB_COMMON_COL_ID, 0, BP_DB_COL_TYPE_INT, &id,
			&errorcode) < 0) {
		TRACE_ERROR("[ERROR][SQL][%d] DELETE", id);
	}
	pthread_mutex_unlock(&g_db_mutex);
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

bp_error_defs bp_history_ready_resource()
{
#ifdef DATABASE_HISTORY_FILE
	pthread_mutex_lock(&g_db_mutex);
	// By client type, db_handle indicate sql handle
	int phighwater = 0;
	int pcur = 0;
	int ret = 0;
	if (g_db_handle == 0 ||
			(ret = sqlite3_db_status(g_db_handle,
					SQLITE_DBSTATUS_STMT_USED, &pcur, &phighwater,
					0)) != SQLITE_OK) {
		if (g_db_handle != 0) {
			TRACE_INFO("sql(%p) error:%d, used memory:%d, highwater:%d",
					g_db_handle, ret, pcur, phighwater);
			if (g_db_basic_get_info_stmt != NULL) {
				bp_db_finalize(g_db_basic_get_info_stmt);
				g_db_basic_get_info_stmt = NULL;
			}
			bp_db_close(g_db_handle);
			g_db_handle = 0;
		}
		if (bp_db_open(&g_db_handle, DATABASE_HISTORY_FILE) < 0) {
			TRACE_ERROR("[CRITICAL] can not open SQL");
			int sql_errorcode = SQLITE_OK;
			if (g_db_handle != 0) {
				sql_errorcode = sqlite3_errcode(g_db_handle);
				bp_db_close(g_db_handle);
				g_db_handle = 0;
			} else {
				sql_errorcode = SQLITE_FULL;
			}
			pthread_mutex_unlock(&g_db_mutex);
			if (sql_errorcode == SQLITE_FULL)
				return BP_ERROR_DISK_FULL;
			return BP_ERROR_DISK_BUSY;
		}
	}
	if (g_db_handle != 0 && g_db_basic_get_info_stmt == NULL) {
		g_db_basic_get_info_stmt =
			bp_db_prepare_basic_get_info_stmt(g_db_handle, BP_DB_TABLE_HISTORY, BP_DB_COMMON_COL_ID);
	}
	pthread_mutex_unlock(&g_db_mutex);
	return BP_ERROR_NONE;
#else
	TRACE_ERROR("[CRITICAL] Missing SQL info in compile option");
	return BP_ERROR_UNKNOWN;
#endif
}

void bp_history_free_resource()
{
	pthread_mutex_lock(&g_db_mutex);
	if (g_db_handle != 0) {
		TRACE_SECURE_DEBUG("TRY to close [%s]", DATABASE_HISTORY_FILE);
		if (g_db_basic_get_info_stmt != NULL)
			bp_db_finalize(g_db_basic_get_info_stmt);
		g_db_basic_get_info_stmt = NULL;
		bp_db_close(g_db_handle);
		g_db_handle = 0;
	}
	pthread_mutex_unlock(&g_db_mutex);
}

bp_error_defs bp_history_handle_requests(bp_client_slots_defs *slots,
	bp_client_defs *client, bp_command_fmt *client_cmd)
{
	bp_command_defs cmd = BP_CMD_NONE;
	bp_error_defs errorcode = BP_ERROR_NONE;
	int id = 0;
	int cid = 0;
	int sock = -1;

	if (slots == NULL) {
		TRACE_ERROR("[ERROR] NULL slots");
		return BP_ERROR_INVALID_PARAMETER;
	}
	if (client == NULL) {
		TRACE_ERROR("[ERROR] NULL client");
		return BP_ERROR_INVALID_PARAMETER;
	}
	if (client->cmd_socket < 0) {
		TRACE_ERROR("[ERROR] sock establish");
		return BP_ERROR_INVALID_PARAMETER;
	}
	if (client_cmd == NULL) {
		TRACE_ERROR("[ERROR] NULL Command");
		if (bp_ipc_send_errorcode(client->cmd_socket,
				BP_ERROR_INVALID_PARAMETER) < 0)
			return BP_ERROR_IO_ERROR;
		return BP_ERROR_INVALID_PARAMETER;
	}

	id = client_cmd->id;
	cmd = client_cmd->cmd;
	cid = client_cmd->cid;
	sock = client->cmd_socket;

	if ((errorcode = bp_history_ready_resource()) != BP_ERROR_NONE) {
		bp_ipc_send_errorcode(sock, errorcode);
		return errorcode;
	}

	switch (cmd) {
	case BP_CMD_COMMON_GET_CONDS_TIMESTAMP_IDS:
		errorcode = __bp_history_get_cond_timestamp_ids(sock);
		break;
	case BP_CMD_COMMON_GET_CONDS_DATE_COUNT:
		errorcode = __bp_history_get_date_ids_count(sock);
		break;
	case BP_CMD_COMMON_GET_CONDS_DATE_IDS: // ids with date
		errorcode = __bp_history_get_cond_ids(sock, 0);
		break;
	case BP_CMD_COMMON_GET_CONDS_RAW_IDS: // search without excluding http:// or www
		errorcode = __bp_history_get_cond_ids(sock, 1);
		break;
	case BP_CMD_COMMON_GET_CONDS_ORDER_IDS: // duplicated
		errorcode = __bp_history_get_duplicated_ids(sock);
		break;
	case BP_CMD_COMMON_GET_URL:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_HISTORY,
				BP_DB_COMMON_COL_URL, sock, id);
		break;
	case BP_CMD_COMMON_GET_TITLE:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_HISTORY,
				BP_DB_COMMON_COL_TITLE, sock, id);
		break;
	case BP_CMD_COMMON_GET_FAVICON:
		errorcode = bp_common_get_blob
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_FAVICONS,
				BP_DB_COMMON_COL_BLOB, sock, id);
		break;
	case BP_CMD_COMMON_GET_FAVICON_WIDTH:
		errorcode = bp_common_get_blob_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_FAVICONS,
				BP_DB_COMMON_COL_BLOB_WIDTH, sock, id);
		break;
	case BP_CMD_COMMON_GET_FAVICON_HEIGHT:
		errorcode = bp_common_get_blob_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_FAVICONS,
				BP_DB_COMMON_COL_BLOB_HEIGHT, sock, id);
		break;
	case BP_CMD_COMMON_GET_THUMBNAIL:
		errorcode = bp_common_get_blob
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_THUMBNAILS,
				BP_DB_COMMON_COL_BLOB, sock, id);
		break;
	case BP_CMD_COMMON_GET_THUMBNAIL_WIDTH:
		errorcode = bp_common_get_blob_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_THUMBNAILS,
				BP_DB_COMMON_COL_BLOB_WIDTH, sock, id);
		break;
	case BP_CMD_COMMON_GET_THUMBNAIL_HEIGHT:
		errorcode = bp_common_get_blob_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_THUMBNAILS,
				BP_DB_COMMON_COL_BLOB_HEIGHT, sock, id);
		break;
	case BP_CMD_COMMON_GET_ICON:
		errorcode = bp_common_get_blob_shm(g_db_handle,
				&g_db_mutex, BP_DB_TABLE_FAVICONS, sock, id, &client->shm);
		break;
	case BP_CMD_COMMON_GET_SNAPSHOT:
		errorcode = bp_common_get_blob_shm(g_db_handle,
				&g_db_mutex, BP_DB_TABLE_THUMBNAILS, sock, id, &client->shm);
		break;
	case BP_CMD_COMMON_GET_WEBICON:
		errorcode = bp_common_get_blob_shm(g_db_handle,
				&g_db_mutex, BP_DB_TABLE_WEBICONS, sock, id, &client->shm);
		break;
	case BP_CMD_HISTORY_GET_FREQUENCY:
		errorcode = bp_common_get_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_HISTORY,
				BP_DB_HISTORY_COL_FREQUENCY, sock, id);
		break;
	case BP_CMD_COMMON_GET_DATE_CREATED:
		errorcode = bp_common_get_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_HISTORY,
				BP_DB_COMMON_COL_DATETIME_CREATED, sock, id);
		break;
	case BP_CMD_COMMON_GET_DATE_MODIFIED:
		errorcode = bp_common_get_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_HISTORY,
				BP_DB_COMMON_COL_DATETIME_MODIFIED, sock, id);
		break;
	case BP_CMD_COMMON_GET_DATE_VISITED:
		errorcode = bp_common_get_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_HISTORY,
				BP_DB_COMMON_COL_DATETIME_VISITED, sock, id);
		break;
	case BP_CMD_COMMON_GET_INFO_OFFSET:
		errorcode = __bp_history_get_info_offset(sock, id, &client->shm);
		break;
	case BP_CMD_HISTORY_SET_FREQUENCY:
		errorcode = bp_common_set_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_HISTORY,
				BP_DB_HISTORY_COL_FREQUENCY, sock, id);
		break;
	case BP_CMD_COMMON_SET_URL:
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_HISTORY,
				BP_DB_COMMON_COL_URL, sock, id);
		break;
	case BP_CMD_COMMON_SET_TITLE:
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_HISTORY,
				BP_DB_COMMON_COL_TITLE, sock, id);
		break;
	case BP_CMD_COMMON_SET_FAVICON:
		errorcode = bp_common_set_blob
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_FAVICONS,
				BP_DB_COMMON_COL_BLOB, sock, id);
		break;
	case BP_CMD_COMMON_SET_FAVICON_WIDTH:
		errorcode = bp_common_replace_int(g_db_handle,
				&g_db_mutex, BP_DB_TABLE_FAVICONS,
				BP_DB_COMMON_COL_BLOB_WIDTH, sock, id);
		break;
	case BP_CMD_COMMON_SET_FAVICON_HEIGHT:
		errorcode = bp_common_replace_int(g_db_handle,
				&g_db_mutex, BP_DB_TABLE_FAVICONS,
				BP_DB_COMMON_COL_BLOB_HEIGHT, sock, id);
		break;
	case BP_CMD_COMMON_SET_THUMBNAIL:
		errorcode = bp_common_set_blob
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_THUMBNAILS,
				BP_DB_COMMON_COL_BLOB, sock, id);
		break;
	case BP_CMD_COMMON_SET_THUMBNAIL_WIDTH:
		errorcode = bp_common_replace_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_THUMBNAILS,
				BP_DB_COMMON_COL_BLOB_WIDTH, sock, id);
		break;
	case BP_CMD_COMMON_SET_THUMBNAIL_HEIGHT:
		errorcode = bp_common_replace_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_THUMBNAILS,
				BP_DB_COMMON_COL_BLOB_HEIGHT, sock, id);
		break;
	case BP_CMD_COMMON_SET_ICON:
		errorcode = bp_common_set_blob_shm(g_db_handle,
				&g_db_mutex, BP_DB_TABLE_FAVICONS, sock, id, &client->shm);
		break;
	case BP_CMD_COMMON_SET_SNAPSHOT:
		errorcode = bp_common_set_blob_shm(g_db_handle,
				&g_db_mutex, BP_DB_TABLE_THUMBNAILS, sock, id, &client->shm);
		break;
	case BP_CMD_COMMON_SET_WEBICON:
		errorcode = bp_common_set_blob_shm(g_db_handle,
				&g_db_mutex, BP_DB_TABLE_WEBICONS, sock, id, &client->shm);
		break;
	case BP_CMD_COMMON_SET_DATE_CREATED:
		errorcode = bp_common_set_date
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_HISTORY,
				BP_DB_COMMON_COL_DATE_CREATED, sock, id);
		break;
	case BP_CMD_COMMON_SET_DATE_MODIFIED:
		errorcode = bp_common_set_date
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_HISTORY,
				BP_DB_COMMON_COL_DATE_MODIFIED, sock, id);
		break;
	case BP_CMD_COMMON_SET_DATE_VISITED:
		errorcode = bp_common_set_date
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_HISTORY,
				BP_DB_COMMON_COL_DATE_VISITED, sock, id);
		break;
	case BP_CMD_HISTORY_SET_VISIT:
		errorcode = __bp_history_set_visit(sock, id);
		break;
	case BP_CMD_HISTORY_SET_LIMIT_SIZE:
		errorcode = __bp_history_set_limit_size(sock);
		break;
	case BP_CMD_COMMON_CREATE:
		errorcode = bp_common_create
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_HISTORY, sock, id);
		break;
	case BP_CMD_COMMON_DELETE:

#if 0
#ifdef SUPPORT_CLOUD_SYSTEM
		if (client->type == BP_CLIENT_HISTORY &&
				bp_common_is_connected_my_sync_adaptor(slots,
					client->type) == 0) {
			errorcode = __bp_history_set_is_deleted(sock, id);
			break;
		}
#endif
#endif

		errorcode = __bp_history_delete(sock, id);
		break;
	case BP_CMD_COMMON_RESET:

#if 0
#ifdef SUPPORT_CLOUD_SYSTEM
		if (client->type == BP_CLIENT_HISTORY &&
				bp_common_is_connected_my_sync_adaptor(slots,
					client->type) == 0) { // set is_deleted
			errorcode = __bp_history_delete_all_childs(sock, 0);
			break;
		}
#endif
#endif

		errorcode = __bp_history_reset(sock);
		break;
	case BP_CMD_DEINITIALIZE:
		bp_ipc_send_errorcode(sock, errorcode);
		break;
	case BP_CMD_COMMON_NOTI:
		bp_common_send_noti_all(slots, client->type, cid);
		errorcode = BP_ERROR_NONE;
		break;
	case BP_CMD_SET_NOTI_CB:
		client->noti_enable = 1;
		errorcode = BP_ERROR_NONE;
		bp_ipc_send_errorcode(sock, errorcode);
		break;
	case BP_CMD_UNSET_NOTI_CB:
		client->noti_enable = 0;
		errorcode = BP_ERROR_NONE;
		bp_ipc_send_errorcode(sock, errorcode);
		break;
	default:
		TRACE_ERROR("[ERROR][%d] Wrong Command [%d]", cid, cmd);
		errorcode = BP_ERROR_INVALID_PARAMETER;
		if (bp_ipc_send_errorcode(sock, BP_ERROR_INVALID_PARAMETER) < 0)
			errorcode = BP_ERROR_IO_ERROR;
		break;
	}
	return errorcode;
}
