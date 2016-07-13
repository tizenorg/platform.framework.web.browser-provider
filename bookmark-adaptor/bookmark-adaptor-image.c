/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd All Rights Reserved
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

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <bookmark-adaptor.h>
#include <common-adaptor.h>
#include <browser-provider.h>

#include <common-adaptor-png.h>

extern bp_adaptor_defs *g_adaptorinfo;
extern pthread_mutex_t g_adaptor_mutex;
extern bp_command_fmt g_bp_command;
extern bp_error_defs errorcode;

int __browser_adaptor_connect(int callback);
int __browser_adaptor_disconnect(void);

static int __bp_bookmark_adaptor_get_blob(const long long int id,
	bp_command_defs cmd, unsigned char **value, int *length)
{
	if (id < 0 || value == NULL || length == NULL)
		return -1;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	g_bp_command.cmd = cmd;
	g_bp_command.id = id;
	int ret = bp_common_adaptor_get_blob(sock, &g_bp_command, value, length, &errorcode);
	if (ret < 0 && errorcode == BP_ERROR_IO_ERROR)
		__browser_adaptor_disconnect();
	pthread_mutex_unlock(&g_adaptor_mutex);
	return ret;
}

static int __bp_bookmark_adaptor_set_blob(const long long int id,
	bp_command_defs cmd, const unsigned char *value, const int length)
{
	if (id < 0)
		return -1;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	g_bp_command.cmd = cmd;
	g_bp_command.id = id;
	int ret = bp_common_adaptor_set_blob(sock, &g_bp_command, value, length, &errorcode);
	if (ret < 0 && errorcode == BP_ERROR_IO_ERROR)
		__browser_adaptor_disconnect();
	pthread_mutex_unlock(&g_adaptor_mutex);
	return ret;
}

static int __bp_bookmark_adaptor_get_blob_shm(const long long int id,
	bp_command_defs cmd, int *width, int *height, unsigned char **value,
	int *length)
{
	if (id < 0 || value == NULL || length == NULL)
		return -1;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	g_bp_command.cmd = cmd;
	g_bp_command.id = id;
	int ret = bp_common_adaptor_get_blob_shm(sock, &g_bp_command, width,
			height, value, length, &errorcode, &g_adaptorinfo->shm);
	if (ret < 0 && errorcode == BP_ERROR_IO_ERROR)
		__browser_adaptor_disconnect();
	pthread_mutex_unlock(&g_adaptor_mutex);
	return ret;
}

static int __bp_bookmark_adaptor_set_blob_shm(const long long int id,
	bp_command_defs cmd, const int width, const int height,
	const unsigned char *value, const int length)
{
	if (id < 0)
		return -1;

	BP_CHECK_PROVIDER_STATUS;

	int sock = BP_CHECK_IPC_SOCK;
	g_bp_command.cmd = cmd;
	g_bp_command.id = id;
	int ret = bp_common_adaptor_set_blob_shm(sock, &g_bp_command, width,
			height, value, length, &errorcode, &g_adaptorinfo->shm);
	if (ret < 0 && errorcode == BP_ERROR_IO_ERROR)
		__browser_adaptor_disconnect();
	pthread_mutex_unlock(&g_adaptor_mutex);
	return ret;
}

/////////////////////// APIs /////////////////////////////////
// return
// 0 : Success
// -1 : Failed

int bp_bookmark_adaptor_get_icon(const long long int id, int *width,
	int *height, unsigned char **value, int *length)
{
	return __bp_bookmark_adaptor_get_blob_shm
		(id, BP_CMD_COMMON_GET_ICON, width, height, value, length);
}

int bp_bookmark_adaptor_get_snapshot(const long long int id, int *width,
	int *height, unsigned char **value, int *length)
{
	return __bp_bookmark_adaptor_get_blob_shm
		(id, BP_CMD_COMMON_GET_SNAPSHOT, width, height, value, length);
}

int bp_bookmark_adaptor_get_webicon(const long long int id, int *width,
	int *height, unsigned char **value, int *length)
{
	return __bp_bookmark_adaptor_get_blob_shm
		(id, BP_CMD_COMMON_GET_WEBICON, width, height, value, length);
}

int bp_bookmark_adaptor_get_icon_png(const long long int id,
	unsigned char **value, int *length)
{
	unsigned char *raw_data = NULL;
	int raw_length = 0;
	int width = 0;
	int height = 0;
	int ret = __bp_bookmark_adaptor_get_blob_shm(id,
		BP_CMD_COMMON_GET_ICON, &width, &height, &raw_data, &raw_length);
	if (ret == 0 && width > 0 && height > 0 && raw_length > 0 &&
			raw_data != NULL)
		ret = bp_common_raw_to_png(raw_data, width, height, value,
				length);
	return ret;
}

int bp_bookmark_adaptor_set_icon(const long long int id, const int width,
	const int height, const unsigned char *value, const int length)
{
	return __bp_bookmark_adaptor_set_blob_shm
		(id, BP_CMD_COMMON_SET_ICON, width, height, value, length);
}

int bp_bookmark_adaptor_set_snapshot(const long long int id, const int width,
	const int height, const unsigned char *value, const int length)
{
	return __bp_bookmark_adaptor_set_blob_shm
		(id, BP_CMD_COMMON_SET_SNAPSHOT, width, height, value, length);
}

int bp_bookmark_adaptor_set_webicon(const long long int id, const int width,
	const int height, const unsigned char *value, const int length)
{
	return __bp_bookmark_adaptor_set_blob_shm
		(id, BP_CMD_COMMON_SET_WEBICON, width, height, value, length);
}

int bp_bookmark_adaptor_set_icon_png(const long long int id,
	const unsigned char *value)
{
	if (value == NULL)
		return -1;
	unsigned char *raws_buffer = NULL;
	int raws_length = 0;
	int width = 0;
	int height = 0;
	int ret = bp_common_png_to_raw(value, &raws_buffer, &width, &height,
			&raws_length);
	if (ret == 0)
		ret = __bp_bookmark_adaptor_set_blob_shm(id,
				BP_CMD_COMMON_SET_ICON, width, height, raws_buffer,
				raws_length);
	free(raws_buffer);
	return ret;
}

