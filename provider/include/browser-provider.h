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

#ifndef BROWSER_PROVIDER_H
#define BROWSER_PROVIDER_H

#ifdef __cplusplus
extern "C" {
#endif

#define BP_MAX_STR_LEN 4096
#define BP_DEFAULT_BUFFER_SIZE 1024

#ifndef IPC_SOCKET
#define IPC_SOCKET "/tmp/.browser-provider.sock"
#endif

#define SECURITY_PRIVILEGE_BOOKMARK "http://tizen.org/privilege/bookmark.admin"
#define SECURITY_PRIVILEGE_TAB "inhouse"
#define SECURITY_PRIVILEGE_SCRAP "inhouse"
#define SECURITY_PRIVILEGE_HISTORY "http://tizen.org/privilege/web-history.admin"

#define WEB_BOOKMARK_ROOT_ID 0

typedef enum {
	BP_CMD_NONE = 0,

	BP_CMD_COMMON_NOTI,
	BP_CMD_INITIALIZE,
	BP_CMD_DEINITIALIZE,
	BP_CMD_SET_NOTI_CB,
	BP_CMD_UNSET_NOTI_CB,

	BP_CMD_GET_SECT = 100,

	BP_CMD_COMMON_GET_CONDS_ORDER_IDS, // duplicated ids
	BP_CMD_COMMON_GET_CONDS_INT_IDS,   // ordering ids
	BP_CMD_COMMON_GET_FULL_IDS,
	BP_CMD_COMMON_GET_FULL_WITH_DELETED_IDS,
	BP_CMD_COMMON_GET_DIRTY_IDS,
	BP_CMD_COMMON_GET_DELETED_IDS,
	BP_CMD_COMMON_GET_URL,
	BP_CMD_COMMON_GET_TITLE,
	BP_CMD_COMMON_GET_DATE_CREATED,
	BP_CMD_COMMON_GET_DATE_MODIFIED,
	BP_CMD_COMMON_GET_DATE_VISITED,
	BP_CMD_COMMON_GET_ACCOUNT_NAME,
	BP_CMD_COMMON_GET_ACCOUNT_TYPE,
	BP_CMD_COMMON_GET_DEVICE_NAME,
	BP_CMD_COMMON_GET_DEVICE_ID,
	BP_CMD_COMMON_GET_SYNC,
	BP_CMD_COMMON_GET_FAVICON,
	BP_CMD_COMMON_GET_FAVICON_WIDTH,
	BP_CMD_COMMON_GET_FAVICON_HEIGHT,
	BP_CMD_COMMON_GET_THUMBNAIL,
	BP_CMD_COMMON_GET_THUMBNAIL_WIDTH,
	BP_CMD_COMMON_GET_THUMBNAIL_HEIGHT,
	BP_CMD_COMMON_GET_ICON,
	BP_CMD_COMMON_GET_SNAPSHOT,
	BP_CMD_COMMON_GET_WEBICON,
	BP_CMD_COMMON_GET_TAG,
	BP_CMD_COMMON_GET_TAG_IDS,
	BP_CMD_COMMON_GET_INFO_OFFSET,
	BP_CMD_COMMON_GET_CONDS_DATE_COUNT,
	BP_CMD_COMMON_GET_CONDS_DATE_IDS,
	BP_CMD_COMMON_GET_CONDS_TIMESTAMP_IDS,
	BP_CMD_COMMON_GET_CONDS_RAW_IDS,
	BP_CMD_TABS_GET_INDEX,
	BP_CMD_TABS_GET_ACTIVATED,
	BP_CMD_TABS_GET_INCOGNITO,
	BP_CMD_TABS_GET_BROWSER_INSTANCE,
	BP_CMD_TABS_GET_USAGE,
	BP_CMD_BOOKMARK_GET_TYPE,
	BP_CMD_BOOKMARK_GET_PARENT,
	BP_CMD_BOOKMARK_GET_SEQUENCE,
	BP_CMD_BOOKMARK_GET_IS_EDITABLE,
	BP_CMD_BOOKMARK_GET_IS_OPERATOR,
	BP_CMD_BOOKMARK_GET_ACCESS_COUNT,
	BP_CMD_BOOKMARK_BACKUP,
	BP_CMD_SCRAP_GET_BASE_DIR,
	BP_CMD_SCRAP_GET_PAGE_PATH,
	BP_CMD_SCRAP_GET_IS_READ,
	BP_CMD_SCRAP_GET_IS_READER,
	BP_CMD_SCRAP_GET_IS_NIGHT_MODE,
	BP_CMD_SCRAP_GET_MAIN_CONTENT,
	BP_CMD_CSC_BOOKMARK_GET_ALL,
	BP_CMD_HISTORY_GET_FREQUENCY,

	BP_CMD_SET_SECT = 200,

	BP_CMD_COMMON_SET_URL,
	BP_CMD_COMMON_SET_TITLE,
	BP_CMD_COMMON_SET_DATE_CREATED,
	BP_CMD_COMMON_SET_DATE_MODIFIED,
	BP_CMD_COMMON_SET_DATE_VISITED,
	BP_CMD_COMMON_SET_ACCOUNT_NAME,
	BP_CMD_COMMON_SET_ACCOUNT_TYPE,
	BP_CMD_COMMON_SET_DEVICE_NAME,
	BP_CMD_COMMON_SET_DEVICE_ID,
	BP_CMD_COMMON_SET_SYNC,
	BP_CMD_COMMON_SET_FAVICON,
	BP_CMD_COMMON_SET_FAVICON_WIDTH,
	BP_CMD_COMMON_SET_FAVICON_HEIGHT,
	BP_CMD_COMMON_SET_THUMBNAIL,
	BP_CMD_COMMON_SET_THUMBNAIL_WIDTH,
	BP_CMD_COMMON_SET_THUMBNAIL_HEIGHT,
	BP_CMD_COMMON_SET_ICON,
	BP_CMD_COMMON_SET_SNAPSHOT,
	BP_CMD_COMMON_SET_WEBICON,
	BP_CMD_COMMON_SET_TAG,
	BP_CMD_COMMON_UNSET_TAG,
	BP_CMD_COMMON_SET_DIRTY,
	BP_CMD_COMMON_SET_IS_DELETED,
	BP_CMD_COMMON_RESET,
	BP_CMD_COMMON_CREATE,
	BP_CMD_COMMON_DELETE,
	BP_CMD_COMMON_SET_EASY_ALL,
	BP_CMD_COMMON_CLEAR_DIRTY_IDS,
	BP_CMD_COMMON_CLEAR_DELETED_IDS,
	BP_CMD_TABS_SET_INDEX,
	BP_CMD_TABS_SET_ACTIVATED,
	BP_CMD_TABS_SET_INCOGNITO,
	BP_CMD_TABS_SET_BROWSER_INSTANCE,
	BP_CMD_TABS_SET_USAGE,
	BP_CMD_TABS_ACTIVATE,
	BP_CMD_BOOKMARK_SET_TYPE,
	BP_CMD_BOOKMARK_SET_PARENT,
	BP_CMD_BOOKMARK_SET_SEQUENCE,
	BP_CMD_BOOKMARK_SET_ACCESS_COUNT,
	BP_CMD_BOOKMARK_RESTORE,
	BP_CMD_BOOKMARK_DELETE_NO_CARE_CHILD,
	BP_CMD_SCRAP_SET_BASE_DIR,
	BP_CMD_SCRAP_SET_PAGE_PATH,
	BP_CMD_SCRAP_SET_IS_READ,
	BP_CMD_SCRAP_SET_IS_READER,
	BP_CMD_SCRAP_SET_IS_NIGHT_MODE,
	BP_CMD_SCRAP_SET_MAIN_CONTENT,
	BP_CMD_HISTORY_SET_FREQUENCY,
	BP_CMD_HISTORY_SET_VISIT,
	BP_CMD_HISTORY_SET_LIMIT_SIZE,

	BP_CMD_LAST_SECT = 300
} bp_command_defs;

typedef enum {
	BP_ERROR_NONE = 10,
	BP_ERROR_INVALID_PARAMETER,
	BP_ERROR_OUT_OF_MEMORY,
	BP_ERROR_IO_ERROR,
	BP_ERROR_IO_EAGAIN,
	BP_ERROR_IO_EINTR,
	BP_ERROR_NO_DATA,
	BP_ERROR_ID_NOT_FOUND,
	BP_ERROR_DUPLICATED_ID,
	BP_ERROR_PERMISSION_DENY,
	BP_ERROR_DISK_BUSY,
	BP_ERROR_DISK_FULL,
	BP_ERROR_TOO_BIG_DATA,
	BP_ERROR_UNKNOWN
} bp_error_defs;

typedef enum {
	BP_CLIENT_NONE = 0,
	BP_CLIENT_TABS,
	BP_CLIENT_TABS_SYNC,
	BP_CLIENT_BOOKMARK,
	BP_CLIENT_BOOKMARK_SYNC,
	BP_CLIENT_SCRAP,
	BP_CLIENT_SCRAP_SYNC,
	BP_CLIENT_BOOKMARK_CSC,
	BP_CLIENT_HISTORY,
	BP_CLIENT_HISTORY_SYNC
} bp_client_type_defs;

typedef struct {
	bp_command_defs cmd;
	long long int cid; // client id. publish by provider in connect time
	long long int id; // bp_command_defs
} bp_command_fmt;
// Usage IPC : send(bp_command_defs);send(data);

typedef struct {
	int limit; // the number of rows, negative : no limitation
	unsigned int offset; // first index of rows in all result
	unsigned int order_column_offset;
	unsigned ordering; // 0 : ASC, 1 : DESC, negative : ASC
} bp_db_base_conds_fmt;

typedef struct {
	int index;
	int is_activated;
	int is_incognito;
	int browser_instance;
	int date_created;
	int date_modified;
	int favicon_width;
	int favicon_height;
	int thumbnail_width;
	int thumbnail_height;
} bp_tab_base_fmt;

typedef struct {
	int type;
	int parent;
	int sequence;
	int editable;
	int is_operator;
	int access_count;
	int date_created;
	int date_modified;
	int date_visited;
	int favicon_width;
	int favicon_height;
	int thumbnail_width;
	int thumbnail_height;
	int webicon_width;
	int webicon_height;
} bp_bookmark_base_fmt;


typedef struct {
	int type;
	unsigned parent;
	unsigned editable;
} bp_bookmark_csc_base_fmt;

#ifdef __cplusplus
}
#endif
#endif
