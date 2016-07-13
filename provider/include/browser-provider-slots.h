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

#ifndef BROWSER_PROVIDER_SLOTS_H
#define BROWSER_PROVIDER_SLOTS_H

#include "browser-provider.h"
#include "browser-provider-shm.h"

// Backgound Daemon should has the limitation of resource.
#define BP_MAX_CLIENT 30

typedef struct {
	unsigned write;
	unsigned read;
} bp_privilege_defs;

typedef struct {
	// unique id
	long long int cid;
	// send command * get return value.
	int cmd_socket;
	// send noti to client
	int notify;
	int noti_enable;
	int access_time;
	bp_client_type_defs type;
	bp_privilege_defs permission;
	pthread_t tid;
	bp_shm_defs shm;
	char *cynara_clientSmack;
	char *cynara_uid;
	char *cynara_session;
	char *privilege_label;
} bp_client_defs;

typedef struct {
	pthread_mutex_t mutex;
	bp_client_defs *client;
} bp_client_slots_defs;

// functions
long long int bp_create_unique_id(void);
bp_client_slots_defs *bp_client_slots_new(int size);
int bp_client_free(bp_client_defs *client);
void bp_client_slots_free(bp_client_slots_defs *slots, int size);

#endif
