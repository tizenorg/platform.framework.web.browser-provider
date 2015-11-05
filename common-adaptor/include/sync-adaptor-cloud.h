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

#ifndef __BROWSER_PROVIDER_SYNC_ADAPTOR_CLOUD_H__
#define __BROWSER_PROVIDER_SYNC_ADAPTOR_CLOUD_H__

typedef enum {
    SYNC_NONE = 0,                /* No changes */
    SYNC_UPDATE,                 /* Need to update */
    SYNC_DELETE,                 /* Need to delete */
    SYNC_DELETED_NONE,          /* No need to upsync to delete*/
    SYNC_DELETED_UPSYNC			/* Need to upsync to delete*/
}sync_mark_type;

int cloud_init(char* device_id, char* samsung_account_app_id, char* app_package_name);
int cloud_login(char* guid, char* accesstoken);
void cloud_write(char* manifest_id, int manifest_cn, char* id, char* url, char* title,
	const char* favicon_data, int favicon_width, int favicon_height);


#endif /* __BROWSER_PROVIDER_SYNC_ADAPTOR_CLOUD_H__ */
