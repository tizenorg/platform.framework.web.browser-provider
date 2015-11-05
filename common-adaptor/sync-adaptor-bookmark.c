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

#include <web_bookmark.h>
#include <browser-provider.h>
#include <browser-provider-log.h>
#include <stdbool.h>
#include <glib.h>
#include <sync-adaptor.h>
#include <sync-adaptor-bookmark.h>
#include <sync-adaptor-cloud.h>

static sync_bookmark_local_list _bookmark_local;
static sync_bookmark_cloud_list _bookmark_cloud;

static void find_bookmark_conflict(sync_bookmark_cloud_list *cloud, sync_bookmark_local_list *local)
{
	TRACE_DEBUG("");

	CLOUD_LOOP_INIT(sync_bookmark_cloud_list,cloud);
	LOCAL_LOOP_INIT(sync_bookmark_local_list,local);

	CLOUD_LOOP(){
		LOCAL_LOOP(){
			if(IS_CLOUD_TO_UPDATE()){
				if(IS_LOCAL_NORMAL()){
					MARK_UPDATE_SYNC(get_device_id());
				}else if (IS_LOCAL_DELETED()){
					MARK_DELETED_SYNC();
				}
			}
			else if(IS_CLOUD_TO_DELETE()){
				if(IS_SAME_CLOUD_RECORD_AS_LOCAL()){
					MARK_DELETE_SYNC();
				}
			}
		}
	}
}

static find_new_bookmark_local(sync_bookmark_local_list *local)
{
	TRACE_DEBUG("");

	LOCAL_LOOP_INIT(sync_bookmark_local_list,local);

	LOCAL_LOOP(){
		if (IS_LOCAL_NORMAL() && IS_MADE_BY_LOCAL()){
				UPSYNC_NEW_LOCAL();
		}
	}
}

static int bookmark_write_local(char* url, char* title, char* sync_value, char* favicon_data, int favicon_width, int favicon_height)
{
	TRACE_DEBUG("");
	int root_id = 0, id = -1;

	bp_bookmark_adaptor_get_root(&root_id);
	bp_bookmark_info_fmt info;
	memset(&info, 0x00, sizeof(bp_bookmark_info_fmt));
	info.url = url;
	info.title = title;
	info.type = 0;
	info.parent = root_id;
	info.sequence = -1;
	info.access_count = -1;
	info.editable = 1;
	info.sync = sync_value; //id for cloud

	int ret = bp_bookmark_adaptor_easy_create(&id, &info);

	if (ret == 0){
		ret = bp_bookmark_adaptor_set_sequence(id, -1); // set max sequence
		if (favicon_data != NULL){
			gsize len = 0;
			const unsigned char * decoded_data;
			decoded_data = g_base64_decode(favicon_data,&len);
			TRACE_DEBUG("favicon %d x %d len: %d", favicon_width, favicon_height, len);
			int ret_icon = bp_bookmark_adaptor_set_icon(id, favicon_width, favicon_height, (const unsigned char *)decoded_data, len);
			if (ret_icon < 0) {
				TRACE_DEBUG("bp_bookmark_adaptor_set_favicon is failed");
			}
		}
		bp_bookmark_adaptor_publish_notification();
		TRACE_DEBUG("");
		return id;
	}
	else{
		TRACE_DEBUG("ERROR adding bookmark! %d", bp_bookmark_adaptor_get_errorcode());
		return -1;
	}
}

static int bookmark_get_favicon(int id, int *width, int *height, const char **encoded_data)
{
	unsigned char *favicon_data = NULL;
	int len = 0;
	int ret = bp_bookmark_adaptor_get_icon(id, width, height, (unsigned char **)&favicon_data, &len);
	if (ret < 0) {
		TRACE_DEBUG("bp_bookmark_adaptor_get_icon is failed");
		return -1;
	}else{
		*encoded_data = (const char *)g_base64_encode((const guchar *)favicon_data, (gsize) len);
		TRACE_DEBUG("bp_bookmark_adaptor_get_icon (len:%d, %d x %d)", len, *width, *height);
		return 1;
	}
}

static void downsync_bookmark_update(sync_bookmark_cloud_list* list)
{
	TRACE_DEBUG("");

	int i;
	for (i = 0; i < list->count; i++) {
		if(list->mark[i] == SYNC_UPDATE){
			char *id = list->item_list[i].id; //for sync_value
			char *url = list->item_list[i].url;
			char *title = list->item_list[i].title;
			char *favicon_data = list->item_list[i].favicon_data;
			int favicon_width = list->item_list[i].favicon_width;
			int favicon_height = list->item_list[i].favicon_height;

			TRACE_DEBUG("UPDATE local bookmark [%s] %s %s",id,url,title);
			int local_id = bookmark_write_local(url, title, id, favicon_data, favicon_width, favicon_height);
		}
	}
}

