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
#include <web_bookmark.h>

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_web_bookmark_get_cond_ids_p_n(void);
static void utc_web_bookmark_get_cond_ids_p_p(void);


struct tet_testlist tet_testlist[] = {
	{ utc_web_bookmark_get_cond_ids_p_n, NEGATIVE_TC_IDX },
	{ utc_web_bookmark_get_cond_ids_p_p, POSITIVE_TC_IDX },
	{ NULL, 0 }
};


static void startup(void)
{
	/* start of TC */
	tet_printf("\n TC start");
	bp_bookmark_adaptor_initialize();
}


static void cleanup(void)
{
	/* end of TC */
	bp_bookmark_adaptor_deinitialize();
	tet_printf("\n TC end");
}

/**
 * @brief Negative test case of bp_bookmark_adaptor_get_cond_ids_p()
 */
static void utc_web_bookmark_get_cond_ids_p_n(void)
{
	int ret = bp_bookmark_adaptor_get_cond_ids_p(NULL, NULL, NULL, NULL,
		0, NULL, 0);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_bookmark_adaptor_get_cond_ids_p()
 */
static void utc_web_bookmark_get_cond_ids_p_p(void)
{
	bp_bookmark_property_cond_fmt properties;
	bp_bookmark_rows_cond_fmt conds;
	memset(&properties, 0x00, sizeof(bp_bookmark_property_cond_fmt));
	memset(&conds, 0x00, sizeof(bp_bookmark_rows_cond_fmt));
	properties.parent = -1;
	properties.type = 0;
	properties.is_operator = 0;
	properties.is_editable = -1;
	conds.limit = -1;
	conds.offset = 0;
	conds.order_offset = BP_BOOKMARK_O_SEQUENCE;
	conds.ordering = 0;
	conds.period_offset = BP_BOOKMARK_O_DATE_CREATED;
	conds.period_type = BP_BOOKMARK_DATE_ALL;
	int *ids = NULL;
	int ids_count = 0;
	int ret = bp_bookmark_adaptor_get_cond_ids_p(&ids, &ids_count,
		&properties, &conds, BP_BOOKMARK_O_TITLE, "%T%", 1);
	free(ids);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_bookmark_adaptor_get_cond_ids_p failed");
}
