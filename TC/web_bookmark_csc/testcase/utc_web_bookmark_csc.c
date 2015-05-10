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
#include <web_bookmark_csc.h>

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

static int g_testcase_id = -1;

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_web_bookmark_csc_get_root_n(void);
static void utc_web_bookmark_csc_get_root_p(void);
static void utc_web_bookmark_csc_create_n(void);
static void utc_web_bookmark_csc_create_p(void);
static void utc_web_bookmark_csc_delete_n(void);
static void utc_web_bookmark_csc_delete_p(void);
static void utc_web_bookmark_csc_get_info_n(void);
static void utc_web_bookmark_csc_get_info_p(void);
static void utc_web_bookmark_csc_get_full_ids_p_n(void);
static void utc_web_bookmark_csc_get_full_ids_p_p(void);
static void utc_web_bookmark_csc_get_ids_p_n(void);
static void utc_web_bookmark_csc_get_ids_p_p(void);
static void utc_web_bookmark_csc_get_duplicated_title_ids_p_n(void);
static void utc_web_bookmark_csc_get_duplicated_title_ids_p_p(void);
static void utc_web_bookmark_csc_get_errorcode_p(void);
static void utc_web_bookmark_csc_free_p(void);
static void utc_web_bookmark_csc_reset_p(void);


struct tet_testlist tet_testlist[] = {
	{ utc_web_bookmark_csc_get_root_n, NEGATIVE_TC_IDX },
	{ utc_web_bookmark_csc_get_root_p, POSITIVE_TC_IDX },
	{ utc_web_bookmark_csc_create_n, NEGATIVE_TC_IDX },
	{ utc_web_bookmark_csc_create_p, POSITIVE_TC_IDX },
	{ utc_web_bookmark_csc_delete_n, NEGATIVE_TC_IDX },
	{ utc_web_bookmark_csc_delete_p, POSITIVE_TC_IDX },
	{ utc_web_bookmark_csc_get_info_n, NEGATIVE_TC_IDX },
	{ utc_web_bookmark_csc_get_info_p, POSITIVE_TC_IDX },
	{ utc_web_bookmark_csc_get_full_ids_p_n, NEGATIVE_TC_IDX },
	{ utc_web_bookmark_csc_get_full_ids_p_p, POSITIVE_TC_IDX },
	{ utc_web_bookmark_csc_get_ids_p_n, NEGATIVE_TC_IDX },
	{ utc_web_bookmark_csc_get_ids_p_p, POSITIVE_TC_IDX },
	{ utc_web_bookmark_csc_get_duplicated_title_ids_p_n, NEGATIVE_TC_IDX },
	{ utc_web_bookmark_csc_get_duplicated_title_ids_p_p, POSITIVE_TC_IDX },
	{ utc_web_bookmark_csc_get_errorcode_p, POSITIVE_TC_IDX },
	{ utc_web_bookmark_csc_free_p, POSITIVE_TC_IDX },
	{ utc_web_bookmark_csc_reset_p, POSITIVE_TC_IDX },
	{ NULL, 0 }
};


static void startup(void)
{
	/* start of TC */
	tet_printf("\n TC start");
	bookmark_csc_initialize();
}


static void cleanup(void)
{
	/* end of TC */
	bookmark_csc_deinitialize();
	tet_printf("\n TC end");
}

/**
 * @brief Negative test case of bookmark_csc_get_root()
 */
