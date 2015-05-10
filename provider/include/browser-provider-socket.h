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

#ifndef BROWSER_PROVIDER_SOCKET_H
#define BROWSER_PROVIDER_SOCKET_H

#include <unistd.h>

#include "browser-provider.h"
#include "browser-provider-slots.h"


#define BP_PRE_CHECK do {\
	if (sock < 0) {\
		TRACE_ERROR("[ERROR] sock establish");\
		return BP_ERROR_IO_ERROR;\
	}\
} while(0)

ssize_t bp_ipc_recv(int sock, void *value, size_t type_size,
	const char *func);
int bp_ipc_send_errorcode(int fd, bp_error_defs errorcode);
bp_error_defs bp_ipc_read_errorcode(int fd);
bp_error_defs bp_ipc_simple_response(int fd, bp_command_fmt *cmd);
int bp_ipc_send_command(int fd, bp_command_fmt *cmd);
char *bp_ipc_read_string(int fd);
int bp_ipc_send_string(int fd, const char *str);
int bp_ipc_send_custom_type(int fd, void *value, size_t type_size);
int bp_ipc_read_custom_type(int fd, void *value, size_t type_size);
int bp_ipc_read_blob(int fd, void *value, size_t type_size);
int bp_accept_socket_new();
int bp_socket_free(int sockfd);
bp_error_defs bp_ipc_check_stderr(bp_error_defs basecode);

#endif
