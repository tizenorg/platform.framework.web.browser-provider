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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <time.h>
#include <sys/time.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#include <signal.h>
#include <pthread.h>
#include <cynara-client.h>
#include <cynara-creds-socket.h>
#include <cynara-session.h>

#include "browser-provider.h"
#include "browser-provider-log.h"
#include "browser-provider-config.h"
#include "browser-provider-slots.h"
#include "browser-provider-socket.h"
#include "browser-provider-shm.h"
#include "browser-provider-notify.h"
#include "browser-provider-requests.h"

#include "browser-provider-tabs.h"
#include "browser-provider-bookmarks.h"
#include "browser-provider-history.h"

void bp_terminate(int signo);

#define SUPPORT_CLIENT_THREAD

bp_client_slots_defs *g_bp_slots = NULL;
cynara *p_cynara = NULL;
pthread_mutex_t mutex_for_cynara_check;


static char *__print_cmd(bp_command_defs cmd)
{
	switch (cmd) {
	case BP_CMD_COMMON_NOTI:
		return "NOTI";
	case BP_CMD_INITIALIZE:
		return "INIT";
	case BP_CMD_DEINITIALIZE:
		return "DEINIT";
	case BP_CMD_COMMON_GET_CONDS_ORDER_IDS:
		return "DUP_ORDER_IDS";
	case BP_CMD_COMMON_GET_CONDS_INT_IDS:
		return "INT_IDS";
	case BP_CMD_COMMON_GET_FULL_IDS:
		return "FULL_IDS";
	case BP_CMD_COMMON_GET_FULL_WITH_DELETED_IDS:
		return "FULL_DELETED_IDS";
	case BP_CMD_COMMON_GET_DIRTY_IDS:
		return "DIRTY_IDS";
	case BP_CMD_COMMON_GET_DELETED_IDS:
		return "DELETED_IDS";
	case BP_CMD_COMMON_GET_URL:
		return "GET_URL";
	case BP_CMD_COMMON_GET_TITLE:
		return "GET_TITLE";
	case BP_CMD_COMMON_GET_DATE_CREATED:
		return "GET_CREATED";
	case BP_CMD_COMMON_GET_DATE_MODIFIED:
		return "GET_MODIFIED";
	case BP_CMD_COMMON_GET_DATE_VISITED:
		return "GET_DATE_VISITED";
	case BP_CMD_COMMON_GET_ACCOUNT_NAME:
		return "GET_ACCOUNT_NAME";
	case BP_CMD_COMMON_GET_ACCOUNT_TYPE:
		return "GET_ACCOUNT_TYPE";
	case BP_CMD_COMMON_GET_DEVICE_NAME:
		return "GET_DEV_NAME";
	case BP_CMD_COMMON_GET_DEVICE_ID:
		return "GET_DEV_ID";
	case BP_CMD_COMMON_GET_SYNC:
		return "GET_SYNC";
	case BP_CMD_COMMON_GET_FAVICON:
		return "GET_FAVICON";
	case BP_CMD_COMMON_GET_FAVICON_WIDTH:
		return "GET_FAVICON_W";
	case BP_CMD_COMMON_GET_FAVICON_HEIGHT:
		return "GET_FAVICON_H";
	case BP_CMD_COMMON_GET_THUMBNAIL:
		return "GET_TUMB";
	case BP_CMD_COMMON_GET_THUMBNAIL_WIDTH:
		return "GET_THUMB_W";
	case BP_CMD_COMMON_GET_THUMBNAIL_HEIGHT:
		return "GET_THUMB_H";
	case BP_CMD_COMMON_GET_ICON:
		return "GET_ICON";
	case BP_CMD_COMMON_GET_SNAPSHOT:
		return "GET_SNAPSHOT";
	case BP_CMD_COMMON_GET_WEBICON:
		return "GET_WEBICON";
	case BP_CMD_COMMON_GET_TAG:
		return "GET_TAG";
	case BP_CMD_COMMON_GET_TAG_IDS:
		return "TAG_IDS";
	case BP_CMD_COMMON_GET_INFO_OFFSET:
		return "GET_INFO_OFFSET";
	case BP_CMD_TABS_GET_INDEX:
		return "GET_INDEX";
	case BP_CMD_TABS_GET_ACTIVATED:
		return "GET_ACTIVATED";
	case BP_CMD_TABS_GET_INCOGNITO:
		return "GET_INCOGNITO";
	case BP_CMD_TABS_GET_BROWSER_INSTANCE:
		return "GET_BROWSER_INSTANCE";
	case BP_CMD_TABS_GET_USAGE:
		return "GET_USAGE";
	case BP_CMD_BOOKMARK_GET_TYPE:
		return "GET_TYPE";
	case BP_CMD_BOOKMARK_GET_PARENT:
		return "GET_PARENT";
	case BP_CMD_BOOKMARK_GET_SEQUENCE:
		return "GET_SEQUENCE";
	case BP_CMD_BOOKMARK_GET_IS_EDITABLE:
		return "GET_EDIT";
	case BP_CMD_BOOKMARK_GET_IS_OPERATOR:
		return "GET_OPERATOR";
	case BP_CMD_BOOKMARK_GET_ACCESS_COUNT:
		return "GET_ACCESS_COUNT";
	case BP_CMD_BOOKMARK_BACKUP:
		return "BACKUP";
	case BP_CMD_CSC_BOOKMARK_GET_ALL:
		return "CSC_GET_ALL";
	case BP_CMD_HISTORY_GET_FREQUENCY:
		return "GET_FREQUENCY";
	case BP_CMD_COMMON_GET_CONDS_DATE_COUNT:
		return "GET_DATE_COUNT";
	case BP_CMD_COMMON_GET_CONDS_DATE_IDS:
		return "GET_DATE_IDS";
	case BP_CMD_COMMON_GET_CONDS_TIMESTAMP_IDS:
		return "GET_TIMESTAMP_IDS";
	case BP_CMD_COMMON_GET_CONDS_RAW_IDS:
		return "GET_CONDS_RAW_IDS";
	case BP_CMD_COMMON_SET_URL:
		return "SET_URL";
	case BP_CMD_COMMON_SET_TITLE:
		return "SET_TITLE";
	case BP_CMD_COMMON_SET_DATE_CREATED:
		return "SET_CREATED";
	case BP_CMD_COMMON_SET_DATE_MODIFIED:
		return "SET_MODIFIED";
	case BP_CMD_COMMON_SET_DATE_VISITED:
		return "SET_DATE_VISITED";
	case BP_CMD_COMMON_SET_ACCOUNT_NAME:
		return "SET_ACCOUNT_NAME";
	case BP_CMD_COMMON_SET_ACCOUNT_TYPE:
		return "SET_ACCOUNT_TYPE";
	case BP_CMD_COMMON_SET_DEVICE_NAME:
		return "SET_DEV_NAME";
	case BP_CMD_COMMON_SET_DEVICE_ID:
		return "SET_DEV_ID";
	case BP_CMD_COMMON_SET_SYNC:
		return "SET_SYNC";
	case BP_CMD_COMMON_SET_FAVICON:
		return "SET_FAVICON";
	case BP_CMD_COMMON_SET_FAVICON_WIDTH:
		return "SET_FAVICON_W";
	case BP_CMD_COMMON_SET_FAVICON_HEIGHT:
		return "SET_FAVICON_H";
	case BP_CMD_COMMON_SET_THUMBNAIL:
		return "SET_THUMB";
	case BP_CMD_COMMON_SET_THUMBNAIL_WIDTH:
		return "SET_THUMB_W";
	case BP_CMD_COMMON_SET_THUMBNAIL_HEIGHT:
		return "SET_THUMB_H";
	case BP_CMD_COMMON_SET_ICON:
		return "SET_ICON";
	case BP_CMD_COMMON_SET_SNAPSHOT:
		return "SET_SNAPSHOT";
	case BP_CMD_COMMON_SET_WEBICON:
		return "SET_WEBICON";
	case BP_CMD_COMMON_SET_TAG:
		return "SET_TAG";
	case BP_CMD_COMMON_UNSET_TAG:
		return "UNSET_TAG";
	case BP_CMD_COMMON_SET_DIRTY:
		return "SET_DIRTY";
	case BP_CMD_COMMON_SET_IS_DELETED:
		return "SET_IS_DELETED";
	case BP_CMD_COMMON_RESET:
		return "RESET";
	case BP_CMD_COMMON_CREATE:
		return "CREATE";
	case BP_CMD_COMMON_DELETE:
		return "DELETE";
	case BP_CMD_COMMON_SET_EASY_ALL:
		return "SET_EASY_ALL";
	case BP_CMD_COMMON_CLEAR_DIRTY_IDS:
		return "CLEAR_DIRTY_IDS";
	case BP_CMD_COMMON_CLEAR_DELETED_IDS:
		return "CLEAR_DELETED_IDS";
	case BP_CMD_TABS_SET_INDEX:
		return "SET_INDEX";
	case BP_CMD_TABS_SET_ACTIVATED:
		return "SET_ACTIVATED";
	case BP_CMD_TABS_SET_INCOGNITO:
		return "SET_INCOGNITO";
	case BP_CMD_TABS_SET_BROWSER_INSTANCE:
		return "SET_BROWSER_INSTANCE";
	case BP_CMD_TABS_SET_USAGE:
		return "SET_USAGE";
	case BP_CMD_TABS_ACTIVATE:
		return "ACTIVATE";
	case BP_CMD_BOOKMARK_SET_TYPE:
		return "SET_TYPE";
	case BP_CMD_BOOKMARK_SET_PARENT:
		return "SET_PARENT";
	case BP_CMD_BOOKMARK_SET_SEQUENCE:
		return "SET_SEQUENCE";
	case BP_CMD_BOOKMARK_SET_ACCESS_COUNT:
		return "SET_ACCESS_COUNT";
	case BP_CMD_BOOKMARK_RESTORE:
		return "RESTORE";
	case BP_CMD_BOOKMARK_DELETE_NO_CARE_CHILD:
		return "DELETE_NO_CARE_CHILD";
	case BP_CMD_HISTORY_SET_FREQUENCY:
		return "SET_FREQUENCY";
	case BP_CMD_HISTORY_SET_VISIT:
		return "SET_VISIT";
	case BP_CMD_HISTORY_SET_LIMIT_SIZE:
		return "SET_LIMIT_SIZE";
	default:
		break;
	}
	return "UNKNOWN";
}