static void downsync_bookmark_delete(sync_bookmark_local_list* list)
{
	TRACE_DEBUG("");

	int i = 0, cnt = 0;
	for (i = 0; i < list->count; i++) {
		if(list->mark[i] == SYNC_DELETE){
			int id = list->id_list[i];
			TRACE_DEBUG("DELETE local bookmark [%d]",id);
			bp_bookmark_adaptor_delete(id);
			cnt++;
		}
	}
	if (cnt > 0)
	{
		bp_bookmark_adaptor_publish_notification();
		bp_bookmark_adaptor_clear_deleted_ids();
	}
}

static void upsync_bookmark_update(sync_bookmark_local_list *list)
{
	TRACE_DEBUG("");

	int i;
	char *id;
	id = malloc(SYNC_CLOUD_ID_MAX_LENGTH);
	for (i = 0; i < list->count; i++) {
		if(list->mark[i] == SYNC_UPDATE){
			memset(id,0x00,SYNC_CLOUD_ID_MAX_LENGTH);
			//use bookmark 'sync' value to keep tracking ID on cloud
			if (list->item_list[i].sync == NULL){
				sprintf(id,"%d%s",list->id_list[i],get_device_id()); //new ID
				list->item_list[i].sync = id;
				bp_bookmark_adaptor_set_sync(list->id_list[i],id);
			}else{
				sprintf(id,"%s",list->item_list[i].sync); // used ID
			}
			char *url = list->item_list[i].url;
			char *title = list->item_list[i].title;
			TRACE_DEBUG("UPDATE Cloud [%s] %s",id,url);

			const char *encoded_data = NULL;
			int w = 0;
			int h = 0;
			if (bookmark_get_favicon(list->id_list[i], &w, &h, &encoded_data) > 0)
			{
				cloud_write_bookmark(id, url, title, encoded_data, w, h);
			}
		}
	}
	free(id);
}

static void upsync_bookmark_delete_cb(int result)
{
	if (result < 0)
		TRACE_DEBUG("DELETED failed");//TODO
	else
		TRACE_DEBUG("DELETED succeeded");
}

static void upsync_bookmark_delete(sync_bookmark_local_list* local)
{
	TRACE_DEBUG("");

	int i = 0, cnt = 0;;
	for (i = 0; i < local->count; i++) {
		if(local->mark[i] == SYNC_DELETED_UPSYNC && local->item_list[i].sync != NULL){
			TRACE_DEBUG("DELETE Cloud [%d/%d] mark:%d sync:%s",i,local->count,local->mark[i],local->item_list[i].sync);
			cloud_delete_bookmark(local->item_list[i].sync,upsync_bookmark_delete_cb);
			cnt++;
		}
	}
	if (cnt > 0)
	{
		bp_bookmark_adaptor_clear_deleted_ids();
	}
}

static void bookmark_init_cold_start(sync_bookmark_local_list* list)
{
	//mark synched record to SYNC_DELETE.
	//After find_sync(), if it is still SYNC_DELETE , it should be deleted.
	int i = 0;
	int cnt = list->count;
	for(i = 0; i < cnt; i++){
		if (list->item_list[i].sync != NULL){
			list->mark[i] = SYNC_DELETE;
		}
	}
}

static void bookmark_get_local(sync_bookmark_local_list* list)
{
	int *ids = NULL;
	int ids_count = 0;
	int i = 0;
	int value;
	char * * url;
	char * * title;
	bp_bookmark_adaptor_get_ids_p(&ids, &ids_count, -1, 0, WEB_BOOKMARK_ROOT_ID,
		-1, -1, -1, BP_BOOKMARK_O_SEQUENCE, 0);

	list->count = ids_count;
	list->id_list = malloc(ids_count*sizeof(int));
	list->item_list = malloc(ids_count*sizeof(bp_bookmark_info_fmt));
	list->mark = malloc(ids_count*sizeof(int));
	for (i = 0; i < ids_count; i++) {
		list->mark[i] = SYNC_NONE;
		list->id_list[i] = ids[i];
		bp_bookmark_adaptor_get_type(ids[i],&value);
		if (value==0/*only bookmark not parent*/){
			unsigned int b_offset = (BP_BOOKMARK_O_TYPE | BP_BOOKMARK_O_DATE_CREATED | BP_BOOKMARK_O_DATE_MODIFIED | BP_BOOKMARK_O_URL
				| BP_BOOKMARK_O_TITLE | BP_BOOKMARK_O_SYNC );
			memset(&(list->item_list[i]), 0x00, sizeof(bp_bookmark_info_fmt));
			int ret = bp_bookmark_adaptor_get_info(list->id_list[i], b_offset, &(list->item_list[i]));
			TRACE_DEBUG("[Local bookmark] id:%d title: %s url: %s sync: %s, created: %d, modified: %d",ids[i],
				list->item_list[i].title, list->item_list[i].url, list->item_list[i].sync,
				list->item_list[i].date_created, list->item_list[i].date_modified);
		}
	}
	free(ids);
}

