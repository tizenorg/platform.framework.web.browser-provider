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

#ifndef BROWSER_PROVIDER_DB_DEFS_H
#define BROWSER_PROVIDER_DB_DEFS_H

// Tables
#define BP_DB_TABLE_TABS "tabs"
#define BP_DB_TABLE_BOOKMARK "bookmarks"
#define BP_DB_TABLE_HISTORY "history"
#define BP_DB_TABLE_FAVICONS "favicons"
#define BP_DB_TABLE_THUMBNAILS "thumbnails"
#define BP_DB_TABLE_WEBICONS "webicons"
#define BP_DB_TABLE_TAGS "tags"

// Columns // maximun length is 19.
// common
#define BP_DB_COMMON_COL_ID "id"
#define BP_DB_COMMON_COL_URL "url"
#define BP_DB_COMMON_COL_TITLE "title"
#define BP_DB_COMMON_COL_DATE_CREATED "date_created"
#define BP_DB_COMMON_COL_DATETIME_CREATED "(CASE WHEN (date_created IS 0) THEN 0 ELSE CAST(strftime('%s', date_created) as integer) END)"
#define BP_DB_COMMON_COL_DATE_MODIFIED "date_modified"
#define BP_DB_COMMON_COL_DATETIME_MODIFIED "(CASE WHEN (date_modified IS 0) THEN 0 ELSE CAST(strftime('%s', date_modified) as integer) END)"
#define BP_DB_COMMON_COL_DATE_VISITED "date_visited"
#define BP_DB_COMMON_COL_DATETIME_VISITED "(CASE WHEN (date_visited IS 0) THEN 0 ELSE CAST(strftime('%s', date_visited) as integer) END)"
#define BP_DB_COMMON_COL_IS_DELETED "is_deleted"
#define BP_DB_COMMON_COL_DIRTY "dirty"
#define BP_DB_COMMON_COL_ACCOUNT_NAME "account_name"
#define BP_DB_COMMON_COL_ACCOUNT_TYPE "account_type"
#define BP_DB_COMMON_COL_DEVICE_NAME "device_name"
#define BP_DB_COMMON_COL_DEVICE_ID "device_id"
#define BP_DB_COMMON_TAGS_COL_TAG_ID "tag_id" // 0,1,2,3,4,....
#define BP_DB_COMMON_TAGS_COL_TAG "tag_info" // content of tag
#define BP_DB_COMMON_COL_SYNC "sync"
#define BP_DB_COMMON_COL_LOWER_URL "LOWER(url)"
#define BP_DB_COMMON_COL_LOWER_TITLE "LOWER(title)"

// blob like favicons and thumbnails
#define BP_DB_COMMON_COL_BLOB "data"
#define BP_DB_COMMON_COL_BLOB_WIDTH "width"
#define BP_DB_COMMON_COL_BLOB_HEIGHT "height"

// tabs
#define BP_DB_TABS_COL_INDEX "ordering"
#define BP_DB_TABS_COL_ACTIVATED "is_activated"
#define BP_DB_TABS_COL_IS_INCOGNITO "is_incognito"
#define BP_DB_TABS_COL_USAGE "usage"
#define BP_DB_TABS_COL_BROWSER_INSTANCE "browser_instance"

// bookmarks
#define BP_DB_BOOKMARK_COL_TYPE "type"
#define BP_DB_BOOKMARK_COL_PARENT "parent"
#define BP_DB_BOOKMARK_COL_SEQUENCE "sequence"
#define BP_DB_BOOKMARK_COL_SEQUENCE_MAX "MAX(sequence) + 1"
#define BP_DB_BOOKMARK_COL_IS_EDITABLE "editable"
#define BP_DB_BOOKMARK_COL_ACCESS_COUNT "accesscount"
#define BP_DB_BOOKMARK_COL_IS_OPERATOR "is_operator"

// history
#define BP_DB_HISTORY_COL_FREQUENCY "frequency"

#include <limits.h> // overflow check, the limitation of malloc
#define MAX_LIMIT_ROWS_COUNT (int)(USHRT_MAX) // 65535

typedef enum {
	BP_DB_COL_TYPE_NONE = 0,
	BP_DB_COL_TYPE_INT = 10,
	BP_DB_COL_TYPE_INT64 = 20,
	BP_DB_COL_TYPE_TEXT = 30,
	BP_DB_COL_TYPE_BLOB = 40,
	BP_DB_COL_TYPE_DATETIME = 50, // only for bp_db_set_columns and bp_db_get_columns
	BP_DB_COL_TYPE_DATETIME_NOW = 60
} bp_column_data_defs;

