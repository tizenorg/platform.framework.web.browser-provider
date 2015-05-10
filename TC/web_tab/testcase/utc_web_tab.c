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
#include <errno.h>

#include <tet_api.h>
#include <web_tab.h>

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

static int g_testcase_id = -1;

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_web_tab_get_full_ids_p_n(void);
static void utc_web_tab_get_full_ids_p_p(void);
static void utc_web_tab_get_full_with_deleted_ids_p_n(void);
static void utc_web_tab_get_full_with_deleted_ids_p_p(void);
static void utc_web_tab_get_dirty_ids_p_n(void);
static void utc_web_tab_get_dirty_ids_p_p(void);
static void utc_web_tab_get_deleted_ids_p_n(void);
static void utc_web_tab_get_deleted_ids_p_p(void);
static void utc_web_tab_get_index_n(void);
static void utc_web_tab_get_index_p(void);
static void utc_web_tab_get_activated_n(void);
static void utc_web_tab_get_activated_p(void);
static void utc_web_tab_get_incognito_p(void);
static void utc_web_tab_get_incognito_n(void);
static void utc_web_tab_get_url_n(void);
static void utc_web_tab_get_url_p(void);
static void utc_web_tab_get_title_n(void);
static void utc_web_tab_get_title_p(void);
static void utc_web_tab_get_date_created_n(void);
static void utc_web_tab_get_date_created_p(void);
static void utc_web_tab_get_date_modified_n(void);
static void utc_web_tab_get_date_modified_p(void);
static void utc_web_tab_get_account_name_n(void);
static void utc_web_tab_get_account_name_p(void);
static void utc_web_tab_get_account_type_n(void);
static void utc_web_tab_get_account_type_p(void);
static void utc_web_tab_get_device_name_n(void);
static void utc_web_tab_get_device_name_p(void);
static void utc_web_tab_get_device_id_n(void);
static void utc_web_tab_get_device_id_p(void);
static void utc_web_tab_get_usage_n(void);
static void utc_web_tab_get_usage_p(void);
static void utc_web_tab_get_sync_n(void);
static void utc_web_tab_get_sync_p(void);
static void utc_web_tab_get_icon_n(void);
static void utc_web_tab_get_icon_p(void);
static void utc_web_tab_get_icon_png_p(void);
static void utc_web_tab_get_icon_png_n(void);
static void utc_web_tab_get_snapshot_n(void);
static void utc_web_tab_get_snapshot_p(void);
static void utc_web_tab_set_dirty_n(void);
static void utc_web_tab_set_dirty_p(void);
static void utc_web_tab_set_deleted_n(void);
static void utc_web_tab_set_deleted_p(void);
static void utc_web_tab_set_index_n(void);
static void utc_web_tab_set_index_p(void);
static void utc_web_tab_set_activated_n(void);
static void utc_web_tab_set_activated_p(void);
static void utc_web_tab_set_incognito_p(void);
static void utc_web_tab_set_incognito_n(void);
static void utc_web_tab_set_url_n(void);
static void utc_web_tab_set_url_p(void);
static void utc_web_tab_set_title_n(void);
static void utc_web_tab_set_title_p(void);
static void utc_web_tab_set_date_created_n(void);
static void utc_web_tab_set_date_created_p(void);
static void utc_web_tab_set_date_modified_n(void);
static void utc_web_tab_set_date_modified_p(void);
static void utc_web_tab_set_account_name_n(void);
static void utc_web_tab_set_account_name_p(void);
static void utc_web_tab_set_account_type_n(void);
static void utc_web_tab_set_account_type_p(void);
static void utc_web_tab_set_device_name_n(void);
static void utc_web_tab_set_device_name_p(void);
static void utc_web_tab_set_device_id_n(void);
static void utc_web_tab_set_device_id_p(void);
static void utc_web_tab_set_usage_n(void);
static void utc_web_tab_set_usage_p(void);
static void utc_web_tab_set_sync_n(void);
static void utc_web_tab_set_sync_p(void);
static void utc_web_tab_set_icon_n(void);
static void utc_web_tab_set_icon_p(void);
static void utc_web_tab_set_icon_png_n(void);
static void utc_web_tab_set_icon_png_p(void);
static void utc_web_tab_set_snapshot_n(void);
static void utc_web_tab_set_snapshot_p(void);
static void utc_web_tab_create_n(void);
static void utc_web_tab_create_p(void);
static void utc_web_tab_delete_n(void);
static void utc_web_tab_delete_p(void);
static void utc_web_tab_easy_create_n(void);
static void utc_web_tab_easy_create_p(void);
static void utc_web_tab_get_easy_all_n(void);
static void utc_web_tab_get_easy_all_p(void);
static void utc_web_tab_get_info_n(void);
static void utc_web_tab_get_info_p(void);
static void utc_web_tab_activate_n(void);
static void utc_web_tab_activate_p(void);
static void utc_web_tab_get_duplicated_ids_p_n(void);
static void utc_web_tab_get_duplicated_ids_p_p(void);
static void utc_web_tab_set_data_changed_cb_p(void);
static void utc_web_tab_set_data_changed_cb_n(void);
static void utc_web_tab_unset_data_changed_cb_n(void);
static void utc_web_tab_unset_data_changed_cb_p(void);
static void utc_web_tab_is_setted_data_changed_cb_n(void);
static void utc_web_tab_is_setted_data_changed_cb_p(void);
static void utc_web_tab_publish_notification_p(void);
static void utc_web_tab_clear_dirty_ids_p(void);
static void utc_web_tab_clear_deleted_ids_p(void);
static void utc_web_tab_get_errorcode_p(void);
static void utc_web_tab_easy_free_p(void);


