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

#ifndef BROWSER_PROVIDER_TABS_H
#define BROWSER_PROVIDER_TABS_H

#include "browser-provider.h"


bp_error_defs bp_tabs_handle_requests(bp_client_slots_defs *groups,
	bp_client_defs *client, bp_command_fmt *client_cmd);
void bp_tabs_free_resource();
bp_error_defs bp_tabs_ready_resource();
#endif
