/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
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

#ifndef __BROWSER_PROVIDER_SYNC_ADAPTOR_BOOKMARK_H__
#define __BROWSER_PROVIDER_SYNC_ADAPTOR_BOOKMARK_H__

#define MANIFEST_ID_BOOKMARK 	"org.tizen.manifest.web_bookmark"
#define IS_BOOKMARK_SYNC_COLD_START (_bookmark_cloud.synctime>0 ? 0 : 1)

typedef struct {
	int count;
	int *id_list;
	int *mark;
	bp_bookmark_info_fmt *item_list;
}sync_bookmark_local_list;

typedef struct {
	char *id;
	char *url;
	char *title;
	char *favicon_data;
	int favicon_width;
	int favicon_height;
	int64_t create_time;
	int64_t update_time;
} sync_bookmark_cloud_info_fmt;

typedef struct {
	int count;
	int64_t synctime;
	int *mark;
	sync_bookmark_cloud_info_fmt *item_list;
} sync_bookmark_cloud_list;

typedef void(*bookmark_sync_cb)(sync_bookmark_cloud_list*, sync_bookmark_local_list*);
typedef void(*cloud_delete_bookmark_cb_func)(int result);

int bp_sync_bookmark(void);

#endif /* __BROWSER_PROVIDER_SYNC_ADAPTOR_BOOKMARK_H__ */