static char *__print_client_type(bp_client_type_defs type)
{
	switch (type) {
	case BP_CLIENT_TABS:
		return "TABS";
	case BP_CLIENT_TABS_SYNC:
		return "TABS_SYNC";
	case BP_CLIENT_BOOKMARK:
		return "BOOKMARK";
	case BP_CLIENT_BOOKMARK_SYNC:
		return "BOOKMARK_SYNC";
	case BP_CLIENT_BOOKMARK_CSC:
		return "BOOKMARK_CSC";
	case BP_CLIENT_HISTORY:
		return "HISTORY";
	case BP_CLIENT_HISTORY_SYNC:
		return "HISTORY_SYNC";
	default:
		break;
	}
	return "UNKNOWN";
}

static void __update_client_permission(bp_client_defs *client)
{
	pthread_mutex_lock(&mutex_for_cynara_check);
	if(cynara_check(p_cynara, client->cynara_clientSmack, client->cynara_session, 
		client->cynara_uid, client->privilege_label) == CYNARA_API_ACCESS_ALLOWED) {
		client->permission.read = 1;
		client->permission.write = 1;
		TRACE_ERROR("client has %s privilege.", client->privilege_label);
    } else {
    	client->permission.read = 0;
    	client->permission.write = 0;
    	TRACE_ERROR("client does not have %s privilege.", client->privilege_label);
    }
    pthread_mutex_unlock(&mutex_for_cynara_check);
}

