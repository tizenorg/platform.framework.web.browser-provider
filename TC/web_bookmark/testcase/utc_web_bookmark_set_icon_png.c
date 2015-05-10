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
#include <web_bookmark.h>

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

static int g_testcase_id = -1;

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_web_bookmark_set_icon_png_n(void);
static void utc_web_bookmark_set_icon_png_p(void);


struct tet_testlist tet_testlist[] = {
	{ utc_web_bookmark_set_icon_png_n, NEGATIVE_TC_IDX },
	{ utc_web_bookmark_set_icon_png_p, POSITIVE_TC_IDX },
	{ NULL, 0 }
};


static void startup(void)
{
	/* start of TC */
	tet_printf("\n TC start");
	bp_bookmark_adaptor_initialize();
	bp_bookmark_adaptor_create(&g_testcase_id);
}


static void cleanup(void)
{
	/* end of TC */
	bp_bookmark_adaptor_deinitialize();
	tet_printf("\n TC end");
}

/**
 * @brief Negative test case of bp_bookmark_adaptor_set_icon_png()
 */
static void utc_web_bookmark_set_icon_png_n(void)
{
	unsigned char *value = NULL;
	int ret = bp_bookmark_adaptor_set_icon_png(-1, &value);
	if (ret < 0)
		dts_pass(__FUNCTION__);
	else
		dts_fail(__FUNCTION__, "-1 must be returned when parameter is NULL.");
}

/**
 * @brief Positive test case of bp_bookmark_adaptor_set_icon_png()
 */
static void utc_web_bookmark_set_icon_png_p(void)
{
	int ret = -1;
	FILE *fp = fopen("/home/capi-web-bookmark/test.png", "rb");
	if (fp != NULL) {
		int length = 0;
		unsigned char buffer[3145728];
		length = fread(buffer, 1, 3145728, fp);
		if (length > 0) {
			ret = bp_bookmark_adaptor_set_icon_png(g_testcase_id, buffer);
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
