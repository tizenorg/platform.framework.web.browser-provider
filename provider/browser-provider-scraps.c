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

#include <sys/types.h>
#include <sys/stat.h>

#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#include <unistd.h> // unlink

#include "browser-provider.h"
#include "browser-provider-log.h"
#include "browser-provider-slots.h"
#include "browser-provider-socket.h"
#include "browser-provider-db.h"
#include "browser-provider-requests.h"

#include "scrap-adaptor.h"

static sqlite3 *g_db_handle = 0;
static sqlite3_stmt *g_db_basic_get_info_stmt = NULL;
static pthread_mutex_t g_db_mutex = PTHREAD_MUTEX_INITIALIZER;

static int __bp_scrap_is_file_exist(const char *file_path)
{
	struct stat file_state;
	int stat_ret;
	if (file_path == NULL) {
		TRACE_ERROR("[NULL-CHECK] file path is NULL");
		return -1;
	}
	stat_ret = stat(file_path, &file_state);
	if (stat_ret == 0)
		if (file_state.st_mode & S_IFREG)
			return 0;
	return -1;
}

static int __bp_scrap_copy_mhtml(char *src, char *mhtmlpath)
{
	int ret = -1;
	if (src != NULL && mhtmlpath != NULL) {
		FILE *sfp = fopen(src, "r");
		if (sfp != NULL) {
			FILE *tfp = fopen(mhtmlpath, "w");
			if (tfp != NULL) {
				char buffer[2048];
				while (!feof(sfp)) {
					if (fgets(buffer, 2048, sfp) != NULL)
						fputs(buffer, tfp);
				}
				fclose(tfp);
				ret = 0;
			} else {
				TRACE_STRERROR("target:%s", mhtmlpath);
			}
			fclose(sfp);
		} else {
			TRACE_STRERROR("src:%s", src);
		}
	}
	return ret;
}