struct tet_testlist tet_testlist[] = {
	{ utc_web_tab_set_index_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_set_index_p, POSITIVE_TC_IDX },
	{ utc_web_tab_set_activated_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_set_activated_p, POSITIVE_TC_IDX },
	{ utc_web_tab_set_incognito_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_set_incognito_p, POSITIVE_TC_IDX },
	{ utc_web_tab_set_url_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_set_url_p, POSITIVE_TC_IDX },
	{ utc_web_tab_set_title_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_set_title_p, POSITIVE_TC_IDX },
	{ utc_web_tab_set_date_created_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_set_date_created_p, POSITIVE_TC_IDX },
	{ utc_web_tab_set_date_modified_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_set_date_modified_p, POSITIVE_TC_IDX },
	{ utc_web_tab_set_account_name_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_set_account_name_p, POSITIVE_TC_IDX },
	{ utc_web_tab_set_account_type_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_set_account_type_p, POSITIVE_TC_IDX },
	{ utc_web_tab_set_device_name_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_set_device_name_p, POSITIVE_TC_IDX },
	{ utc_web_tab_set_device_id_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_set_device_id_p, POSITIVE_TC_IDX },
	{ utc_web_tab_set_usage_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_set_usage_p, POSITIVE_TC_IDX },
	{ utc_web_tab_set_sync_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_set_sync_p, POSITIVE_TC_IDX },
	{ utc_web_tab_set_icon_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_set_icon_p, POSITIVE_TC_IDX },
	{ utc_web_tab_set_icon_png_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_set_icon_png_p, POSITIVE_TC_IDX },
	{ utc_web_tab_set_snapshot_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_set_snapshot_p, POSITIVE_TC_IDX },
	{ utc_web_tab_set_dirty_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_set_dirty_p, POSITIVE_TC_IDX },
	{ utc_web_tab_set_deleted_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_set_deleted_p, POSITIVE_TC_IDX },
	{ utc_web_tab_get_full_ids_p_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_get_full_ids_p_p, POSITIVE_TC_IDX },
	{ utc_web_tab_get_full_with_deleted_ids_p_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_get_full_with_deleted_ids_p_p, POSITIVE_TC_IDX },
	{ utc_web_tab_get_dirty_ids_p_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_get_dirty_ids_p_p, POSITIVE_TC_IDX },
	{ utc_web_tab_get_deleted_ids_p_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_get_deleted_ids_p_p, POSITIVE_TC_IDX },
	{ utc_web_tab_get_index_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_get_index_p, POSITIVE_TC_IDX },
	{ utc_web_tab_get_activated_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_get_activated_p, POSITIVE_TC_IDX },
	{ utc_web_tab_get_incognito_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_get_incognito_p, POSITIVE_TC_IDX },
	{ utc_web_tab_get_url_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_get_url_p, POSITIVE_TC_IDX },
	{ utc_web_tab_get_title_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_get_title_p, POSITIVE_TC_IDX },
	{ utc_web_tab_get_date_created_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_get_date_created_p, POSITIVE_TC_IDX },
	{ utc_web_tab_get_date_modified_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_get_date_modified_p, POSITIVE_TC_IDX },
	{ utc_web_tab_get_account_name_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_get_account_name_p, POSITIVE_TC_IDX },
	{ utc_web_tab_get_account_type_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_get_account_type_p, POSITIVE_TC_IDX },
	{ utc_web_tab_get_device_name_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_get_device_name_p, POSITIVE_TC_IDX },
	{ utc_web_tab_get_device_id_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_get_device_id_p, POSITIVE_TC_IDX },
	{ utc_web_tab_get_usage_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_get_usage_p, POSITIVE_TC_IDX },
	{ utc_web_tab_get_sync_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_get_sync_p, POSITIVE_TC_IDX },
	{ utc_web_tab_get_icon_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_get_icon_p, POSITIVE_TC_IDX },
	{ utc_web_tab_get_icon_png_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_get_icon_png_p, POSITIVE_TC_IDX },
	{ utc_web_tab_get_snapshot_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_get_snapshot_p, POSITIVE_TC_IDX },
	{ utc_web_tab_create_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_create_p, POSITIVE_TC_IDX },
	{ utc_web_tab_delete_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_delete_p, POSITIVE_TC_IDX },
	{ utc_web_tab_activate_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_activate_p, POSITIVE_TC_IDX },
	{ utc_web_tab_easy_create_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_easy_create_p, POSITIVE_TC_IDX },
	{ utc_web_tab_get_info_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_get_info_p, POSITIVE_TC_IDX },
	{ utc_web_tab_get_easy_all_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_get_easy_all_p, POSITIVE_TC_IDX },
	{ utc_web_tab_get_duplicated_ids_p_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_get_duplicated_ids_p_p, POSITIVE_TC_IDX },
	{ utc_web_tab_set_data_changed_cb_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_set_data_changed_cb_p, POSITIVE_TC_IDX },
	{ utc_web_tab_is_setted_data_changed_cb_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_is_setted_data_changed_cb_p, POSITIVE_TC_IDX },
	{ utc_web_tab_unset_data_changed_cb_n, NEGATIVE_TC_IDX },
	{ utc_web_tab_unset_data_changed_cb_p, POSITIVE_TC_IDX },
	{ utc_web_tab_publish_notification_p, POSITIVE_TC_IDX },
	{ utc_web_tab_clear_dirty_ids_p, POSITIVE_TC_IDX },
	{ utc_web_tab_clear_deleted_ids_p, POSITIVE_TC_IDX },
	{ utc_web_tab_get_errorcode_p, POSITIVE_TC_IDX },
	{ utc_web_tab_easy_free_p, POSITIVE_TC_IDX },
	{ NULL, 0 }
};


