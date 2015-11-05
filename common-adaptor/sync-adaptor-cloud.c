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
#include <iotcloud.h>
#include <glib.h>
#include <sync-adaptor.h>
#include <sync-adaptor-cloud.h>

static int g_cloud_init = 0;

char* __cloud_device_id = NULL;

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

void cloud_write(char* manifest_id, int manifest_cn, char* id, char* url, char* title,
	const char* favicon_data, int favicon_width, int favicon_height)
{
	TRACE_DEBUG("");
	void* iotcloud_handle;
	iotcloud_handle = iot_cloud_create(IOTCLOUD_CREATE_UPDATE, manifest_id, manifest_cn);

	if(url == NULL)
		return;
	else
		iot_cloud_add_message(iotcloud_handle, id, __cloud_device_id, "url", (void*)url, KEYVAL_STRING);

	if(title != NULL)
		iot_cloud_add_message(iotcloud_handle, id, __cloud_device_id, "title", (void*)title, KEYVAL_STRING);

	TRACE_DEBUG("");
	if(favicon_data != NULL && favicon_width > 0 && favicon_height > 0){
		iot_cloud_add_message(iotcloud_handle, id, __cloud_device_id, "favicon_data", (void*)favicon_data, KEYVAL_BLOB);
		iot_cloud_add_message(iotcloud_handle, id, __cloud_device_id, "favicon_width", (void*)&favicon_width, KEYVAL_INT);
		iot_cloud_add_message(iotcloud_handle, id, __cloud_device_id, "favicon_height", (void*)&favicon_height, KEYVAL_INT);
	}
	iot_cloud_write(iotcloud_handle, cloud_write_cb);
}

int cloud_login(char* guid, char* accesstoken)
{
	TRACE_DEBUG("");

	return(iot_cloud_set_credential(guid, accesstoken));
}

int cloud_init(char* device_id, char* app_id, char* app_package_name)
{
	TRACE_DEBUG("");

	int ret = 0;
	if (g_cloud_init == 1){
		TRACE_DEBUG("Already called iot_cloud_init, just return");
		return 0;
	}
	__cloud_device_id = device_id;

	ret = iot_cloud_init(app_id, app_package_name, __cloud_device_id);
	if (ret != 0 /*IOT_ERROR_NONE*/) {
		TRACE_ERROR("iot_cloud_init failed %d", ret);
		return -1;
	}

	g_cloud_init = 1;
	return 0;
}

