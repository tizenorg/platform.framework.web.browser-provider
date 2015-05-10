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

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_web_history_get_inquired_ids_p_n(void);
static void utc_web_history_get_inquired_ids_p_p(void);


struct tet_testlist tet_testlist[] = {
	{ utc_web_history_get_inquired_ids_p_n, NEGATIVE_TC_IDX },
	{ utc_web_history_get_inquired_ids_p_p, POSITIVE_TC_IDX },
	{ NULL, 0 }
};


static void startup(void)
{
	/* start of TC */
	tet_printf("\n TC start");
	bp_history_adaptor_initialize();
}


static void cleanup(void)
{
	/* end of TC */
	bp_history_adaptor_deinitialize();
	tet_printf("\n TC end");
}

/**
 * @brief Negative test case of bp_history_adaptor_get_inquired_ids_p()
 */
static void utc_web_history_get_inquired_ids_p_n(void)
{
	int ret = bp_history_adaptor_get_inquired_ids_p(NULL, NULL, -1, 0,
		0, 0, NULL, 0);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_history_adaptor_get_inquired_ids_p()
 */
static void utc_web_history_get_inquired_ids_p_p(void)
{
	int *ids = NULL;
	int ids_count = 0;
	int ret = bp_history_adaptor_get_inquired_ids_p(&ids, &ids_count, -1, 0,
				BP_HISTORY_O_DATE_VISITED, 0/*ASC*/, "%T%", 1);
	free(ids);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_history_adaptor_get_inquired_ids_p failed");
}
