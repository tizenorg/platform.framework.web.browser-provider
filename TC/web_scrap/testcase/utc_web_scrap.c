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
#include <web_scrap.h>

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

static int g_testcase_id = -1;

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_web_scrap_get_full_ids_p_n(void);
static void utc_web_scrap_get_full_ids_p_p(void);
static void utc_web_scrap_get_full_with_deleted_ids_p_n(void);
static void utc_web_scrap_get_full_with_deleted_ids_p_p(void);
static void utc_web_scrap_get_dirty_ids_p_n(void);
static void utc_web_scrap_get_dirty_ids_p_p(void);
static void utc_web_scrap_get_deleted_ids_p_n(void);
static void utc_web_scrap_get_deleted_ids_p_p(void);
static void utc_web_scrap_get_is_read_n(void);
static void utc_web_scrap_get_is_read_p(void);
static void utc_web_scrap_get_page_path_n(void);
static void utc_web_scrap_get_page_path_p(void);
static void utc_web_scrap_get_url_n(void);
static void utc_web_scrap_get_url_p(void);
static void utc_web_scrap_get_title_n(void);
static void utc_web_scrap_get_title_p(void);
static void utc_web_scrap_get_date_created_n(void);
static void utc_web_scrap_get_date_created_p(void);
static void utc_web_scrap_get_date_modified_n(void);
static void utc_web_scrap_get_date_modified_p(void);
static void utc_web_scrap_get_account_name_n(void);
static void utc_web_scrap_get_account_name_p(void);
static void utc_web_scrap_get_account_type_n(void);
static void utc_web_scrap_get_account_type_p(void);
static void utc_web_scrap_get_device_name_n(void);
static void utc_web_scrap_get_device_name_p(void);
static void utc_web_scrap_get_device_id_n(void);
static void utc_web_scrap_get_device_id_p(void);
static void utc_web_scrap_get_icon_n(void);
static void utc_web_scrap_get_icon_p(void);
static void utc_web_scrap_get_snapshot_n(void);
static void utc_web_scrap_get_snapshot_p(void);
static void utc_web_scrap_set_dirty_n(void);
static void utc_web_scrap_set_dirty_p(void);
static void utc_web_scrap_set_is_read_n(void);
static void utc_web_scrap_set_is_read_p(void);
static void utc_web_scrap_set_page_path_n(void);
static void utc_web_scrap_set_page_path_p(void);
static void utc_web_scrap_set_url_n(void);
static void utc_web_scrap_set_url_p(void);
static void utc_web_scrap_set_title_n(void);
static void utc_web_scrap_set_title_p(void);
static void utc_web_scrap_set_date_created_n(void);
static void utc_web_scrap_set_date_created_p(void);
static void utc_web_scrap_set_date_modified_n(void);
static void utc_web_scrap_set_date_modified_p(void);
static void utc_web_scrap_set_account_name_n(void);
static void utc_web_scrap_set_account_name_p(void);
static void utc_web_scrap_set_account_type_n(void);
static void utc_web_scrap_set_account_type_p(void);
static void utc_web_scrap_set_device_name_n(void);
static void utc_web_scrap_set_device_name_p(void);
static void utc_web_scrap_set_device_id_n(void);
static void utc_web_scrap_set_device_id_p(void);
static void utc_web_scrap_set_icon_n(void);
static void utc_web_scrap_set_icon_p(void);
static void utc_web_scrap_set_snapshot_n(void);
static void utc_web_scrap_set_snapshot_p(void);
static void utc_web_scrap_create_n(void);
static void utc_web_scrap_create_p(void);
static void utc_web_scrap_delete_n(void);
static void utc_web_scrap_delete_p(void);
static void utc_web_scrap_easy_create_n(void);
static void utc_web_scrap_easy_create_p(void);
static void utc_web_scrap_get_info_n(void);
static void utc_web_scrap_get_info_p(void);
static void utc_web_scrap_get_cond_ids_p_n(void);
static void utc_web_scrap_get_cond_ids_p_p(void);
static void utc_web_scrap_get_inquired_ids_p_n(void);
static void utc_web_scrap_get_inquired_ids_p_p(void);
static void utc_web_scrap_get_duplicated_ids_p_n(void);
static void utc_web_scrap_get_duplicated_ids_p_p(void);
static void utc_web_scrap_set_data_changed_cb_p(void);
static void utc_web_scrap_set_data_changed_cb_n(void);
static void utc_web_scrap_unset_data_changed_cb_p(void);
static void utc_web_scrap_is_setted_data_changed_cb_p(void);
static void utc_web_scrap_publish_notification_p(void);
static void utc_web_scrap_clear_dirty_ids_p(void);
static void utc_web_scrap_clear_deleted_ids_p(void);
static void utc_web_scrap_get_errorcode_p(void);
static void utc_web_scrap_easy_free_p(void);


