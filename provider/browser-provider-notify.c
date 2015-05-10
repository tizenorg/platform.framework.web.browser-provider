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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "browser-provider-log.h"
#include "browser-provider-notify.h"

static char *__bp_notify_get_path(int pid)
{
	size_t path_size = sizeof(NOTIFY_DIR) + 21;
	char *notify_fifo = (char *)calloc(path_size, sizeof(char));
	if (notify_fifo == NULL) {
		TRACE_STRERROR("failed to alocalte fifo path pid:%d", (int)pid);
		return NULL;
	}
	if (snprintf(notify_fifo, path_size,"%s/%d", NOTIFY_DIR, pid) < 0) {
		TRACE_STRERROR("failed to make fifo path pid:%d", (int)pid);
		free(notify_fifo);
		return NULL;
	}
	return notify_fifo;
}

int bp_notify_init(int pid)
{
	char *notify_fifo = __bp_notify_get_path(pid);
	if (notify_fifo == NULL)
		return -1;
	int notify_fd = -1;
	struct stat fifo_state;
	if (stat(notify_fifo, &fifo_state) == 0) // found
		unlink(notify_fifo);
	if (mkfifo(notify_fifo, 0644/*-rwrr*/) < 0) {
		TRACE_STRERROR("failed to make fifo %s", notify_fifo);
	} else {
		notify_fd = open(notify_fifo, O_RDWR | O_NONBLOCK, 0600);
	}
	free(notify_fifo);
	return notify_fd;
}

void bp_notify_deinit(int pid)
{
	char *notify_fifo = __bp_notify_get_path(pid);
	if (notify_fifo == NULL)
		return ;
	struct stat fifo_state;
	if (stat(notify_fifo, &fifo_state) == 0) // found
		unlink(notify_fifo);
	free(notify_fifo);
}
