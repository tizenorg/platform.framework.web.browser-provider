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

#include "bookmark-adaptor.h"

static sqlite3 *g_db_handle = 0;
static sqlite3_stmt *g_db_basic_get_info_stmt = NULL;
static pthread_mutex_t g_db_mutex = PTHREAD_MUTEX_INITIALIZER;

static char *__bp_bookmark_basic_conditions(int parent, int type, int is_operator, int is_editable)
{
	char *conditions = NULL;
	if (parent >= 0) {
		char *tmp_cond = sqlite3_mprintf("(%s IS %d)",
				BP_DB_BOOKMARK_COL_PARENT, parent);
		conditions = bp_merge_strings(conditions, tmp_cond);
		sqlite3_free(tmp_cond);
	}

	if (type >= 0) {
		char *tmp_cond = sqlite3_mprintf("%s(%s IS %d)",
				(conditions == NULL ? "" : " AND "),
				BP_DB_BOOKMARK_COL_TYPE, type);
		conditions = bp_merge_strings(conditions, tmp_cond);
		sqlite3_free(tmp_cond);
	}

	if (is_operator >= 0) {
		char *tmp_cond = sqlite3_mprintf("%s(%s IS %d)",
				(conditions == NULL ? "" : " AND "),
				BP_DB_BOOKMARK_COL_IS_OPERATOR, is_operator);
		conditions = bp_merge_strings(conditions, tmp_cond);
		sqlite3_free(tmp_cond);
	}

	if (is_editable >= 0) {
		char *tmp_cond = sqlite3_mprintf("%s(%s IS %d)",
				(conditions == NULL ? "" : " AND "),
				BP_DB_BOOKMARK_COL_IS_EDITABLE, is_editable);
		conditions = bp_merge_strings(conditions, tmp_cond);
		sqlite3_free(tmp_cond);
	}
	return conditions;
}