struct tet_testlist tet_testlist[] = {
	{ utc_web_scrap_set_is_read_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_set_is_read_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_set_page_path_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_set_page_path_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_set_url_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_set_url_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_set_title_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_set_title_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_set_date_created_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_set_date_created_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_set_date_modified_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_set_date_modified_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_set_account_name_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_set_account_name_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_set_account_type_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_set_account_type_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_set_device_name_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_set_device_name_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_set_device_id_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_set_device_id_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_set_icon_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_set_icon_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_set_snapshot_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_set_snapshot_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_get_full_ids_p_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_get_full_ids_p_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_get_full_with_deleted_ids_p_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_get_full_with_deleted_ids_p_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_get_dirty_ids_p_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_get_dirty_ids_p_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_get_deleted_ids_p_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_get_deleted_ids_p_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_get_is_read_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_get_is_read_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_get_page_path_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_get_page_path_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_get_url_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_get_url_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_get_title_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_get_title_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_get_date_created_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_get_date_created_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_get_date_modified_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_get_date_modified_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_get_account_name_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_get_account_name_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_get_account_type_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_get_account_type_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_get_device_name_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_get_device_name_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_get_device_id_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_get_device_id_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_get_icon_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_get_icon_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_get_snapshot_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_get_snapshot_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_set_dirty_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_set_dirty_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_create_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_create_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_delete_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_delete_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_easy_create_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_easy_create_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_get_info_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_get_info_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_get_cond_ids_p_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_get_cond_ids_p_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_get_inquired_ids_p_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_get_inquired_ids_p_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_get_duplicated_ids_p_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_get_duplicated_ids_p_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_set_data_changed_cb_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_set_data_changed_cb_n, NEGATIVE_TC_IDX },
	{ utc_web_scrap_unset_data_changed_cb_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_is_setted_data_changed_cb_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_publish_notification_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_clear_dirty_ids_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_clear_deleted_ids_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_get_errorcode_p, POSITIVE_TC_IDX },
	{ utc_web_scrap_easy_free_p, POSITIVE_TC_IDX },
	{ NULL, 0 }
};


static void startup(void)
{
	/* start of TC */
	tet_printf("\n TC start");
	bp_scrap_adaptor_initialize();
	bp_scrap_adaptor_create(&g_testcase_id);
}


static void cleanup(void)
{
	/* end of TC */
	bp_scrap_adaptor_deinitialize();
	tet_printf("\n TC end");
}

/**
 * @brief Callback function
 */
void scrap_changed(void* user_data)
{
}


/**
 * @brief Negative test case of bp_scrap_adaptor_get_full_ids_p()
 */