static int __check_smack_privilege(bp_privilege_defs *permission,
	bp_command_defs cmd)
{
	// cmd is for get or write ?
	if (cmd > BP_CMD_GET_SECT && cmd < BP_CMD_SET_SECT) { // GET
		if (permission != NULL && permission->read == 1)
			return 0;
		return -1;
	} else if (cmd > BP_CMD_SET_SECT && cmd < BP_CMD_LAST_SECT) { // GET
		if (permission != NULL && permission->write == 1)
			return 0;
		return -1;
	} else if (cmd > BP_CMD_NONE && cmd < BP_CMD_GET_SECT) { // Basic
		return 0;
	}
	return -1;
}

static bp_error_defs __handle_client_request(bp_client_defs *client)
{
	bp_error_defs errorcode = BP_ERROR_NONE;
	bp_command_fmt client_cmd;
	client_cmd.cmd = 0;
	client_cmd.cid = 0;
	client_cmd.id = 0;

	if (bp_ipc_recv(client->cmd_socket, &client_cmd,
			sizeof(bp_command_fmt), __FUNCTION__) <= 0)
		errorcode = BP_ERROR_IO_ERROR;
	else if (client_cmd.cmd == 0 || client_cmd.cid <= 0) {
		// Client meet some Error.
		TRACE_ERROR("[Closed Peer] sock:%d", client->cmd_socket);
		errorcode = BP_ERROR_IO_ERROR;
	}

	if (errorcode == BP_ERROR_NONE) {
		// print detail info
		TRACE_WARN("client(%s:%d) sock:%d req(%s:%d)",
			__print_client_type(client->type), client_cmd.cid,
			client->cmd_socket,
			__print_cmd(client_cmd.cmd), client_cmd.id);

		// update client permission
		if (!strcmp(client->privilege_label, "inhouse")) {
			// inhouse APIs (tab & scrap) have no privilege label yet.
			// temporarily grant permission to tab & scrap APIs without checking privilege.
			client->permission.read = 1;
			client->permission.write = 1;
		} else {
			__update_client_permission(client);
		}

		// check privilege
		if (__check_smack_privilege(&client->permission,
				client_cmd.cmd) < 0) {
#ifdef SUPPORT_SERVER_PRIVILEGE
			TRACE_SECURE_ERROR
				("[PERMISSION] [%d] Deny by Server Privilege",
				client_cmd.cmd);
			errorcode = BP_ERROR_PERMISSION_DENY;
			bp_ipc_send_errorcode(client->cmd_socket, errorcode);
			return errorcode;
#endif
		}

		switch(client->type) {
		case BP_CLIENT_TABS:
		case BP_CLIENT_TABS_SYNC:
			errorcode = bp_tabs_handle_requests
					(g_bp_slots, client, &client_cmd);
			break;
		case BP_CLIENT_BOOKMARK:
		case BP_CLIENT_BOOKMARK_SYNC:
		case BP_CLIENT_BOOKMARK_CSC:
			errorcode = bp_bookmark_handle_requests
					(g_bp_slots, client, &client_cmd);
			break;
		case BP_CLIENT_HISTORY:
			errorcode = bp_history_handle_requests
					(g_bp_slots, client, &client_cmd);
			break;
		case BP_CLIENT_HISTORY_SYNC:
			TRACE_ERROR("No Support Yet");
			errorcode = BP_ERROR_PERMISSION_DENY;
			break;
		default:
			// unknown client
			errorcode = BP_ERROR_IO_ERROR;
			break;
		}
	}
	return errorcode;
}