static void bookmark_get_deleted(sync_bookmark_local_list* local)
{
	int deleted_count = 0;
	int i = 0, start_idx = 0;
	int value;
	char * * url;
	char * * title;
	int *local_delete_ids = NULL;

	start_idx = local->count;
	bp_bookmark_adaptor_get_deleted_ids_p(&local_delete_ids,&deleted_count);
	TRACE_DEBUG("[Local Deleted] cnt:%d local_list(start_idx):%d", deleted_count, start_idx);

	if(deleted_count > 0 && start_idx > 0){
		local->count += deleted_count;
		local->id_list = realloc(local->id_list, local->count*sizeof(int));
		local->item_list = realloc(local->item_list, local->count*sizeof(bp_bookmark_info_fmt));
		local->mark = realloc(local->mark, local->count*sizeof(int));
	}else if (deleted_count > 0 && start_idx == 0){
		local->count = deleted_count;
		local->id_list = malloc(deleted_count*sizeof(int));
		local->item_list = malloc(deleted_count*sizeof(bp_bookmark_info_fmt));
		local->mark = malloc(deleted_count*sizeof(int));
	}

	for (i=start_idx; i < (start_idx + deleted_count); i++)
	{
		local->mark[i] = SYNC_DELETED_UPSYNC;
		local->id_list[i] = local_delete_ids[i-start_idx];
		bp_bookmark_adaptor_get_type(local_delete_ids[i-start_idx],&value);
		if (value==0/*only bookmark not parent*/){
			unsigned int b_offset = (BP_BOOKMARK_O_TYPE | BP_BOOKMARK_O_DATE_CREATED | BP_BOOKMARK_O_DATE_MODIFIED | BP_BOOKMARK_O_URL
				| BP_BOOKMARK_O_TITLE | BP_BOOKMARK_O_SYNC);
			memset(&(local->item_list[i]), 0x00, sizeof(bp_bookmark_info_fmt));
			int ret = bp_bookmark_adaptor_get_info(local->id_list[i], b_offset, &(local->item_list[i]));
			TRACE_DEBUG("[Local Deleted] id:%d url: %s sync: %s",local_delete_ids[i-start_idx], local->item_list[i].url, local->item_list[i].sync);
		}
	}
}

static void clear_bookmark_list(sync_bookmark_cloud_list *cloud, sync_bookmark_local_list *local)
{
	int i;

	/* TODO
	for (i = 0; i < local->count; i++) {
		bp_bookmark_adaptor_easy_free(&_bookmark_local.item_list[i]);
	}*/

	if (local->count > 0){
		free(local->id_list);
		free(local->item_list);
		free(local->mark);
	}
	if (cloud->count > 0 ){
		free(cloud->item_list);
		free(cloud->mark);
	}

	cloud->count = 0;
	local->count = 0;
}

static void bookmark_init()
{
	bp_bookmark_adaptor_deinitialize();
	bp_bookmark_adaptor_initialize();
}

static void bookmark_load(sync_bookmark_local_list* local, bool cold_start)
{
	bookmark_get_local(local);
	if (cold_start){
		bookmark_init_cold_start(local);
	}
	else
	{
		bookmark_get_deleted(local);
	}
}

static void print_bookmark_log(sync_bookmark_cloud_list *cloud, sync_bookmark_local_list *local)
{
	LOCAL_LOOP_INIT(sync_bookmark_local_list,local);
	LOCAL_LOOP(){PRINT_LOCAL_LOG();}

	CLOUD_LOOP_INIT(sync_bookmark_cloud_list,cloud);
	CLOUD_LOOP(){PRINT_CLOUD_LOG();}
}

static void do_bookmark_cb(sync_bookmark_cloud_list *cloud, sync_bookmark_local_list *local)
{
	find_bookmark_conflict(cloud,local);	//1. Cloud Update/Delete vs. Local Update/Deleted
	find_new_bookmark_local(local);			//2. Local New Update -> Cloud
	print_bookmark_log(cloud,local);
	downsync_bookmark_update(cloud);
	downsync_bookmark_delete(local);
	upsync_bookmark_update(local);
	upsync_bookmark_delete(local);
}

static int sync_bookmark_init(char* app_id, char* package_name)
{
	TRACE_DEBUG("");

	int ret=-1;

	cloud_set_bookmark_list(&_bookmark_cloud,&_bookmark_local);

	ret = sync_init(app_id, package_name);

	if (ret != 0) {
		return -1;
	}else{
		return 0;
	}
}

int bp_sync_bookmark()
{
	clear_bookmark_list(&_bookmark_cloud,&_bookmark_local);
	bookmark_load(&_bookmark_local, IS_BOOKMARK_SYNC_COLD_START);
	cloud_sync_bookmark(do_bookmark_cb);
}

int bp_sync_bookmark_login(char* guid, char* accesstoken, char* app_id, char* package_name)
{
	int ret = bp_sync_login(guid,accesstoken);
	bookmark_init();
	sync_bookmark_init(app_id, package_name);
	cloud_bookmark_init();
	return ret;
}