#define BP_SCHEMA_BOOKMARKS "CREATE TABLE IF NOT EXISTS bookmarks(\
id INTEGER UNIQUE PRIMARY KEY DESC NOT NULL,\
type INTEGER DEFAULT 0,\
parent INTEGER DEFAULT 0,\
url TEXT DEFAULT NULL,\
title TEXT DEFAULT NULL,\
sequence INTEGER DEFAULT 0,\
editable INTEGER DEFAULT 1,\
accesscount INTEGER DEFAULT 0,\
is_deleted BOOLEAN DEFAULT 0,\
dirty BOOLEAN DEFAULT 0,\
is_operator BOOLEAN DEFAULT 0,\
account_name TEXT DEFAULT NULL,\
account_type TEXT DEFAULT NULL,\
device_name TEXT DEFAULT NULL,\
device_id TEXT DEFAULT NULL,\
sync TEXT DEFAULT NULL,\
date_created INTEGER DEFAULT 0,\
date_modified INTEGER DEFAULT 0,\
date_visited INTEGER DEFAULT 0\
)"

#define BP_SCHEMA_BOOKMARKS_INDEX "CREATE INDEX IF NOT EXISTS bookmark_index ON bookmarks (id, type, parent, editable, is_deleted, dirty)"

#define BP_SCHEMA_TABS "CREATE TABLE tabs(\
id INTEGER UNIQUE PRIMARY KEY DESC NOT NULL,\
ordering INTEGER DEFAULT 0,\
url TEXT DEFAULT NULL,\
title TEXT DEFAULT NULL,\
is_activated BOOLEAN DEFAULT 0,\
is_deleted BOOLEAN DEFAULT 0,\
is_incognito BOOLEAN DEFAULT 0,\
dirty BOOLEAN DEFAULT 0,\
account_name TEXT DEFAULT NULL,\
account_type TEXT DEFAULT NULL,\
device_name TEXT DEFAULT NULL,\
device_id TEXT DEFAULT NULL,\
usage TEXT DEFAULT NULL,\
sync TEXT DEFAULT NULL,\
browser_instance INTEGER DEFAULT 0,\
date_created INTEGER DEFAULT 0,\
date_modified INTEGER DEFAULT 0\
)"

#define BP_SCHEMA_HISTORY "CREATE TABLE history(\
id INTEGER UNIQUE PRIMARY KEY DESC NOT NULL,\
is_deleted BOOLEAN DEFAULT 0,\
dirty BOOLEAN DEFAULT 0,\
frequency INTEGER DEFAULT 0,\
date_created INTEGER DEFAULT 0,\
date_modified INTEGER DEFAULT 0,\
date_visited INTEGER DEFAULT 0,\
url TEXT DEFAULT NULL,\
title TEXT DEFAULT NULL,\
account_name TEXT DEFAULT NULL,\
account_type TEXT DEFAULT NULL,\
device_name TEXT DEFAULT NULL,\
device_id TEXT DEFAULT NULL,\
sync TEXT DEFAULT NULL\
)"

#define BP_SCHEMA_HISTORY_INDEX "CREATE INDEX IF NOT EXISTS history_index ON history (id, frequency, date_created, date_visited)"

#define BP_SCHEMA_IMAGE(table, parent) "CREATE TABLE IF NOT EXISTS "table"(\
id INTEGER UNIQUE PRIMARY KEY NOT NULL,\
width INTEGER DEFAULT 0,\
height INTEGER DEFAULT 0,\
data BLOB,\
FOREIGN KEY(id) REFERENCES "parent"(id) ON DELETE CASCADE\
)"

#define BP_SCHEMA_TAGS(table, parent) "CREATE TABLE IF NOT EXISTS "table"(\
id INTEGER NOT NULL,\
tag_id INTERGER NOT NULL,\
tag_info TEXT,\
FOREIGN KEY(id) REFERENCES "parent"(id) ON DELETE CASCADE\
)"

#define DATABASE_BOOKMARK_FILE DATABASE_DIR"/.browser-provider-bookmarks.db"
#define DATABASE_TAB_FILE DATABASE_DIR"/.browser-provider-tabs.db"
#define DATABASE_HISTORY_FILE DATABASE_DIR"/.browser-provider-history.db"

#endif
