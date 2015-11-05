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

#include <browser-provider.h>
#include <browser-provider-log.h>
#include <stdbool.h>
#include <web_bookmark.h>
#include <iotcloud.h>
#include <json.h>
#include <glib.h>
#include <sync-adaptor.h>
#include <sync-adaptor-bookmark.h>
#include <sync-adaptor-cloud.h>
#include <sync-adaptor-cloud-json.h>

static int bookmark_manifest_cn = 0;

static void* __cloud_cb_bookmark_func = NULL;
static void* __cloud_delete_bookmark_cb_func = NULL;

sync_bookmark_local_list* __cloud_bookmark_local = NULL;
sync_bookmark_cloud_list* __cloud_bookmark_cloud = NULL;

static GSList *_bookmark_json = NULL;

#define GET_BOOKMARK_JSON_STRING(json, key) \
	_get_json_string(_bookmark_json, json, key)

#define GET_BOOKMARK_JSON_INT(json, key) \
	_get_json_int(_bookmark_json, json, key)

#define  GET_BOOKMARK_JSON_ARRAY(json, key, idx ) \
	_get_json_array_object_by_idx(_bookmark_json, json, key, idx)

#define GET_BOOKMARK_JSON_INIT(string_msg) \
	_get_json_tokener_parse(_bookmark_json, string_msg)

#define GET_BOOKMARK_JSON_ARRAY_COUNT(json, key) \
	_get_json_array_length(_bookmark_json, json, key)



static void cloud_bookmark_read_json_record(json_object *record_item, sync_bookmark_cloud_info_fmt *data){
	TRACE_DEBUG("");

	data->id = GET_BOOKMARK_JSON_STRING(record_item, "datauuid");
	data->url = GET_BOOKMARK_JSON_STRING(record_item, "url");
	data->title = GET_BOOKMARK_JSON_STRING(record_item, "title");
	data->favicon_data = GET_BOOKMARK_JSON_STRING(record_item, "favicon_data");

	if (data->favicon_data){
		data->favicon_width = GET_BOOKMARK_JSON_INT(record_item, "favicon_width");
		data->favicon_height = GET_BOOKMARK_JSON_INT(record_item, "favicon_height");
	}else
	{
		data->favicon_width = NULL;
		data->favicon_height = NULL;
	}

	char *bookmark_ctime_str = GET_BOOKMARK_JSON_STRING(record_item, "create_time");
	char *bookmark_utime_str = GET_BOOKMARK_JSON_STRING(record_item, "update_time");

	if (bookmark_ctime_str)
		data->create_time = strtoll(bookmark_ctime_str, NULL, 10);

	if (bookmark_utime_str)
		data->update_time = strtoll(bookmark_utime_str, NULL, 10);
}

static int cloud_bookmark_new_json(json_object *my_object, sync_bookmark_cloud_list *list)
{
	int i = 0, cnt = 0;

	char *bookmark_synctime_str = GET_BOOKMARK_JSON_STRING(my_object, "synced_timestamp");

	if(bookmark_synctime_str == NULL) {
		return 0;
	}

	int64_t bookmark_synctime = strtoll(bookmark_synctime_str, NULL, 10);
	cnt = GET_BOOKMARK_JSON_ARRAY_COUNT(my_object, "records");
	TRACE_DEBUG("Cloud Bookmark Count: %d last_synctime:%lld", cnt, bookmark_synctime);

	list->count = cnt;
	list->synctime = bookmark_synctime;
	list->item_list = malloc(cnt*sizeof(sync_bookmark_cloud_info_fmt));
	list->mark = malloc(cnt*sizeof(int));

	for(i =0 ; i<cnt ; i++){
		list->mark[i] = SYNC_UPDATE;
		json_object *record_item = GET_BOOKMARK_JSON_ARRAY(my_object,"records",i);
		cloud_bookmark_read_json_record(record_item,&(list->item_list[i]));
	}

	return cnt;
}

static int cloud_bookmark_update_json(json_object *my_object, sync_bookmark_cloud_list *list)
{
	TRACE_DEBUG("");

	int i = 0, j = 0, cnt = 0;
	char *id;

	cnt = GET_BOOKMARK_JSON_ARRAY_COUNT(my_object, "records");

	for(i =0 ; i<cnt ; i++){
		json_object *record_item = GET_BOOKMARK_JSON_ARRAY(my_object, "records",i);
		id = GET_BOOKMARK_JSON_STRING(record_item, "datauuid");

		for (j = 0; j < list->count; j++){
			if (strcmp(id,list->item_list[j].id) == 0){
				cloud_bookmark_read_json_record(record_item,&(list->item_list[j]));
				break;
			}
		}
	}
}

