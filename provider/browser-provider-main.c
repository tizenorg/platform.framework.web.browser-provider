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
#include <signal.h>
#include <pthread.h>

#include <systemd/sd-daemon.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#include "browser-provider-config.h"
#include "browser-provider-log.h"
#include "browser-provider-socket.h"
#include "browser-provider-slots.h"

#include "browser-provider-tabs.h"
#include "browser-provider-bookmarks.h"
#include "browser-provider-scraps.h"
#include "browser-provider-history.h"

bp_privates_defs *g_privates = NULL;

// declare functions
int bp_lock_pid(char *path);
void bp_thread_requests_manager(bp_privates_defs *privates);

#ifdef SUPPORT_BOOTING_DONE
#include <glib.h>
#include <glib-object.h>
#include <Ecore.h>
#include <E_DBus.h>
#define BUS_NAME				"org.tizen.system.deviced"
#define DEVICED_PATH_CORE		"/Org/Tizen/System/DeviceD/Core"
#define DEVICED_INTERFACE_CORE	BUS_NAME".core"
#define SIGNAL_BOOTING_DONE		"BootingDone"

E_DBus_Signal_Handler *g_e_dbus_handler = NULL;
E_DBus_Connection *g_e_dbus_conn = NULL;

static void __unregister_dbus_booting_done_service(void)
{
	if (g_e_dbus_conn != NULL) {
		ecore_main_loop_quit();
		if (g_e_dbus_handler != NULL)
			e_dbus_signal_handler_del(g_e_dbus_conn, g_e_dbus_handler);
		g_e_dbus_handler = NULL;
		e_dbus_connection_close(g_e_dbus_conn);
	}
	g_e_dbus_conn = NULL;
}

static void on_changed_receive(void *data, DBusMessage *msg)
{
	TRACE_DEBUG("BootingDone");
	if (dbus_message_is_signal(msg, DEVICED_INTERFACE_CORE, SIGNAL_BOOTING_DONE) == FALSE) {
		TRACE_ERROR("dbus_message_is_signal error");
		return;
	}

	TRACE_DEBUG("%s - %s", DEVICED_INTERFACE_CORE, SIGNAL_BOOTING_DONE);

	__unregister_dbus_booting_done_service();

	if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
		TRACE_ERROR("failed to register signal callback");
	}
	pid_t child_pid = 0;
	if((child_pid = fork()) < 0 ) {
		TRACE_STRERROR("[ERROR] fock");
	} else {
		if(child_pid == 0) {
			execl("/usr/apps/org.tizen.browser/bin/browser",
				"/usr/apps/org.tizen.browser/bin/browser",
				"precaching", NULL);
			TRACE_STRERROR("[ERROR] execl browser precaching");
			exit(EXIT_SUCCESS);
		}
	}
}

static gboolean __bp_idle_booting_done_service(void *data)
{
	TRACE_DEBUG("");
	ecore_init();
	e_dbus_init();
	g_e_dbus_conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
	if (g_e_dbus_conn == NULL) {
		TRACE_ERROR("e_dbus_bus_get error");
		return -1;
	}

	if (e_dbus_request_name(g_e_dbus_conn, BUS_NAME, 0, NULL, NULL) == NULL) {
		TRACE_ERROR("e_dbus_request_name error");
		return -1;
	}

	g_e_dbus_handler =
		e_dbus_signal_handler_add(g_e_dbus_conn, NULL, DEVICED_PATH_CORE,
						DEVICED_INTERFACE_CORE, SIGNAL_BOOTING_DONE,
						on_changed_receive, NULL);
	if (g_e_dbus_handler == NULL) {
		TRACE_ERROR("e_dbus_signal_handler_add error");
		return -1;
	}
	ecore_main_loop_begin();
	return 0;
}

static void __register_dbus_booting_done_service(void)
{
	g_idle_add(__bp_idle_booting_done_service, NULL);
}
#endif

static int __bp_accept_socket_new()
{
	int fd_base, listen_fds = sd_listen_fds(1);
	TRACE_DEBUG("sd_listen_fds:%d", listen_fds);
	for (fd_base = 0 ; fd_base < listen_fds; fd_base++) {
		if (sd_is_socket_unix(fd_base + SD_LISTEN_FDS_START, SOCK_STREAM, 1, IPC_SOCKET, 0) >= 0) {
			TRACE_INFO("listen systemd socket:%d", fd_base + SD_LISTEN_FDS_START);
			return fd_base + SD_LISTEN_FDS_START;
		}
	}
	return -1;
}

void bp_terminate(int signo)
{
	TRACE_WARN("terminating:%d", signo);
	close(g_privates->listen_fd);
	g_privates->listen_fd = -1;
}

int main()
{
	// ready socket ( listen )
	int socket = __bp_accept_socket_new();
	if (socket < 0) {
		TRACE_ERROR("[CRITICAL] failed to bind SOCKET");
		exit(EXIT_FAILURE);
	}

	if (signal(SIGTERM, bp_terminate) == SIG_ERR ||
			signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		TRACE_ERROR("failed to register signal callback");
	}

#ifdef SUPPORT_BOOTING_DONE
	__register_dbus_booting_done_service();
#endif

	g_privates = (bp_privates_defs *)calloc(1, sizeof(bp_privates_defs));
	if (g_privates == NULL) {
		TRACE_ERROR("[CRITICAL] failed to alloc for private info");
	} else {
		g_privates->listen_fd = socket;
		bp_thread_requests_manager(g_privates);
	}

	TRACE_WARN("Browser-Provider will be terminated.");

	if (g_privates != NULL) {
		if (g_privates->listen_fd >= 0) {
			close(g_privates->listen_fd);
			g_privates->listen_fd = -1;
		}
		bp_client_slots_free(g_privates->slots, BP_MAX_CLIENT);
		g_privates->slots = NULL;
		free(g_privates->device_id);
		free(g_privates);
		g_privates = NULL;
	}
	bp_tabs_free_resource();
	bp_bookmark_free_resource();
	bp_scraps_free_resource();
	bp_history_free_resource();

#ifdef SUPPORT_BOOTING_DONE
	__unregister_dbus_booting_done_service();
#endif

	exit(EXIT_SUCCESS);
}