#ifdef SUPPORT_CLIENT_THREAD
void *client_thread_idle(void *arg)
{
	bp_client_slots_defs *slot = (bp_client_slots_defs *)arg;
	if (slot == NULL) {
		TRACE_ERROR("slot null, can not launch the thread for client");
		return 0;
	}

	fd_set imask, emask;
	bp_client_defs *client = slot->client;
	bp_error_defs errorcode = BP_ERROR_NONE;
	if (client != NULL) {
		switch(client->type) {
		case BP_CLIENT_TABS:
		case BP_CLIENT_TABS_SYNC:
			errorcode = bp_tabs_ready_resource();
			break;
		case BP_CLIENT_BOOKMARK:
		case BP_CLIENT_BOOKMARK_SYNC:
		case BP_CLIENT_BOOKMARK_CSC:
			errorcode = bp_bookmark_ready_resource();
			break;
		case BP_CLIENT_HISTORY:
		case BP_CLIENT_HISTORY_SYNC:
			errorcode = bp_history_ready_resource();
			break;
		default:
			break;
		}
		if (errorcode != BP_ERROR_NONE) {
			TRACE_ERROR("ready resource client:%s/%d sock:%d error:%d",
					__print_client_type(client->type), client->cid,
					client->cmd_socket, errorcode);
		}
	}

	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	while (client != NULL && client->cmd_socket >= 0) {

		FD_ZERO(&imask );
		FD_ZERO(&emask );
		FD_SET(client->cmd_socket, &imask);
		FD_SET(client->cmd_socket, &emask);
		if (select(client->cmd_socket + 1, &imask, 0, &emask, 0) < 0 ) {
			TRACE_ERROR("select");
			break;
		}
		if (FD_ISSET(client->cmd_socket, &imask) > 0) {

			pthread_mutex_lock(&slot->mutex);
			// check again
			if (slot->client == NULL) {
				pthread_mutex_unlock(&slot->mutex);
				TRACE_ERROR("[CHECK] closed by other thread");
				pthread_setcancelstate (PTHREAD_CANCEL_ENABLE,  NULL);
				return 0;
			}
			pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
			client->access_time = (int)time(NULL);
			errorcode = BP_ERROR_NONE;
			errorcode = __handle_client_request(client);
			pthread_setcancelstate (PTHREAD_CANCEL_ENABLE,  NULL);
			pthread_mutex_unlock(&slot->mutex);

			if (errorcode == BP_ERROR_IO_ERROR ||
					errorcode == BP_ERROR_OUT_OF_MEMORY) {
				TRACE_ERROR("disconnect client:%s/%d sock:%d",
					__print_client_type(client->type), client->cid,
					client->cmd_socket);
				break;
			}

		}
		if (FD_ISSET(client->cmd_socket, &emask) > 0) {
			TRACE_ERROR("[EXCEPTION]");
			break;
		}

	}

	if (client != NULL) {
		FD_CLR(client->cmd_socket, &imask);
		FD_CLR(client->cmd_socket, &emask);
	}
	bp_client_free(client);
	slot->client = NULL;
	return 0;
}
#endif

