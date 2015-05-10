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

#include <sys/ipc.h>
#include <sys/shm.h>

#include "browser-provider-shm.h"
#include "browser-provider-log.h"

void bp_shm_free(bp_shm_defs *shm)
{
	if (shm != NULL) {
		if (shm->mem != NULL)
			shmdt(shm->mem);
		shm->mem = NULL;
		if (shm->id > 0)
			shmctl(shm->id, IPC_RMID, 0);
		shm->id = -1;
		free(shm->local);
		shm->local = NULL;
	}
}

int bp_shm_alloc(bp_shm_defs *shm)
{
	if (shm == NULL)
		return -1;
	bp_shm_free(shm);
	shm->id = shmget((key_t)shm->key,
			sizeof(unsigned char) * BASIC_SHM_SIZE, 0666|IPC_CREAT);
	if (shm->id < 0) {
		TRACE_ERROR("[ERROR][SHM][%d] shmget", shm->key);
		return -1;
	}
	if (shm->id > 0) {
		shm->mem = shmat(shm->id, (void *)0, 0);
		if (shm->mem == (void *)-1) {
			TRACE_ERROR("[ERROR][SHM][%d] shmat", shm->key);
			bp_shm_free(shm);
			return -1;
		}
	}
	return 0;
}

int bp_shm_write(bp_shm_defs *shm, unsigned char *value,
	size_t value_size)
{
	if (bp_shm_is_ready(shm, value_size) == 0) {
		memcpy(shm->mem, value, sizeof(unsigned char) * value_size);
		return 0;
	}
	return -1;
}

int bp_shm_read_copy(bp_shm_defs *shm, unsigned char **value,
	size_t value_size)
{
	if (bp_shm_is_ready(shm, value_size) == 0) {
		unsigned char *blob_data = NULL;
		blob_data = (unsigned char *)calloc(value_size,
				sizeof(unsigned char));
		if (blob_data != NULL) {
			memcpy(blob_data, shm->mem,
					sizeof(unsigned char) * value_size);
			*value = blob_data;
			return 0;
		}
		return -1;
	}
	return -1;
}

int bp_shm_is_ready(bp_shm_defs *shm, int length)
{
	if (shm != NULL) {
		if (shm->id < 0 || shm->mem == NULL) {
			if (bp_shm_alloc(shm) < 0)
				return -1;
		}
		if (shm->mem != NULL && length <= BASIC_SHM_SIZE)
			return 0;
	}
	return -1;
}
