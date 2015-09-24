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

#ifndef BROWSER_PROVIDER_SHM_H
#define BROWSER_PROVIDER_SHM_H

#define BASIC_SHM_SIZE 3145728 //3M // 1445760 (720*502*4)
#define NO_USE_SHM //Not using bp_shm_xxx, smack deny during the shmget on browser application.

typedef struct {
	int key;
	int id;
	void *mem;
	unsigned char *local;
} bp_shm_defs;

int bp_shm_alloc(bp_shm_defs *shm);
void bp_shm_free(bp_shm_defs *shm);
int bp_shm_write(bp_shm_defs *shm, unsigned char *value, size_t value_size);
int bp_shm_read_copy(bp_shm_defs *shm, unsigned char **value, size_t value_size);
int bp_shm_is_ready(bp_shm_defs *shm, int length);

#endif