static void utc_web_scrap_get_full_ids_p_n(void)
{
	int ret = bp_scrap_adaptor_get_full_ids_p(NULL, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_get_full_ids_p()
 */
static void utc_web_scrap_get_full_ids_p_p(void)
{
	int *ids = NULL;
	int ids_count = 0;
	int ret = bp_scrap_adaptor_get_full_ids_p(&ids, &ids_count);
	free(ids);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_get_full_ids_p failed");
}


/**
 * @brief Negative test case of bp_scrap_adaptor_get_full_with_deleted_ids_p()
 */
static void utc_web_scrap_get_full_with_deleted_ids_p_n(void)
{
	int ret = bp_scrap_adaptor_get_full_with_deleted_ids_p(NULL, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_get_full_with_deleted_ids_p()
 */
static void utc_web_scrap_get_full_with_deleted_ids_p_p(void)
{
	int *ids = NULL;
	int ids_count = 0;
	int ret = bp_scrap_adaptor_get_full_with_deleted_ids_p(&ids, &ids_count);
	free(ids);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_get_full_with_deleted_ids_p failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_get_dirty_ids_p()
 */
static void utc_web_scrap_get_dirty_ids_p_n(void)
{
	int ret = bp_scrap_adaptor_get_dirty_ids_p(NULL, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_get_dirty_ids_p()
 */
static void utc_web_scrap_get_dirty_ids_p_p(void)
{
	int *ids = NULL;
	int ids_count = 0;
	int ret = bp_scrap_adaptor_get_dirty_ids_p(&ids, &ids_count);
	free(ids);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_get_dirty_ids_p failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_get_deleted_ids_p()
 */
static void utc_web_scrap_get_deleted_ids_p_n(void)
{
	int ret = bp_scrap_adaptor_get_deleted_ids_p(NULL, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_get_deleted_ids_p()
 */
static void utc_web_scrap_get_deleted_ids_p_p(void)
{
	int *ids = NULL;
	int ids_count = 0;
	int ret = bp_scrap_adaptor_get_deleted_ids_p(&ids, &ids_count);
	free(ids);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_get_deleted_ids_p failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_get_is_read()
 */
static void utc_web_scrap_get_is_read_n(void)
{
	int ret = bp_scrap_adaptor_get_is_read(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_get_is_read()
 */
static void utc_web_scrap_get_is_read_p(void)
{
	int value = -1;
	int ret = bp_scrap_adaptor_get_is_read(g_testcase_id, &value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_get_is_read failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_get_is_reader()
 */
static void utc_web_scrap_get_is_reader_n(void)
{
	int ret = bp_scrap_adaptor_get_is_reader(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_get_is_reader()
 */
static void utc_web_scrap_get_is_reader_p(void)
{
	int value = -1;
	int ret = bp_scrap_adaptor_get_is_reader(g_testcase_id, &value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_get_is_reader failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_get_page_path()
 */
static void utc_web_scrap_get_page_path_n(void)
{
	int ret = bp_scrap_adaptor_get_page_path(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_get_page_path()
 */
static void utc_web_scrap_get_page_path_p(void)
{
	char *value = NULL;
	int ret = bp_scrap_adaptor_get_page_path(g_testcase_id, &value);
	free(value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_get_page_path failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_get_url()
 */
static void utc_web_scrap_get_url_n(void)
{
	int ret = bp_scrap_adaptor_get_url(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_get_url()
 */
static void utc_web_scrap_get_url_p(void)
{
	char *value = NULL;
	int ret = bp_scrap_adaptor_get_url(g_testcase_id, &value);
	free(value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_get_url failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_get_title()
 */
static void utc_web_scrap_get_title_n(void)
{
	int ret = bp_scrap_adaptor_get_title(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_get_title()
 */
static void utc_web_scrap_get_title_p(void)
{
	char *value = NULL;
	int ret = bp_scrap_adaptor_get_title(g_testcase_id, &value);
	free(value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_get_title failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_get_date_created()
 */
static void utc_web_scrap_get_date_created_n(void)
{
	int ret = bp_scrap_adaptor_get_date_created(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_get_date_created()
 */
static void utc_web_scrap_get_date_created_p(void)
{
	int value = -1;
	int ret = bp_scrap_adaptor_get_date_created(g_testcase_id, &value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_get_date_created failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_get_date_modified()
 */
static void utc_web_scrap_get_date_modified_n(void)
{
	int ret = bp_scrap_adaptor_get_date_modified(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_get_date_modified()
 */
static void utc_web_scrap_get_date_modified_p(void)
{
	int value = -1;
	int ret = bp_scrap_adaptor_get_date_modified(g_testcase_id, &value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_get_date_modified failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_get_account_name()
 */
static void utc_web_scrap_get_account_name_n(void)
{
	int ret = bp_scrap_adaptor_get_account_name(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_get_account_name()
 */
static void utc_web_scrap_get_account_name_p(void)
{
	char *value = NULL;
	int ret = bp_scrap_adaptor_get_account_name(g_testcase_id, &value);
	free(value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_get_account_name failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_get_account_type()
 */
static void utc_web_scrap_get_account_type_n(void)
{
	int ret = bp_scrap_adaptor_get_account_type(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_get_account_type()
 */
static void utc_web_scrap_get_account_type_p(void)
{
	char *value = NULL;
	int ret = bp_scrap_adaptor_get_account_type(g_testcase_id, &value);
	free(value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_get_account_type failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_get_device_name()
 */
static void utc_web_scrap_get_device_name_n(void)
{
	int ret = bp_scrap_adaptor_get_device_name(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_get_device_name()
 */
static void utc_web_scrap_get_device_name_p(void)
{
	char *value = NULL;
	int ret = bp_scrap_adaptor_get_device_name(g_testcase_id, &value);
	free(value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_get_device_name failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_get_device_id()
 */
static void utc_web_scrap_get_device_id_n(void)
{
	int ret = bp_scrap_adaptor_get_device_id(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_get_device_id()
 */
static void utc_web_scrap_get_device_id_p(void)
{
	char *value = NULL;
	int ret = bp_scrap_adaptor_get_device_id(g_testcase_id, &value);
	free(value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_get_device_id failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_get_main_content()
 */
static void utc_web_scrap_get_main_content_n(void)
{
	int ret = bp_scrap_adaptor_get_main_content(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_get_main_content()
 */
static void utc_web_scrap_get_main_content_p(void)
{
	char *value = NULL;
	int ret = bp_scrap_adaptor_get_main_content(g_testcase_id, &value);
	free(value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_get_main_content failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_get_icon()
 */
static void utc_web_scrap_get_icon_n(void)
{
	int ret = bp_scrap_adaptor_get_icon(-1, NULL, NULL, NULL, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_get_icon()
 */
static void utc_web_scrap_get_icon_p(void)
{
	int length = 0;
	int width = 0;
	int height = 0;
	unsigned char *value = NULL;
	int ret = bp_scrap_adaptor_get_icon(g_testcase_id, &width,
		&height, &value, &length);
	free(value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_get_snapshot failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_get_snapshot()
 */
static void utc_web_scrap_get_snapshot_n(void)
{
	int ret = bp_scrap_adaptor_get_snapshot(-1, NULL, NULL, NULL, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_get_snapshot()
 */
static void utc_web_scrap_get_snapshot_p(void)
{
	int length = 0;
	int width = 0;
	int height = 0;
	unsigned char *value = NULL;
	int ret = bp_scrap_adaptor_get_snapshot(g_testcase_id, &width,
		&height, &value, &length);
	free(value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_get_snapshot failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_set_dirty()
 */
static void utc_web_scrap_set_dirty_n(void)
{
	int ret = bp_scrap_adaptor_set_dirty(-1);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_set_dirty()
 */
static void utc_web_scrap_set_dirty_p(void)
{
	int ret = bp_scrap_adaptor_set_dirty(g_testcase_id);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_set_dirty failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_set_is_read()
 */
static void utc_web_scrap_set_is_read_n(void)
{
	int ret = bp_scrap_adaptor_set_is_read(-1, 0);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_set_is_read()
 */
static void utc_web_scrap_set_is_read_p(void)
{
	int ret = bp_scrap_adaptor_set_is_read(g_testcase_id, 0);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_set_is_read failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_set_is_reader()
 */
static void utc_web_scrap_set_is_reader_n(void)
{
	int ret = bp_scrap_adaptor_set_is_reader(-1, 0);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_set_is_reader()
 */
static void utc_web_scrap_set_is_reader_p(void)
{
	int ret = bp_scrap_adaptor_set_is_reader(g_testcase_id, 0);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_set_is_reader failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_set_page_path()
 */
static void utc_web_scrap_set_page_path_n(void)
{
	int ret = bp_scrap_adaptor_set_page_path(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_set_page_path()
 */
static void utc_web_scrap_set_page_path_p(void)
{
	int ret = bp_scrap_adaptor_set_page_path(g_testcase_id, "/home/capi-web-scrap/test.mhtml");
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_set_page_path failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_set_url()
 */
static void utc_web_scrap_set_url_n(void)
{
	int ret = bp_scrap_adaptor_set_url(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_set_url()
 */
static void utc_web_scrap_set_url_p(void)
{
	int ret = bp_scrap_adaptor_set_url(g_testcase_id, "URL");
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_set_url failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_set_title()
 */
static void utc_web_scrap_set_title_n(void)
{
	int ret = bp_scrap_adaptor_set_title(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_set_title()
 */
static void utc_web_scrap_set_title_p(void)
{
	int ret = bp_scrap_adaptor_set_title(g_testcase_id, "TITLE");
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_set_title failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_set_date_created()
 */
static void utc_web_scrap_set_date_created_n(void)
{
	int ret = bp_scrap_adaptor_set_date_created(-1, (int)time(NULL));
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_set_date_created()
 */
static void utc_web_scrap_set_date_created_p(void)
{
	int ret = bp_scrap_adaptor_set_date_created(g_testcase_id, (int)time(NULL));
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_set_date_created failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_set_date_modified()
 */
static void utc_web_scrap_set_date_modified_n(void)
{
	int ret = bp_scrap_adaptor_set_date_modified(-1, (int)time(NULL));
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_set_date_modified()
 */
static void utc_web_scrap_set_date_modified_p(void)
{
	int ret = bp_scrap_adaptor_set_date_modified(g_testcase_id, (int)time(NULL));
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_set_date_modified failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_set_account_name()
 */
static void utc_web_scrap_set_account_name_n(void)
{
	int ret = bp_scrap_adaptor_set_account_name(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_set_account_name()
 */
static void utc_web_scrap_set_account_name_p(void)
{
	int ret = bp_scrap_adaptor_set_account_name(g_testcase_id, "ACCOUNT_NAME");
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_set_account_name failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_set_account_type()
 */
static void utc_web_scrap_set_account_type_n(void)
{
	int ret = bp_scrap_adaptor_set_account_type(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_set_account_type()
 */
static void utc_web_scrap_set_account_type_p(void)
{
	int ret = bp_scrap_adaptor_set_account_type(g_testcase_id, "ACCOUNT_TYPE");
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_set_account_type failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_set_device_name()
 */
static void utc_web_scrap_set_device_name_n(void)
{
	int ret = bp_scrap_adaptor_set_device_name(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_set_device_name()
 */
static void utc_web_scrap_set_device_name_p(void)
{
	int ret = bp_scrap_adaptor_set_device_name(g_testcase_id, "DEVICE_NAME");
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_set_device_name failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_set_device_id()
 */
static void utc_web_scrap_set_device_id_n(void)
{
	int ret = bp_scrap_adaptor_set_device_id(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_set_device_id()
 */
static void utc_web_scrap_set_device_id_p(void)
{
	int ret = bp_scrap_adaptor_set_device_id(g_testcase_id, "DEVICE_ID");
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_set_device_id failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_set_main_content()
 */
static void utc_web_scrap_set_main_content_n(void)
{
	int ret = bp_scrap_adaptor_set_main_content(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_set_main_content()
 */
static void utc_web_scrap_set_main_content_p(void)
{
	int ret = bp_scrap_adaptor_set_main_content(g_testcase_id, "MAIN_CONTENT");
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_set_main_content failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_set_icon()
 */
static void utc_web_scrap_set_icon_n(void)
{
	int ret = bp_scrap_adaptor_set_icon(-1, 0, 0, NULL, 0);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_set_icon()
 */
static void utc_web_scrap_set_icon_p(void)
{
	unsigned char *value = (unsigned char *)"icon test";
	int ret = bp_scrap_adaptor_set_icon(g_testcase_id, 1, 1, (const unsigned char *)value, 9);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_set_icon failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_set_snapshot()
 */
static void utc_web_scrap_set_snapshot_n(void)
{
	int ret = bp_scrap_adaptor_set_snapshot(-1, 0, 0, NULL, 0);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_set_snapshot()
 */
static void utc_web_scrap_set_snapshot_p(void)
{
	unsigned char *value = (unsigned char *)"icon test";
	int ret = bp_scrap_adaptor_set_snapshot(g_testcase_id, 1, 1, (const unsigned char *)value, 9);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_set_snapshot failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_create()
 */
static void utc_web_scrap_create_n(void)
{
	int ret = bp_scrap_adaptor_create(NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_create()
 */
static void utc_web_scrap_create_p(void)
{
	int id = -1;
	int ret = bp_scrap_adaptor_create(&id);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_create failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_delete()
 */
static void utc_web_scrap_delete_n(void)
{
	int ret = bp_scrap_adaptor_delete(-1);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_delete()
 */
static void utc_web_scrap_delete_p(void)
{
	int id = -1;
	int ret = bp_scrap_adaptor_create(&id);
	if (ret == 0)
		ret = bp_scrap_adaptor_delete(id);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_create failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_easy_create()
 */
static void utc_web_scrap_easy_create_n(void)
{
	int ret = bp_scrap_adaptor_easy_create(NULL, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_easy_create()
 */
static void utc_web_scrap_easy_create_p(void)
{
	bp_scrap_info_fmt info;
	memset(&info, 0x00, sizeof(bp_scrap_info_fmt));
	info.url = "URL";
	info.title = "TITLE";
	info.date_modified = -1;
	int id = -1;
	int ret = bp_scrap_adaptor_easy_create(&id, &info);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_easy_create failed");
}


/**
 * @brief Negative test case of bp_scrap_adaptor_get_info()
 */
static void utc_web_scrap_get_info_n(void)
{
	int ret = bp_scrap_adaptor_get_info(-1, 0, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_get_info()
 */
static void utc_web_scrap_get_info_p(void)
{
	unsigned int b_offset = (BP_SCRAP_O_DATE_CREATED | BP_SCRAP_O_DATE_MODIFIED | BP_SCRAP_O_URL | BP_SCRAP_O_TITLE);
	bp_scrap_info_fmt info;
	memset(&info, 0x00, sizeof(bp_scrap_info_fmt));
	int ret = bp_scrap_adaptor_get_info(g_testcase_id, b_offset, &info);
	bp_scrap_adaptor_easy_free(&info);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_get_info failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_get_cond_ids_p()
 */
static void utc_web_scrap_get_cond_ids_p_n(void)
{
	int ret = bp_scrap_adaptor_get_cond_ids_p(NULL, NULL, NULL,
		0, NULL, 0);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_get_cond_ids_p()
 */
static void utc_web_scrap_get_cond_ids_p_p(void)
{
	bp_scrap_rows_cond_fmt conds;
	memset(&conds, 0x00, sizeof(bp_scrap_rows_cond_fmt));
	conds.limit = -1;
	conds.offset = 0;
	conds.order_offset = BP_SCRAP_O_DATE_CREATED;
	conds.ordering = 0;
	conds.period_offset = BP_SCRAP_O_DATE_CREATED;
	conds.period_type = BP_SCRAP_DATE_ALL;
	int *ids = NULL;
	int ids_count = 0;
	int ret = bp_scrap_adaptor_get_cond_ids_p(&ids, &ids_count,
		&conds, BP_SCRAP_O_TITLE, "%T%", 1);
	free(ids);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_get_cond_ids_p failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_get_inquired_ids_p()
 */
static void utc_web_scrap_get_inquired_ids_p_n(void)
{
	int ret = bp_scrap_adaptor_get_inquired_ids_p(NULL, NULL, 0, 0,
		0, 0, NULL, 0);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_get_inquired_ids_p()
 */
static void utc_web_scrap_get_inquired_ids_p_p(void)
{
	int *ids = NULL;
	int ids_count = 0;
	int ret = bp_scrap_adaptor_get_inquired_ids_p(&ids, &ids_count, -1, 0,
				BP_SCRAP_O_DATE_CREATED, 0/*ASC*/, "%T%", 1);
	free(ids);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_get_cond_ids_p failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_get_duplicated_ids_p()
 */
static void utc_web_scrap_get_duplicated_ids_p_n(void)
{
	int ret = bp_scrap_adaptor_get_duplicated_ids_p(NULL, NULL, 0, 0, 0,
		0, 0, NULL, 0);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_get_duplicated_ids_p()
 */
static void utc_web_scrap_get_duplicated_ids_p_p(void)
{
	int *ids = NULL;
	int ids_count = 0;
	int ret = bp_scrap_adaptor_get_duplicated_ids_p(&ids, &ids_count, -1, 0,
				BP_SCRAP_O_DATE_CREATED, 0/*ASC*/, BP_SCRAP_O_TITLE, "%T%", 1);
	free(ids);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_get_duplicated_ids_p failed");
}









/**
 * @brief Negative test case of bp_scrap_adaptor_set_data_changed_cb()
 */
static void utc_web_scrap_set_data_changed_cb_n(void)
{
	int ret = bp_scrap_adaptor_set_data_changed_cb(NULL, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_set_data_changed_cb()
 */
static void utc_web_scrap_set_data_changed_cb_p(void)
{
	int ret = bp_scrap_adaptor_set_data_changed_cb(scrap_changed, NULL);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_set_data_changed_cb failed");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_get_errorcode()
 */
static void utc_web_scrap_unset_data_changed_cb_p(void)
{
	int ret = bp_scrap_adaptor_unset_data_changed_cb(scrap_changed);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_unset_data_changed_cb failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_is_setted_data_changed_cb()
 */
static void utc_web_scrap_is_setted_data_changed_cb_p(void)
{
	int ret = bp_scrap_adaptor_set_data_changed_cb(scrap_changed, NULL);
	if (ret == 0)
		ret = bp_scrap_adaptor_is_setted_data_changed_cb();
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_is_setted_data_changed_cb failed");
}

/**
 * @brief Negative test case of bp_scrap_adaptor_publish_notification()
 */
static void utc_web_scrap_publish_notification_p(void)
{
	int ret = bp_scrap_adaptor_publish_notification();
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_publish_notification failed");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_clear_dirty_ids()
 */
static void utc_web_scrap_clear_dirty_ids_p(void)
{
	int ret = bp_scrap_adaptor_clear_dirty_ids();
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_clear_dirty_ids failed");
}

/**
 * @brief Positive test case of bp_scrap_adaptor_clear_deleted_ids()
 */
static void utc_web_scrap_clear_deleted_ids_p(void)
{
	int ret = bp_scrap_adaptor_clear_deleted_ids();
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_scrap_adaptor_clear_deleted_ids failed");
}


/**
 * @brief Positive test case of bp_scrap_adaptor_get_errorcode()
 */
static void utc_web_scrap_get_errorcode_p(void)
{
	bp_scrap_adaptor_get_errorcode();
	dts_pass(__FUNCTION__);
}

/**
 * @brief Positive test case of bp_scrap_adaptor_easy_free()
 */
static void utc_web_scrap_easy_free_p(void)
{
	bp_scrap_adaptor_easy_free(NULL);
	dts_pass(__FUNCTION__);
}