static void utc_web_bookmark_csc_get_root_n(void)
{
	int ret = bookmark_csc_get_root(NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bookmark_csc_get_root()
 */
static void utc_web_bookmark_csc_get_root_p(void)
{
	int root_id = -1;
	int ret = bookmark_csc_get_root(&root_id);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bookmark_csc_get_root failed");
}

/**
 * @brief Negative test case of bookmark_csc_create()
 */
static void utc_web_bookmark_csc_create_n(void)
{
	int ret = bookmark_csc_create(NULL, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bookmark_csc_create()
 */
static void utc_web_bookmark_csc_create_p(void)
{
	int root_id = -1;
	int ret = bookmark_csc_get_root(&root_id);
	bookmark_csc_info_fmt info;
	memset(&info, 0x00, sizeof(bookmark_csc_info_fmt));
	info.type = BOOKMARK_CSC_TYPE_BOOKMARK;
	info.title = "TITLE";
	info.url = "URL";
	info.parent = root_id;
	info.editable = 1;
	ret = bookmark_csc_create(&g_testcase_id, &info);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bookmark_csc_create failed");
}

/**
 * @brief Negative test case of bookmark_csc_delete()
 */
static void utc_web_bookmark_csc_delete_n(void)
{
	int ret = bookmark_csc_delete(-1);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bookmark_csc_delete()
 */
static void utc_web_bookmark_csc_delete_p(void)
{
	int id = -1;
	int root_id = -1;
	int ret = bookmark_csc_get_root(&root_id);
	bookmark_csc_info_fmt info;
	memset(&info, 0x00, sizeof(bookmark_csc_info_fmt));
	info.type = BOOKMARK_CSC_TYPE_BOOKMARK;
	info.title = "TITLE";
	info.url = "URL";
	info.parent = root_id;
	info.editable = 1;
	ret = bookmark_csc_create(&id, &info);
	if (ret == 0)
		ret = bookmark_csc_delete(id);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bookmark_csc_create failed");
}

/**
 * @brief Negative test case of bookmark_csc_get_info()
 */
static void utc_web_bookmark_csc_get_info_n(void)
{
	int ret = bookmark_csc_get_info(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bookmark_csc_get_info()
 */
static void utc_web_bookmark_csc_get_info_p(void)
{
	bookmark_csc_info_fmt info;
	memset(&info, 0x00, sizeof(bookmark_csc_info_fmt));
	int ret = bookmark_csc_get_info(g_testcase_id, &info);
	bookmark_csc_free(&info);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bookmark_csc_get_info failed");
}

/**
 * @brief Negative test case of bookmark_csc_get_duplicated_title_ids_p()
 */
static void utc_web_bookmark_csc_get_duplicated_title_ids_p_n(void)
{
	int ret = bookmark_csc_get_duplicated_title_ids_p(NULL, NULL, 0, 0,
		0, 0, 0, 0, 0, NULL, 0);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bookmark_csc_get_duplicated_title_ids_p()
 */
static void utc_web_bookmark_csc_get_duplicated_title_ids_p_p(void)
{
	int *ids = NULL;
	int ids_count = 0;
	int ret = bookmark_csc_get_duplicated_title_ids_p(&ids, &ids_count,
		1, 0, -1, -1, -1, -1, 0/*ASC*/, "%T%", 1);
	free(ids);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bookmark_csc_get_duplicated_title_ids_p failed");
}

/**
 * @brief Negative test case of bookmark_csc_get_full_ids_p()
 */
static void utc_web_bookmark_csc_get_full_ids_p_n(void)
{
	int ret = bookmark_csc_get_full_ids_p(BOOKMARK_CSC_TYPE_BOOKMARK, NULL, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bookmark_csc_get_full_ids_p()
 */
static void utc_web_bookmark_csc_get_full_ids_p_p(void)
{
	int *ids = NULL;
	int ids_count = 0;
	int ret = bookmark_csc_get_full_ids_p(BOOKMARK_CSC_TYPE_BOOKMARK, &ids, &ids_count);
	free(ids);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bookmark_csc_get_full_ids_p failed");
}


/**
 * @brief Negative test case of bookmark_csc_get_ids_p()
 */
static void utc_web_bookmark_csc_get_ids_p_n(void)
{
	int ret = bookmark_csc_get_ids_p(-1, NULL, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bookmark_csc_get_ids_p()
 */
static void utc_web_bookmark_csc_get_ids_p_p(void)
{
	int root_id = -1;
	int ret = bookmark_csc_get_root(&root_id);
	int *ids = NULL;
	int ids_count = 0;
	ret = bookmark_csc_get_ids_p(root_id, &ids, &ids_count);
	free(ids);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bookmark_csc_get_ids_p failed");
}



/**
 * @brief Positive test case of bookmark_csc_get_errorcode()
 */
static void utc_web_bookmark_csc_get_errorcode_p(void)
{
	bookmark_csc_get_errorcode();
	dts_pass(__FUNCTION__);
}

/**
 * @brief Positive test case of bookmark_csc_free()
 */
static void utc_web_bookmark_csc_free_p(void)
{
	bookmark_csc_free(NULL);
	dts_pass(__FUNCTION__);
}

/**
 * @brief Positive test case of bookmark_csc_reset()
 */
static void utc_web_bookmark_csc_reset_p(void)
{
	bookmark_csc_reset();
	dts_pass(__FUNCTION__);
}
