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

#include <web_tab.h>
#include <browser-provider.h>
#include <browser-provider-log.h>
#include <stdbool.h>
#include <glib.h>
#include <sync-adaptor.h>
#include <sync-adaptor-tab.h>
#include <sync-adaptor-cloud.h>

static sync_tab_local_list _tab_local;
static sync_tab_cloud_list _tab_cloud;

static find_new_tab_local(sync_tab_local_list *local)
{
	TRACE_DEBUG("");

	LOCAL_LOOP_INIT(sync_tab_local_list,local);

	LOCAL_LOOP(){
		if (IS_LOCAL_NORMAL() && IS_MADE_BY_LOCAL()){
				UPSYNC_NEW_LOCAL();
		}
	}
}

static int tab_get_favicon(int id, int *width, int *height, const char **encoded_data)
{
	unsigned char *favicon_data = NULL;
	int len = 0;
	int ret = bp_tab_adaptor_get_icon(id, width, height, (unsigned char **)&favicon_data, &len);
	if (ret < 0) {
		TRACE_DEBUG("bp_tab_adaptor_get_icon is failed");
		return -1;
	}else{
		*encoded_data = (const char *)g_base64_encode((const guchar *)favicon_data, (gsize) len);
		TRACE_DEBUG("bp_tab_adaptor_get_icon (len:%d, %d x %d)", len, *width, *height);
		return 1;
	}
}

static void downsync_others_tab(sync_tab_cloud_list* list)
{
	TRACE_DEBUG("");

	int i;
	for (i = 0; i < list->count; i++) {
		if(list->mark[i] == SYNC_UPDATE
			&& strcmp(list->item_list[i].device_id, get_device_id()) != 0)
		{
			char *id = list->item_list[i].id; //for sync_value
			char *url = list->item_list[i].url;
			char *title = list->item_list[i].title;
			char *device_id = list->item_list[i].device_id;
			char *favicon_data = list->item_list[i].favicon_data;
			int favicon_width = list->item_list[i].favicon_width;
			int favicon_height = list->item_list[i].favicon_height;

			TRACE_DEBUG("Others Tab [%s] %s %s %s",id,device_id,url,title);
			//TODO : update tab list of other devices  (url, title, id, favicon_data, favicon_width, favicon_height);
		}
	}
}

static void upsync_tab_update(sync_tab_local_list *list)
{
	TRACE_DEBUG("");

	int i;
	char *id;
	id = malloc(SYNC_CLOUD_ID_MAX_LENGTH);
	for (i = 0; i < list->count; i++) {
		if(list->mark[i] == SYNC_UPDATE){
			memset(id,0x00,SYNC_CLOUD_ID_MAX_LENGTH);
			//use tab 'sync' value to keep tracking ID on cloud
			if (list->item_list[i].sync == NULL){
				sprintf(id,"%d%s",list->id_list[i],get_device_id()); //new ID
				list->item_list[i].sync = id;
				bp_tab_adaptor_set_sync(list->id_list[i],id);
			}else{
				sprintf(id,"%s",list->item_list[i].sync); // used ID
			}
			char *url = list->item_list[i].url;
			char *title = list->item_list[i].title;
			TRACE_DEBUG("UPDATE Cloud [%s] %s",id,url);

			const char *encoded_data = NULL;
			int w = 0;
			int h = 0;
			if (tab_get_favicon(list->id_list[i], &w, &h, &encoded_data) > 0)
			{
				cloud_write_tab(id, url, title, encoded_data, w, h);
			}
		}
	}
	free(id);
}

static void upsync_tab_delete_cb(int result)
{
	if (result < 0)
		TRACE_DEBUG("DELETED failed");//TODO
	else
		TRACE_DEBUG("DELETED succeeded");
}

static void upsync_tab_delete(sync_tab_local_list* local)
{
	TRACE_DEBUG("");

	int i = 0, cnt = 0;;
	for (i = 0; i < local->count; i++) {
		if(local->mark[i] == SYNC_DELETED_UPSYNC && local->item_list[i].sync != NULL){
			TRACE_DEBUG("DELETE Cloud [%d/%d] mark:%d sync:%s",i,local->count,local->mark[i],local->item_list[i].sync);
			cloud_delete_tab(local->item_list[i].sync,upsync_tab_delete_cb);
			cnt++;
		}
	}
	if (cnt > 0)
	{
		bp_tab_adaptor_clear_deleted_ids();
	}
}