static void startup(void)
{
	/* start of TC */
	tet_printf("\n TC start");
	bp_tab_adaptor_initialize();
	bp_tab_adaptor_create(&g_testcase_id);
}


static void cleanup(void)
{
	/* end of TC */
	bp_tab_adaptor_deinitialize();
	tet_printf("\n TC end");
}

/**
 * @brief Callback function
 */
void tab_changed(void* user_data)
{
}


/**
 * @brief Negative test case of bp_tab_adaptor_get_full_ids_p()
 */
static void utc_web_tab_get_full_ids_p_n(void)
{
	int ret = bp_tab_adaptor_get_full_ids_p(NULL, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_get_full_ids_p()
 */
static void utc_web_tab_get_full_ids_p_p(void)
{
	int *ids = NULL;
	int ids_count = 0;
	int ret = bp_tab_adaptor_get_full_ids_p(&ids, &ids_count);
	free(ids);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_get_full_ids_p failed");
}


/**
 * @brief Negative test case of bp_tab_adaptor_get_full_with_deleted_ids_p()
 */
static void utc_web_tab_get_full_with_deleted_ids_p_n(void)
{
	int ret = bp_tab_adaptor_get_full_with_deleted_ids_p(NULL, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_get_full_with_deleted_ids_p()
 */
static void utc_web_tab_get_full_with_deleted_ids_p_p(void)
{
	int *ids = NULL;
	int ids_count = 0;
	int ret = bp_tab_adaptor_get_full_with_deleted_ids_p(&ids, &ids_count);
	free(ids);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_get_full_with_deleted_ids_p failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_get_dirty_ids_p()
 */
static void utc_web_tab_get_dirty_ids_p_n(void)
{
	int ret = bp_tab_adaptor_get_dirty_ids_p(NULL, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_get_dirty_ids_p()
 */
static void utc_web_tab_get_dirty_ids_p_p(void)
{
	int *ids = NULL;
	int ids_count = 0;
	int ret = bp_tab_adaptor_get_dirty_ids_p(&ids, &ids_count);
	free(ids);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_get_dirty_ids_p failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_get_deleted_ids_p()
 */
static void utc_web_tab_get_deleted_ids_p_n(void)
{
	int ret = bp_tab_adaptor_get_deleted_ids_p(NULL, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_get_deleted_ids_p()
 */
static void utc_web_tab_get_deleted_ids_p_p(void)
{
	int *ids = NULL;
	int ids_count = 0;
	int ret = bp_tab_adaptor_get_deleted_ids_p(&ids, &ids_count);
	free(ids);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_get_deleted_ids_p failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_get_index()
 */
static void utc_web_tab_get_index_n(void)
{
	int ret = bp_tab_adaptor_get_index(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_get_index()
 */
static void utc_web_tab_get_index_p(void)
{
	int value = -1;
	int ret = bp_tab_adaptor_get_index(g_testcase_id, &value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_get_index failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_get_activated()
 */
static void utc_web_tab_get_activated_n(void)
{
	int ret = bp_tab_adaptor_get_activated(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_get_activated()
 */
static void utc_web_tab_get_activated_p(void)
{
	int value = -1;
	int ret = bp_tab_adaptor_get_activated(g_testcase_id, &value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_get_index failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_get_incognito()
 */
static void utc_web_tab_get_incognito_n(void)
{
	int ret = bp_tab_adaptor_get_incognito(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_get_incognito()
 */
static void utc_web_tab_get_incognito_p(void)
{
	int value = -1;
	int ret = bp_tab_adaptor_get_incognito(g_testcase_id, &value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_get_incognito failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_get_url()
 */
static void utc_web_tab_get_url_n(void)
{
	int ret = bp_tab_adaptor_get_url(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_get_url()
 */
static void utc_web_tab_get_url_p(void)
{
	char *value = NULL;
	int ret = bp_tab_adaptor_get_url(g_testcase_id, &value);
	free(value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_get_url failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_get_title()
 */
static void utc_web_tab_get_title_n(void)
{
	int ret = bp_tab_adaptor_get_title(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_get_title()
 */
static void utc_web_tab_get_title_p(void)
{
	char *value = NULL;
	int ret = bp_tab_adaptor_get_title(g_testcase_id, &value);
	free(value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_get_title failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_get_date_created()
 */
static void utc_web_tab_get_date_created_n(void)
{
	int ret = bp_tab_adaptor_get_date_created(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_get_date_created()
 */
static void utc_web_tab_get_date_created_p(void)
{
	int value = -1;
	int ret = bp_tab_adaptor_get_date_created(g_testcase_id, &value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_get_date_created failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_get_date_modified()
 */
static void utc_web_tab_get_date_modified_n(void)
{
	int ret = bp_tab_adaptor_get_date_modified(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_get_date_modified()
 */
static void utc_web_tab_get_date_modified_p(void)
{
	int value = -1;
	int ret = bp_tab_adaptor_get_date_modified(g_testcase_id, &value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_get_date_modified failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_get_account_name()
 */
static void utc_web_tab_get_account_name_n(void)
{
	int ret = bp_tab_adaptor_get_account_name(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_get_account_name()
 */
static void utc_web_tab_get_account_name_p(void)
{
	char *value = NULL;
	int ret = bp_tab_adaptor_get_account_name(g_testcase_id, &value);
	free(value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_get_account_name failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_get_account_type()
 */
static void utc_web_tab_get_account_type_n(void)
{
	int ret = bp_tab_adaptor_get_account_type(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_get_account_type()
 */
static void utc_web_tab_get_account_type_p(void)
{
	char *value = NULL;
	int ret = bp_tab_adaptor_get_account_type(g_testcase_id, &value);
	free(value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_get_account_type failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_get_device_name()
 */
static void utc_web_tab_get_device_name_n(void)
{
	int ret = bp_tab_adaptor_get_device_name(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_get_device_name()
 */
static void utc_web_tab_get_device_name_p(void)
{
	char *value = NULL;
	int ret = bp_tab_adaptor_get_device_name(g_testcase_id, &value);
	free(value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_get_device_name failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_get_device_id()
 */
static void utc_web_tab_get_device_id_n(void)
{
	int ret = bp_tab_adaptor_get_device_id(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_get_device_id()
 */
static void utc_web_tab_get_device_id_p(void)
{
	char *value = NULL;
	int ret = bp_tab_adaptor_get_device_id(g_testcase_id, &value);
	free(value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_get_device_id failed");
}


/**
 * @brief Negative test case of bp_tab_adaptor_get_usage()
 */
static void utc_web_tab_get_usage_n(void)
{
	int ret = bp_tab_adaptor_get_usage(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_get_usage()
 */
static void utc_web_tab_get_usage_p(void)
{
	char *value = NULL;
	int ret = bp_tab_adaptor_get_usage(g_testcase_id, &value);
	free(value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_get_usage failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_get_sync()
 */
static void utc_web_tab_get_sync_n(void)
{
	int ret = bp_tab_adaptor_get_sync(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_get_sync()
 */
static void utc_web_tab_get_sync_p(void)
{
	char *value = NULL;
	int ret = bp_tab_adaptor_get_sync(g_testcase_id, &value);
	free(value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_get_sync failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_get_icon()
 */
static void utc_web_tab_get_icon_n(void)
{
	unsigned char *value = NULL;
	int ret = bp_tab_adaptor_get_icon(-1, NULL, NULL, &value, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_get_icon()
 */
static void utc_web_tab_get_icon_p(void)
{
	int length = 0;
	int width = 0;
	int height = 0;
	unsigned char *value = NULL;
	int ret = bp_tab_adaptor_get_icon(g_testcase_id, &width,
		&height, &value, &length);
	free(value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_get_snapshot failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_get_icon_png()
 */
static void utc_web_tab_get_icon_png_n(void)
{
	unsigned char *value = NULL;
	int ret = bp_tab_adaptor_get_icon_png(-1, &value, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_get_icon_png()
 */
static void utc_web_tab_get_icon_png_p(void)
{
	int length = 0;
	unsigned char *value = NULL;
	int ret = bp_tab_adaptor_get_icon_png(g_testcase_id, &value,
		&length);
	free(value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "utc_web_tab_get_icon_png_p failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_get_snapshot()
 */
static void utc_web_tab_get_snapshot_n(void)
{
	unsigned char *value = NULL;
	int ret = bp_tab_adaptor_get_snapshot(-1, NULL, NULL, &value, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_get_snapshot()
 */
static void utc_web_tab_get_snapshot_p(void)
{
	int length = 0;
	int width = 0;
	int height = 0;
	unsigned char *value = NULL;
	int ret = bp_tab_adaptor_get_snapshot(g_testcase_id, &width,
		&height, &value, &length);
	free(value);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_get_snapshot failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_set_dirty()
 */
static void utc_web_tab_set_dirty_n(void)
{
	int ret = bp_tab_adaptor_set_dirty(-1);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_set_dirty()
 */
static void utc_web_tab_set_dirty_p(void)
{
	int ret = bp_tab_adaptor_set_dirty(g_testcase_id);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_set_dirty failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_set_deleted()
 */
static void utc_web_tab_set_deleted_n(void)
{
	int ret = bp_tab_adaptor_set_deleted(-1);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_set_deleted()
 */
static void utc_web_tab_set_deleted_p(void)
{
	int ret = bp_tab_adaptor_set_deleted(g_testcase_id);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_set_deleted failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_set_index()
 */
static void utc_web_tab_set_index_n(void)
{
	int ret = bp_tab_adaptor_set_index(-1, 0);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_set_index()
 */
static void utc_web_tab_set_index_p(void)
{
	int ret = bp_tab_adaptor_set_index(g_testcase_id, 0);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_set_index failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_set_activated()
 */
static void utc_web_tab_set_activated_n(void)
{
	int ret = bp_tab_adaptor_set_activated(-1, 0);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_set_activated()
 */
static void utc_web_tab_set_activated_p(void)
{
	int ret = bp_tab_adaptor_set_activated(g_testcase_id, 0);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_set_activated failed");
}
/**
 * @brief Negative test case of bp_tab_adaptor_set_incognito()
 */
static void utc_web_tab_set_incognito_n(void)
{
	int ret = bp_tab_adaptor_set_incognito(-1, 0);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_set_incognito()
 */
static void utc_web_tab_set_incognito_p(void)
{
	int ret = bp_tab_adaptor_set_incognito(g_testcase_id, 0);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_set_incognito failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_set_usage()
 */
static void utc_web_tab_set_usage_n(void)
{
	int ret = bp_tab_adaptor_set_usage(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_set_usage()
 */
static void utc_web_tab_set_usage_p(void)
{
	int ret = bp_tab_adaptor_set_usage(g_testcase_id, "URL");
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_set_usage failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_set_sync()
 */
static void utc_web_tab_set_sync_n(void)
{
	int ret = bp_tab_adaptor_set_sync(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_set_sync()
 */
static void utc_web_tab_set_sync_p(void)
{
	int ret = bp_tab_adaptor_set_sync(g_testcase_id, "URL");
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_set_sync failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_set_url()
 */
static void utc_web_tab_set_url_n(void)
{
	int ret = bp_tab_adaptor_set_url(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_set_url()
 */
static void utc_web_tab_set_url_p(void)
{
	int ret = bp_tab_adaptor_set_url(g_testcase_id, "URL");
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_set_url failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_set_title()
 */
static void utc_web_tab_set_title_n(void)
{
	int ret = bp_tab_adaptor_set_title(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_set_title()
 */
static void utc_web_tab_set_title_p(void)
{
	int ret = bp_tab_adaptor_set_title(g_testcase_id, "TITLE");
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_set_title failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_set_date_created()
 */
static void utc_web_tab_set_date_created_n(void)
{
	int ret = bp_tab_adaptor_set_date_created(-1, (int)time(NULL));
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_set_date_created()
 */
static void utc_web_tab_set_date_created_p(void)
{
	int ret = bp_tab_adaptor_set_date_created(g_testcase_id, (int)time(NULL));
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_set_date_created failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_set_date_modified()
 */
static void utc_web_tab_set_date_modified_n(void)
{
	int ret = bp_tab_adaptor_set_date_modified(-1, (int)time(NULL));
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_set_date_modified()
 */
static void utc_web_tab_set_date_modified_p(void)
{
	int ret = bp_tab_adaptor_set_date_modified(g_testcase_id, (int)time(NULL));
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_set_date_modified failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_set_account_name()
 */
static void utc_web_tab_set_account_name_n(void)
{
	int ret = bp_tab_adaptor_set_account_name(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_set_account_name()
 */
static void utc_web_tab_set_account_name_p(void)
{
	int ret = bp_tab_adaptor_set_account_name(g_testcase_id, "ACCOUNT_NAME");
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_set_account_name failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_set_account_type()
 */
static void utc_web_tab_set_account_type_n(void)
{
	int ret = bp_tab_adaptor_set_account_type(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_set_account_type()
 */
static void utc_web_tab_set_account_type_p(void)
{
	int ret = bp_tab_adaptor_set_account_type(g_testcase_id, "ACCOUNT_TYPE");
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_set_account_type failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_set_device_name()
 */
static void utc_web_tab_set_device_name_n(void)
{
	int ret = bp_tab_adaptor_set_device_name(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_set_device_name()
 */
static void utc_web_tab_set_device_name_p(void)
{
	int ret = bp_tab_adaptor_set_device_name(g_testcase_id, "DEVICE_NAME");
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_set_device_name failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_set_device_id()
 */
static void utc_web_tab_set_device_id_n(void)
{
	int ret = bp_tab_adaptor_set_device_id(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_set_device_id()
 */
static void utc_web_tab_set_device_id_p(void)
{
	int ret = bp_tab_adaptor_set_device_id(g_testcase_id, "DEVICE_ID");
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_set_device_id failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_set_icon()
 */
static void utc_web_tab_set_icon_n(void)
{
	int ret = bp_tab_adaptor_set_icon(-1, 0, 0, NULL, 0);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_set_icon()
 */
static void utc_web_tab_set_icon_p(void)
{
	unsigned char *value = (unsigned char *)"icon test";
	int ret = bp_tab_adaptor_set_icon(g_testcase_id, 1, 1, (const unsigned char *)value, 9);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_set_icon failed");
}


/**
 * @brief Negative test case of bp_tab_adaptor_set_icon_png()
 */
static void utc_web_tab_set_icon_png_n(void)
{
	int ret = bp_tab_adaptor_set_icon_png(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_set_icon_png()
 */
static void utc_web_tab_set_icon_png_p(void)
{
	int ret = -1;
	FILE *fp = fopen("/home/capi-web-tab/test.png", "rb");
	if (fp != NULL) {
		int length = 0;
		unsigned char buffer[3145728];
		length = fread(buffer, 1, 3145728, fp);
		if (length > 0) {
			ret = bp_tab_adaptor_set_icon_png(g_testcase_id, buffer);
			if (ret == 0)
				dts_pass(__FUNCTION__);
			else
				dts_fail(__FUNCTION__, "bp_tab_adaptor_set_icon_png failed");
		} else {
			dts_fail(__FUNCTION__, "read %d bytes from png", length);
		}
		fclose(fp);
	} else {
		dts_fail(__FUNCTION__, "can not open png:%s", strerror(errno));
	}
}

/**
 * @brief Negative test case of bp_tab_adaptor_set_snapshot()
 */
static void utc_web_tab_set_snapshot_n(void)
{
	int ret = bp_tab_adaptor_set_snapshot(-1, 0, 0, NULL, 0);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_set_snapshot()
 */
static void utc_web_tab_set_snapshot_p(void)
{
	unsigned char *value = (unsigned char *)"icon test";
	int ret = bp_tab_adaptor_set_snapshot(g_testcase_id, 1, 1, (const unsigned char *)value, 9);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_set_snapshot failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_create()
 */
static void utc_web_tab_create_n(void)
{
	int ret = bp_tab_adaptor_create(NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_create()
 */
static void utc_web_tab_create_p(void)
{
	int id = -1;
	int ret = bp_tab_adaptor_create(&id);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_create failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_delete()
 */
static void utc_web_tab_delete_n(void)
{
	int ret = bp_tab_adaptor_delete(-1);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_delete()
 */
static void utc_web_tab_delete_p(void)
{
	int id = -1;
	int ret = bp_tab_adaptor_create(&id);
	if (ret == 0)
		ret = bp_tab_adaptor_delete(id);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_create failed");
}


/**
 * @brief Negative test case of bp_tab_adaptor_activate()
 */
static void utc_web_tab_activate_n(void)
{
	int ret = bp_tab_adaptor_activate(-1);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_activate()
 */
static void utc_web_tab_activate_p(void)
{
	int ret = bp_tab_adaptor_activate(g_testcase_id);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_activate failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_easy_create()
 */
static void utc_web_tab_easy_create_n(void)
{
	int ret = bp_tab_adaptor_easy_create(NULL, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_easy_create()
 */
static void utc_web_tab_easy_create_p(void)
{
	bp_tab_info_fmt info;
	memset(&info, 0x00, sizeof(bp_tab_info_fmt));
	info.url = "URL";
	info.title = "TITLE";
	info.date_modified = -1;
	int id = -1;
	int ret = bp_tab_adaptor_easy_create(&id, &info);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_easy_create failed");
}


/**
 * @brief Negative test case of bp_tab_adaptor_get_info()
 */
static void utc_web_tab_get_info_n(void)
{
	int ret = bp_tab_adaptor_get_info(-1, 0, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_get_info()
 */
static void utc_web_tab_get_info_p(void)
{
	unsigned int b_offset = (BP_TAB_O_DATE_CREATED | BP_TAB_O_DATE_MODIFIED | BP_TAB_O_URL | BP_TAB_O_TITLE);
	bp_tab_info_fmt info;
	memset(&info, 0x00, sizeof(bp_tab_info_fmt));
	int ret = bp_tab_adaptor_get_info(g_testcase_id, b_offset, &info);
	bp_tab_adaptor_easy_free(&info);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_get_info failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_get_easy_all()
 */
static void utc_web_tab_get_easy_all_n(void)
{
	int ret = bp_tab_adaptor_get_easy_all(-1, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_get_easy_all()
 */
static void utc_web_tab_get_easy_all_p(void)
{
	bp_tab_info_fmt info;
	memset(&info, 0x00, sizeof(bp_tab_info_fmt));
	int ret = bp_tab_adaptor_get_easy_all(g_testcase_id, &info);
	bp_tab_adaptor_easy_free(&info);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_get_easy_all failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_get_duplicated_ids_p()
 */
static void utc_web_tab_get_duplicated_ids_p_n(void)
{
	int ret = bp_tab_adaptor_get_duplicated_ids_p(NULL, NULL, 0, 0, 0,
		0, 0, NULL, 0);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_get_duplicated_ids_p()
 */
static void utc_web_tab_get_duplicated_ids_p_p(void)
{
	int *ids = NULL;
	int ids_count = 0;
	int ret = bp_tab_adaptor_get_duplicated_ids_p(&ids, &ids_count, -1, 0,
				BP_TAB_O_DATE_CREATED, 0/*ASC*/, BP_TAB_O_TITLE, "%T%", 1);
	free(ids);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_get_duplicated_ids_p failed");
}









/**
 * @brief Negative test case of bp_tab_adaptor_set_data_changed_cb()
 */
static void utc_web_tab_set_data_changed_cb_n(void)
{
	int ret = bp_tab_adaptor_set_data_changed_cb(NULL, NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_tab_adaptor_set_data_changed_cb()
 */
static void utc_web_tab_set_data_changed_cb_p(void)
{
	int ret = bp_tab_adaptor_set_data_changed_cb(tab_changed, NULL);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_get_root failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_unset_data_changed_cb()
 */
static void utc_web_tab_unset_data_changed_cb_n(void)
{
	int ret = bp_tab_adaptor_unset_data_changed_cb(NULL);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**`
 * @brief Positive test case of bp_tab_adaptor_get_errorcode()
 */
static void utc_web_tab_unset_data_changed_cb_p(void)
{
	int ret = bp_tab_adaptor_unset_data_changed_cb(tab_changed);
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_unset_data_changed_cb failed");
}


/**
 * @brief Negative test case of bp_tab_adaptor_is_setted_data_changed_cb()
 */
static void utc_web_tab_is_setted_data_changed_cb_n(void)
{
	int ret = bp_tab_adaptor_unset_data_changed_cb(tab_changed);
	if (ret == 0)
		ret = bp_tab_adaptor_is_setted_data_changed_cb();
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Negative test case of bp_tab_adaptor_is_setted_data_changed_cb()
 */
static void utc_web_tab_is_setted_data_changed_cb_p(void)
{
	int ret = bp_tab_adaptor_set_data_changed_cb(tab_changed, NULL);
	ret = bp_tab_adaptor_is_setted_data_changed_cb();
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_is_setted_data_changed_cb failed");
}

/**
 * @brief Negative test case of bp_tab_adaptor_publish_notification()
 */
static void utc_web_tab_publish_notification_p(void)
{
	int ret = bp_tab_adaptor_publish_notification();
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_publish_notification failed");
}

/**
 * @brief Positive test case of bp_tab_adaptor_clear_dirty_ids()
 */
static void utc_web_tab_clear_dirty_ids_p(void)
{
	int ret = bp_tab_adaptor_clear_dirty_ids();
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_clear_dirty_ids failed");
}

/**
 * @brief Positive test case of bp_tab_adaptor_clear_deleted_ids()
 */
static void utc_web_tab_clear_deleted_ids_p(void)
{
	int ret = bp_tab_adaptor_clear_deleted_ids();
	if (ret == 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "bp_tab_adaptor_clear_deleted_ids failed");
}


/**
 * @brief Positive test case of bp_tab_adaptor_get_errorcode()
 */
static void utc_web_tab_get_errorcode_p(void)
{
	bp_tab_adaptor_get_errorcode();
	dts_pass(__FUNCTION__);
}

/**
 * @brief Positive test case of bp_tab_adaptor_easy_free()
 */
static void utc_web_tab_easy_free_p(void)
{
	bp_tab_adaptor_easy_free(NULL);
	dts_pass(__FUNCTION__);
}

