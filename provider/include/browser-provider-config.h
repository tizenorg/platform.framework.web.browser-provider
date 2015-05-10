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

#ifndef BROWSER_PROVIDER_CONFIG_H
#define BROWSER_PROVIDER_CONFIG_H

#include <browser-provider.h>
#include <browser-provider-slots.h>

#define BP_LOCK_PID "/tmp/browser-provider.lock"

#define BP_CARE_CLIENT_MIN_INTERVAL 5
#define BP_CARE_CLIENT_MAX_INTERVAL 60

// Share the structure for all threads
typedef struct {
	int listen_fd;
	bp_client_slots_defs *slots;
	char *device_id;
} bp_privates_defs;

#endif
