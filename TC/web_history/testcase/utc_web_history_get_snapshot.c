/*
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
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

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <tet_api.h>
#include <web_history.h>

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

static int g_testcase_id = -1;

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_web_history_get_snapshot_n(void);
static void utc_web_history_get_snapshot_p(void);


struct tet_testlist tet_testlist[] = {
	{ utc_web_history_get_snapshot_n, NEGATIVE_TC_IDX },
	{ utc_web_history_get_snapshot_p, POSITIVE_TC_IDX },
	{ NULL, 0 }
};


static void startup(void)
{
	/* start of TC */
	tet_printf("\n TC start");
	bp_history_adaptor_initialize();
	bp_history_adaptor_create(&g_testcase_id);
}


static void cleanup(void)
{
	/* end of TC */
	bp_history_adaptor_deinitialize();
	tet_printf("\n TC end");
}

/**
 * @brief Negative test case of bp_history_adaptor_get_snapshot()
 */
static void utc_web_history_get_snapshot_n(void)
{
	int ret = bp_history_adaptor_get_snapshot(-1, NULL, NULL, NULL, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_history_adaptor_get_snapshot()
 */
static void utc_web_history_get_snapshot_p(void)
{
	int length = 0;
	int width = 0;
	int height = 0;
	unsigned char *value = NULL;
	int ret = bp_history_adaptor_get_snapshot(g_testcase_id, &width,
		&height, &value, &length);
	free(value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_history_adaptor_get_snapshot failed");
}
