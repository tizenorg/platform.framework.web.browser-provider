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

#ifndef __BROWSER_PROVIDER_COMMON_ADAPTOR_H__
#define __BROWSER_PROVIDER_COMMON_ADAPTOR_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <pthread.h>

#include <browser-provider.h>
#include <browser-provider-shm.h>
#include <browser-provider-log.h>

// define type
typedef struct {
	int cid; // publish by provider
	int cmd_socket; // set/get
	int notify; // used for callback
	bp_shm_defs shm;
} bp_adaptor_defs;

typedef void (*bp_common_adaptor_data_changed_cb)(void *user_data);
int bp_common_adaptor_event_manager(bp_adaptor_defs *adaptorinfo,
	bp_common_adaptor_data_changed_cb *notify_cb, void **data);

int bp_common_precheck_string(const char *str);
void bp_common_print_errorcode(const char *funcname, const int line,
	const int id, const bp_error_defs errorcode);
int bp_common_adaptor_connect_to_provider(bp_adaptor_defs **adaptorinfo,
	bp_client_type_defs client_type);
void bp_common_adaptor_close_all(bp_adaptor_defs *adaptorinfo);

// default APIs
int bp_adaptor_ipc_send_int(int fd, int value);
int bp_adaptor_ipc_read_int(int fd);

int bp_common_adaptor_get_ids_p(const int sock, bp_command_fmt *cmd,
	int **ids, bp_error_defs *errorcode);
int bp_common_adaptor_get_string(const int sock, bp_command_fmt *cmd,
	char **value, bp_error_defs *errorcode);
int bp_common_adaptor_set_string(const int sock, bp_command_fmt *cmd,
	const char *value, bp_error_defs *errorcode);
int bp_common_adaptor_get_int(const int sock, bp_command_fmt *cmd,
	int *value, bp_error_defs *errorcode);
int bp_common_adaptor_set_int(const int sock, bp_command_fmt *cmd,
	const int value, bp_error_defs *errorcode);
int bp_common_adaptor_get_blob(const int sock, bp_command_fmt *cmd,
	unsigned char **value, int *length, bp_error_defs *errorcode);
int bp_common_adaptor_set_blob(const int sock, bp_command_fmt *cmd,
	const unsigned char *value, const int length,
	bp_error_defs *errorcode);

int bp_common_adaptor_get_info_blob(int sock, unsigned char **value,
	bp_shm_defs *shm);
int bp_common_adaptor_get_blob_shm(const int sock,
	bp_command_fmt *cmd, int *width, int *height, unsigned char **value,
	int *length, bp_error_defs *errorcode, bp_shm_defs *shm);
int bp_common_adaptor_set_blob_shm(const int sock,
	bp_command_fmt *cmd, const int width, const int height,
	const unsigned char *value, const int length,
	bp_error_defs *errorcode, bp_shm_defs *shm);

int bp_common_adaptor_get_blob_with_size(const int sock,
	bp_command_fmt *cmd, int *width, int *height, unsigned char **value,
	int *length, bp_error_defs *errorcode);
int bp_common_adaptor_set_blob_with_size(const int sock,
	bp_command_fmt *cmd, const int width, const int height,
	const unsigned char *value, const int length,
	bp_error_defs *errorcode);

int bp_common_adaptor_copy_blob(int sock, unsigned char **value, int blob_length,
	bp_shm_defs *shm, bp_error_defs *errorcode);

#ifdef SUPPORT_CLOUD_SYSTEM
int bp_common_adaptor_is_sync_adaptor(void);
#endif

int bp_common_adaptor_disconnect(bp_adaptor_defs **info, pthread_t *tid);
int bp_common_adaptor_clear_read_buffer(int sock, size_t length);


// Statement Macro
#define BP_CHECK_PROVIDER_STATUS do {\
	pthread_mutex_lock(&g_adaptor_mutex);\
	if (__browser_adaptor_connect(1) < 0) {\
		TRACE_ERROR("[CHECK connection]");\
		pthread_mutex_unlock(&g_adaptor_mutex);\
		return -1;\
	}\
} while(0)

#define BP_CHECK_IPC_SOCK (g_adaptorinfo == NULL ? -1 : g_adaptorinfo->cmd_socket)

#define BP_PRINT_ERROR(id, errorcode) bp_common_print_errorcode(__FUNCTION__, __LINE__, id, errorcode)

#ifdef __cplusplus
}
#endif

#endif /* __BROWSER_PROVIDER_COMMON_ADAPTOR_H__ */
