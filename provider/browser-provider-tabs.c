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

#include "tab-adaptor.h"

static sqlite3 *g_db_handle = 0;
static sqlite3_stmt *g_db_basic_insert_stmt = NULL;
static pthread_mutex_t g_db_mutex = PTHREAD_MUTEX_INITIALIZER;

static bp_error_defs __bp_tab_get_duplicated_ids(int sock)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	bp_tab_offset offset = 0;
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
	// check_column_offset
	if (bp_ipc_read_custom_type(sock, &offset, sizeof(bp_tab_offset)) < 0) {
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

	if (conds.order_column_offset & BP_TAB_O_URL)
		ordercolumn = BP_DB_COMMON_COL_URL;
	else if (conds.order_column_offset & BP_TAB_O_TITLE)
		ordercolumn = BP_DB_COMMON_COL_TITLE;
	else if (conds.order_column_offset & BP_TAB_O_DEVICE_ID)
		ordercolumn = BP_DB_COMMON_COL_DEVICE_ID;
	else if (conds.order_column_offset & BP_TAB_O_INDEX)
		ordercolumn = BP_DB_TABS_COL_INDEX;
	else if (conds.order_column_offset & BP_TAB_O_DATE_MODIFIED)
		ordercolumn = BP_DB_COMMON_COL_DATE_MODIFIED;
	else
		ordercolumn = BP_DB_COMMON_COL_DATE_CREATED;

	if ((offset & BP_TAB_O_TITLE) && (offset & BP_TAB_O_URL)) {
		// inquired
		errorcode = bp_common_get_inquired_ids(g_db_handle,
			&g_db_mutex, BP_DB_TABLE_TABS, &ids,
			&ids_count, conds.limit, conds.offset, ordercolumn,
			conds.ordering, is_like, keyword, NULL, 0);
	} else {

		if (offset & BP_TAB_O_URL) {
			errorcode = bp_common_get_duplicated_url_ids(g_db_handle,
			&g_db_mutex, BP_DB_TABLE_TABS, &ids, &ids_count,
			conds.limit, conds.offset,
			ordercolumn, conds.ordering, is_like, keyword, NULL);
		} else {
			if (offset & BP_TAB_O_TITLE) {
				checkcolumn = BP_DB_COMMON_COL_TITLE;
			} else if (offset & BP_TAB_O_DEVICE_NAME) {
				checkcolumn = BP_DB_COMMON_COL_DEVICE_NAME;
			} else if (offset & BP_TAB_O_DEVICE_ID) {
				checkcolumn = BP_DB_COMMON_COL_DEVICE_ID;
			} else {
				errorcode = BP_ERROR_INVALID_PARAMETER;
			}
			if (checkcolumn != NULL) {
				errorcode = bp_common_get_duplicated_ids(g_db_handle,
				&g_db_mutex, BP_DB_TABLE_TABS, &ids, &ids_count,
				conds.limit, conds.offset, checkcolumn,
				ordercolumn, conds.ordering, is_like, keyword, NULL);
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
	return errorcode;
}

static bp_error_defs __bp_tab_get_info_offset(int sock, long long int id, bp_shm_defs *shm)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	int columns_count = 0;
	int columns_index = 0;
	bp_tab_offset offset = 0;
	bp_tab_base_fmt info;

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	memset(&info, 0x00, sizeof(bp_tab_base_fmt));
	if (bp_ipc_read_custom_type(sock, &offset, sizeof(bp_tab_offset)) < 0) {
		TRACE_ERROR("[ERROR][%d] GET_OFFSET [BP_ERROR_IO_ERROR]", id);
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}

	// check the number of integer values
	if (offset & BP_TAB_O_INDEX)
		columns_count++;
	if (offset & BP_TAB_O_IS_ACTIVATED)
		columns_count++;
	if (offset & BP_TAB_O_IS_INCOGNITO)
		columns_count++;
	if (offset & BP_TAB_O_BROWSER_INSTANCE)
		columns_count++;
	if (offset & BP_TAB_O_DATE_CREATED)
		columns_count++;
	if (offset & BP_TAB_O_DATE_MODIFIED)
		columns_count++;
	if (offset & BP_TAB_O_URL)
		columns_count++;
	if (offset & BP_TAB_O_TITLE)
		columns_count++;
	if (offset & BP_TAB_O_ACCOUNT_NAME)
		columns_count++;
	if (offset & BP_TAB_O_ACCOUNT_TYPE)
		columns_count++;
	if (offset & BP_TAB_O_DEVICE_NAME)
		columns_count++;
	if (offset & BP_TAB_O_DEVICE_ID)
		columns_count++;
	if (offset & BP_TAB_O_USAGE)
		columns_count++;
	if (offset & BP_TAB_O_SYNC)
		columns_count++;

	if (columns_count > 0) {
		// get int first
		bp_db_get_columns_fmt columns[columns_count];
		memset(&columns, 0x00, columns_count * sizeof(bp_db_get_columns_fmt));
		if (offset & BP_TAB_O_INDEX) {
			columns[columns_index].column = BP_DB_TABS_COL_INDEX;
			columns_index++;
		}
		if (offset & BP_TAB_O_IS_ACTIVATED) {
			columns[columns_index].column = BP_DB_TABS_COL_ACTIVATED;
			columns_index++;
		}
		if (offset & BP_TAB_O_IS_INCOGNITO) {
			columns[columns_index].column = BP_DB_TABS_COL_IS_INCOGNITO;
			columns_index++;
		}
		if (offset & BP_TAB_O_BROWSER_INSTANCE) {
			columns[columns_index].column = BP_DB_TABS_COL_BROWSER_INSTANCE;
			columns_index++;
		}
		if (offset & BP_TAB_O_DATE_CREATED) {
			columns[columns_index].column = BP_DB_COMMON_COL_DATETIME_CREATED;
			columns_index++;
		}
		if (offset & BP_TAB_O_DATE_MODIFIED) {
			columns[columns_index].column = BP_DB_COMMON_COL_DATETIME_MODIFIED;
			columns_index++;
		}
		if (offset & BP_TAB_O_URL) {
			columns[columns_index].column = BP_DB_COMMON_COL_URL;
			columns_index++;
		}
		if (offset & BP_TAB_O_TITLE) {
			columns[columns_index].column = BP_DB_COMMON_COL_TITLE;
			columns_index++;
		}
		if (offset & BP_TAB_O_ACCOUNT_NAME) {
			columns[columns_index].column = BP_DB_COMMON_COL_ACCOUNT_NAME;
			columns_index++;
		}
		if (offset & BP_TAB_O_ACCOUNT_TYPE) {
			columns[columns_index].column = BP_DB_COMMON_COL_ACCOUNT_TYPE;
			columns_index++;
		}
		if (offset & BP_TAB_O_DEVICE_NAME) {
			columns[columns_index].column = BP_DB_COMMON_COL_DEVICE_NAME;
			columns_index++;
		}
		if (offset & BP_TAB_O_DEVICE_ID) {
			columns[columns_index].column = BP_DB_COMMON_COL_DEVICE_ID;
			columns_index++;
		}
		if (offset & BP_TAB_O_USAGE) {
			columns[columns_index].column = BP_DB_TABS_COL_USAGE;
			columns_index++;
		}
		if (offset & BP_TAB_O_SYNC) {
			columns[columns_index].column = BP_DB_COMMON_COL_SYNC;
			columns_index++;
		}
		pthread_mutex_lock(&g_db_mutex);
		if (bp_db_get_columns(g_db_handle, id, BP_DB_TABLE_TABS,
				columns_count, columns, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] GET INFOs by offset", id);
			pthread_mutex_unlock(&g_db_mutex);
			bp_ipc_send_errorcode(sock, errorcode);
			return errorcode;
		}
		pthread_mutex_unlock(&g_db_mutex);

		columns_index = 0;
		if (offset & BP_TAB_O_INDEX) {
			int *recvint = columns[columns_index].value;
			info.index = *recvint;
			columns_index++;
		}
		if (offset & BP_TAB_O_IS_ACTIVATED) {
			int *recvint = columns[columns_index].value;
			info.is_activated = *recvint;
			columns_index++;
		}
		if (offset & BP_TAB_O_IS_INCOGNITO) {
			int *recvint = columns[columns_index].value;
			info.is_incognito = *recvint;
			columns_index++;
		}
		if (offset & BP_TAB_O_BROWSER_INSTANCE) {
			int *recvint = columns[columns_index].value;
			info.browser_instance = *recvint;
			columns_index++;
		}
		if (offset & BP_TAB_O_DATE_CREATED) {
			int *recvint = columns[columns_index].value;
			info.date_created = *recvint;
			columns_index++;
		}
		if (offset & BP_TAB_O_DATE_MODIFIED) {
			int *recvint = columns[columns_index].value;
			info.date_modified = *recvint;
			columns_index++;
		}

		if (offset & BP_TAB_O_ICON) {
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
		if (offset & BP_TAB_O_SNAPSHOT) {
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

		bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
		bp_ipc_send_custom_type(sock, &info, sizeof(bp_tab_base_fmt));

		// send strings . keep the order with adaptor
		if (offset & BP_TAB_O_URL) {
			char *recvstr = columns[columns_index].value;
			if (recvstr == NULL) {
				bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
			} else {
				bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
				bp_ipc_send_string(sock, recvstr);
			}
			columns_index++;
		}
		if (offset & BP_TAB_O_TITLE) {
			char *recvstr = columns[columns_index].value;
			if (recvstr == NULL) {
				bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
			} else {
				bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
				bp_ipc_send_string(sock, recvstr);
			}
			columns_index++;
		}
		if (offset & BP_TAB_O_ACCOUNT_NAME) {
			char *recvstr = columns[columns_index].value;
			if (recvstr == NULL) {
				bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
			} else {
				bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
				bp_ipc_send_string(sock, recvstr);
			}
			columns_index++;
		}
		if (offset & BP_TAB_O_ACCOUNT_TYPE) {
			char *recvstr = columns[columns_index].value;
			if (recvstr == NULL) {
				bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
			} else {
				bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
				bp_ipc_send_string(sock, recvstr);
			}
			columns_index++;
		}
		if (offset & BP_TAB_O_DEVICE_NAME) {
			char *recvstr = columns[columns_index].value;
			if (recvstr == NULL) {
				bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
			} else {
				bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
				bp_ipc_send_string(sock, recvstr);
			}
			columns_index++;
		}
		if (offset & BP_TAB_O_DEVICE_ID) {
			char *recvstr = columns[columns_index].value;
			if (recvstr == NULL) {
				bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
			} else {
				bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
				bp_ipc_send_string(sock, recvstr);
			}
			columns_index++;
		}
		if (offset & BP_TAB_O_USAGE) {
			char *recvstr = columns[columns_index].value;
			if (recvstr == NULL) {
				bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
			} else {
				bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
				bp_ipc_send_string(sock, recvstr);
			}
		}
		if (offset & BP_TAB_O_SYNC) {
			char *recvstr = columns[columns_index].value;
			if (recvstr == NULL) {
				bp_ipc_send_errorcode(sock, BP_ERROR_NO_DATA);
			} else {
				bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
				bp_ipc_send_string(sock, recvstr);
			}
		}
		bp_db_free_columns_fmt_values(columns_count, columns);
		if (offset & BP_TAB_O_ICON) {
			bp_common_get_info_send_blob(g_db_handle, &g_db_mutex,
				BP_DB_TABLE_FAVICONS, sock, id, shm);
		}
		if (offset & BP_TAB_O_SNAPSHOT) {
			bp_common_get_info_send_blob(g_db_handle, &g_db_mutex,
				BP_DB_TABLE_THUMBNAILS, sock, id, shm);
		}
	} else {
		pthread_mutex_lock(&g_db_mutex);
		int check_id = bp_db_get_int_column(g_db_handle, id,
				BP_DB_TABLE_TABS, BP_DB_COMMON_COL_ID, &errorcode);
		pthread_mutex_unlock(&g_db_mutex);
		if (check_id < 0) {
			bp_ipc_send_errorcode(sock, errorcode);
			return errorcode;
		}
		if (check_id != id) {
			bp_ipc_send_errorcode(sock, BP_ERROR_ID_NOT_FOUND);
			return BP_ERROR_ID_NOT_FOUND;
		}
	}
	return BP_ERROR_NONE;
}

static bp_error_defs __bp_tab_activate(int sock, long long int id)
{
	bp_error_defs errorcode = BP_ERROR_NONE;
	int activate = 0;
	int is_dirty = 1;
	int activated_id = 0;
	char *device_id = NULL;

	BP_PRE_CHECK;

	pthread_mutex_lock(&g_db_mutex);
	device_id = bp_db_get_text_column(g_db_handle, id,
			BP_DB_TABLE_TABS, BP_DB_COMMON_COL_DEVICE_ID, &errorcode);
	// find id already activated even if device_id is null.
	activate = 1;
	activated_id = bp_db_get_cond2_int_column(g_db_handle,
			BP_DB_TABLE_TABS, BP_DB_COMMON_COL_ID,
			BP_DB_TABS_COL_ACTIVATED, BP_DB_COL_TYPE_INT, &activate,
			BP_DB_COMMON_COL_DEVICE_ID, BP_DB_COL_TYPE_TEXT,
			device_id, &errorcode);
	pthread_mutex_unlock(&g_db_mutex);
	TRACE_SECURE_DEBUG("[%d] current activated:%d device-id:%s", id,
		activated_id, device_id);
	free(device_id);
	if (activated_id == id) {
		TRACE_ERROR("[ACTIVATE][%d] already activated", id);
		bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
		return BP_ERROR_NONE;
	}
	if (activated_id >= 0) { //inactivate current activated id
		activate = 0;
		pthread_mutex_lock(&g_db_mutex);
		if (bp_db_set_column(g_db_handle, activated_id,
				BP_DB_TABLE_TABS, BP_DB_TABS_COL_ACTIVATED,
				BP_DB_COL_TYPE_INT, &activate, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] DEACTIVATE %d", id,
				activated_id);
		} else {
			// set dirty
			if (bp_db_set_column(g_db_handle, activated_id,
					BP_DB_TABLE_TABS, BP_DB_COMMON_COL_DIRTY,
					BP_DB_COL_TYPE_INT, &is_dirty, &errorcode) < 0) {
				TRACE_ERROR("[ERROR][SQL][%d] DIRTY", activated_id);
			}
		}
		pthread_mutex_unlock(&g_db_mutex);
	}
	// if inactivated current activated one. try to activate new one
	activate = 1;
	pthread_mutex_lock(&g_db_mutex);
	if (bp_db_set_column(g_db_handle, id, BP_DB_TABLE_TABS,
			BP_DB_TABS_COL_ACTIVATED,
			BP_DB_COL_TYPE_INT, &activate, &errorcode) < 0) {
		TRACE_ERROR("[ERROR][SQL][%d] ACTIVATE", id);
	} else {
		if (bp_db_set_column(g_db_handle, id, BP_DB_TABLE_TABS,
				BP_DB_COMMON_COL_DIRTY, BP_DB_COL_TYPE_INT,
				&is_dirty, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] DIRTY", id);
		}
	}
	pthread_mutex_unlock(&g_db_mutex);
	bp_ipc_send_errorcode(sock, errorcode);
	return errorcode;
}

static bp_error_defs __bp_tab_set_easy_all(int sock, long long int id, int ctype)
{
	BP_PRE_CHECK;

	bp_error_defs errorcode = BP_ERROR_NONE;
	bp_tab_offset offset = 0; // string properties
	int columns_count = 0;
	int is_update = 0;
	bp_tab_base_fmt tabinfo;

	bp_ipc_send_errorcode(sock, BP_ERROR_NONE);
	memset(&tabinfo, 0x00, sizeof(bp_tab_base_fmt));
	if (bp_ipc_read_custom_type
			(sock, &tabinfo, sizeof(bp_tab_base_fmt)) < 0) {
		TRACE_ERROR("[ERROR][%d] bp_tab_base_fmt [IO_ERROR]", id);
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}

	if (bp_ipc_read_custom_type(sock, &offset, sizeof(bp_tab_offset)) < 0) {
		TRACE_ERROR("[ERROR][%d] GET_OFFSET [IO_ERROR]", id);
		bp_ipc_send_errorcode(sock, BP_ERROR_IO_ERROR);
		return BP_ERROR_IO_ERROR;
	}

	// add conditions
	char *url = NULL;
	char *title = NULL;
	char *account_name = NULL;
	char *account_type = NULL;
	char *device_name = NULL;
	char *device_id = NULL;
	char *usage = NULL;
	char *sync = NULL;
	if (offset & BP_TAB_O_URL) {
		if ((url = bp_ipc_read_string(sock)) == NULL) {
			TRACE_ERROR("[ERROR][%d] READ URL [IO_ERROR]", id);
			errorcode = BP_ERROR_IO_ERROR;
		}
	}
	if (errorcode == BP_ERROR_NONE && offset & BP_TAB_O_TITLE) {
		if ((title = bp_ipc_read_string(sock)) == NULL) {
			TRACE_ERROR("[ERROR][%d] READ TITLE [IO_ERROR]", id);
			errorcode = BP_ERROR_IO_ERROR;
		}
	}
	if (errorcode == BP_ERROR_NONE && offset & BP_TAB_O_ACCOUNT_NAME) {
		if ((account_name = bp_ipc_read_string(sock)) == NULL) {
			TRACE_ERROR("[ERROR][%d] READ ACCOUNT NAME [IO_ERROR]", id);
			errorcode = BP_ERROR_IO_ERROR;
		}
	}
	if (errorcode == BP_ERROR_NONE && offset & BP_TAB_O_ACCOUNT_TYPE) {
		if ((account_type = bp_ipc_read_string(sock)) == NULL) {
			TRACE_ERROR("[ERROR][%d] READ ACCOUNT TYPE [IO_ERROR]", id);
			errorcode = BP_ERROR_IO_ERROR;
		}
	}
	if (errorcode == BP_ERROR_NONE && offset & BP_TAB_O_DEVICE_NAME) {
		if ((device_name = bp_ipc_read_string(sock)) == NULL) {
			TRACE_ERROR("[ERROR][%d] READ DEVICE_NAME [IO_ERROR]", id);
			errorcode = BP_ERROR_IO_ERROR;
		}
	}
	if (errorcode == BP_ERROR_NONE && offset & BP_TAB_O_DEVICE_ID) {
		if ((device_id = bp_ipc_read_string(sock)) == NULL) {
			TRACE_ERROR("[ERROR][%d] READ DEVICE ID [IO_ERROR]", id);
			errorcode = BP_ERROR_IO_ERROR;
		}
	}
	if (errorcode == BP_ERROR_NONE && offset & BP_TAB_O_USAGE) {
		if ((usage = bp_ipc_read_string(sock)) == NULL) {
			TRACE_ERROR("[ERROR][%d] READ USAGE [IO_ERROR]", id);
			errorcode = BP_ERROR_IO_ERROR;
		}
	}
	if (errorcode == BP_ERROR_NONE && offset & BP_TAB_O_SYNC) {
		if ((sync = bp_ipc_read_string(sock)) == NULL) {
			TRACE_ERROR("[ERROR][%d] READ USAGE [IO_ERROR]", id);
			errorcode = BP_ERROR_IO_ERROR;
		}
	}
	if (errorcode != BP_ERROR_NONE) {
		goto TAB_SET_ALL_ERROR;
	}

	// check the number of extra columns to update
	if (tabinfo.is_activated >= 0)
		columns_count++;
	// counting strings
	if (offset & BP_TAB_O_ACCOUNT_NAME)
		columns_count++;
	if (offset & BP_TAB_O_ACCOUNT_TYPE)
		columns_count++;
	if (offset & BP_TAB_O_USAGE)
		columns_count++;
	if (offset & BP_TAB_O_SYNC)
		columns_count++;

	int check_id = -1;
	if (id < 0) {
		id = bp_common_make_unique_id(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS);
			TRACE_INFO("bp_common_make_unique_id:%lld", id);
		if (id < 0) {
			errorcode = BP_ERROR_DISK_BUSY;
			goto TAB_SET_ALL_ERROR;
		}
	} else {
		pthread_mutex_lock(&g_db_mutex);
		check_id = bp_db_get_int_column(g_db_handle, id,
			BP_DB_TABLE_TABS, BP_DB_COMMON_COL_ID, &errorcode);
		pthread_mutex_unlock(&g_db_mutex);
		if (check_id < 0 && errorcode != BP_ERROR_ID_NOT_FOUND) {
			goto TAB_SET_ALL_ERROR;
		}
	}

	errorcode = BP_ERROR_NONE;
	if (id != check_id) { // insert
		pthread_mutex_lock(&g_db_mutex);
		// re-use prepared stmt
		if (g_db_handle != 0 && g_db_basic_insert_stmt == NULL) {
			g_db_basic_insert_stmt =
				bp_db_prepare_basic_insert_stmt(g_db_handle, BP_DB_TABLE_TABS);
		}
		if (g_db_basic_insert_stmt == NULL) {
			errorcode = BP_ERROR_DISK_BUSY;
			pthread_mutex_unlock(&g_db_mutex);
			goto TAB_SET_ALL_ERROR;
		}
		bp_db_bind_value(g_db_basic_insert_stmt, BP_DB_COL_TYPE_INT, &id, 1);
		if (tabinfo.index >= 0) // if negative, ignore
			bp_db_bind_value(g_db_basic_insert_stmt, BP_DB_COL_TYPE_INT, &tabinfo.index, 2);
		if (tabinfo.is_incognito >= 0) // if negative, ignore
			bp_db_bind_value(g_db_basic_insert_stmt, BP_DB_COL_TYPE_INT, &tabinfo.is_incognito, 3);
		if (tabinfo.browser_instance >= 0) // if negative, ignore
			bp_db_bind_value(g_db_basic_insert_stmt, BP_DB_COL_TYPE_INT, &tabinfo.browser_instance, 4);
		if (offset & BP_TAB_O_TITLE)
			bp_db_bind_value(g_db_basic_insert_stmt, BP_DB_COL_TYPE_TEXT, title, 5);
		if (offset & BP_TAB_O_URL)
			bp_db_bind_value(g_db_basic_insert_stmt, BP_DB_COL_TYPE_TEXT, url, 6);
		if (offset & BP_TAB_O_DEVICE_NAME)
			bp_db_bind_value(g_db_basic_insert_stmt, BP_DB_COL_TYPE_TEXT, device_name, 7);
		if (offset & BP_TAB_O_DEVICE_ID)
			bp_db_bind_value(g_db_basic_insert_stmt, BP_DB_COL_TYPE_TEXT, device_id, 8);
		int ret = bp_db_exec_stmt(g_db_basic_insert_stmt);
		if (ret != SQLITE_OK && ret != SQLITE_DONE) {
			errorcode = bp_common_sql_errorcode(ret);
		}
		bp_db_reset(g_db_basic_insert_stmt);
		pthread_mutex_unlock(&g_db_mutex);
	} else { // update
		is_update = 1; // check
		if (ctype == BP_CLIENT_TABS)
			columns_count++; // default dirty
		if (tabinfo.date_created > 0)
			columns_count++;
		if (tabinfo.date_modified >= 0)
			columns_count++;
		if (tabinfo.index >= 0)  // if negative, ignore
			columns_count++;
		if (tabinfo.is_incognito >= 0)
			columns_count++;
		if (tabinfo.browser_instance >= 0)
			columns_count++;
		if (offset & BP_TAB_O_URL)
			columns_count++;
		if (offset & BP_TAB_O_TITLE)
			columns_count++;
		if (offset & BP_TAB_O_DEVICE_NAME)
			columns_count++;
		if (offset & BP_TAB_O_DEVICE_ID)
			columns_count++;
	}

	if (columns_count > 0 && errorcode == BP_ERROR_NONE) {

		bp_db_conds_list_fmt columns[columns_count];
		unsigned index = 0;
		memset(&columns, 0x00, columns_count * sizeof(bp_db_conds_list_fmt));

		if (is_update == 1) {
			if (ctype == BP_CLIENT_TABS) {
				int is_dirty = 1;
				columns[index].column = BP_DB_COMMON_COL_DIRTY;
				columns[index].type = BP_DB_COL_TYPE_INT;
				columns[index].value = &is_dirty;
				index++;
			}
			if (index < columns_count && tabinfo.index >= 0) {
				columns[index].column = BP_DB_TABS_COL_INDEX;
				columns[index].type = BP_DB_COL_TYPE_INT;
				columns[index].value = &tabinfo.index;
				index++;
			}
			if (index < columns_count && tabinfo.date_created > 0) {
				columns[index].column = BP_DB_COMMON_COL_DATE_CREATED;
				columns[index].type = BP_DB_COL_TYPE_DATETIME;
				columns[index].value = &tabinfo.date_created;
				index++;
			}
			if (index < columns_count) {
				if (tabinfo.date_modified > 0) {
					columns[index].column =
						BP_DB_COMMON_COL_DATE_MODIFIED;
					columns[index].type = BP_DB_COL_TYPE_DATETIME;
					columns[index].value = &tabinfo.date_modified;
					index++;
				} else if (tabinfo.date_modified == 0) {
					columns[index].column =
						BP_DB_COMMON_COL_DATE_MODIFIED;
					columns[index].type = BP_DB_COL_TYPE_DATETIME_NOW;
					columns[index].value = 0;
					index++;
				}
			}
			if (index < columns_count && tabinfo.is_incognito >= 0) {
				columns[index].column = BP_DB_TABS_COL_IS_INCOGNITO;
				columns[index].type = BP_DB_COL_TYPE_INT;
				columns[index].value = &tabinfo.is_incognito;
				index++;
			}
			if (index < columns_count && tabinfo.browser_instance >= 0) {
				columns[index].column = BP_DB_TABS_COL_BROWSER_INSTANCE;
				columns[index].type = BP_DB_COL_TYPE_INT;
				columns[index].value = &tabinfo.browser_instance;
				index++;
			}
			if (index < columns_count && offset & BP_TAB_O_URL) {
				columns[index].column = BP_DB_COMMON_COL_URL;
				columns[index].type = BP_DB_COL_TYPE_TEXT;
				columns[index].value = url;
				index++;
			}
			if (index < columns_count && offset & BP_TAB_O_TITLE) {
				columns[index].column = BP_DB_COMMON_COL_TITLE;
				columns[index].type = BP_DB_COL_TYPE_TEXT;
				columns[index].value = title;
				index++;
			}
			if (index < columns_count && offset & BP_TAB_O_DEVICE_NAME) {
				columns[index].column = BP_DB_COMMON_COL_DEVICE_NAME;
				columns[index].type = BP_DB_COL_TYPE_TEXT;
				columns[index].value = device_name;
				index++;
			}
			if (index < columns_count && offset & BP_TAB_O_DEVICE_ID) {
				columns[index].column = BP_DB_COMMON_COL_DEVICE_ID;
				columns[index].type = BP_DB_COL_TYPE_TEXT;
				columns[index].value = device_id;
				index++;
			}
		}
		if (index < columns_count && tabinfo.is_activated >= 0) {
			columns[index].column = BP_DB_TABS_COL_ACTIVATED;
			columns[index].type = BP_DB_COL_TYPE_INT;
			columns[index].value = &tabinfo.is_activated;
			index++;
		}
		if (index < columns_count && offset & BP_TAB_O_ACCOUNT_NAME) {
			columns[index].column = BP_DB_COMMON_COL_ACCOUNT_NAME;
			columns[index].type = BP_DB_COL_TYPE_TEXT;
			columns[index].value = account_name;
			index++;
		}
		if (index < columns_count && offset & BP_TAB_O_ACCOUNT_TYPE) {
			columns[index].column = BP_DB_COMMON_COL_ACCOUNT_TYPE;
			columns[index].type = BP_DB_COL_TYPE_TEXT;
			columns[index].value = account_type;
			index++;
		}
		if (index < columns_count && offset & BP_TAB_O_USAGE) {
			columns[index].column = BP_DB_TABS_COL_USAGE;
			columns[index].type = BP_DB_COL_TYPE_TEXT;
			columns[index].value = usage;
			index++;
		}
		if (index < columns_count && offset & BP_TAB_O_SYNC) {
			columns[index].column = BP_DB_COMMON_COL_SYNC;
			columns[index].type = BP_DB_COL_TYPE_TEXT;
			columns[index].value = sync;
			index++;
		}

		pthread_mutex_lock(&g_db_mutex);
		errorcode = BP_ERROR_NONE;
		if (bp_db_set_columns(g_db_handle, id, BP_DB_TABLE_TABS,
				columns_count, columns, &errorcode) < 0) {
			TRACE_ERROR("[ERROR][SQL][%d] UPDATE INFOs", id);
		}
		pthread_mutex_unlock(&g_db_mutex);
	}

TAB_SET_ALL_ERROR:

	// free strings
	free(url);
	free(title);
	free(account_name);
	free(account_type);
	free(device_name);
	free(device_id);
	free(usage);
	free(sync);

	bp_ipc_send_errorcode(sock, errorcode);
	if (errorcode == BP_ERROR_NONE)
		bp_ipc_send_custom_type(sock, &id, sizeof(int));
	return errorcode;
}

bp_error_defs bp_tabs_ready_resource()
{
#ifdef DATABASE_TAB_FILE
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
			if (g_db_basic_insert_stmt != NULL) {
				bp_db_finalize(g_db_basic_insert_stmt);
				g_db_basic_insert_stmt = NULL;
			}
			bp_db_close(g_db_handle);
			g_db_handle = 0;
		}
		if (bp_db_open(&g_db_handle, DATABASE_TAB_FILE) < 0) {
			TRACE_ERROR("[CRITICAL] can not open SQL");
			int sql_errorcode = SQLITE_OK;
			if (g_db_handle != 0) {
				sql_errorcode = sqlite3_errcode(g_db_handle);
				bp_db_close(g_db_handle);
				g_db_handle = 0;
			} else {
				sql_errorcode = SQLITE_FULL;
			}
			TRACE_ERROR("sql errorcode:%d", sql_errorcode);
			pthread_mutex_unlock(&g_db_mutex);
			if (sql_errorcode == SQLITE_FULL)
				return BP_ERROR_DISK_FULL;
			return BP_ERROR_DISK_BUSY;
		}
	}
	if (g_db_handle != 0 && g_db_basic_insert_stmt == NULL) {
		g_db_basic_insert_stmt =
			bp_db_prepare_basic_insert_stmt(g_db_handle, BP_DB_TABLE_TABS);
	}
	pthread_mutex_unlock(&g_db_mutex);
	return BP_ERROR_NONE;
#else
	TRACE_ERROR("[CRITICAL] Missing SQL info in compile option");
	return BP_ERROR_UNKNOWN;
#endif
}

void bp_tabs_free_resource()
{
	pthread_mutex_lock(&g_db_mutex);
	if (g_db_handle != 0) {
		TRACE_SECURE_DEBUG("TRY to close [%s]", DATABASE_TAB_FILE);
		if (g_db_basic_insert_stmt != NULL)
			bp_db_finalize(g_db_basic_insert_stmt);
		g_db_basic_insert_stmt = NULL;
		bp_db_close(g_db_handle);
		g_db_handle = 0;
	}
	pthread_mutex_unlock(&g_db_mutex);
}

bp_error_defs bp_tabs_handle_requests(bp_client_slots_defs *slots,
	bp_client_defs *client, bp_command_fmt *client_cmd)
{
	bp_command_defs cmd = BP_CMD_NONE;
	bp_error_defs errorcode = BP_ERROR_NONE;
	long long int id = 0;
	long long int cid = 0;
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

	if ((errorcode = bp_tabs_ready_resource()) != BP_ERROR_NONE) {
		bp_ipc_send_errorcode(sock, errorcode);
		return errorcode;
	}

	switch (cmd) {
	case BP_CMD_COMMON_CREATE:
		errorcode = bp_common_create
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS, sock, id);
		break;
	case BP_CMD_COMMON_DELETE:

#ifdef SUPPORT_CLOUD_SYSTEM
		if (client->type == BP_CLIENT_TABS &&
				bp_common_is_connected_my_sync_adaptor(slots,
					client->type) == 0) {
			errorcode = bp_common_set_is_deleted
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS, sock, id);
			break;
		}
#endif
		errorcode = bp_common_delete
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS, sock, id);
		break;
	case BP_CMD_DEINITIALIZE:
		if (client->type == BP_CLIENT_TABS_SYNC) {
			int is_deleted = 1;
			pthread_mutex_lock(&g_db_mutex);
			if (bp_db_remove_cond(g_db_handle, BP_DB_TABLE_TABS,
					BP_DB_COMMON_COL_IS_DELETED, 0, BP_DB_COL_TYPE_INT,
					&is_deleted, &errorcode) < 0) {
				TRACE_ERROR("[ERROR][SQL] CLEAR_DELETED_IDS");
			}
			// remove all items except me
			char *device_id = bp_get_my_deviceid();
			if (device_id != NULL) {
				if (bp_db_remove_cond(g_db_handle, BP_DB_TABLE_TABS,
						BP_DB_COMMON_COL_DEVICE_ID, -1,
						BP_DB_COL_TYPE_TEXT, device_id,
						&errorcode) < 0) {
					TRACE_ERROR("[ERROR][SQL] CLEAR_OTHER_DEVICE_IDS");
				}
				free(device_id);
			}
			int is_dirty = 1;
			if (bp_db_set_column(g_db_handle, -1, BP_DB_TABLE_TABS,
					BP_DB_COMMON_COL_DIRTY, BP_DB_COL_TYPE_INT,
					&is_dirty, &errorcode) < 0) {
				TRACE_ERROR("[ERROR][SQL] SET DIRTY all");
			}
			pthread_mutex_unlock(&g_db_mutex);
		}
		bp_ipc_send_errorcode(sock, errorcode);
		break;
	case BP_CMD_COMMON_SET_IS_DELETED:
		errorcode = bp_common_set_is_deleted
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS, sock, id);
		break;
	case BP_CMD_COMMON_GET_CONDS_ORDER_IDS: // duplicated
		errorcode = __bp_tab_get_duplicated_ids(sock);
		break;
	case BP_CMD_COMMON_GET_FULL_IDS: // without deleted
		errorcode = bp_common_get_full_ids
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS, sock);
		break;
	case BP_CMD_COMMON_GET_FULL_WITH_DELETED_IDS:
		errorcode = bp_common_get_full_with_deleted_ids
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS, sock);
		break;
	case BP_CMD_COMMON_GET_DIRTY_IDS: // only dirty
		errorcode = bp_common_get_dirty_ids
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS, sock);
		break;
	case BP_CMD_COMMON_GET_DELETED_IDS:
		errorcode = bp_common_get_deleted_ids
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS, sock);
		break;
	case BP_CMD_COMMON_CLEAR_DIRTY_IDS:
		errorcode = bp_common_clear_dirty_ids
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS, sock);
		break;
	case BP_CMD_COMMON_CLEAR_DELETED_IDS:
		errorcode = bp_common_clear_deleted_ids
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS, sock);
		break;
	case BP_CMD_TABS_GET_INDEX:
		errorcode = bp_common_get_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_TABS_COL_INDEX, sock, id);
		break;
	case BP_CMD_COMMON_GET_URL:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_COMMON_COL_URL, sock, id);
		break;
	case BP_CMD_COMMON_GET_TITLE:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_COMMON_COL_TITLE, sock, id);
		break;
	case BP_CMD_COMMON_GET_ICON:
		errorcode = bp_common_get_blob_shm(g_db_handle,
				&g_db_mutex, BP_DB_TABLE_FAVICONS, sock, id, &client->shm);
		break;
	case BP_CMD_COMMON_GET_SNAPSHOT:
		errorcode = bp_common_get_blob_shm(g_db_handle,
				&g_db_mutex, BP_DB_TABLE_THUMBNAILS, sock, id, &client->shm);
		break;
	case BP_CMD_TABS_GET_ACTIVATED:
		errorcode = bp_common_get_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_TABS_COL_ACTIVATED, sock, id);
		break;
	case BP_CMD_TABS_GET_INCOGNITO:
		errorcode = bp_common_get_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_TABS_COL_IS_INCOGNITO, sock, id);
		break;
	case BP_CMD_TABS_GET_BROWSER_INSTANCE:
		errorcode = bp_common_get_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_TABS_COL_BROWSER_INSTANCE, sock, id);
		break;
	case BP_CMD_COMMON_GET_DATE_CREATED:
		errorcode = bp_common_get_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_COMMON_COL_DATETIME_CREATED, sock, id);
		break;
	case BP_CMD_COMMON_GET_DATE_MODIFIED:
		errorcode = bp_common_get_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_COMMON_COL_DATETIME_MODIFIED, sock, id);
		break;
	case BP_CMD_COMMON_GET_ACCOUNT_NAME:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_COMMON_COL_ACCOUNT_NAME, sock, id);
		break;
	case BP_CMD_COMMON_GET_ACCOUNT_TYPE:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_COMMON_COL_ACCOUNT_TYPE, sock, id);
		break;
	case BP_CMD_COMMON_GET_DEVICE_NAME:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_COMMON_COL_DEVICE_NAME, sock, id);
		break;
	case BP_CMD_COMMON_GET_DEVICE_ID:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_COMMON_COL_DEVICE_ID, sock, id);
		break;
	case BP_CMD_TABS_GET_USAGE:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_TABS_COL_USAGE, sock, id);
		break;
	case BP_CMD_COMMON_GET_SYNC:
		errorcode = bp_common_get_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_COMMON_COL_SYNC, sock, id);
		break;
	case BP_CMD_TABS_SET_INDEX:
		errorcode = bp_common_set_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_TABS_COL_INDEX, sock, id);
		break;
	case BP_CMD_COMMON_SET_URL:
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_COMMON_COL_URL, sock, id);
		break;
	case BP_CMD_COMMON_SET_TITLE:
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_COMMON_COL_TITLE, sock, id);
		break;
	case BP_CMD_COMMON_SET_ICON:
		errorcode = bp_common_set_blob_shm(g_db_handle,
				&g_db_mutex, BP_DB_TABLE_FAVICONS, sock, id, &client->shm);
		break;
	case BP_CMD_COMMON_SET_SNAPSHOT:
		errorcode = bp_common_set_blob_shm(g_db_handle,
				&g_db_mutex, BP_DB_TABLE_THUMBNAILS, sock, id, &client->shm);
		break;
	case BP_CMD_TABS_SET_ACTIVATED:
		errorcode = bp_common_set_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_TABS_COL_ACTIVATED, sock, id);
		break;
	case BP_CMD_TABS_SET_INCOGNITO:
		errorcode = bp_common_set_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_TABS_COL_IS_INCOGNITO, sock, id);
		break;
	case BP_CMD_TABS_SET_BROWSER_INSTANCE:
		errorcode = bp_common_set_int
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_TABS_COL_BROWSER_INSTANCE, sock, id);
		break;
	case BP_CMD_COMMON_SET_DATE_CREATED:
		errorcode = bp_common_set_date
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_COMMON_COL_DATE_CREATED, sock, id);
		break;
	case BP_CMD_COMMON_SET_DATE_MODIFIED:
		errorcode = bp_common_set_date
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_COMMON_COL_DATE_MODIFIED, sock, id);
		break;
	case BP_CMD_COMMON_SET_ACCOUNT_NAME:
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_COMMON_COL_ACCOUNT_NAME, sock, id);
		break;
	case BP_CMD_COMMON_SET_ACCOUNT_TYPE:
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_COMMON_COL_ACCOUNT_TYPE, sock, id);
		break;
	case BP_CMD_COMMON_SET_DEVICE_NAME:
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_COMMON_COL_DEVICE_NAME, sock, id);
		break;
	case BP_CMD_COMMON_SET_DEVICE_ID:
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_COMMON_COL_DEVICE_ID, sock, id);
		break;
	case BP_CMD_TABS_SET_USAGE:
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_TABS_COL_USAGE, sock, id);
		break;
	case BP_CMD_COMMON_SET_SYNC:
		errorcode = bp_common_set_string
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS,
				BP_DB_COMMON_COL_SYNC, sock, id);
		break;
	case BP_CMD_COMMON_NOTI:
		//__tabs_send_noti_all(slots, cid);
		bp_common_send_noti_all(slots, client->type, cid);
		errorcode = BP_ERROR_NONE;
		break;
	case BP_CMD_COMMON_SET_EASY_ALL:
		errorcode = __bp_tab_set_easy_all(sock, id, client->type);
		break;
	case BP_CMD_COMMON_GET_INFO_OFFSET:
		errorcode = __bp_tab_get_info_offset(sock, id, &client->shm);
		break;
	case BP_CMD_COMMON_SET_DIRTY:
		errorcode = bp_common_set_dirty
				(g_db_handle, &g_db_mutex, BP_DB_TABLE_TABS, id);
		break;
	case BP_CMD_TABS_ACTIVATE:
		errorcode = __bp_tab_activate(sock, id);
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