static bp_error_defs __bp_scrap_save_page(int sock, int id)
{
	bp_error_defs errorcode = BP_ERROR_NONE;
	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	char *recv_str = bp_ipc_read_string(sock);
	if (recv_str == NULL) {
		TRACE_ERROR("[ERROR][%d] SET_STRING [IO_ERROR]", id);
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	if (__bp_scrap_is_file_exist(recv_str) < 0) {
		TRACE_ERROR("[ERROR][%d] not found %s", id, recv_str);
		bp_ipc_send_errorcode(sock, BP_ERROR_INVALID_PARAMETER);
		free(recv_str);
		return BP_ERROR_INVALID_PARAMETER;
	}
	pthread_mutex_lock(&g_db_mutex);
	if (bp_db_get_int_column(g_db_handle, id, BP_DB_TABLE_SCRAP,
			BP_DB_COMMON_COL_ID, &errorcode) >= 0) {

#ifdef PROVIDER_DIR
		bp_rebuild_dir(PROVIDER_DIR);
#endif
#ifdef SCRAP_DIR
		bp_rebuild_dir(SCRAP_DIR);
		char *mhtmlpath =
			sqlite3_mprintf("%s/%d.mht", SCRAP_DIR, id);
		if (mhtmlpath != NULL) {
			if (__bp_scrap_is_file_exist(mhtmlpath) == 0)
				unlink(mhtmlpath);
			if (__bp_scrap_copy_mhtml(recv_str, mhtmlpath) < 0) {
				errorcode = BP_ERROR_INVALID_PARAMETER;
			} else {
				if (bp_db_set_column(g_db_handle, id, BP_DB_TABLE_SCRAP,
						BP_DB_SCRAP_COL_PAGE_PATH, BP_DB_COL_TYPE_TEXT,
						mhtmlpath, &errorcode) < 0) {
					TRACE_ERROR("[ERROR][SQL][%d] SET_STRING", id);
					unlink(mhtmlpath);
				}
			}
			sqlite3_free(mhtmlpath);
		} else {
			errorcode = BP_ERROR_OUT_OF_MEMORY;
		}
#endif
	}
	pthread_mutex_unlock(&g_db_mutex);
	free(recv_str);
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

static char *__bp_scrap_get_date_query(int is_deleted, bp_scrap_date_defs date_type, char *checkcolumn)
{
	char *conditions = NULL;
	char *date_cond = NULL;
	char *delete_cond = NULL;
	if (is_deleted >= 0)
		delete_cond = sqlite3_mprintf("%s IS %d", BP_DB_COMMON_COL_IS_DELETED,
			is_deleted);
	if (date_type == BP_SCRAP_DATE_TODAY) {
		date_cond =
			sqlite3_mprintf("DATE(%s) = DATE('now')", checkcolumn);
	} else if (date_type == BP_SCRAP_DATE_YESTERDAY) {
		date_cond = sqlite3_mprintf("DATE(%s) = DATE('now', '-1 day')",
				checkcolumn);
	} else if (date_type == BP_SCRAP_DATE_LAST_7_DAYS) {
		date_cond =
			sqlite3_mprintf("DATE(%s) < DATE('now', '-2 days') AND DATE(%s) > DATE('now','-7 days')",
				checkcolumn, checkcolumn);
	} else if (date_type == BP_SCRAP_DATE_LAST_MONTH) {
		date_cond =
			sqlite3_mprintf("DATE(%s) <= DATE('now','-1 months') AND DATE(%s) > DATE('now', '-2 months')",
				checkcolumn, checkcolumn);
	} else if (date_type == BP_SCRAP_DATE_OLDER) {
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

static bp_error_defs __bp_scrap_get_cond_ids(int sock)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	int ids_count = 0;
	int *ids = NULL;
	int is_like = 0;
	bp_scrap_offset check_offset = 0;
	char *order_column = NULL;
	char *period_column = NULL;
	char *keyword = NULL;
	char *conditions = NULL;
	bp_scrap_rows_cond_fmt conds;

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	memset(&conds, 0x00, sizeof(bp_scrap_rows_cond_fmt));
	if (bp_ipc_read_custom_type(sock, &conds,
			sizeof(bp_scrap_rows_cond_fmt)) < 0) {
		TRACE_ERROR("[ERROR] CONDs [BP_ERROR_IO_ERROR]");
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}
	// check_column_offset
	if (bp_ipc_read_custom_type
			(sock, &check_offset, sizeof(bp_scrap_offset)) < 0) {
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

	if (conds.order_offset & BP_SCRAP_O_URL)
		order_column = BP_DB_COMMON_COL_URL;
	else if (conds.order_offset & BP_SCRAP_O_TITLE)
		order_column = BP_DB_COMMON_COL_TITLE;
	else if (conds.order_offset & BP_SCRAP_O_DEVICE_ID)
		order_column = BP_DB_COMMON_COL_DEVICE_ID;
	else if (conds.order_offset & BP_SCRAP_O_DATE_MODIFIED)
		order_column = BP_DB_COMMON_COL_DATE_MODIFIED;
	else
		order_column = BP_DB_COMMON_COL_DATE_CREATED;

	if (conds.period_offset & BP_SCRAP_O_DATE_MODIFIED)
		period_column = BP_DB_COMMON_COL_DATE_MODIFIED;
	else
		period_column = BP_DB_COMMON_COL_DATE_CREATED;

	conditions =
		__bp_scrap_get_date_query(0, conds.period_type, period_column);

	if (check_offset > 0) {

		if ((check_offset & BP_SCRAP_O_TITLE) &&
				(check_offset & BP_SCRAP_O_URL)) {
			// inquired
			errorcode = bp_common_get_inquired_ids(g_db_handle,
				&g_db_mutex, BP_DB_TABLE_SCRAP, &ids,
				&ids_count, conds.limit, conds.offset, order_column,
				conds.ordering, is_like, keyword, conditions, 0);
		} else {

			if (check_offset & BP_SCRAP_O_URL) {
				errorcode = bp_common_get_duplicated_url_ids(g_db_handle,
				&g_db_mutex, BP_DB_TABLE_SCRAP, &ids, &ids_count,
				conds.limit, conds.offset,
				order_column, conds.ordering, is_like, keyword, conditions);
			} else {
				char *checkcolumn = NULL;
				if (check_offset & BP_SCRAP_O_TITLE)
					checkcolumn = BP_DB_COMMON_COL_TITLE;
				else if(check_offset & BP_SCRAP_O_PAGE)
					checkcolumn = BP_DB_SCRAP_COL_PAGE_PATH;
				else
					errorcode = BP_ERROR_INVALID_PARAMETER;
				if (checkcolumn != NULL) {
					errorcode = bp_common_get_duplicated_ids(g_db_handle,
						&g_db_mutex, BP_DB_TABLE_SCRAP, &ids, &ids_count,
						conds.limit, conds.offset, checkcolumn, order_column,
						conds.ordering, is_like, keyword, conditions);
				}
			}

		}

	} else {
		errorcode = bp_common_get_ids(g_db_handle, &g_db_mutex,
			BP_DB_TABLE_SCRAP, &ids, &ids_count,
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

static bp_error_defs __bp_scrap_get_duplicated_ids(int sock)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	bp_scrap_offset offset = 0;
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
	if (bp_ipc_read_custom_type(sock, &offset, sizeof(bp_scrap_offset)) < 0) {
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

	if (conds.order_column_offset & BP_SCRAP_O_URL)
		ordercolumn = BP_DB_COMMON_COL_URL;
	else if (conds.order_column_offset & BP_SCRAP_O_TITLE)
		ordercolumn = BP_DB_COMMON_COL_TITLE;
	else if (conds.order_column_offset & BP_SCRAP_O_DATE_MODIFIED)
		ordercolumn = BP_DB_COMMON_COL_DATE_MODIFIED;
	else
		ordercolumn = BP_DB_COMMON_COL_DATE_CREATED;

	if ((offset & BP_SCRAP_O_TITLE) && (offset & BP_SCRAP_O_URL)) {
		// inquired
		errorcode = bp_common_get_inquired_ids(g_db_handle, &g_db_mutex,
			BP_DB_TABLE_SCRAP, &ids,
			&ids_count, conds.limit, conds.offset, ordercolumn,
			conds.ordering, is_like, keyword, NULL, 0);
	} else {

		if (offset & BP_SCRAP_O_URL) {
			errorcode = bp_common_get_duplicated_url_ids(g_db_handle,
			&g_db_mutex, BP_DB_TABLE_SCRAP, &ids, &ids_count,
			conds.limit, conds.offset,
			ordercolumn, conds.ordering, is_like, keyword, NULL);
		} else if (offset & BP_SCRAP_O_TITLE) {
			errorcode = bp_common_get_duplicated_ids(g_db_handle,
			&g_db_mutex, BP_DB_TABLE_SCRAP, &ids, &ids_count,
			conds.limit, conds.offset, BP_DB_COMMON_COL_TITLE,
			ordercolumn, conds.ordering, is_like, keyword, NULL);
		} else if (offset & BP_SCRAP_O_PAGE) {
			errorcode = bp_common_get_duplicated_ids(g_db_handle,
			&g_db_mutex, BP_DB_TABLE_SCRAP, &ids, &ids_count,
			conds.limit, conds.offset, BP_DB_SCRAP_COL_PAGE_PATH,
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

static bp_error_defs __bp_scrap_get_info_offset(int sock, int id, bp_shm_defs *shm)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	int columns_count = 0;
	int columns_index = 0;
	bp_scrap_offset offset = 0;
	bp_scrap_info_fmt info;
	bp_db_get_columns_fmt *columns = NULL;

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	memset(&info, 0x00, sizeof(bp_scrap_info_fmt));
	if (bp_ipc_read_custom_type(sock, &offset, sizeof(bp_scrap_offset)) < 0) {
		TRACE_ERROR("[ERROR][%d] GET_OFFSET [BP_ERROR_IO_ERROR]", id);
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}

	pthread_mutex_lock(&g_db_mutex);
	if (g_db_handle != 0 && g_db_basic_get_info_stmt == NULL) {
		g_db_basic_get_info_stmt =
			bp_db_prepare_basic_get_info_stmt(g_db_handle, BP_DB_TABLE_SCRAP, BP_DB_COMMON_COL_ID);
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

	if (offset & BP_SCRAP_O_DATE_CREATED)
		info.date_created = sqlite3_column_int(g_db_basic_get_info_stmt, 4);
	if (offset & BP_SCRAP_O_DATE_MODIFIED)
		info.date_modified = sqlite3_column_int(g_db_basic_get_info_stmt, 5);
	if (offset & BP_SCRAP_O_IS_READ)
		info.is_read = sqlite3_column_int(g_db_basic_get_info_stmt, 6);
	if (offset & BP_SCRAP_O_IS_READER)
		info.is_reader = sqlite3_column_int(g_db_basic_get_info_stmt, 7);
	if (offset & BP_SCRAP_O_IS_NIGHT_MODE)
		info.is_reader = sqlite3_column_int(g_db_basic_get_info_stmt, 8);

	// getting extra columns

	// check the number of integer values
	if (offset & BP_SCRAP_O_ACCOUNT_NAME)
		columns_count++;
	if (offset & BP_SCRAP_O_ACCOUNT_TYPE)
		columns_count++;
	if (offset & BP_SCRAP_O_DEVICE_NAME)
		columns_count++;
	if (offset & BP_SCRAP_O_DEVICE_ID)
		columns_count++;
	if (offset & BP_SCRAP_O_MAIN_CONTENT)
		columns_count++;

	if (columns_count > 0) {
		// get int first
		columns = (bp_db_get_columns_fmt *)calloc(columns_count, sizeof(bp_db_get_columns_fmt));
		if (columns == NULL) {
			bp_ipc_send_errorcode(sock, BP_ERROR_OUT_OF_MEMORY);
			bp_db_reset(g_db_basic_get_info_stmt);
			pthread_mutex_unlock(&g_db_mutex);
			TRACE_ERROR("[ERROR][%d] GET_OFFSET [BP_ERROR_OUT_OF_MEMORY]", id);
			return BP_ERROR_OUT_OF_MEMORY;
		}

		if (offset & BP_SCRAP_O_ACCOUNT_NAME) {
			columns[columns_index].column = BP_DB_COMMON_COL_ACCOUNT_NAME;
			columns_index++;
		}
		if (offset & BP_SCRAP_O_ACCOUNT_TYPE) {
			columns[columns_index].column = BP_DB_COMMON_COL_ACCOUNT_TYPE;
			columns_index++;
		}
		if (offset & BP_SCRAP_O_DEVICE_NAME) {
			columns[columns_index].column = BP_DB_COMMON_COL_DEVICE_NAME;
			columns_index++;
		}
		if (offset & BP_SCRAP_O_DEVICE_ID) {
			columns[columns_index].column = BP_DB_COMMON_COL_DEVICE_ID;
			columns_index++;
		}
		if (offset & BP_SCRAP_O_MAIN_CONTENT) {
			columns[columns_index].column = BP_DB_SCRAP_COL_MAIN_CONTENT;
			columns_index++;
		}

		if (bp_db_get_columns(g_db_handle, id, BP_DB_TABLE_SCRAP,
				columns_count, columns, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] GET INFOs by offset", id);
			bp_ipc_send_errorcode(sock, errorcode);
			bp_db_reset(g_db_basic_get_info_stmt);
			pthread_mutex_unlock(&g_db_mutex);
			free(columns);
			return errorcode;
		}
	}

	if (offset & BP_SCRAP_O_FAVICON) {
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
	if (offset & BP_SCRAP_O_SNAPSHOT) {
		int recvint = bp_db_get_int_column
				(g_db_handle, id, BP_DB_TABLE_THUMBNAILS,
				BP_DB_COMMON_COL_BLOB_WIDTH, &errorcode);
		if (recvint > 0)
			info.snapshot_width = recvint;
		recvint = bp_db_get_int_column
				(g_db_handle, id, BP_DB_TABLE_THUMBNAILS,
				BP_DB_COMMON_COL_BLOB_HEIGHT, &errorcode);
		if (recvint > 0)
			info.snapshot_height = recvint;
	}

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	bp_ipc_send_custom_type(sock, &info, sizeof(bp_scrap_info_fmt));

	// send strings . keep the order with adaptor
	if (offset & BP_SCRAP_O_PAGE) {
		char *getstr = bp_db_get_text_stmt(g_db_basic_get_info_stmt, 1);
		if (getstr != NULL) {
			bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
			bp_ipc_send_string(sock, getstr);
		} else {
			bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
		}
		free(getstr);
	}
	if (offset & BP_SCRAP_O_URL) {
		char *getstr = bp_db_get_text_stmt(g_db_basic_get_info_stmt, 2);
		if (getstr != NULL) {
			bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
			bp_ipc_send_string(sock, getstr);
		} else {
			bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
		}
		free(getstr);
	}
	if (offset & BP_SCRAP_O_TITLE) {
		char *getstr = bp_db_get_text_stmt(g_db_basic_get_info_stmt, 3);
		if (getstr != NULL) {
			bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
			bp_ipc_send_string(sock, getstr);
		} else {
			bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
		}
		free(getstr);
	}
	if (columns_count > 0 && columns != NULL) {
		columns_index = 0;
		if (offset & BP_SCRAP_O_ACCOUNT_NAME) {
			char *recvstr = columns[columns_index].value;
			if (recvstr == NULL) {
				bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
			} else {
				bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
				bp_ipc_send_string(sock, recvstr);
			}
			columns_index++;
		}
		if (offset & BP_SCRAP_O_ACCOUNT_TYPE) {
			char *recvstr = columns[columns_index].value;
			if (recvstr == NULL) {
				bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
			} else {
				bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
				bp_ipc_send_string(sock, recvstr);
			}
			columns_index++;
		}
		if (offset & BP_SCRAP_O_DEVICE_NAME) {
			char *recvstr = columns[columns_index].value;
			if (recvstr == NULL) {
				bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
			} else {
				bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
				bp_ipc_send_string(sock, recvstr);
			}
			columns_index++;
		}
		if (offset & BP_SCRAP_O_DEVICE_ID) {
			char *recvstr = columns[columns_index].value;
			if (recvstr == NULL) {
				bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
			} else {
				bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
				bp_ipc_send_string(sock, recvstr);
			}
			columns_index++;
		}
		if (offset & BP_SCRAP_O_MAIN_CONTENT) {
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
	if (offset & BP_SCRAP_O_FAVICON) {
		bp_common_get_info_send_blob(g_db_handle, &g_db_mutex,
			BP_DB_TABLE_FAVICONS, sock, id, shm);
	}
	if (offset & BP_SCRAP_O_SNAPSHOT) {
		bp_common_get_info_send_blob(g_db_handle, &g_db_mutex,
			BP_DB_TABLE_THUMBNAILS, sock, id, shm);
	}
	bp_db_free_columns_fmt_values(columns_count, columns);
	free(columns);
	bp_db_reset(g_db_basic_get_info_stmt);
	pthread_mutex_unlock(&g_db_mutex);
	return BP_ERROR_NONE;
}

bp_error_defs bp_scraps_ready_resource()
{
#ifdef DATABASE_SCRAP_FILE
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
		if (bp_db_open(&g_db_handle, DATABASE_SCRAP_FILE) < 0) {
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
			bp_db_prepare_basic_get_info_stmt(g_db_handle, BP_DB_TABLE_SCRAP, BP_DB_COMMON_COL_ID);
	}
	pthread_mutex_unlock(&g_db_mutex);
	return BP_ERROR_NONE;
#else
	TRACE_ERROR("[CRITICAL] Missing SQL info in compile option");
	return BP_ERROR_UNKNOWN;
#endif
}

void bp_scraps_free_resource()
{
	pthread_mutex_lock(&g_db_mutex);
	if (g_db_handle != 0) {
		TRACE_SECURE_DEBUG("TRY to close [%s]", DATABASE_SCRAP_FILE);
		if (g_db_basic_get_info_stmt != NULL)
			bp_db_finalize(g_db_basic_get_info_stmt);
		g_db_basic_get_info_stmt = NULL;
		bp_db_close(g_db_handle);
		g_db_handle = 0;
	}
	pthread_mutex_unlock(&g_db_mutex);
}

bp_error_defs bp_scraps_handle_requests(bp_client_slots_defs *slots,
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

	if ((errorcode = bp_scraps_ready_resource()) != BP_ERROR_NONE) {
		bp_ipc_send_errorcode(sock, errorcode);
		return errorcode;
	}

	switch (cmd) {
	case BP_CMD_COMMON_GET_CONDS_DATE_IDS: // ids with date
		errorcode = __bp_scrap_get_cond_ids(sock);
		break;
	case BP_CMD_COMMON_CREATE:
		errorcode = bp_common_create
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP, sock, id);
		break;
	case BP_CMD_COMMON_DELETE:

#ifdef SUPPORT_CLOUD_SYSTEM
		if (client->type == BP_CLIENT_SCRAP &&
				bp_common_is_connected_my_sync_adaptor(slots,
					client->type) == 0) {
			errorcode = bp_common_set_is_deleted
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP, sock, id);
			break;
		}
#endif
		errorcode = bp_common_delete
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP, sock, id);
		if (errorcode == BP_ERROR_NONE) {
#ifdef SCRAP_DIR
			char *mhtmlpath =
				sqlite3_mprintf("%s/%d.mht", SCRAP_DIR, id);
			if (mhtmlpath != NULL) {
				if (__bp_scrap_is_file_exist(mhtmlpath) == 0)
					unlink(mhtmlpath);
				sqlite3_free(mhtmlpath);
			} else {
				errorcode = BP_ERROR_OUT_OF_MEMORY;
			}
#endif
		}
		break;
	case BP_CMD_DEINITIALIZE:
		if (client->type == BP_CLIENT_SCRAP_SYNC) {
			int is_deleted = 1;
			pthread_mutex_lock(&g_db_mutex);
			if (bp_db_remove_cond(g_db_handle, BP_DB_TABLE_SCRAP,
					BP_DB_COMMON_COL_IS_DELETED, 0, BP_DB_COL_TYPE_INT,
					&is_deleted, &errorcode) < 0) {
				TRACE_ERROR("[ERROR][SQL] CLEAR_DELETED_IDS");
			}
			pthread_mutex_unlock(&g_db_mutex);
		}
		bp_ipc_send_errorcode(sock, errorcode);
		break;
	case BP_CMD_COMMON_SET_IS_DELETED:
		errorcode = bp_common_set_is_deleted
			(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP, sock, id);
		if (errorcode == BP_ERROR_NONE) {
#ifdef SCRAP_DIR
			char *mhtmlpath =
				sqlite3_mprintf("%s/%d.mht", SCRAP_DIR, id);
			if (mhtmlpath != NULL) {
				if (__bp_scrap_is_file_exist(mhtmlpath) == 0)
					unlink(mhtmlpath);
				sqlite3_free(mhtmlpath);
			} else {
				errorcode = BP_ERROR_OUT_OF_MEMORY;
			}
#endif
		}
		break;
	case BP_CMD_COMMON_GET_FULL_IDS:
		errorcode = bp_common_get_full_ids
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP, sock);
		break;
	case BP_CMD_COMMON_GET_FULL_WITH_DELETED_IDS:
		errorcode = bp_common_get_full_with_deleted_ids
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP, sock);
		break;
	case BP_CMD_COMMON_GET_DIRTY_IDS:
		errorcode = bp_common_get_dirty_ids
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP, sock);
		break;
	case BP_CMD_COMMON_GET_DELETED_IDS:
		errorcode = bp_common_get_deleted_ids
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP, sock);
		break;
	case BP_CMD_COMMON_CLEAR_DIRTY_IDS:
		errorcode = bp_common_clear_dirty_ids
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP, sock);
		break;
	case BP_CMD_COMMON_CLEAR_DELETED_IDS:
		errorcode = bp_common_clear_deleted_ids
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP, sock);
		break;
	case BP_CMD_COMMON_GET_CONDS_ORDER_IDS: // duplicated
		errorcode = __bp_scrap_get_duplicated_ids(sock);
		break;
	case BP_CMD_COMMON_GET_INFO_OFFSET:
		errorcode = __bp_scrap_get_info_offset(sock, id, &client->shm);
		break;
	case BP_CMD_COMMON_GET_URL:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_COMMON_COL_URL, sock, id);
		break;
	case BP_CMD_COMMON_GET_TITLE:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_COMMON_COL_TITLE, sock, id);
		break;
	case BP_CMD_SCRAP_GET_BASE_DIR:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_SCRAP_COL_BASE_DIR, sock, id);
		break;
	case BP_CMD_SCRAP_GET_PAGE_PATH:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_SCRAP_COL_PAGE_PATH, sock, id);
		break;
	case BP_CMD_COMMON_GET_ICON:
		errorcode = bp_common_get_blob_shm(g_db_handle, &g_db_mutex,
				BP_DB_TABLE_FAVICONS, sock, id, &client->shm);
		break;
	case BP_CMD_COMMON_GET_SNAPSHOT:
		errorcode = bp_common_get_blob_shm(g_db_handle, &g_db_mutex,
				BP_DB_TABLE_THUMBNAILS, sock, id, &client->shm);
		break;
	case BP_CMD_COMMON_GET_DATE_CREATED:
		errorcode = bp_common_get_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_COMMON_COL_DATETIME_CREATED, sock, id);
		break;
	case BP_CMD_COMMON_GET_DATE_MODIFIED:
		errorcode = bp_common_get_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_COMMON_COL_DATETIME_MODIFIED, sock, id);
		break;
	case BP_CMD_SCRAP_GET_IS_READ:
		errorcode = bp_common_get_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_SCRAP_COL_IS_READ, sock, id);
		break;
	case BP_CMD_SCRAP_GET_IS_READER:
		errorcode = bp_common_get_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_SCRAP_COL_IS_READER, sock, id);
		break;
	case BP_CMD_SCRAP_GET_IS_NIGHT_MODE:
		errorcode = bp_common_get_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_SCRAP_COL_IS_NIGHT_MODE, sock, id);
		break;
	case BP_CMD_SCRAP_GET_MAIN_CONTENT:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_SCRAP_COL_MAIN_CONTENT, sock, id);
		break;
	case BP_CMD_COMMON_GET_ACCOUNT_NAME:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_COMMON_COL_ACCOUNT_NAME, sock, id);
		break;
	case BP_CMD_COMMON_GET_ACCOUNT_TYPE:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_COMMON_COL_ACCOUNT_TYPE, sock, id);
		break;
	case BP_CMD_COMMON_GET_DEVICE_NAME:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_COMMON_COL_DEVICE_NAME, sock, id);
		break;
	case BP_CMD_COMMON_GET_DEVICE_ID:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_COMMON_COL_DEVICE_ID, sock, id);
		break;
	case BP_CMD_COMMON_GET_TAG:
		errorcode = bp_common_get_tag
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TAGS, sock, id);
		break;
	case BP_CMD_COMMON_GET_TAG_IDS:
		errorcode = bp_common_get_tag_ids
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TAGS, sock, id);
		break;
	case BP_CMD_COMMON_SET_URL:
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_COMMON_COL_URL, sock, id);
		break;
	case BP_CMD_COMMON_SET_TITLE:
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_COMMON_COL_TITLE, sock, id);
		break;
	case BP_CMD_SCRAP_SET_BASE_DIR:
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_SCRAP_COL_BASE_DIR, sock, id);
		break;
	case BP_CMD_SCRAP_SET_PAGE_PATH:
		errorcode = __bp_scrap_save_page(sock, id);
		break;
	case BP_CMD_COMMON_SET_ICON:
		errorcode = bp_common_set_blob_shm(g_db_handle, &g_db_mutex,
				BP_DB_TABLE_FAVICONS, sock, id, &client->shm);
		break;
	case BP_CMD_COMMON_SET_SNAPSHOT:
		errorcode = bp_common_set_blob_shm(g_db_handle, &g_db_mutex,
				BP_DB_TABLE_THUMBNAILS, sock, id, &client->shm);
		break;
	case BP_CMD_COMMON_SET_DATE_CREATED:
		errorcode = bp_common_set_date
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_COMMON_COL_DATE_CREATED, sock, id);
		break;
	case BP_CMD_COMMON_SET_DATE_MODIFIED:
		errorcode = bp_common_set_date
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_COMMON_COL_DATE_MODIFIED, sock, id);
		break;
	case BP_CMD_SCRAP_SET_IS_READ:
		errorcode = bp_common_set_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_SCRAP_COL_IS_READ, sock, id);
		break;
	case BP_CMD_SCRAP_SET_IS_READER:
		errorcode = bp_common_set_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_SCRAP_COL_IS_READER, sock, id);
		break;
	case BP_CMD_SCRAP_SET_IS_NIGHT_MODE:
		errorcode = bp_common_set_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_SCRAP_COL_IS_NIGHT_MODE, sock, id);
		break;
	case BP_CMD_SCRAP_SET_MAIN_CONTENT:
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_SCRAP_COL_MAIN_CONTENT, sock, id);
		break;
	case BP_CMD_COMMON_SET_ACCOUNT_NAME:
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_COMMON_COL_ACCOUNT_NAME, sock, id);
		break;
	case BP_CMD_COMMON_SET_ACCOUNT_TYPE:
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_COMMON_COL_ACCOUNT_TYPE, sock, id);
		break;
	case BP_CMD_COMMON_SET_DEVICE_NAME:
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_COMMON_COL_DEVICE_NAME, sock, id);
		break;
	case BP_CMD_COMMON_SET_DEVICE_ID:
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP,
				BP_DB_COMMON_COL_DEVICE_ID, sock, id);
		break;
	case BP_CMD_COMMON_SET_TAG:
		errorcode = bp_common_set_tag
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TAGS, sock, id);
		break;
	case BP_CMD_COMMON_UNSET_TAG:
		errorcode = bp_common_unset_tag
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TAGS, sock, id);
		break;
	case BP_CMD_COMMON_NOTI:
		bp_common_send_noti_all(slots, client->type, cid);
		errorcode = BP_ERROR_NONE;
		break;
	case BP_CMD_COMMON_SET_DIRTY:
		errorcode = bp_common_set_dirty
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_SCRAP, id);
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