static int __get_client_id(bp_client_slots_defs *slots)
{
	int cid = 0;
	int checkid = 0;
	int i = 0;

	if (slots == NULL)
		return -1;

	do {
		checkid = 0;
		cid = bp_create_unique_id();
		for (i = 0; i < BP_MAX_CLIENT; i++) {
			if (slots[i].client == NULL)
				continue;
			if (slots[i].client->cid == cid) {
				checkid = 1;
				break;
			}
		}
	} while (checkid == 1);
	return cid;
}

// after accepting, fill info to pacakgelist.
// 3 socket per 1 package ( info/request/progress )
void bp_thread_requests_manager(bp_privates_defs *privates)
{
	fd_set rset, eset, listen_fdset, except_fdset;
	struct timeval timeout; // for timeout of select
	long flexible_timeout = BP_CARE_CLIENT_MAX_INTERVAL;
	int listenfd, clientfd, maxfd;
	socklen_t clientlen;
	struct sockaddr_un clientaddr;
	unsigned i, is_timeout;
	int prev_timeout = 0;
	cynara_configuration *conf = NULL;

	if (privates == NULL) {
		TRACE_ERROR("[CRITICAL] Invalid Address");
		return ;
	}
	privates->slots = bp_client_slots_new(BP_MAX_CLIENT);
	if (privates->slots == NULL) {
		TRACE_ERROR("[CRITICAL]  failed to alloc for slots");
		return ;
	}

	if (cynara_initialize(&p_cynara, conf) != CYNARA_API_SUCCESS) {
		TRACE_ERROR("[CRITICAL] failed to initialize cynara");
		return ;
	}

	g_bp_slots = privates->slots;

	listenfd = privates->listen_fd;
	maxfd = listenfd;

	TRACE_SECURE_DEBUG("Ready to listen [%d][%s]", listenfd, IPC_SOCKET);

	FD_ZERO(&listen_fdset);
	FD_ZERO(&except_fdset);
	FD_SET(listenfd, &listen_fdset);
	FD_SET(listenfd, &except_fdset);

	while (privates != NULL && privates->listen_fd >= 0) {

		// select with timeout
		// initialize timeout structure for calling timeout exactly
		memset(&timeout, 0x00, sizeof(struct timeval));
		timeout.tv_sec = flexible_timeout;
		clientfd = -1;
		is_timeout = 1;

		rset = listen_fdset;
		eset = except_fdset;

		if (select((maxfd + 1), &rset, 0, &eset, &timeout) < 0) {
			TRACE_STRERROR("[CRITICAL] select");
			break;
		}

		if (privates == NULL || privates->listen_fd < 0) {
			TRACE_DEBUG("Terminate Thread");
			break;
		}

		if (FD_ISSET(listenfd, &eset) > 0) {
			TRACE_ERROR("[CRITICAL] listenfd Exception of socket");
			break;
		} else if (FD_ISSET(listenfd, &rset) > 0) {
			// update g_bp_request_max_fd & g_bp_info_max_fd
			// add to socket to g_bp_request_rset & g_bp_info_rset

			is_timeout = 0;

			// Anyway accept client.
			clientlen = sizeof(clientaddr);
			clientfd = accept(listenfd, (struct sockaddr *)&clientaddr,
							&clientlen);
			if (clientfd < 0) {
				TRACE_ERROR("[CRITICAL] accept provider was crashed ?");
				// provider need the time of refresh.
				break;
			}

			// set timeout, if client crashed,
			// provider should wake up by itself.
			struct timeval tv_timeo; // 2.5 sec
			tv_timeo.tv_sec = 2;
			tv_timeo.tv_usec = 500000;
			if (setsockopt(clientfd, SOL_SOCKET, SO_RCVTIMEO, &tv_timeo,
				sizeof( tv_timeo ) ) < 0) {
				TRACE_STRERROR("[CRITICAL] setsockopt SO_SNDTIMEO");
				close(clientfd);
				continue;
			}

			bp_command_defs connect_cmd = BP_CMD_NONE;
			if (bp_ipc_read_custom_type(clientfd,
					&connect_cmd, sizeof(bp_command_defs)) < 0) {
				TRACE_ERROR("[CRITICAL] not support CONNECT CMD");
				bp_ipc_send_errorcode(clientfd, BP_ERROR_IO_ERROR);
				close(clientfd);
				continue;
			}
			if (connect_cmd <= BP_CMD_NONE) {
				TRACE_ERROR("[CRITICAL] peer terminate ?");
				bp_ipc_send_errorcode(clientfd,
					BP_ERROR_INVALID_PARAMETER);
				close(clientfd);
				continue;
			}

			if (connect_cmd == BP_CMD_INITIALIZE) {

				bp_client_type_defs client_type = BP_CLIENT_NONE;
				if (read(clientfd, &client_type, sizeof(bp_client_type_defs)) <= 0) {
					TRACE_ERROR("[CRITICAL] bad client");
					bp_ipc_send_errorcode(clientfd, BP_ERROR_IO_ERROR);
					close(clientfd);
					continue;
				}

				// search empty slot in slots
				for (i = 0; i < BP_MAX_CLIENT; i++)
					if (privates->slots[i].client == NULL)
						break;
				if (i >= BP_MAX_CLIENT) {
					TRACE_WARN("[WARN] Slots Limits:%d", BP_MAX_CLIENT);

					// search oldest access slot
					int oldest_time = (int)time(NULL);
					int now_time = (int)time(NULL);
					int oldest_index = -1;
					int iter = 0;
					oldest_time++; // most last time
					now_time++;
					for (; iter < BP_MAX_CLIENT; iter++) {
						bp_client_defs *client =
							privates->slots[iter].client;
						if (client != NULL) {
							if (client->access_time > 0 &&
								client->access_time < oldest_time) {
								oldest_time = client->access_time;
								oldest_index = iter;
							}
						}
					}

					if (oldest_index >= 0) {
						bp_client_defs *client =
							privates->slots[oldest_index].client;
						TRACE_WARN
							("clear client[%s:%d] slot:%d sock:%d",
							__print_client_type(client->type),
							client->cid, oldest_index,
							client->cmd_socket);
						if (client->cmd_socket >= 0)
							FD_CLR(client->cmd_socket, &listen_fdset);
						bp_client_free(client);
						privates->slots[oldest_index].client = NULL;
						i = oldest_index;
					} else {
						TRACE_ERROR("[CRITICAL] No space in slots");
						bp_ipc_send_errorcode(clientfd,
							BP_ERROR_OUT_OF_MEMORY);
						close(clientfd);
						continue;
					}
				}
				// allocation
				int cid = __get_client_id(privates->slots);
				privates->slots[i].client =
					(bp_client_defs *)calloc(1,
					sizeof(bp_client_defs));
				if (privates->slots[i].client == NULL) {
					TRACE_STRERROR("calloc, ignore this client");
					bp_ipc_send_errorcode(clientfd,
						BP_ERROR_OUT_OF_MEMORY);
					close(clientfd);
					continue;
				}
				TRACE_WARN("new-client[%s:%d] slot:%d sock:%d",
					__print_client_type(client_type), cid, i, clientfd);
				// fill info
				privates->slots[i].client->cid = cid;
				privates->slots[i].client->cmd_socket = clientfd;
				privates->slots[i].client->notify = -1;
				privates->slots[i].client->noti_enable = 0;
				privates->slots[i].client->tid = 0;
				privates->slots[i].client->type = client_type;
				privates->slots[i].client->access_time = (int)time(NULL);
				privates->slots[i].client->shm.key = cid;
				privates->slots[i].client->shm.mem = NULL;
				privates->slots[i].client->shm.local = NULL;
				privates->slots[i].client->privilege_label = NULL;

				// user-space-smack
				switch(client_type) {
				case BP_CLIENT_TABS:
				case BP_CLIENT_TABS_SYNC:
					privates->slots[i].client->privilege_label = SECURITY_PRIVILEGE_TAB;
					break;
				case BP_CLIENT_BOOKMARK:
				case BP_CLIENT_BOOKMARK_SYNC:
				case BP_CLIENT_BOOKMARK_CSC:
					privates->slots[i].client->privilege_label = SECURITY_PRIVILEGE_BOOKMARK;
					break;
				case BP_CLIENT_HISTORY:
				case BP_CLIENT_HISTORY_SYNC:
					privates->slots[i].client->privilege_label = SECURITY_PRIVILEGE_HISTORY;
					break;
				default:
					break;
				}
				if (privates->slots[i].client->privilege_label == NULL) {
					TRACE_ERROR("client type");
					bp_ipc_send_errorcode(clientfd,
						BP_ERROR_PERMISSION_DENY);
					bp_client_free(privates->slots[i].client);
					privates->slots[i].client = NULL;
					continue;
				} else if (!strcmp(privates->slots[i].client->privilege_label, "inhouse")) {
					// inhouse APIs (tab & scrap) have no privilege label.
					// temporarily grant permission to tab & scrap APIs without checking privilege.
					privates->slots[i].client->permission.read = 1;
					privates->slots[i].client->permission.write = 1;
				} else {
					if(cynara_creds_socket_get_client(clientfd, CLIENT_METHOD_SMACK,
						&(privates->slots[i].client->cynara_clientSmack)) != CYNARA_API_SUCCESS) {
						TRACE_ERROR("failed to create cynara client identification string");
						bp_ipc_send_errorcode(clientfd,
							BP_ERROR_PERMISSION_DENY);
						bp_client_free(privates->slots[i].client);
						privates->slots[i].client = NULL;
						continue;
					}
					if(cynara_creds_socket_get_user(clientfd, USER_METHOD_UID, 
						&(privates->slots[i].client->cynara_uid)) != CYNARA_API_SUCCESS) {
						TRACE_ERROR("failed to create cynara user identification string");
						bp_ipc_send_errorcode(clientfd,
							BP_ERROR_PERMISSION_DENY);
						bp_client_free(privates->slots[i].client);
						privates->slots[i].client = NULL;
						continue;
					}
					pid_t cynara_pid;
					if(cynara_creds_socket_get_pid(clientfd, &cynara_pid) != CYNARA_API_SUCCESS) {
						TRACE_ERROR("failed to create PID string for client session");
						bp_ipc_send_errorcode(clientfd,
							BP_ERROR_PERMISSION_DENY);
						bp_client_free(privates->slots[i].client);
						privates->slots[i].client = NULL;
						continue;
					}
					if((privates->slots[i].client->cynara_session = cynara_session_from_pid(cynara_pid)) == NULL) {
						TRACE_ERROR("failed to create client session");
						bp_ipc_send_errorcode(clientfd,
							BP_ERROR_PERMISSION_DENY);
						bp_client_free(privates->slots[i].client);
						privates->slots[i].client = NULL;
						continue;
					}

					__update_client_permission(privates->slots[i].client);
				}
/*
				TRACE_INFO("[Permission] sock:%d read:%d write:%d",
					clientfd,
					privates->slots[i].client->permission.read,
					privates->slots[i].client->permission.write);
*/
#ifdef SUPPORT_SERVER_PRIVILEGE
				if (privates->slots[i].client->permission.read == 0 &&
					privates->slots[i].client->permission.write == 0) {
					TRACE_ERROR("client have no privilege permission");
					bp_ipc_send_errorcode(clientfd,
						BP_ERROR_PERMISSION_DENY);
					bp_client_free(privates->slots[i].client);
					privates->slots[i].client = NULL;
					continue;
				}
#endif

#ifdef PROVIDER_DIR
				bp_rebuild_dir(PROVIDER_DIR);
#endif
#ifdef NOTIFY_DIR
				bp_rebuild_dir(NOTIFY_DIR);
#endif

				// make notify fifo
				privates->slots[i].client->notify = bp_notify_init(cid);
				if (privates->slots[i].client->notify < 0) {
					TRACE_STRERROR("failed to open fifo slot:%d", clientfd);
					bp_ipc_send_errorcode(clientfd, BP_ERROR_IO_ERROR);
					bp_client_free(privates->slots[i].client);
					privates->slots[i].client = NULL;
					continue;
				}

#ifndef SUPPORT_CLIENT_THREAD
				FD_SET(privates->slots[i].client->cmd_socket,
					&listen_fdset);
				if (privates->slots[i].client->cmd_socket > maxfd)
					maxfd = privates->slots[i].client->cmd_socket;
#endif

#ifdef SUPPORT_CLIENT_THREAD
				if (pthread_create(&(privates->slots[i].client->tid),
						NULL, client_thread_idle,
						(void *)&privates->slots[i]) != 0) {
					// check zombie client. disconnect it
					TRACE_ERROR("[ERROR][main_thread] create\n");
					bp_ipc_send_errorcode(clientfd,
						BP_ERROR_OUT_OF_MEMORY);
					bp_client_free(privates->slots[i].client);
					privates->slots[i].client = NULL;
					continue;
				}
				pthread_detach(privates->slots[i].client->tid);
#endif

				// ok. ready to accept API of this client
				bp_ipc_send_errorcode(clientfd, BP_ERROR_NONE);
				bp_ipc_send_custom_type(clientfd,
					&privates->slots[i].client->cid, sizeof(int));

			}
		} // New Connection

#ifndef SUPPORT_CLIENT_THREAD
		// listen cmd_socket of all client
		for (i = 0; i < BP_MAX_CLIENT; i++) {
			if (privates->slots[i].client == NULL)
				continue;
			if (privates->slots[i].client->cmd_socket < 0){
				TRACE_ERROR("[CRITICAL] [i] Found Bad socket", i);
				bp_client_free(privates->slots[i].client);
				privates->slots[i].client = NULL;
				continue;
			}

			if (FD_ISSET(privates->slots[i].client->cmd_socket, &rset)
				> 0) {
				is_timeout = 0;

				privates->slots[i].client->access_time = (int)time(NULL);
				if (__handle_client_request
					(privates->slots[i].client) == BP_ERROR_IO_ERROR) {
					TRACE_ERROR("disconnect client slot:%d sock:%d", i,
						client->cmd_socket);
					FD_CLR(client->cmd_socket, &listen_fdset);
					bp_client_free(client);
					privates->slots[i].client = NULL;
				}
			} // FD_ISSET
		} // BP_MAX_CLIENT

#endif

		// timeout
		if (is_timeout == 1) {
			int now_timeout = (int)time(NULL);
			if (prev_timeout == 0) {
				prev_timeout = now_timeout;
			} else if (now_timeout < prev_timeout ||
				(now_timeout - prev_timeout) >
				(flexible_timeout + BP_CARE_CLIENT_MIN_INTERVAL)) {
				TRACE_WARN("[WARN] check system date prev[%ld]now[%ld]",
					prev_timeout, now_timeout);
			} else {
				if ((now_timeout - prev_timeout) <
						BP_CARE_CLIENT_MIN_INTERVAL) {
					// this is error.
					// terminate Process
					TRACE_STRERROR
					("[CRITICAL] Sock exception prev[%ld]now[%ld][%ld]",
					prev_timeout, now_timeout, flexible_timeout);
					break;
				}
			}

			int connected_count = 0;
			for (i = 0; i < BP_MAX_CLIENT; i++) {
				int locked =
					pthread_mutex_trylock(&privates->slots[i].mutex);
				if (locked == 0) {
					bp_client_defs *client = privates->slots[i].client;
					if (client != NULL) {
						// if older than BP_CARE_CLIENT_MAX_INTERVAL, close
						if (client->access_time > 0 &&
								client->noti_enable == 0 &&
								(now_timeout - client->access_time) >
									BP_CARE_CLIENT_MAX_INTERVAL) {
							TRACE_WARN("old client[%s:%d] slot:%d sock:%d",
								__print_client_type(client->type),
								client->cid, i, client->cmd_socket);
							if (client->notify >= 0) {
								bp_command_defs cmd = BP_CMD_DEINITIALIZE;
								bp_ipc_send_custom_type(client->notify,
									&cmd, sizeof(bp_command_defs));
							}
						}
						// increase even if closed by above condition.
						// to skip one time in case of no clients by closing
						connected_count++;
					}

					pthread_mutex_unlock(&privates->slots[i].mutex);
				} else {
					connected_count++;
				}
			}

			if (connected_count == 0) {
				TRACE_WARN("Expire Idle state. No Client.");
				break;
			}

			prev_timeout = now_timeout;
		} else {
			prev_timeout = 0;
		}
	}
	TRACE_WARN("terminate main thread ...");
	pthread_mutex_lock(&mutex_for_cynara_check);
	cynara_finish(p_cynara);
	pthread_mutex_unlock(&mutex_for_cynara_check);
}
