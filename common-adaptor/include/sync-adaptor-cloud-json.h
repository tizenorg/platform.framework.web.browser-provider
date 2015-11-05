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
#ifndef __BROWSER_PROVIDER_SYNC_ADAPTOR_CLOUD_JSON_H__
#define __BROWSER_PROVIDER_SYNC_ADAPTOR_CLOUD_JSON_H__

#include <glib.h>
#include <json.h>

inline char* _get_json_string(GSList * list, json_object *record_item, char *key_name);
inline int _get_json_int(GSList * list, json_object *record_item, char *key_name);
inline int _get_json_array_length(GSList * list, json_object *record_item, char *key_name);
inline json_object* _get_json_array_object_by_idx(GSList * list, json_object *record_item, char *key_name, int idx);
inline json_object* _get_json_tokener_parse(GSList * list, char *msg);
void free_json(GSList * list);

#endif /* __BROWSER_PROVIDER_SYNC_ADAPTOR_CLOUD_JSON_H__ */
