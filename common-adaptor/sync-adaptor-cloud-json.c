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

#include <glib.h>
#include <json.h>

static inline void add_obj(GSList * list, void* obj)
{
	list = g_slist_prepend(list,obj);
}

inline char* _get_json_string(GSList * list, json_object *record_item, char *key_name)
{
	json_object *obj = NULL;
	char* ret_value = NULL;

	obj = json_object_object_get(record_item, key_name);
	if(!obj) return NULL;

	add_obj(list,obj);
	ret_value = json_object_get_string(obj);
	return(ret_value);
}

inline int _get_json_int(GSList * list, json_object *record_item, char *key_name)
{
	json_object *obj = NULL;
	int ret_value = 0;

	obj = json_object_object_get(record_item, key_name);
	if(!obj) return 0;

	add_obj(list,obj);
	ret_value = json_object_get_int(obj);
	return(ret_value);
}

inline int _get_json_array_length(GSList * list, json_object *record_item, char *key_name)
{
	json_object *obj = NULL;
	int ret_value = 0;

	obj = json_object_object_get(record_item, key_name);

	if(!obj) return 0;

	add_obj(list,obj);
	ret_value = json_object_array_length(obj);
	return(ret_value);
}

inline json_object* _get_json_array_object_by_idx(GSList * list, json_object *record_item, char *key_name, int idx)
{
	json_object *obj = NULL;
	json_object *ret_value = NULL;

	obj = json_object_object_get(record_item, key_name);

	if(!obj) return 0;

	add_obj(list,obj);
	ret_value = json_object_array_get_idx(obj,idx);
	add_obj(list,ret_value);
	return(ret_value);
}

inline json_object* _get_json_tokener_parse(GSList * list, char *msg)
{
	json_object *my_object = json_tokener_parse(msg);
	add_obj(list, my_object);
	return(my_object);
}

void free_json(GSList *obj_list)
{
	GSList *i;

	if (g_slist_length (obj_list) > 0){

		for(i = obj_list; i != NULL; i = i->next)
		{
			if (i->data) {
				json_object_put((json_object *)i->data);
			}
		}

		if (obj_list){
			g_slist_free (obj_list);
			obj_list = NULL;
		}
	}
}

