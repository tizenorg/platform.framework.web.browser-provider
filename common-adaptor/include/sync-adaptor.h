/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
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

#ifndef __BROWSER_PROVIDER_SYNC_ADAPTOR_H__
#define __BROWSER_PROVIDER_SYNC_ADAPTOR_H__

typedef enum {
    SYNC_LOGOUT = 0,
    SYNC_LOGIN
}sync_login_type;

#define SYNC_CLOUD_ID_MAX_LENGTH 40

#define _LOOP_INIT(idx, list_type, list_var, list) int idx=0;\
	list_type* list_var = list;
#define CLOUD_LOOP_INIT(list_type,list) _LOOP_INIT(cloud_idx, list_type, cloud_list, list)
#define LOCAL_LOOP_INIT(list_type,list) _LOOP_INIT(local_idx, list_type, local_list, list)

#define _FOR_LOOP(idx,list) for (idx = 0; idx < list->count; idx++)
#define CLOUD_LOOP() _FOR_LOOP(cloud_idx,cloud_list)
#define LOCAL_LOOP() _FOR_LOOP(local_idx,local_list)

#define IS_SAME_CLOUD_URL_AS_LOCAL() \
	(strcmp(cloud_list->item_list[cloud_idx].url,local_list->item_list[local_idx].url) == 0)

#define IS_LOCAL_NORMAL() \
	(local_list->mark[local_idx] == SYNC_NONE)

#define IS_LOCAL_DELETED() \
	(local_list->mark[local_idx] == SYNC_DELETED_UPSYNC)

#define IS_CLOUD_TO_UPDATE() \
	(cloud_list->mark[cloud_idx] == SYNC_UPDATE)

#define IS_CLOUD_TO_DELETE() \
	(cloud_list->mark[cloud_idx] == SYNC_DELETE)

#define IS_SAME_CLOUD_RECORD_AS_LOCAL() \
	(strcmp(local_list->item_list[local_idx].sync,cloud_list->item_list[cloud_idx].id) == 0)

#define IS_CLOUD_OLDER_THAN_LOCAL() \
	(cloud_list->item_list[cloud_idx].update_time <= (int64_t)((int64_t)local_list->item_list[local_idx].date_modified*(int64_t)1000) )

#define IS_SYNCED_RECORD_WITHOUT_DIRTY() \
	(strcmp(local_list->item_list[local_idx].sync,cloud_list->item_list[cloud_idx].id) == 0 \
	&& local_list->item_list[local_idx].date_modified == local_list->item_list[local_idx].date_created)

#define IS_MADE_BY_LOCAL() \
	(local_list->item_list[local_idx].sync == NULL)

#define HOLD_LOCAL() do {\
	cloud_list->mark[cloud_idx] = SYNC_NONE;\
	local_list->mark[local_idx] = SYNC_NONE;\
	TRACE_DEBUG("hold local[%d]",local_idx);\
} while(0)

#define UPSYNC() do {\
	cloud_list->mark[cloud_idx] = SYNC_NONE;\
	local_list->mark[local_idx] = SYNC_UPDATE;\
	local_list->item_list[local_idx].sync = cloud_list->item_list[cloud_idx].id;\
	TRACE_DEBUG("cloud is old. local(%d)->cloud(%d)",local_idx,cloud_idx);\
} while(0)

#define UPSYNC_REVIVE() do {\
	cloud_list->mark[cloud_idx] = SYNC_NONE;\
	local_list->mark[local_idx] = SYNC_UPDATE;\
	TRACE_DEBUG("local(%d)->cloud, deleted cloud (%d) is old.",local_idx,cloud_idx);\
} while(0)

#define UPSYNC_NEW_LOCAL() do {\
	local_list->mark[local_idx] = SYNC_UPDATE;\
	TRACE_DEBUG("local(%d)->cloud as a new item.",local_idx);\
} while(0)

#define DOWNSYNC() do {\
	cloud_list->mark[cloud_idx] = SYNC_UPDATE;\
	local_list->mark[local_idx] = SYNC_DELETE;\
	TRACE_DEBUG("local is old. local(%d)<-cloud(%d)",local_idx,cloud_idx);\
} while(0)

#define DOWNSYNC_DELETE() do {\
	cloud_list->mark[cloud_idx] = SYNC_NONE;\
	local_list->mark[local_idx] = SYNC_DELETE;\
	TRACE_DEBUG("local(%d) should be deleted. cloud(%d) was deleted.",local_idx,cloud_idx);\
} while(0)

#define UPSYNC_DELETED() do {\
	cloud_list->mark[cloud_idx] = SYNC_NONE;\
	local->mark[local_idx] = SYNC_DELETED_UPSYNC;\
	TRACE_DEBUG("cloud[%d] should be deleted. local[%d] was deleted",cloud_idx,local_idx);\
} while(0)

#define NO_UPSYNC_DELETED() local_list->mark[local_idx] = SYNC_DELETED_NONE;

#define MAKE_CLOUD_ID(device_id) do {\
	cloud_list->item_list[cloud_idx].id = malloc(SYNC_CLOUD_ID_MAX_LENGTH);/*TODO : free later */\
	memset(cloud_list->item_list[cloud_idx].id,0x00,SYNC_CLOUD_ID_MAX_LENGTH);\
	if (local_list->item_list[local_idx].sync == NULL){\
		sprintf(cloud_list->item_list[cloud_idx].id,"%d%s",local_list->id_list[local_idx],device_id);\
	}else{\
		sprintf(cloud_list->item_list[cloud_idx].id,"%s",local_list->item_list[local_idx].sync);\
	}\
} while(0)

#define MARK_UPDATE_SYNC(device_id) do {\
	if (( IS_MADE_BY_LOCAL() || IS_SAME_CLOUD_RECORD_AS_LOCAL() ) && (IS_SAME_CLOUD_URL_AS_LOCAL())){\
		if (IS_CLOUD_OLDER_THAN_LOCAL()){\
			if(IS_SYNCED_RECORD_WITHOUT_DIRTY()){\
				HOLD_LOCAL();\
			}else{\
				UPSYNC();\
			}\
		}else{\
			DOWNSYNC();\
			MAKE_CLOUD_ID(device_id);\
		}\
	}\
} while(0)

#define MARK_DELETED_SYNC() do {\
	if (IS_CLOUD_OLDER_THAN_LOCAL())\
	{\
		UPSYNC_DELETED();\
	}else {\
		NO_UPSYNC_DELETED();\
	}\
} while(0)


#define MARK_DELETE_SYNC() do {\
	if (IS_CLOUD_OLDER_THAN_LOCAL()){\
		UPSYNC_REVIVE();\
	}else{\
		DOWNSYNC_DELETE();\
	}\
} while(0)

#define PRINT_LOCAL_LOG() TRACE_DEBUG("LOCAL[%d/%d] %d",local_idx+1,local_list->count,local_list->mark[local_idx]);
#define PRINT_CLOUD_LOG() TRACE_DEBUG("CLOUD[%d/%d] %d",cloud_idx+1,cloud_list->count,cloud_list->mark[cloud_idx]);
#define PRINT_DELETED_LOG() TRACE_DEBUG("DELETED[%d/%d] %d",deleted_idx+1,deleted_list->count,deleted_list->mark[deleted_idx]);

char* get_device_id(void);
int bp_sync_login(char* guid, char* accesstoken);

#endif /* __BROWSER_PROVIDER_SYNC_ADAPTOR_H__ */