static void cloud_bookmark_read(sync_bookmark_cloud_list* cloud_list, iotcloud_read_cb callback)
{
	TRACE_DEBUG("");

	void* iotcloud_handle;
	int i;
	iotcloud_handle = iot_cloud_create(IOTCLOUD_READ, MANIFEST_ID_BOOKMARK, bookmark_manifest_cn);
	for (i = 0; i < cloud_list->count; i++) {
		if (cloud_list->mark[i] == SYNC_UPDATE){
			iot_cloud_add_datauuid(iotcloud_handle,cloud_list->item_list[i].id);
		}
	}
	iot_cloud_read(iotcloud_handle, callback);
}

static int cloud_bookmark_get_all( char* resp_msg_json)
{
	int i=0, cnt=0;

	if (resp_msg_json == NULL){
		return 0;
	}

	json_object *my_object = GET_BOOKMARK_JSON_INIT(resp_msg_json);

	cnt = cloud_bookmark_new_json(my_object,__cloud_bookmark_cloud);
	return cnt;
}

static bool cloud_write_cb(iot_error_e error, char* err_msg)
{
	void* iotcloud_handle;

	if(error != IOT_ERROR_NONE)
	{
		TRACE_DEBUG("Error : %s", err_msg);
		return false;
	}
	else
	{
		TRACE_DEBUG("Success");
		return true;
	}
}

/*
static bool cloud_get_manifest_cb(iot_error_e error, char* resp_msg_json)
{
	TRACE_DEBUG("Response : %s", resp_msg_json);

	json_object *my_object = json_tokener_parse(resp_msg_json);
	save_bookmark_obj(my_object);
	json_object *manifest_cn_obj = json_object_object_get(my_object, "manifest_cn");
	save_bookmark_obj(manifest_cn_obj);
	g_manifest_cn = json_object_get_int(manifest_cn_obj);

	TRACE_DEBUG("g_manifest_cn : %d", g_manifest_cn);
}
*/

static void cloud_bookmark_read_data(char* resp_msg_json)
{
	TRACE_DEBUG("Changes: %s", resp_msg_json);
	if (resp_msg_json == NULL) {
		__cloud_bookmark_cloud->count = 0;
		return 0;
	}

	json_object *my_object = GET_BOOKMARK_JSON_INIT(resp_msg_json);

	cloud_bookmark_update_json(my_object,__cloud_bookmark_cloud);
}

static bool cloud_bookmark_read_cb(iot_error_e error, char* resp_msg_json)
{
	cloud_bookmark_read_data(resp_msg_json);
	((bookmark_sync_cb)__cloud_cb_bookmark_func)(__cloud_bookmark_cloud,__cloud_bookmark_local);
}

static int cloud_bookmark_changes(sync_bookmark_cloud_list *list, char* resp_msg_json)
{
	TRACE_DEBUG("");

	int i=0, cnt=0, ret_cnt=0;

	if (resp_msg_json == NULL) {
		list->count = 0;
		TRACE_DEBUG("");
		return 0;
	}

	json_object *my_object = GET_BOOKMARK_JSON_INIT(resp_msg_json);

	cnt = GET_BOOKMARK_JSON_ARRAY_COUNT(my_object, "changes");

	if (cnt == 0) {
		list->count = 0;
		TRACE_DEBUG("");
		return 0;
	}

	char *bookmark_synctime_str = GET_BOOKMARK_JSON_STRING(my_object, "synced_timestamp");
	int64_t bookmark_synctime = strtoll(bookmark_synctime_str, NULL, 10);
	TRACE_DEBUG("Cloud Bookmark Count: %d last_synctime:%lld", cnt, bookmark_synctime);

	list->count = cnt;
	list->synctime = bookmark_synctime;
	list->item_list = malloc(cnt*sizeof(sync_bookmark_cloud_info_fmt));
	list->mark = malloc(cnt*sizeof(int));

	for(i =0 ; i<cnt ; i++){
		json_object *record_item = GET_BOOKMARK_JSON_ARRAY(my_object, "changes",i);
		list->item_list[i].id = GET_BOOKMARK_JSON_STRING(record_item, "datauuid");
		int action = GET_BOOKMARK_JSON_INT(record_item, "action");
		char *update_time_str = GET_BOOKMARK_JSON_STRING(record_item, "update_time");
		list->item_list[i].update_time = strtoll(update_time_str, NULL, 10);

		TRACE_DEBUG("%s => action: %d",list->item_list[i].id,action);
		if (action == 1){
			list->mark[i] = SYNC_UPDATE;
			ret_cnt++;
		}
		else if (action == 0){
			list->mark[i] = SYNC_DELETE;
		}
		else {
			list->mark[i] = SYNC_NONE;
		}
	}
	return ret_cnt;
}

