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

#include <sys/time.h>

#include <pthread.h>

#include "browser-provider.h"
#include "browser-provider-log.h"

#include "browser-provider-slots.h"
#include "browser-provider-socket.h"
#include "browser-provider-shm.h"
#include "browser-provider-notify.h"

int bp_create_unique_id(void)
{
	static int last_uniquetime = 0;
	int uniquetime = 0;

	do {
		struct timeval tval;
		int cipher = 1;
		int c = 0;

		gettimeofday(&tval, NULL);

		int usec = tval.tv_usec;
		for (c = 0; ; c++, cipher++) {
			if ((usec /= 10) <= 0)
				break;
		}
		if (tval.tv_usec == 0)
			tval.tv_usec = (tval.tv_sec & 0x0fff);
		int disit_unit = 10;
		for (c = 0; c < cipher - 3; c++)
			disit_unit = disit_unit * 10;
		uniquetime = tval.tv_sec + ((tval.tv_usec << 2) * 100) +
				((tval.tv_usec >> (cipher - 1)) * disit_unit) +
				((tval.tv_usec + (tval.tv_usec % 10)) & 0x0fff);
	} while (last_uniquetime == uniquetime);
	last_uniquetime = uniquetime;
	return uniquetime;
}

bp_client_slots_defs *bp_client_slots_new(int size)
{
	bp_client_slots_defs *slots = NULL;
	if (size <= 0)
		return NULL;
	slots = (bp_client_slots_defs *)calloc(size,
			sizeof(bp_client_slots_defs));
	if (slots != NULL) {
		int i = 0;
		for (; i < size; i++)
			pthread_mutex_init(&slots[i].mutex, NULL);
	}
	return slots;
}

int bp_client_free(bp_client_defs *client)
{
	if (client == NULL)
		return -1;

	if (client->cmd_socket >= 0)
		close(client->cmd_socket);
	client->cmd_socket = -1;
	if (client->notify >= 0) {
		close(client->notify);
		bp_notify_deinit(client->cid);
	}
	client->notify = -1;
	client->cid = 0;
	client->permission.read = 0;
	client->permission.write = 0;
	pthread_t tid = client->tid;
	bp_shm_free(&client->shm);
	if (client->cynara_clientSmack != NULL)
		free(client->cynara_clientSmack);
	if (client->cynara_uid != NULL)
		free(client->cynara_uid);
	if (client->cynara_session != NULL)
		free(client->cynara_session);
	client->privilege_label = NULL;
	free(client);
	client = NULL;

	if (tid > 0)
		pthread_cancel(tid);
	return 0;
}

void bp_client_slots_free(bp_client_slots_defs *slots, int size)
{
	int i = 0;
	if (slots != NULL) {
		for (; i < size; i++) {
			if (slots[i].client == NULL)
				bp_client_free(slots[i].client);
			slots[i].client = NULL;
			pthread_mutex_destroy(&slots[i].mutex);
		}
		free(slots);
		slots = NULL;
	}
}