static char *__bp_bookmark_get_date_query(int is_deleted, bp_bookmark_date_defs date_type, char *checkcolumn)
{
	char *conditions = NULL;
	char *date_cond = NULL;
	char *delete_cond = NULL;
	if (is_deleted >= 0)
		delete_cond = sqlite3_mprintf("%s IS %d", BP_DB_COMMON_COL_IS_DELETED,
			is_deleted);
	if (date_type == BP_BOOKMARK_DATE_TODAY) {
		date_cond =
			sqlite3_mprintf("DATE(%s) = DATE('now')", checkcolumn);
	} else if (date_type == BP_BOOKMARK_DATE_YESTERDAY) {
		date_cond = sqlite3_mprintf("DATE(%s) = DATE('now', '-1 day')",
				checkcolumn);
	} else if (date_type == BP_BOOKMARK_DATE_LAST_7_DAYS) {
		date_cond =
			sqlite3_mprintf("DATE(%s) < DATE('now', '-2 days') AND DATE(%s) > DATE('now','-7 days')",
				checkcolumn, checkcolumn);
	} else if (date_type == BP_BOOKMARK_DATE_LAST_MONTH) {
		date_cond =
			sqlite3_mprintf("DATE(%s) <= DATE('now','-1 months') AND DATE(%s) > DATE('now', '-2 months')",
				checkcolumn, checkcolumn);
	} else if (date_type == BP_BOOKMARK_DATE_OLDER) {
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

static char *__get_column_by_offset(bp_bookmark_offset offset)
{
	if (offset & BP_BOOKMARK_O_URL)
		return BP_DB_COMMON_COL_URL;
	else if (offset & BP_BOOKMARK_O_TITLE)
		return BP_DB_COMMON_COL_TITLE;
	else if (offset & BP_BOOKMARK_O_DATE_VISITED)
		return BP_DB_COMMON_COL_DATE_VISITED;
	else if (offset & BP_BOOKMARK_O_DATE_MODIFIED)
		return BP_DB_COMMON_COL_DATE_MODIFIED;
	else if (offset & BP_BOOKMARK_O_SEQUENCE)
		return BP_DB_BOOKMARK_COL_SEQUENCE;
	return BP_DB_COMMON_COL_DATE_CREATED;
}

static char *__get_operator(bp_bookmark_timestamp_op_defs op)
{
	switch(op) {
	case BP_BOOKMARK_OP_NOT_EQUAL: // !=
		return "IS NOT";
	case BP_BOOKMARK_OP_GREATER: // >
		return ">";
	case BP_BOOKMARK_OP_LESS: // <
		return "<";
	case BP_BOOKMARK_OP_GREATER_QUEAL: // >=
		return ">=";
	case BP_BOOKMARK_OP_LESS_QUEAL: // <=
		return "<=";
	default:
		break;
	}
	return "IS";
}

static bp_error_defs __bp_bookmark_get_cond_ids(int sock, const int raw_search)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	int ids_count = 0;
	int *ids = NULL;
	int is_like = 0;
	bp_bookmark_offset check_offset = 0;
	char *order_column = NULL;
	char *period_column = NULL;
	char *keyword = NULL;
	char *conditions = NULL;
	bp_bookmark_property_cond_fmt props;
	bp_bookmark_rows_cond_fmt conds;

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	memset(&props, 0x00, sizeof(bp_bookmark_property_cond_fmt));
	memset(&conds, 0x00, sizeof(bp_bookmark_rows_cond_fmt));
	if (bp_ipc_read_custom_type(sock, &props,
			sizeof(bp_bookmark_property_cond_fmt)) < 0) {
		TRACE_ERROR("[ERROR] CONDs [BP_ERROR_IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	if (bp_ipc_read_custom_type(sock, &conds,
			sizeof(bp_bookmark_rows_cond_fmt)) < 0) {
		TRACE_ERROR("[ERROR] CONDs [BP_ERROR_IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	// check_column_offset
	if (bp_ipc_read_custom_type
			(sock, &check_offset, sizeof(bp_bookmark_offset)) < 0) {
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

	if (conds.order_offset & BP_BOOKMARK_O_URL)
		order_column = BP_DB_COMMON_COL_URL;
	else if (conds.order_offset & BP_BOOKMARK_O_TITLE)
		order_column = BP_DB_COMMON_COL_TITLE;
	else if (conds.order_offset & BP_BOOKMARK_O_DATE_VISITED)
		order_column = BP_DB_COMMON_COL_DATE_VISITED;
	else if (conds.order_offset & BP_BOOKMARK_O_DATE_MODIFIED)
		order_column = BP_DB_COMMON_COL_DATE_MODIFIED;
	else if (conds.order_offset & BP_BOOKMARK_O_SEQUENCE)
		order_column = BP_DB_BOOKMARK_COL_SEQUENCE;
	else
		order_column = BP_DB_COMMON_COL_DATE_CREATED;

	if (conds.period_offset & BP_BOOKMARK_O_DATE_VISITED)
		period_column = BP_DB_COMMON_COL_DATE_VISITED;
	else if (conds.period_offset & BP_BOOKMARK_O_DATE_MODIFIED)
		period_column = BP_DB_COMMON_COL_DATE_MODIFIED;
	else
		period_column = BP_DB_COMMON_COL_DATE_CREATED;

	conditions =
		__bp_bookmark_get_date_query(0, conds.period_type, period_column);

	char *basic_conditions =
		__bp_bookmark_basic_conditions(props.parent, props.type,
			props.is_operator, props.is_editable);
	if (basic_conditions != NULL) {
		if (conditions != NULL)
			conditions = bp_merge_strings(conditions, " AND ");
		conditions = bp_merge_strings(conditions, basic_conditions);
		sqlite3_free(basic_conditions);
	}

	if (check_offset > 0) {

		if ((check_offset & BP_BOOKMARK_O_TITLE) &&
				(check_offset & BP_BOOKMARK_O_URL)) {
			// inquired
			errorcode = bp_common_get_inquired_ids(g_db_handle,
				&g_db_mutex, BP_DB_TABLE_BOOKMARK, &ids,
				&ids_count, conds.limit, conds.offset, order_column,
				conds.ordering, is_like, keyword, conditions, raw_search);
		} else {
			if (raw_search == 0 && (check_offset & BP_BOOKMARK_O_URL)) {
				errorcode = bp_common_get_duplicated_url_ids(g_db_handle,
				&g_db_mutex, BP_DB_TABLE_BOOKMARK, &ids, &ids_count,
				conds.limit, conds.offset,
				order_column, conds.ordering, is_like, keyword, conditions);
			} else {
				char *checkcolumn = NULL;
				if (check_offset & BP_BOOKMARK_O_TITLE) {
					checkcolumn = BP_DB_COMMON_COL_TITLE;
				} else if (check_offset & BP_BOOKMARK_O_URL) {
					checkcolumn = BP_DB_COMMON_COL_URL;
				} else {
					errorcode = BP_ERROR_INVALID_PARAMETER;
				}
				if (checkcolumn != NULL) {
					errorcode = bp_common_get_duplicated_ids(g_db_handle,
					&g_db_mutex, BP_DB_TABLE_BOOKMARK, &ids, &ids_count,
					conds.limit, conds.offset, checkcolumn,
					order_column, conds.ordering, is_like, keyword, conditions);
				}
			}
		}

	} else {
		errorcode = bp_common_get_ids(g_db_handle, &g_db_mutex,
			BP_DB_TABLE_BOOKMARK, &ids, &ids_count,
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

static bp_error_defs __bp_bookmark_get_cond_timestamp_ids(int sock)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	int ids_count = 0;
	int *ids = NULL;
	int is_like = 0;
	int timestamp_count = 0;
	bp_bookmark_offset check_offset = 0;
	char *order_column = NULL;
	char *keyword = NULL;
	char *conditions = NULL;
	bp_bookmark_property_cond_fmt props;
	bp_bookmark_rows_fmt limits;

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	memset(&props, 0x00, sizeof(bp_bookmark_property_cond_fmt));
	memset(&limits, 0x00, sizeof(bp_bookmark_rows_fmt));
	if (bp_ipc_read_custom_type(sock, &props,
			sizeof(bp_bookmark_property_cond_fmt)) < 0) {
		TRACE_ERROR("[ERROR] CONDs [BP_ERROR_IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	if (bp_ipc_read_custom_type(sock, &limits,
			sizeof(bp_bookmark_rows_fmt)) < 0) {
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
		bp_bookmark_timestamp_fmt t_timestamps[timestamp_count];
		int i = 0;
		for (; i < timestamp_count; i++) {
			if (bp_ipc_read_custom_type(sock, &t_timestamps[i],
				sizeof(bp_bookmark_timestamp_fmt)) < 0) {
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
			(sock, &check_offset, sizeof(bp_bookmark_offset)) < 0) {
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

	char *basic_conditions =
		__bp_bookmark_basic_conditions(props.parent, props.type,
			props.is_operator, props.is_editable);
	if (basic_conditions != NULL) {
		if (conditions != NULL)
			conditions = bp_merge_strings(conditions, " AND ");
		conditions = bp_merge_strings(conditions, basic_conditions);
		sqlite3_free(basic_conditions);
	}

	if (check_offset > 0) {

		if ((check_offset & BP_BOOKMARK_O_TITLE) &&
				(check_offset & BP_BOOKMARK_O_URL)) {
			// inquired
			errorcode = bp_common_get_inquired_ids(g_db_handle,
				&g_db_mutex, BP_DB_TABLE_BOOKMARK, &ids,
				&ids_count, limits.limit, limits.offset, order_column,
				limits.ordering, is_like, keyword, conditions, 0);
		} else {
			if (check_offset & BP_BOOKMARK_O_URL) {
				errorcode =
					bp_common_get_duplicated_url_ids(g_db_handle,
						&g_db_mutex, BP_DB_TABLE_BOOKMARK, &ids,
						&ids_count, limits.limit, limits.offset,
						order_column, limits.ordering, is_like, keyword,
						conditions);
			} else if (check_offset & BP_BOOKMARK_O_TITLE) {
				errorcode = bp_common_get_duplicated_ids(g_db_handle,
						&g_db_mutex, BP_DB_TABLE_BOOKMARK, &ids,
						&ids_count, limits.limit, limits.offset,
						BP_DB_COMMON_COL_TITLE, order_column,
						limits.ordering, is_like, keyword, conditions);
			} else {
				errorcode = BP_ERROR_INVALID_PARAMETER;
			}
		}

	} else {
		errorcode = bp_common_get_ids(g_db_handle, &g_db_mutex,
			BP_DB_TABLE_BOOKMARK, &ids, &ids_count,
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

static bp_error_defs __bp_bookmark_get_duplicated_ids(int sock)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	bp_bookmark_offset offset = 0;
	int parent = -1;
	int type = 0;
	int is_operator = -1;
	int is_editable = -1;
	int ids_count = 0;
	int *ids = NULL;
	char *checkcolumn = NULL;
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
	if (bp_ipc_read_custom_type(sock, &parent, sizeof(int)) < 0) {
		TRACE_ERROR("[ERROR] COND parent [IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	if (bp_ipc_read_custom_type(sock, &type, sizeof(int)) < 0) {
		TRACE_ERROR("[ERROR] COND type [IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	if (bp_ipc_read_custom_type(sock, &is_operator, sizeof(int)) < 0) {
		TRACE_ERROR("[ERROR] COND parent [IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	if (bp_ipc_read_custom_type(sock, &is_editable, sizeof(int)) < 0) {
		TRACE_ERROR("[ERROR] COND type [IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	// check_column_offset
	if (bp_ipc_read_custom_type
			(sock, &offset, sizeof(bp_bookmark_offset)) < 0) {
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

	if (conds.order_column_offset & BP_BOOKMARK_O_URL)
		ordercolumn = BP_DB_COMMON_COL_URL;
	else if (conds.order_column_offset & BP_BOOKMARK_O_TITLE)
		ordercolumn = BP_DB_COMMON_COL_TITLE;
	else if (conds.order_column_offset & BP_BOOKMARK_O_DATE_VISITED)
		ordercolumn = BP_DB_COMMON_COL_DATE_VISITED;
	else if (conds.order_column_offset & BP_BOOKMARK_O_DATE_MODIFIED)
		ordercolumn = BP_DB_COMMON_COL_DATE_MODIFIED;
	else if (conds.order_column_offset & BP_BOOKMARK_O_SEQUENCE)
		ordercolumn = BP_DB_BOOKMARK_COL_SEQUENCE;
	else
		ordercolumn = BP_DB_COMMON_COL_DATE_CREATED;

	char *conditions = __bp_bookmark_basic_conditions(parent,
			type, is_operator, is_editable);

	if ((offset & BP_BOOKMARK_O_TITLE) && (offset & BP_BOOKMARK_O_URL)) {
		// inquired
		errorcode = bp_common_get_inquired_ids(g_db_handle,
			&g_db_mutex, BP_DB_TABLE_BOOKMARK, &ids,
			&ids_count, conds.limit, conds.offset, ordercolumn,
			conds.ordering, is_like, keyword, conditions, 0);
	} else {

		if (offset & BP_BOOKMARK_O_URL) {
			errorcode = bp_common_get_duplicated_url_ids(g_db_handle,
			&g_db_mutex, BP_DB_TABLE_BOOKMARK, &ids, &ids_count,
			conds.limit, conds.offset,
			ordercolumn, conds.ordering, is_like, keyword, conditions);
		} else {
			if (offset & BP_BOOKMARK_O_TITLE)
				checkcolumn = BP_DB_COMMON_COL_TITLE;
			else if(offset & BP_BOOKMARK_O_SYNC)
				checkcolumn = BP_DB_COMMON_COL_SYNC;
			else
				errorcode = BP_ERROR_INVALID_PARAMETER;
			if (checkcolumn != NULL) {
				errorcode = bp_common_get_duplicated_ids(g_db_handle,
					&g_db_mutex, BP_DB_TABLE_BOOKMARK, &ids, &ids_count,
					conds.limit, conds.offset, checkcolumn, ordercolumn,
					conds.ordering, is_like, keyword, conditions);
			}
		}

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

// this function can cover is_operator and editable property
static bp_error_defs __bp_bookmark_get_ids(int sock)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	int parent = -1;
	int type = -1;
	int is_operator = -1;
	int is_editable = -1;
	int ids_count = 0;
	int *ids = NULL;
	char *ordercolumn = NULL;
	bp_db_base_conds_fmt conds;

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	memset(&conds, 0x00, sizeof(bp_db_base_conds_fmt));
	if (bp_ipc_read_custom_type
		(sock, &conds, sizeof(bp_db_base_conds_fmt)) < 0) {
		TRACE_ERROR("[ERROR] CONDs [BP_ERROR_IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	if (bp_ipc_read_custom_type(sock, &parent, sizeof(int)) < 0) {
		TRACE_ERROR("[ERROR] COND parent [IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	if (bp_ipc_read_custom_type(sock, &type, sizeof(int)) < 0) {
		TRACE_ERROR("[ERROR] COND type [IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	if (bp_ipc_read_custom_type(sock, &is_operator, sizeof(int)) < 0) {
		TRACE_ERROR("[ERROR] COND parent [IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	if (bp_ipc_read_custom_type(sock, &is_editable, sizeof(int)) < 0) {
		TRACE_ERROR("[ERROR] COND type [IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}

	if (conds.order_column_offset & BP_BOOKMARK_O_SEQUENCE)
		ordercolumn = BP_DB_BOOKMARK_COL_SEQUENCE;
	else if (conds.order_column_offset & BP_BOOKMARK_O_URL)
		ordercolumn = BP_DB_COMMON_COL_URL;
	else if (conds.order_column_offset & BP_BOOKMARK_O_TITLE)
		ordercolumn = BP_DB_COMMON_COL_TITLE;
	else if (conds.order_column_offset & BP_BOOKMARK_O_DATE_VISITED)
		ordercolumn = BP_DB_COMMON_COL_DATE_VISITED;
	else if (conds.order_column_offset & BP_BOOKMARK_O_DATE_MODIFIED)
		ordercolumn = BP_DB_COMMON_COL_DATE_MODIFIED;
	else if (conds.order_column_offset & BP_BOOKMARK_O_ACCESS_COUNT)
		ordercolumn = BP_DB_BOOKMARK_COL_ACCESS_COUNT;
	else
		ordercolumn = BP_DB_COMMON_COL_DATE_CREATED;

	char *conditions = __bp_bookmark_basic_conditions(parent,
			type, is_operator, is_editable);

	char *is_deleted = NULL;
	if (conditions == NULL) {
		is_deleted = sqlite3_mprintf("(%s IS 0)", BP_DB_COMMON_COL_IS_DELETED);
	} else {
		is_deleted = sqlite3_mprintf(" AND (%s IS 0)", BP_DB_COMMON_COL_IS_DELETED);
	}
	if (is_deleted != NULL) {
		conditions = bp_merge_strings(conditions, is_deleted);
		sqlite3_free(is_deleted);
	}

	errorcode = bp_common_get_ids(g_db_handle, &g_db_mutex,
			BP_DB_TABLE_BOOKMARK, &ids, &ids_count,
			conds.limit, conds.offset,
			ordercolumn, conds.ordering, conditions);

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

// only full with deleted
static bp_error_defs __bp_sync_bookmark_get_full_with_deleted_ids(int sock)
{
	int *ids = NULL;
	int ids_count = 0;

	BP_PRE_CHECK;

	char *conditions =
			sqlite3_mprintf("(%s IS 0)", BP_DB_BOOKMARK_COL_IS_OPERATOR);

	bp_error_defs errorcode = bp_common_get_ids(g_db_handle,
			&g_db_mutex, BP_DB_TABLE_BOOKMARK, &ids, &ids_count, -1, 0,
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

static bp_error_defs __check_write_permission(int id, int ctype)
{
	if (id == WEB_BOOKMARK_ROOT_ID) {
		return BP_ERROR_PERMISSION_DENY;
	}

	bp_error_defs errorcode = BP_ERROR_NONE;
	if (ctype == BP_CLIENT_BOOKMARK_SYNC) { // check is_operator
		pthread_mutex_lock(&g_db_mutex);
		int is_operator = bp_db_get_int_column(g_db_handle, id,
				BP_DB_TABLE_BOOKMARK, BP_DB_BOOKMARK_COL_IS_OPERATOR, &errorcode);
		pthread_mutex_unlock(&g_db_mutex);
		if (errorcode != BP_ERROR_NONE) {
			TRACE_ERROR("[ERROR][%d] IS_OPERATOR", id);
			return errorcode;
		}
		if (is_operator == 1)
			return BP_ERROR_PERMISSION_DENY;
	} else if (ctype == BP_CLIENT_BOOKMARK) { // check editable
		pthread_mutex_lock(&g_db_mutex);
		int is_editable = bp_db_get_int_column(g_db_handle, id,
				BP_DB_TABLE_BOOKMARK, BP_DB_BOOKMARK_COL_IS_EDITABLE, &errorcode);
		pthread_mutex_unlock(&g_db_mutex);
		if (errorcode != BP_ERROR_NONE) {
			TRACE_ERROR("[ERROR][%d] IS_EDITABLE", id);
			return errorcode;
		}
		if (is_editable != 1)
			return BP_ERROR_PERMISSION_DENY;
	}
	return BP_ERROR_NONE;
}

// only dirty
static bp_error_defs __bp_sync_bookmark_get_dirty_ids(int sock)
{
	int *ids = NULL;
	int ids_count = 0;

	BP_PRE_CHECK;

	char *conditions =
		sqlite3_mprintf("(%s IS 1) AND (%s IS 0) AND (%s IS 0)",
			BP_DB_COMMON_COL_DIRTY, BP_DB_COMMON_COL_IS_DELETED,
			BP_DB_BOOKMARK_COL_IS_OPERATOR);

	bp_error_defs errorcode = bp_common_get_ids(g_db_handle,
			&g_db_mutex, BP_DB_TABLE_BOOKMARK, &ids, &ids_count, -1, 0,
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

// only is_deleted
static bp_error_defs __bp_sync_bookmark_get_deleted_ids(int sock)
{
	int *ids = NULL;
	int ids_count = 0;

	BP_PRE_CHECK;

	char *conditions =
		sqlite3_mprintf("(%s IS 1) AND (%s IS 0)",
			BP_DB_COMMON_COL_IS_DELETED,
			BP_DB_BOOKMARK_COL_IS_OPERATOR);

	bp_error_defs errorcode = bp_common_get_ids(g_db_handle,
			&g_db_mutex, BP_DB_TABLE_BOOKMARK, &ids, &ids_count, -1, 0,
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

static bp_error_defs __bp_sync_bookmark_clear_deleted_ids()
{
	int is_deleted = 1;
	int is_operator = 0;
	bp_error_defs errorcode = BP_ERROR_NONE;
	pthread_mutex_lock(&g_db_mutex);
	if (bp_db_remove_cond2(g_db_handle, BP_DB_TABLE_BOOKMARK,
			BP_DB_COMMON_COL_IS_DELETED,
			BP_DB_COL_TYPE_INT, &is_deleted,
			BP_DB_BOOKMARK_COL_IS_OPERATOR, BP_DB_COL_TYPE_INT,
			&is_operator, &errorcode) < 0) {
		TRACE_ERROR("[ERROR][SQL] SYNC CLEAR_DELETED_IDS");
	}
	pthread_mutex_unlock(&g_db_mutex);
	return errorcode;
}

// set max sequence in parent
static bp_error_defs __bp_bookmark_set_max_sequence(int id)
{
	bp_error_defs errorcode = BP_ERROR_NONE;
	pthread_mutex_lock(&g_db_mutex);
	int parent = bp_db_get_int_column(g_db_handle, id,
			BP_DB_TABLE_BOOKMARK, BP_DB_BOOKMARK_COL_PARENT, &errorcode);
	if (errorcode != BP_ERROR_NONE) {
		pthread_mutex_unlock(&g_db_mutex);
		TRACE_ERROR("[ERROR][SQL][%d] SET_SEQUENCE", id);
		return errorcode;
	}
	int max_sequence = bp_db_get_cond2_int_column(g_db_handle,
			BP_DB_TABLE_BOOKMARK, BP_DB_BOOKMARK_COL_SEQUENCE_MAX,
			BP_DB_BOOKMARK_COL_PARENT, BP_DB_COL_TYPE_INT, &parent,
			NULL, BP_DB_COL_TYPE_NONE, NULL, &errorcode);
	if (max_sequence > 0) {
		if (bp_db_set_column(g_db_handle, id, BP_DB_TABLE_BOOKMARK,
				BP_DB_BOOKMARK_COL_SEQUENCE,
				BP_DB_COL_TYPE_INT, &max_sequence, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] SET_SEQUENCE", id);
		}
	}
	pthread_mutex_unlock(&g_db_mutex);
	return errorcode;
}

static bp_error_defs __bp_bookmark_set_sequence(int sock, int id)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	int recv_int = 0;
	if (bp_ipc_read_custom_type(sock, &recv_int, sizeof(int)) < 0) {
		TRACE_ERROR("[ERROR][%d] SET_SEQUENCE [BP_ERROR_IO_ERROR]", id);
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	if (recv_int < 0) {
		__bp_bookmark_set_max_sequence(id);
	} else {
		pthread_mutex_lock(&g_db_mutex);
		if (bp_db_set_column(g_db_handle, id, BP_DB_TABLE_BOOKMARK,
				BP_DB_BOOKMARK_COL_SEQUENCE,
				BP_DB_COL_TYPE_INT, &recv_int, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] SET_SEQUENCE", id);
		}
		pthread_mutex_unlock(&g_db_mutex);
	}
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

static bp_error_defs __bp_bookmark_set_easy_all(int sock, int id, int ctype)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	bp_bookmark_base_fmt info;
	memset(&info, 0x00, sizeof(bp_bookmark_base_fmt));
	if (bp_ipc_read_custom_type
			(sock, &info, sizeof(bp_bookmark_base_fmt)) < 0) {
		TRACE_ERROR("[ERROR][%d] bp_tab_base_fmt [IO_ERROR]", id);
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}

	if (info.type >= 0) {
		errorcode = __check_write_permission(id, ctype);
		if (errorcode != BP_ERROR_NONE) {
			bp_ipc_send_errorcode(sock, errorcode);
			return errorcode;
		}
	}

	int columns_count = 0;
	if (ctype == BP_CLIENT_BOOKMARK)
		columns_count = 1; // default dirty

	// check the number of the columns to update
	if (info.type >= 0)  // if negative, ignore
		columns_count++;
	if (info.parent >= 0)
		columns_count++;
	if (info.sequence >= 0)
		columns_count++;
	if (info.access_count >= 0)
		columns_count++;
	if (info.date_visited > 0)
		columns_count++;

	bp_db_conds_list_fmt columns[columns_count];
	unsigned index = 0;
	int is_dirty = 1;
	memset(&columns, 0x00, columns_count * sizeof(bp_db_conds_list_fmt));
	if (ctype == BP_CLIENT_BOOKMARK) {
		columns[index].column = BP_DB_COMMON_COL_DIRTY;
		columns[index].type = BP_DB_COL_TYPE_INT;
		columns[index].value = &is_dirty;
		index++;
	}
	if (index < columns_count && info.type >= 0) {
		columns[index].column = BP_DB_BOOKMARK_COL_TYPE;
		columns[index].type = BP_DB_COL_TYPE_INT;
		columns[index].value = &info.type;
		index++;
	}
	if (index < columns_count && info.parent >= 0) {
		columns[index].column = BP_DB_BOOKMARK_COL_PARENT;
		columns[index].type = BP_DB_COL_TYPE_INT;
		columns[index].value = &info.parent;
		index++;
	}
	if (index < columns_count && info.sequence >= 0) {
		columns[index].column = BP_DB_BOOKMARK_COL_SEQUENCE;
		columns[index].type = BP_DB_COL_TYPE_INT;
		columns[index].value = &info.sequence;
		index++;
	}
	if (index < columns_count && info.access_count >= 0) {
		columns[index].column = BP_DB_BOOKMARK_COL_ACCESS_COUNT;
		columns[index].type = BP_DB_COL_TYPE_INT;
		columns[index].value = &info.access_count;
		index++;
	}

	pthread_mutex_lock(&g_db_mutex);
	if (bp_db_set_columns(g_db_handle, id, BP_DB_TABLE_BOOKMARK,
			columns_count, columns, &errorcode) < 0) {
		TRACE_ERROR("[ERROR][SQL][%d] UPDATE INFOs", id);
	}
	if (info.date_created > 0) {
		if (bp_db_set_datetime(g_db_handle, id, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_DATE_CREATED, info.date_created, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] UPDATE DATE_CREATED", id);
		}
	}
	if (info.date_visited > 0) {
		if (bp_db_set_datetime(g_db_handle, id, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_DATE_VISITED, info.date_visited, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] UPDATE DATE_VISITED", id);
		}
	}
	if (errorcode == BP_ERROR_NONE) {
		if (bp_db_set_datetime(g_db_handle, id, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_DATE_MODIFIED, info.date_modified, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] UPDATE DATE_MODIFIED", id);
		}
	}
	pthread_mutex_unlock(&g_db_mutex);

	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

static bp_error_defs __bp_bookmark_get_info_offset(int sock, int id, bp_shm_defs *shm)
{
	BP_PRE_CHECK;

	int columns_count = 0;
	int columns_index = 0;
	bp_bookmark_offset offset = 0;
	bp_bookmark_base_fmt info;
	bp_db_get_columns_fmt *columns = NULL;
	bp_error_defs errorcode = BP_ERROR_NONE;

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	memset(&info, 0x00, sizeof(bp_bookmark_base_fmt));
	if (bp_ipc_read_custom_type(sock, &offset, sizeof(bp_bookmark_offset)) < 0) {
		TRACE_ERROR("[ERROR][%d] GET_OFFSET [BP_ERROR_IO_ERROR]", id);
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}

	pthread_mutex_lock(&g_db_mutex);
	if (g_db_handle != 0 && g_db_basic_get_info_stmt == NULL) {
		g_db_basic_get_info_stmt =
			bp_db_prepare_basic_get_info_stmt(g_db_handle, BP_DB_TABLE_BOOKMARK, BP_DB_COMMON_COL_ID);
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

	if (offset & BP_BOOKMARK_O_TYPE)
		info.type = sqlite3_column_int(g_db_basic_get_info_stmt, 0);
	if (offset & BP_BOOKMARK_O_PARENT)
		info.parent = sqlite3_column_int(g_db_basic_get_info_stmt, 1);
	if (offset & BP_BOOKMARK_O_SEQUENCE)
		info.sequence = sqlite3_column_int(g_db_basic_get_info_stmt, 2);
	if (offset & BP_BOOKMARK_O_IS_EDITABLE)
		info.editable = sqlite3_column_int(g_db_basic_get_info_stmt, 3);
	if (offset & BP_BOOKMARK_O_DATE_CREATED)
		info.date_created = sqlite3_column_int(g_db_basic_get_info_stmt, 7);
	if (offset & BP_BOOKMARK_O_DATE_MODIFIED)
		info.date_modified = sqlite3_column_int(g_db_basic_get_info_stmt, 8);

	// getting extra columns
	// check the number of integer values
	if (offset & BP_BOOKMARK_O_IS_OPERATOR)
		columns_count++;
	if (offset & BP_BOOKMARK_O_ACCESS_COUNT)
		columns_count++;
	if (offset & BP_BOOKMARK_O_DATE_VISITED)
		columns_count++;
	if (offset & BP_BOOKMARK_O_ACCOUNT_NAME)
		columns_count++;
	if (offset & BP_BOOKMARK_O_ACCOUNT_TYPE)
		columns_count++;
	if (offset & BP_BOOKMARK_O_DEVICE_NAME)
		columns_count++;
	if (offset & BP_BOOKMARK_O_DEVICE_ID)
		columns_count++;

	if (columns_count > 0) { // extra columns
		// get int first
		columns = (bp_db_get_columns_fmt *)calloc(columns_count,
				sizeof(bp_db_get_columns_fmt));
		if (columns == NULL) {
			bp_ipc_send_errorcode(sock, BP_ERROR_OUT_OF_MEMORY);
			bp_db_reset(g_db_basic_get_info_stmt);
			pthread_mutex_unlock(&g_db_mutex);
			TRACE_ERROR("[ERROR][%d] GET_OFFSET [OUT_OF_MEMORY]", id);
			return BP_ERROR_OUT_OF_MEMORY;
		}

		if (offset & BP_BOOKMARK_O_IS_OPERATOR) {
			columns[columns_index].column =
				BP_DB_BOOKMARK_COL_IS_OPERATOR;
			columns_index++;
		}
		if (offset & BP_BOOKMARK_O_ACCESS_COUNT) {
			columns[columns_index].column =
				BP_DB_BOOKMARK_COL_ACCESS_COUNT;
			columns_index++;
		}
		if (offset & BP_BOOKMARK_O_DATE_VISITED) {
			columns[columns_index].column =
				BP_DB_COMMON_COL_DATETIME_VISITED;
			columns_index++;
		}
		if (offset & BP_BOOKMARK_O_ACCOUNT_NAME) {
			columns[columns_index].column =
				BP_DB_COMMON_COL_ACCOUNT_NAME;
			columns_index++;
		}
		if (offset & BP_BOOKMARK_O_ACCOUNT_TYPE) {
			columns[columns_index].column =
				BP_DB_COMMON_COL_ACCOUNT_TYPE;
			columns_index++;
		}
		if (offset & BP_BOOKMARK_O_DEVICE_NAME) {
			columns[columns_index].column =
				BP_DB_COMMON_COL_DEVICE_NAME;
			columns_index++;
		}
		if (offset & BP_BOOKMARK_O_DEVICE_ID) {
			columns[columns_index].column = BP_DB_COMMON_COL_DEVICE_ID;
			columns_index++;
		}

		if (bp_db_get_columns(g_db_handle, id, BP_DB_TABLE_BOOKMARK,
				columns_count, columns, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] GET INFOs by offset", id);
			bp_ipc_send_errorcode(sock, errorcode);
			bp_db_reset(g_db_basic_get_info_stmt);
			pthread_mutex_unlock(&g_db_mutex);
			free(columns);
			return errorcode;
		}

		columns_index = 0;
		if (offset & BP_BOOKMARK_O_IS_OPERATOR) {
			int *recvint = columns[columns_index].value;
			info.is_operator = *recvint;
			columns_index++;
		}
		if (offset & BP_BOOKMARK_O_ACCESS_COUNT) {
			int *recvint = columns[columns_index].value;
			info.access_count = *recvint;
			columns_index++;
		}
		if (offset & BP_BOOKMARK_O_DATE_VISITED) {
			int *recvint = columns[columns_index].value;
			info.date_visited = *recvint;
			columns_index++;
		}
	}

	if (offset & BP_BOOKMARK_O_ICON) {
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
	if (offset & BP_BOOKMARK_O_SNAPSHOT) {
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
	if (offset & BP_BOOKMARK_O_WEBICON) {
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
	bp_ipc_send_custom_type(sock, &info, sizeof(bp_bookmark_base_fmt));

	// send strings . keep the order with adaptor
	if (offset & BP_BOOKMARK_O_URL) {
		char *getstr = bp_db_get_text_stmt(g_db_basic_get_info_stmt, 4);
		if (getstr != NULL) {
			bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
			bp_ipc_send_string(sock, getstr);
		} else {
			bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
		}
		free(getstr);
	}
	if (offset & BP_BOOKMARK_O_TITLE) {
		char *getstr = bp_db_get_text_stmt(g_db_basic_get_info_stmt, 5);
		if (getstr != NULL) {
			bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
			bp_ipc_send_string(sock, getstr);
		} else {
			bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
		}
		free(getstr);
	}
	if (columns_count > 0 && columns != NULL) {
		if (offset & BP_BOOKMARK_O_ACCOUNT_NAME) {
			char *recvstr = columns[columns_index].value;
			if (recvstr == NULL) {
				bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
			} else {
				bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
				bp_ipc_send_string(sock, recvstr);
			}
			columns_index++;
		}
		if (offset & BP_BOOKMARK_O_ACCOUNT_TYPE) {
			char *recvstr = columns[columns_index].value;
			if (recvstr == NULL) {
				bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
			} else {
				bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
				bp_ipc_send_string(sock, recvstr);
			}
			columns_index++;
		}
		if (offset & BP_BOOKMARK_O_DEVICE_NAME) {
			char *recvstr = columns[columns_index].value;
			if (recvstr == NULL) {
				bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
			} else {
				bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
				bp_ipc_send_string(sock, recvstr);
			}
			columns_index++;
		}
		if (offset & BP_BOOKMARK_O_DEVICE_ID) {
			char *recvstr = columns[columns_index].value;
			if (recvstr == NULL) {
				bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
			} else {
				bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
				bp_ipc_send_string(sock, recvstr);
			}
			columns_index++;
		}
	}
	if (offset & BP_BOOKMARK_O_SYNC) {
		if (sqlite3_column_bytes(g_db_basic_get_info_stmt, 6) <= 0) {
			bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
		} else {
			char *recvstr = (char *)sqlite3_column_text
					(g_db_basic_get_info_stmt, 6);
			bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
			bp_ipc_send_string(sock, recvstr);
		}
	}
	if (offset & BP_BOOKMARK_O_ICON) {
		bp_common_get_info_send_blob(g_db_handle, &g_db_mutex,
			BP_DB_TABLE_FAVICONS, sock, id, shm);
	}
	if (offset & BP_BOOKMARK_O_SNAPSHOT) {
		bp_common_get_info_send_blob(g_db_handle, &g_db_mutex,
			BP_DB_TABLE_THUMBNAILS, sock, id, shm);
	}
	if (offset & BP_BOOKMARK_O_WEBICON) {
		bp_common_get_info_send_blob(g_db_handle, &g_db_mutex,
			BP_DB_TABLE_WEBICONS, sock, id, shm);
	}
	bp_db_free_columns_fmt_values(columns_count, columns);
	free(columns);
	bp_db_reset(g_db_basic_get_info_stmt);
	pthread_mutex_unlock(&g_db_mutex);
	return BP_ERROR_NONE;
}


static bp_error_defs __bp_bookmark_csc_set_all(int sock, int id)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	bp_bookmark_csc_base_fmt info;
	memset(&info, 0x00, sizeof(bp_bookmark_csc_base_fmt));
	if (bp_ipc_read_custom_type
			(sock, &info, sizeof(bp_bookmark_csc_base_fmt)) < 0) {
		TRACE_ERROR("[ERROR][IO][%d] read", id);
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}

	if (id < 0) {
		id = bp_common_make_unique_id(g_db_handle, &g_db_mutex,
				BP_DB_TABLE_BOOKMARK);
		if (id < 0) {
			bp_ipc_send_errorcode(sock, BP_ERROR_DISK_BUSY);
			return BP_ERROR_DISK_BUSY;
		}
		pthread_mutex_lock(&g_db_mutex);
		if (bp_db_insert_column(g_db_handle, id, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_DATE_CREATED, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] INSERT_CSC", id);
			bp_ipc_send_errorcode(sock, errorcode);
			pthread_mutex_unlock(&g_db_mutex);
			return errorcode;
		}
	} else {
		pthread_mutex_lock(&g_db_mutex);
		int check_id = bp_db_get_int_column(g_db_handle, id,
			BP_DB_TABLE_BOOKMARK, BP_DB_COMMON_COL_ID, &errorcode);
		if (check_id < 0 && errorcode != BP_ERROR_ID_NOT_FOUND) {
			bp_ipc_send_errorcode(sock, errorcode);
			pthread_mutex_unlock(&g_db_mutex);
			return errorcode;
		}
	}

	errorcode = BP_ERROR_NONE;
	if (info.type >= 0) {
		if (bp_db_set_column(g_db_handle, id, BP_DB_TABLE_BOOKMARK,
				BP_DB_BOOKMARK_COL_TYPE,
				BP_DB_COL_TYPE_INT, &info.type, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] SET_TYPE", id);
		}
	}
	if (bp_db_set_column(g_db_handle, id, BP_DB_TABLE_BOOKMARK,
			BP_DB_BOOKMARK_COL_PARENT,
			BP_DB_COL_TYPE_INT, &info.parent, &errorcode) < 0) {
		TRACE_ERROR("[ERROR][SQL][%d] SET_PARENT", id);
	}

	if (errorcode == BP_ERROR_NONE) { // update additional info
		int is_operator = 1;
		if (bp_db_set_column(g_db_handle, id, BP_DB_TABLE_BOOKMARK,
				BP_DB_BOOKMARK_COL_IS_OPERATOR,
				BP_DB_COL_TYPE_INT, &is_operator, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] SET_IS_OPERATOR", id);
		}
		if (bp_db_set_column(g_db_handle, id, BP_DB_TABLE_BOOKMARK,
				BP_DB_BOOKMARK_COL_IS_EDITABLE,
				BP_DB_COL_TYPE_INT, &info.editable, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] SET_IS_EDITABLE", id);
		}
		if (bp_db_set_datetime(g_db_handle, id, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_DATE_MODIFIED, -1, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] SET_DATE_MODIFIED", id);
		}
		int max_sequence = bp_db_get_cond2_int_column(g_db_handle,
			BP_DB_TABLE_BOOKMARK, BP_DB_BOOKMARK_COL_SEQUENCE_MAX,
			BP_DB_BOOKMARK_COL_PARENT, BP_DB_COL_TYPE_INT, &info.parent,
			NULL, BP_DB_COL_TYPE_NONE, NULL, &errorcode);
		if (max_sequence > 0) {
			if (bp_db_set_column(g_db_handle, id, BP_DB_TABLE_BOOKMARK,
					BP_DB_BOOKMARK_COL_SEQUENCE,
					BP_DB_COL_TYPE_INT, &max_sequence, &errorcode) < 0) {
				TRACE_ERROR("[ERROR][SQL][%d] SET_SEQUENCE", id);
			}
		}
	}
	bp_ipc_send_errorcode(sock, errorcode);
	if (errorcode == BP_ERROR_NONE)
		bp_ipc_send_custom_type(sock, &id, sizeof(int));
	pthread_mutex_unlock(&g_db_mutex);
	return errorcode;
}

static bp_error_defs __bp_bookmark_csc_get_all(int sock, int id)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	// check id
	pthread_mutex_lock(&g_db_mutex);
	if (bp_db_get_int_column(g_db_handle, id, BP_DB_TABLE_BOOKMARK,
			BP_DB_COMMON_COL_ID, &errorcode) < 0) {
		pthread_mutex_unlock(&g_db_mutex);
		TRACE_ERROR("[ERROR][%d] CHECK ID", id);
		bp_ipc_send_errorcode(sock, errorcode);
		return errorcode;
	}

	bp_bookmark_csc_base_fmt info;
	memset(&info, 0x00, sizeof(bp_bookmark_csc_base_fmt));
	info.type =
		bp_db_get_int_column(g_db_handle, id, BP_DB_TABLE_BOOKMARK,
			BP_DB_BOOKMARK_COL_TYPE, &errorcode);
	info.parent =
		bp_db_get_int_column(g_db_handle, id, BP_DB_TABLE_BOOKMARK,
			BP_DB_BOOKMARK_COL_PARENT, &errorcode);
	info.editable =
		bp_db_get_int_column(g_db_handle, id, BP_DB_TABLE_BOOKMARK,
			BP_DB_BOOKMARK_COL_IS_EDITABLE, &errorcode);
	pthread_mutex_unlock(&g_db_mutex);
	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	bp_ipc_send_custom_type(sock, &info, sizeof(bp_bookmark_csc_base_fmt));
	return BP_ERROR_NONE;
}

static bp_error_defs __bp_bookmark_csc_reset(int sock)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	int is_operator = 1;
	pthread_mutex_lock(&g_db_mutex);
	if (bp_db_remove_cond(g_db_handle, BP_DB_TABLE_BOOKMARK,
			BP_DB_BOOKMARK_COL_IS_OPERATOR, 0, BP_DB_COL_TYPE_INT,
			&is_operator, &errorcode) < 0) {
		TRACE_ERROR("[ERROR][SQL][%d] RESET");
	}
	pthread_mutex_unlock(&g_db_mutex);
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}


// do not lock mutex
static bp_error_defs __bp_bookmark_delete_all_childs(int parent, int remove)
{
	int *ids = NULL;
	int ids_count = 0;
	int i = 0;
	int is_deleted = 1;
	int is_editable = 1;
	int type = 1;
	bp_error_defs errorcode = BP_ERROR_NONE;

	char *conditions = __bp_bookmark_basic_conditions(parent,
			type, -1, is_editable);

	pthread_mutex_lock(&g_db_mutex);
	int check_ids_count = bp_db_get_custom_conds_rows_count(g_db_handle,
			BP_DB_TABLE_BOOKMARK, BP_DB_COMMON_COL_ID, conditions,
			&errorcode);
	if (check_ids_count > 0) {
		if (check_ids_count > MAX_LIMIT_ROWS_COUNT)
			check_ids_count = MAX_LIMIT_ROWS_COUNT;
		ids = (int *)calloc(check_ids_count, sizeof(int));
		if (ids == NULL) {
			pthread_mutex_unlock(&g_db_mutex);
			if (conditions != NULL)
				sqlite3_free(conditions);
			return BP_ERROR_OUT_OF_MEMORY;
		}
		ids_count = bp_db_get_custom_conds_ids(g_db_handle,
			BP_DB_TABLE_BOOKMARK, ids, BP_DB_COMMON_COL_ID,
			check_ids_count, 0, NULL, "ASC", conditions, &errorcode);
	}
	pthread_mutex_unlock(&g_db_mutex);
	if (conditions != NULL)
		sqlite3_free(conditions);
	for (i = 0; i < ids_count; i++) {
		errorcode = __bp_bookmark_delete_all_childs(ids[i], 0);
		if (errorcode != BP_ERROR_NONE)
			break;
	}
	free(ids);

	if (errorcode == BP_ERROR_NONE) {
		pthread_mutex_lock(&g_db_mutex);
		// if remove == 1, remove . or set deleted flag
		if (remove == 1) {
			if (bp_db_remove_cond(g_db_handle, BP_DB_TABLE_BOOKMARK,
					BP_DB_BOOKMARK_COL_PARENT, 0, BP_DB_COL_TYPE_INT,
					&parent, &errorcode) < 0) {
				TRACE_ERROR("[ERROR][SQL] DELETE childs of %d", parent);
			}
		} else {
			if (bp_db_set_cond2_column(g_db_handle, BP_DB_TABLE_BOOKMARK,
					BP_DB_COMMON_COL_IS_DELETED, BP_DB_COL_TYPE_INT,
					&is_deleted,
					BP_DB_BOOKMARK_COL_PARENT, BP_DB_COL_TYPE_INT,
					&parent, NULL, 0, NULL, &errorcode) < 0) {
				TRACE_ERROR("[ERROR][SQL] DELETE childs of %d", parent);
			}
		}
		pthread_mutex_unlock(&g_db_mutex);
	}
	return errorcode;
}

static bp_error_defs __bp_bookmark_reset(int sock)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	int is_editable = 1;
	pthread_mutex_lock(&g_db_mutex);
	if (bp_db_remove_cond(g_db_handle, BP_DB_TABLE_BOOKMARK,
			BP_DB_BOOKMARK_COL_IS_EDITABLE, 0, BP_DB_COL_TYPE_INT,
			&is_editable, &errorcode) < 0) {
		TRACE_ERROR("[ERROR][SQL][%d] RESET");
	}
	pthread_mutex_unlock(&g_db_mutex);
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

static bp_error_defs __bp_bookmark_set_is_deleted(int sock, int id)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	int is_deleted = 1;
	int type = 0;
	pthread_mutex_lock(&g_db_mutex);
	if (id == WEB_BOOKMARK_ROOT_ID)
		type = 1;
	else
		type = bp_db_get_int_column(g_db_handle, id,
			BP_DB_TABLE_BOOKMARK, BP_DB_BOOKMARK_COL_TYPE, &errorcode);
	pthread_mutex_unlock(&g_db_mutex);
	if (type == 1) {  // set deleted flag of all child
		errorcode = __bp_bookmark_delete_all_childs(id, 0);
	}
	if (errorcode == BP_ERROR_NONE) {
		if (id != WEB_BOOKMARK_ROOT_ID) {
			pthread_mutex_lock(&g_db_mutex);
			if (bp_db_set_column(g_db_handle, id, BP_DB_TABLE_BOOKMARK,
					BP_DB_COMMON_COL_IS_DELETED,
					BP_DB_COL_TYPE_INT, &is_deleted, &errorcode) < 0) {
				TRACE_ERROR("[ERROR][SQL][%d] SET DELETED", id);
			}
			pthread_mutex_unlock(&g_db_mutex);
		}
	}
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

static bp_error_defs __bp_bookmark_delete(int sock, int id)
{
	BP_PRE_CHECK;

	if (id == WEB_BOOKMARK_ROOT_ID)
		return __bp_bookmark_reset(sock);

	bp_error_defs errorcode = BP_ERROR_NONE;
	pthread_mutex_lock(&g_db_mutex);
	int type = bp_db_get_int_column(g_db_handle, id,
			BP_DB_TABLE_BOOKMARK, BP_DB_BOOKMARK_COL_TYPE, &errorcode);
	pthread_mutex_unlock(&g_db_mutex);
	if (type == 1) {  // delete all child
		errorcode = __bp_bookmark_delete_all_childs(id, 1);
	}
	if (errorcode == BP_ERROR_NONE) {
		pthread_mutex_lock(&g_db_mutex);
		if (bp_db_remove_cond(g_db_handle, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_ID, 0, BP_DB_COL_TYPE_INT, &id,
				&errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] DELETE", id);
		}
		pthread_mutex_unlock(&g_db_mutex);
	}
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

static bp_error_defs __bp_bookmark_set_is_deleted_no_care_child(int sock, int id)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	int is_deleted = 1;
	if (id == WEB_BOOKMARK_ROOT_ID) {
		errorcode = BP_ERROR_PERMISSION_DENY;
	} else {
		pthread_mutex_lock(&g_db_mutex);
		if (bp_db_set_column(g_db_handle, id, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_IS_DELETED,
				BP_DB_COL_TYPE_INT, &is_deleted, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] SET_DELETED_NO_CARE_CHILD", id);
		}
		pthread_mutex_unlock(&g_db_mutex);
	}
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

static bp_error_defs __bp_bookmark_delete_no_care_child(int sock, int id)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	if (id == WEB_BOOKMARK_ROOT_ID) {
		errorcode = BP_ERROR_PERMISSION_DENY;
	} else {
		pthread_mutex_lock(&g_db_mutex);
		if (bp_db_remove_cond(g_db_handle, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_ID, 0, BP_DB_COL_TYPE_INT, &id,
				&errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] DELETE_NO_CARE_CHILD", id);
		}
		pthread_mutex_unlock(&g_db_mutex);
	}
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

static bp_error_defs __bp_bookmark_backup(int sock)
{
	BP_PRE_CHECK;

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	bp_error_defs errorcode = BP_ERROR_NONE;
	char *recv_str = bp_ipc_read_string(sock);
	if (recv_str == NULL) {
		TRACE_ERROR("[ERROR] BACKUP [IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}

	pthread_mutex_lock(&g_db_mutex);
	if (bp_db_backup(g_db_handle, recv_str) < 0)
		errorcode = bp_common_sql_errorcode(sqlite3_errcode(g_db_handle));
	pthread_mutex_unlock(&g_db_mutex);

	free(recv_str);
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

static bp_error_defs __bp_bookmark_restore(int sock)
{
	BP_PRE_CHECK;

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	bp_error_defs errorcode = BP_ERROR_NONE;
	char *recv_str = bp_ipc_read_string(sock);
	if (recv_str == NULL) {
		TRACE_ERROR("[ERROR] RESTORE [IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}

	pthread_mutex_lock(&g_db_mutex);
	if (bp_db_restore(g_db_handle, recv_str) < 0)
		errorcode = BP_ERROR_IO_ERROR;
	pthread_mutex_unlock(&g_db_mutex);

	free(recv_str);
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

bp_error_defs bp_bookmark_ready_resource()
{
#ifdef DATABASE_BOOKMARK_FILE
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
		if (bp_db_open(&g_db_handle, DATABASE_BOOKMARK_FILE) < 0) {
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
			bp_db_prepare_basic_get_info_stmt(g_db_handle, BP_DB_TABLE_BOOKMARK, BP_DB_COMMON_COL_ID);
	}
	pthread_mutex_unlock(&g_db_mutex);
	return BP_ERROR_NONE;
#else
	TRACE_ERROR("[CRITICAL] Missing SQL info in compile option");
	return BP_ERROR_UNKNOWN;
#endif
}

void bp_bookmark_free_resource()
{
	// if used by other thread, do not wait and close.
	pthread_mutex_lock(&g_db_mutex);
	if (g_db_handle != 0) {
		TRACE_SECURE_DEBUG("TRY to close [%s]", DATABASE_BOOKMARK_FILE);
		if (g_db_basic_get_info_stmt != NULL)
			bp_db_finalize(g_db_basic_get_info_stmt);
		g_db_basic_get_info_stmt = NULL;
		bp_db_close(g_db_handle);
		g_db_handle = 0;
	}
	pthread_mutex_unlock(&g_db_mutex);
}

bp_error_defs bp_bookmark_handle_requests(bp_client_slots_defs *slots,
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

	if ((errorcode = bp_bookmark_ready_resource()) != BP_ERROR_NONE) {
		bp_ipc_send_errorcode(sock, errorcode);
		return errorcode;
	}

	switch (cmd) {
	case BP_CMD_COMMON_GET_CONDS_TIMESTAMP_IDS:
		errorcode = __bp_bookmark_get_cond_timestamp_ids(sock);
		break;
	case BP_CMD_COMMON_GET_CONDS_DATE_IDS: // ids with date
		errorcode = __bp_bookmark_get_cond_ids(sock, 0);
		break;
	case BP_CMD_COMMON_GET_CONDS_RAW_IDS: // search without excluding http:// or www
		errorcode = __bp_bookmark_get_cond_ids(sock, 1);
		break;
	case BP_CMD_COMMON_GET_FULL_IDS:
		errorcode = bp_common_get_full_ids
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK, sock);
		break;
	case BP_CMD_COMMON_GET_FULL_WITH_DELETED_IDS:
		if (client->type != BP_CLIENT_BOOKMARK_SYNC) {
			errorcode = bp_common_get_full_with_deleted_ids
					(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK, sock);
			break;
		}
		errorcode = __bp_sync_bookmark_get_full_with_deleted_ids(sock);
		break;
	case BP_CMD_COMMON_GET_DIRTY_IDS:
		if (client->type != BP_CLIENT_BOOKMARK_SYNC) {
			// include operator's bookmarks
			errorcode = bp_common_get_dirty_ids
					(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK, sock);
			break;
		}
		errorcode = __bp_sync_bookmark_get_dirty_ids(sock);
		break;
	case BP_CMD_COMMON_GET_DELETED_IDS:
		if (client->type != BP_CLIENT_BOOKMARK_SYNC) {
			errorcode = bp_common_get_deleted_ids
					(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK, sock);
			break;
		}
		errorcode = __bp_sync_bookmark_get_deleted_ids(sock);
		break;
	case BP_CMD_COMMON_GET_CONDS_ORDER_IDS: // duplicated
		errorcode = __bp_bookmark_get_duplicated_ids(sock);
		break;
	case BP_CMD_COMMON_GET_CONDS_INT_IDS: // get ids commonly
		errorcode = __bp_bookmark_get_ids(sock);
		break;
	case BP_CMD_COMMON_CLEAR_DIRTY_IDS:
		errorcode = bp_common_clear_dirty_ids
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK, sock);
		break;
	case BP_CMD_COMMON_CLEAR_DELETED_IDS:
		if (client->type != BP_CLIENT_BOOKMARK_SYNC) {
			errorcode = bp_common_clear_deleted_ids
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK, sock);
			break;
		}
		errorcode = __bp_sync_bookmark_clear_deleted_ids();
		bp_ipc_send_errorcode(sock, errorcode);
		break;
	case BP_CMD_BOOKMARK_GET_TYPE:
		errorcode = bp_common_get_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_BOOKMARK_COL_TYPE, sock, id);
		break;
	case BP_CMD_BOOKMARK_GET_PARENT:
		errorcode = bp_common_get_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_BOOKMARK_COL_PARENT, sock, id);
		break;
	case BP_CMD_COMMON_GET_URL:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_URL, sock, id);
		break;
	case BP_CMD_COMMON_GET_TITLE:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_TITLE, sock, id);
		break;
	case BP_CMD_BOOKMARK_GET_SEQUENCE:
		errorcode = bp_common_get_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_BOOKMARK_COL_SEQUENCE, sock, id);
		break;
	case BP_CMD_BOOKMARK_GET_IS_EDITABLE:
		errorcode = bp_common_get_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_BOOKMARK_COL_IS_EDITABLE, sock, id);
		break;
	case BP_CMD_BOOKMARK_GET_IS_OPERATOR:
		errorcode = bp_common_get_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_BOOKMARK_COL_IS_OPERATOR, sock, id);
		break;
	case BP_CMD_BOOKMARK_GET_ACCESS_COUNT:
		errorcode = bp_common_get_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_BOOKMARK_COL_ACCESS_COUNT, sock, id);
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
	case BP_CMD_COMMON_GET_DATE_CREATED:
		errorcode = bp_common_get_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_DATETIME_CREATED, sock, id);
		break;
	case BP_CMD_COMMON_GET_DATE_MODIFIED:
		errorcode = bp_common_get_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_DATETIME_MODIFIED, sock, id);
		break;
	case BP_CMD_COMMON_GET_DATE_VISITED:
		errorcode = bp_common_get_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_DATETIME_VISITED, sock, id);
		break;
	case BP_CMD_COMMON_GET_ACCOUNT_NAME:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_ACCOUNT_NAME, sock, id);
		break;
	case BP_CMD_COMMON_GET_ACCOUNT_TYPE:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_ACCOUNT_TYPE, sock, id);
		break;
	case BP_CMD_COMMON_GET_DEVICE_NAME:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_DEVICE_NAME, sock, id);
		break;
	case BP_CMD_COMMON_GET_DEVICE_ID:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_DEVICE_ID, sock, id);
		break;
	case BP_CMD_COMMON_GET_SYNC:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_SYNC, sock, id);
		break;
	case BP_CMD_COMMON_GET_TAG:
		errorcode = bp_common_get_tag
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TAGS, sock, id);
		break;
	case BP_CMD_COMMON_GET_TAG_IDS:
		errorcode = bp_common_get_tag_ids
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TAGS, sock, id);
		break;
	case BP_CMD_COMMON_GET_INFO_OFFSET:
		errorcode = __bp_bookmark_get_info_offset(sock, id, &client->shm);
		break;
	case BP_CMD_BOOKMARK_SET_TYPE:
		errorcode = __check_write_permission(id, client->type);
		if (errorcode != BP_ERROR_NONE) {
			bp_ipc_send_errorcode(sock, errorcode);
			break;
		}
		errorcode = bp_common_set_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_BOOKMARK_COL_TYPE, sock, id);
		break;
	case BP_CMD_BOOKMARK_SET_PARENT:
		errorcode = __check_write_permission(id, client->type);
		if (errorcode != BP_ERROR_NONE) {
			bp_ipc_send_errorcode(sock, errorcode);
			break;
		}
		errorcode = bp_common_set_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_BOOKMARK_COL_PARENT, sock, id);
		break;
	case BP_CMD_COMMON_SET_URL:
		errorcode = __check_write_permission(id, client->type);
		if (errorcode != BP_ERROR_NONE) {
			bp_ipc_send_errorcode(sock, errorcode);
			break;
		}
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_URL, sock, id);
		break;
	case BP_CMD_COMMON_SET_TITLE:
		errorcode = __check_write_permission(id, client->type);
		if (errorcode != BP_ERROR_NONE) {
			bp_ipc_send_errorcode(sock, errorcode);
			break;
		}
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_TITLE, sock, id);
		break;
	case BP_CMD_BOOKMARK_SET_SEQUENCE:
		errorcode = __bp_bookmark_set_sequence(sock, id);
		break;
	case BP_CMD_BOOKMARK_SET_ACCESS_COUNT:
		errorcode = bp_common_set_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_BOOKMARK_COL_ACCESS_COUNT, sock, id);
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
	case BP_CMD_COMMON_SET_IS_DELETED:
		errorcode = __check_write_permission(id, client->type);
		if (errorcode != BP_ERROR_NONE) {
			bp_ipc_send_errorcode(sock, errorcode);
			break;
		}
		errorcode = bp_common_set_is_deleted
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK, sock, id);
		break;
	case BP_CMD_COMMON_SET_DATE_CREATED:
		errorcode = bp_common_set_date
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_DATE_CREATED, sock, id);
		break;
	case BP_CMD_COMMON_SET_DATE_MODIFIED:
		errorcode = bp_common_set_date
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_DATE_MODIFIED, sock, id);
		break;
	case BP_CMD_COMMON_SET_DATE_VISITED:
		errorcode = bp_common_set_date
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_DATE_VISITED, sock, id);
		break;
	case BP_CMD_COMMON_SET_ACCOUNT_NAME:
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_ACCOUNT_NAME, sock, id);
		break;
	case BP_CMD_COMMON_SET_ACCOUNT_TYPE:
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_ACCOUNT_TYPE, sock, id);
		break;
	case BP_CMD_COMMON_SET_DEVICE_NAME:
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_DEVICE_NAME, sock, id);
		break;
	case BP_CMD_COMMON_SET_DEVICE_ID:
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_DEVICE_ID, sock, id);
		break;
	case BP_CMD_COMMON_SET_SYNC:
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK,
				BP_DB_COMMON_COL_SYNC, sock, id);
		break;
	case BP_CMD_COMMON_SET_TAG:
		errorcode = bp_common_set_tag
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TAGS, sock, id);
		break;
	case BP_CMD_COMMON_UNSET_TAG:
		errorcode = bp_common_unset_tag
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TAGS, sock, id);
		break;
	case BP_CMD_COMMON_CREATE:
		errorcode = bp_common_create
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK, sock, id);
		if (errorcode == BP_ERROR_NONE &&
			client->type == BP_CLIENT_BOOKMARK_SYNC) {
			__bp_bookmark_set_max_sequence(id);
		}
		break;
	case BP_CMD_BOOKMARK_DELETE_NO_CARE_CHILD:
		// workaround API for WEBAPI TC(favorite api)
		errorcode = __check_write_permission(id, client->type);
		if (errorcode != BP_ERROR_NONE) {
			bp_ipc_send_errorcode(sock, errorcode);
			break;
		}
#ifdef SUPPORT_CLOUD_SYSTEM
		if (client->type == BP_CLIENT_BOOKMARK &&
				bp_common_is_connected_my_sync_adaptor(slots,
					client->type) == 0) {
			errorcode = __bp_bookmark_set_is_deleted_no_care_child(sock, id);
			break;
		}
#endif

		errorcode = __bp_bookmark_delete_no_care_child(sock, id);
		break;
	case BP_CMD_COMMON_DELETE:

		errorcode = __check_write_permission(id, client->type);
		if (errorcode != BP_ERROR_NONE) {
			bp_ipc_send_errorcode(sock, errorcode);
			break;
		}

#ifdef SUPPORT_CLOUD_SYSTEM
		if (client->type == BP_CLIENT_BOOKMARK &&
				bp_common_is_connected_my_sync_adaptor(slots,
					client->type) == 0) {
			errorcode = __bp_bookmark_set_is_deleted(sock, id);
			break;
		}
#endif

		errorcode = __bp_bookmark_delete(sock, id);
		break;
	case BP_CMD_DEINITIALIZE:
		if (client->type == BP_CLIENT_BOOKMARK_SYNC)
			errorcode = __bp_sync_bookmark_clear_deleted_ids();
		bp_ipc_send_errorcode(sock, errorcode);
		break;
	case BP_CMD_COMMON_NOTI:
		bp_common_send_noti_all(slots, client->type, cid);
		errorcode = BP_ERROR_NONE;
		break;
	case BP_CMD_COMMON_SET_DIRTY:
		errorcode = __check_write_permission(id, client->type);
		if (errorcode != BP_ERROR_NONE) {
			bp_ipc_send_errorcode(sock, errorcode);
			break;
		}
		errorcode = bp_common_set_dirty
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_BOOKMARK, id);
		break;
	case BP_CMD_BOOKMARK_BACKUP:
		errorcode = __bp_bookmark_backup(sock);
		break;
	case BP_CMD_BOOKMARK_RESTORE:
		errorcode = __bp_bookmark_restore(sock);
		break;
	case BP_CMD_COMMON_SET_EASY_ALL:
		if (client->type == BP_CLIENT_BOOKMARK_CSC) {
			// default is_operator:1
			errorcode = __bp_bookmark_csc_set_all(sock, id);
		} else {
			errorcode = __bp_bookmark_set_easy_all(sock, id, client->type);
		}
		break;
	case BP_CMD_CSC_BOOKMARK_GET_ALL:
		errorcode = __bp_bookmark_csc_get_all(sock, id);
		break;
	case BP_CMD_COMMON_RESET:
		if (client->type == BP_CLIENT_BOOKMARK_CSC)
			errorcode = __bp_bookmark_csc_reset(sock);
		else {

#ifdef SUPPORT_CLOUD_SYSTEM
			if (client->type == BP_CLIENT_BOOKMARK &&
					bp_common_is_connected_my_sync_adaptor(slots,
						client->type) == 0) { // set is_deleted
				errorcode = __bp_bookmark_delete_all_childs
						(WEB_BOOKMARK_ROOT_ID, 0);
				bp_ipc_send_errorcode(sock, errorcode);
				break;
			}
#endif

			errorcode = __bp_bookmark_reset(sock);
		}
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