static bool cloud_bookmark_get_all_to_sync_cb(iot_error_e error, char* resp_msg_json)
{
	TRACE_DEBUG("Response : %s", resp_msg_json);
	int cnt=0;
	cnt = cloud_bookmark_get_all(resp_msg_json);

	if (cnt == 0) __cloud_bookmark_cloud->count = 0;

	((bookmark_sync_cb)__cloud_cb_bookmark_func)(__cloud_bookmark_cloud,__cloud_bookmark_local);
}

static bool cloud_bookmark_changes_to_sync_cb(iot_error_e error, char* resp_msg_json)
{
	int updated_cnt = 0;
	TRACE_DEBUG("Response : %s", resp_msg_json);
	updated_cnt = cloud_bookmark_changes(__cloud_bookmark_cloud, resp_msg_json);
	if (updated_cnt > 0){
		cloud_bookmark_read(__cloud_bookmark_cloud, cloud_bookmark_read_cb);
	}else{
		((bookmark_sync_cb)__cloud_cb_bookmark_func)(__cloud_bookmark_cloud,__cloud_bookmark_local);
	}
}

static bool cloud_delete_bookmark_cb(iot_error_e error, char* resp_msg_json)
{
	TRACE_DEBUG("Response : %s", resp_msg_json);
	int i = 0, fail_cnt = 0, result = 0;
	char *id;

	json_object *my_object = GET_BOOKMARK_JSON_INIT(resp_msg_json);

	fail_cnt = GET_BOOKMARK_JSON_ARRAY_COUNT(my_object, "fails");

	for(i =0 ; i<fail_cnt ; i++){ //fails
			json_object *record_item = GET_BOOKMARK_JSON_ARRAY(my_object, "fails",i);
			id = GET_BOOKMARK_JSON_STRING(record_item, "datauuid");
			TRACE_DEBUG("Delete failed: %s", id);
	}

	if (fail_cnt > 0)
		result = -1;
	else
		result = 0;

	((cloud_delete_bookmark_cb_func)__cloud_delete_bookmark_cb_func)(result); //TODO : add response information
}

void cloud_delete_bookmark(char* data_id, void *cb)
{
	__cloud_delete_bookmark_cb_func = cb;
	iot_cloud_delete(MANIFEST_ID_BOOKMARK,data_id, cloud_delete_bookmark_cb);
}

void cloud_write_bookmark(char* id, char* url, char* title,
	const char* favicon_data, int favicon_width, int favicon_height){
	cloud_write(MANIFEST_ID_BOOKMARK, bookmark_manifest_cn, id, url, title, favicon_data, favicon_width, favicon_height);
}

static void cloud_get_all_sync_bookmark(void* cb)
{
	void* iotcloud_handle;
	__cloud_cb_bookmark_func = cb;
	iotcloud_handle = iot_cloud_create(IOTCLOUD_GET_ALL, MANIFEST_ID_BOOKMARK, bookmark_manifest_cn);
	iot_cloud_get_all(iotcloud_handle, MANIFEST_ID_BOOKMARK, cloud_bookmark_get_all_to_sync_cb);
}

static void cloud_changes_sync_bookmark(void* cb)
{
	__cloud_cb_bookmark_func = cb;
	iot_cloud_get_changes(MANIFEST_ID_BOOKMARK,__cloud_bookmark_cloud->synctime, NULL /*offset*/, NULL /*limit*/, cloud_bookmark_changes_to_sync_cb);
}

void cloud_sync_bookmark(void* cb)
{
	TRACE_DEBUG("");

	if(_bookmark_json){
		free_json(_bookmark_json);
	}

	//iot_cloud_get_manifest(MANIFEST_ID_BOOKMARK,1,cloud_get_manifest_cb);
	TRACE_DEBUG("Last Synced Time:%lld",__cloud_bookmark_cloud->synctime);

	if (__cloud_bookmark_cloud->synctime > 0){
		cloud_changes_sync_bookmark(cb);
	}else{
		cloud_get_all_sync_bookmark(cb);
	}
}

void cloud_set_bookmark_list(sync_bookmark_cloud_list* cloud, sync_bookmark_local_list* local)
{
	__cloud_bookmark_cloud = cloud;
	__cloud_bookmark_local = local;
}

void cloud_bookmark_init()
{
	__cloud_bookmark_cloud->synctime = 0;
}