static void tab_get_local(sync_tab_local_list* list)
{
	int *ids = NULL;
	int ids_count = 0;
	int i = 0;
	char * * url;
	char * * title;
	bp_tab_adaptor_get_full_ids_p(&ids, &ids_count);

	list->count = ids_count;
	list->id_list = malloc(ids_count*sizeof(int));
	list->item_list = malloc(ids_count*sizeof(bp_tab_info_fmt));
	list->mark = malloc(ids_count*sizeof(int));
	for (i = 0; i < ids_count; i++) {
		list->mark[i] = SYNC_NONE;
		list->id_list[i] = ids[i];

		unsigned int b_offset = (BP_TAB_O_DATE_CREATED | BP_TAB_O_DATE_MODIFIED | BP_TAB_O_URL | BP_TAB_O_TITLE | BP_TAB_O_SYNC );
		memset(&(list->item_list[i]), 0x00, sizeof(bp_tab_info_fmt));
		int ret = bp_tab_adaptor_get_info(list->id_list[i], b_offset, &(list->item_list[i]));
		TRACE_DEBUG("[Local tab] id:%d title: %s url: %s sync: %s, created: %d, modified: %d",ids[i],
			list->item_list[i].title, list->item_list[i].url, list->item_list[i].sync,
			list->item_list[i].date_created, list->item_list[i].date_modified);
	}
	free(ids);
}

static void tab_get_deleted(sync_tab_local_list* local)
{
	int deleted_count = 0;
	int i = 0, start_idx = 0;
	int value;
	char * * url;
	char * * title;
	int *local_delete_ids = NULL;

	start_idx = local->count;
	bp_tab_adaptor_get_deleted_ids_p(&local_delete_ids,&deleted_count);
	TRACE_DEBUG("[Local tab Deleted] cnt:%d", deleted_count);

	if(deleted_count > 0 && start_idx > 0){
		local->count += deleted_count;
		local->id_list = realloc(local->id_list, local->count*sizeof(int));
		local->item_list = realloc(local->item_list, local->count*sizeof(bp_tab_info_fmt));
		local->mark = realloc(local->mark, local->count*sizeof(int));
	}else if (deleted_count > 0 && start_idx == 0){
		local->count = deleted_count;
		local->id_list = malloc(deleted_count*sizeof(int));
		local->item_list = malloc(deleted_count*sizeof(bp_tab_info_fmt));
		local->mark = malloc(deleted_count*sizeof(int));
	}

	for (i=start_idx; i < (start_idx + deleted_count); i++)
	{
		local->mark[i] = SYNC_DELETED_UPSYNC;
		local->id_list[i] = local_delete_ids[i-start_idx];
		unsigned int b_offset = (BP_TAB_O_DATE_CREATED | BP_TAB_O_DATE_MODIFIED | BP_TAB_O_URL | BP_TAB_O_TITLE | BP_TAB_O_SYNC );
		memset(&(local->item_list[i]), 0x00, sizeof(bp_tab_info_fmt));
		int ret = bp_tab_adaptor_get_info(local->id_list[i], b_offset, &(local->item_list[i]));
		TRACE_DEBUG("[Local tab Deleted] id:%d url: %s sync: %s",local_delete_ids[i-start_idx], local->item_list[i].url, local->item_list[i].sync);
	}
}

static void clear_tab_list(sync_tab_cloud_list *cloud, sync_tab_local_list *local)
{
	TRACE_DEBUG("");

	int i;

	if (local->count > 0){
		TRACE_DEBUG("");
		free(local->id_list);
		free(local->item_list);
		free(local->mark);
	}
	if (cloud->count > 0 ){
		TRACE_DEBUG("");
		free(cloud->item_list);
		free(cloud->mark);
	}

	cloud->count = 0;
	local->count = 0;
}

static void tab_init()
{
	bp_tab_adaptor_deinitialize();
	bp_tab_adaptor_initialize();
}

static void tab_load(sync_tab_local_list* local)
{
	TRACE_DEBUG("");

	tab_get_local(local);
	tab_get_deleted(local);
}

static void print_tab_log(sync_tab_cloud_list *cloud, sync_tab_local_list *local)
{
	LOCAL_LOOP_INIT(sync_tab_local_list,local);
	LOCAL_LOOP(){PRINT_LOCAL_LOG();}

	CLOUD_LOOP_INIT(sync_tab_cloud_list,cloud);
	CLOUD_LOOP(){PRINT_CLOUD_LOG();}
}

static void do_tab_cb(sync_tab_cloud_list *cloud, sync_tab_local_list *local)
{
	find_new_tab_local(local);
	print_tab_log(cloud,local);
	upsync_tab_update(local);
	upsync_tab_delete(local);
	downsync_others_tab(cloud);
}

static int sync_tab_init(char* app_id, char* package_name)
{
	int ret=-1;

	cloud_set_tab_list(&_tab_cloud,&_tab_local);

	ret = sync_init(app_id, package_name);

	if (ret != 0) {
		return -1;
	}else{
		return 0;
	}
}

int bp_sync_tab()
{
	TRACE_DEBUG("");
	clear_tab_list(&_tab_cloud,&_tab_local);
	tab_load(&_tab_local);
	cloud_sync_tab(do_tab_cb);
}

int bp_sync_tab_login(char* guid, char* accesstoken, char* app_id, char* package_name)
{
	int ret = bp_sync_login(guid,accesstoken);
	tab_init();
	sync_tab_init(app_id,package_name);
	cloud_tab_init();
	return ret;
}
