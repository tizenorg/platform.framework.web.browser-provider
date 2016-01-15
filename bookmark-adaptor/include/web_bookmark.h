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

#ifndef __TIZEN_WEB_BOOKMARK_H__
#define __TIZEN_WEB_BOOKMARK_H__

#ifndef EXPORT_API
#define EXPORT_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @file web_bookmark.h
 */

/**
 * @addtogroup CAPI_WEB_BOOKMARK_MODULE
 * @{
 */


/**
 * @brief Each offset mask means each property of the storage, respectively.
 * @remarks The caller can set with one or more properties by '|' bit masking.
 */
typedef unsigned int bp_bookmark_offset;
/**
 * @brief Definition for the type @c 0: bookmark @c 1: folder.
 * @since_tizen 2.3
 */
#define BP_BOOKMARK_O_TYPE 0x00001
/**
 * @brief Definition for the ID of the parent folder.
 * @since_tizen 2.3
 */
#define BP_BOOKMARK_O_PARENT 0x00002
/**
 * @brief Definition for the index of ordering.
 * @since_tizen 2.3
 */
#define BP_BOOKMARK_O_SEQUENCE 0x00004
/**
 * @brief Definition for whether the bookmark "is editable", @c 0: editable, @c 1: read only.
 * @since_tizen 2.3
 */
#define BP_BOOKMARK_O_IS_EDITABLE 0x00008
/**
 * @brief Definition for whether the bookmark "is made by the operator", @c 0: made by the user @c 1: by the operator.
 * @since_tizen 2.3
 */
#define BP_BOOKMARK_O_IS_OPERATOR 0x00010
/**
 * @brief Definition for the frequency of use.
 * @since_tizen 2.3
 */
#define BP_BOOKMARK_O_ACCESS_COUNT 0x00020
/**
 * @brief Definition for the creation time.
 * @since_tizen 2.3
 */
#define BP_BOOKMARK_O_DATE_CREATED 0x00040
/**
 * @brief Definition for the last modified time.
 * @since_tizen 2.3
 */
#define BP_BOOKMARK_O_DATE_MODIFIED 0x00080
/**
 * @brief Definition for the last used time.
 * @since_tizen 2.3
 */
#define BP_BOOKMARK_O_DATE_VISITED 0x00100
/**
 * @brief Definition for the address of the website.
 * @since_tizen 2.3
 */
#define BP_BOOKMARK_O_URL 0x00200
/**
 * @brief Definition for the title of the website.
 * @since_tizen 2.3
 */
#define BP_BOOKMARK_O_TITLE 0x00400
/**
 * @brief Definition for the account ID to sync.
 * @since_tizen 2.3
 */
#define BP_BOOKMARK_O_ACCOUNT_NAME 0x00800
/**
 * @brief Definition for the type of account to sync.
 * @since_tizen 2.3
 */
#define BP_BOOKMARK_O_ACCOUNT_TYPE 0x01000
/**
 * @brief Definition for the name of the device.
 * @since_tizen 2.3
 */
#define BP_BOOKMARK_O_DEVICE_NAME 0x02000
/**
 * @brief Definition for the unique ID of the device.
 * @since_tizen 2.3
 */
#define BP_BOOKMARK_O_DEVICE_ID 0x04000
/**
 * @brief Definition for the icon.
 * @since_tizen 2.3
 */
#define BP_BOOKMARK_O_ICON 0x08000
/**
 * @brief Definition for the favicon.
 * @since_tizen 2.3
 */
#define BP_BOOKMARK_O_FAVICON 0x08000
/**
 * @brief Definition for the snapshot (thumbnail).
 * @since_tizen 2.3
 */
#define BP_BOOKMARK_O_SNAPSHOT 0x10000
/**
 * @brief Definition for the snapshot (thumbnail).
 * @since_tizen 2.3
 */
#define BP_BOOKMARK_O_THUMBNAIL 0x10000
/**
 * @brief Definition for the webicon.
 * @since_tizen 2.3
 */
#define BP_BOOKMARK_O_WEBICON 0x20000
/**
 * @brief Definition for the extra property to sync.
 * @since_tizen 2.3
 */
#define BP_BOOKMARK_O_SYNC 0x80000
/**
 * @brief Definition for all the properties.
 * @since_tizen 2.3
 */
#define BP_BOOKMARK_O_ALL 0xfffff



/**
 * @brief Enumeration for the period to be searched.
 * @since_tizen 2.3
 */
typedef enum {
	BP_BOOKMARK_DATE_ALL,			/**< Unlimited */
	BP_BOOKMARK_DATE_TODAY,			/**< Only today */
	BP_BOOKMARK_DATE_YESTERDAY,		/**< Only yesterday */
	BP_BOOKMARK_DATE_LAST_7_DAYS,	/**< From 7 days ago, to the day before yesterday */
	BP_BOOKMARK_DATE_LAST_MONTH,	/**< This month */
	BP_BOOKMARK_DATE_OLDER			/**< Until last month */
} bp_bookmark_date_defs;


/**
 * @brief Enumeration for the operator used to search the date property.
 * @since_tizen 2.3
 * @details Offset GREATER timestamp AND offset2 LESS_EQUAL timestamp.
 */
typedef enum {
	BP_BOOKMARK_OP_NONE = 0,		/**< No operator */
	BP_BOOKMARK_OP_EQUAL,			/**< Equal ( == ) */
	BP_BOOKMARK_OP_NOT_EQUAL,		/**< Not equal ( != ) */
	BP_BOOKMARK_OP_GREATER,			/**< Greater ( > ) */
	BP_BOOKMARK_OP_LESS,			/**< Less than ( < ) */
	BP_BOOKMARK_OP_GREATER_QUEAL,	/**< Greater or equal ( >= ) */
	BP_BOOKMARK_OP_LESS_QUEAL		/**< Less or equal ( <= ) */
} bp_bookmark_timestamp_op_defs;


/**
 * @brief The structure type that includes some conditions for searching by timestamp.
 * @since_tizen 2.3
 * @remarks The default offset is #BP_BOOKMARK_O_DATE_CREATED.
 */
typedef struct {
	bp_bookmark_offset offset;					/**< A property to be searched */
	long timestamp;								/**< The timestamp to search */
	bp_bookmark_timestamp_op_defs cmp_operator;	/**< Operater between offset and timestamp */
	unsigned conds_operator;					/**< The relation with a previous condition @c 0: AND, @c 1: OR */
} bp_bookmark_timestamp_fmt;


/**
 * @brief The structure type that includes some conditions that are related with the ordering and the number of searched rows.
 * @since_tizen 2.3
 * @remarks The default offset is #BP_BOOKMARK_O_DATE_CREATED.
 */
typedef struct {
	int limit;							/**< The maximun number of rows to get, negative means no limitaion */
	int offset;							/**< Starting index to get among entire rows */
	bp_bookmark_offset order_offset;	/**< A property to be sorted, default is #BP_BOOKMARK_O_DATE_CREATED */
	int ordering;						/**< The way of ordering, @c 0: ASC(default) @c 1: DESC */
} bp_bookmark_rows_fmt;


/**
 * @brief Enumeration for error codes of the BP Bookmark.
 * @since_tizen 2.3
 * @details The caller can know why the function failed.
 */
typedef enum {
	BP_BOOKMARK_ERROR_NONE = 10,			/**< Default, no error */
	BP_BOOKMARK_ERROR_INVALID_PARAMETER,	/**< Passed wrong parameter */
	BP_BOOKMARK_ERROR_OUT_OF_MEMORY,		/**< Failed to allocate the memory */
	BP_BOOKMARK_ERROR_IO_ERROR,				/**< Some I/O error in communication with the provider process */
	BP_BOOKMARK_ERROR_NO_DATA,				/**< No data is retrieved */
	BP_BOOKMARK_ERROR_ID_NOT_FOUND,			/**< Passed ID does not exist in the storage */
	BP_BOOKMARK_ERROR_DUPLICATED_ID,		/**< Try to create an ID that already exists */
	BP_BOOKMARK_ERROR_PERMISSION_DENY,		/**< Denied by the user space smack rule */
	BP_BOOKMARK_ERROR_DISK_BUSY,			/**< Failed to write or read (make cache) due to the disk busy error */
	BP_BOOKMARK_ERROR_DISK_FULL,			/**< Failed to write due to the disk full error */
	BP_BOOKMARK_ERROR_TOO_BIG_DATA,			/**< Tried to write too much data, the maximum length of the string is 4K */
	BP_BOOKMARK_ERROR_UNKNOWN				/**< Unknown error */
} bp_bookmark_error_defs;

/**
 * @brief The structure type that includes the variables for all properties.
 * @since_tizen 2.3
 * @remarks The caller only has to access the variables that are related to the bookmark_offset. \n
 * 			The caller has to initialize the structure before using it.\n
 * 			The caller has to free the memory of the structure allocated by bp_bookmark_adaptor_get_info().
 */
typedef struct {
	int type;				/**< Type @c 0: bookmark @c 1: folder */
	int parent;				/**< Parent ID */
	int sequence;			/**< The index of ordering */
	int editable;			/**< Is editable @c 0: editable, @c 1: read only */
	int is_operator;		/**< Is made by the operator, @c 0: made by the user @c 1: by the operator */
	int access_count;		/**< Frequency of use */
	int date_created;		/**< Ignore this variable by setting it to zero */
	int date_modified;		/**< Ignore this variable by setting it to zero */
	int date_visited;		/**< Ignore this variable by setting it to zero */
	int favicon_length;		/**< Ignore this favicon by setting it to zero */
	int favicon_width;	/**< Favicon image's width. Ignore this variable by setting it to zero */
	int favicon_height;	/**< Favicon image's height. Ignore this variable by setting it to zero */
	int thumbnail_length;	/**< Ignore this thumbnail by setting it to zero */
	int thumbnail_width;	/**< Thumbnail image's width. Ignore this variable by setting it to zero */
	int thumbnail_height;	/**< Thumbnail image's height. Ignore this variable by setting it to zero */
	int webicon_length;		/**< Ignore this webicon by setting it to zero */
	int webicon_width;	/**< Web icon image's width. Ignore this variable by setting it to zero */
	int webicon_height;	/**< Web icon image's height. Ignore this variable by setting it to zero */
	char *url;	/**< Address of the bookmark. @c NULL if the type is folder */
	char *title;	/**< Title of the bookmark or folder */
	char *account_name;	/**< Account name for bookmark cloud sync */
	char *account_type;	/**< Acount type for bookmark cloud sync */
	char *device_name;	/**< Device name of cloud sync from which the bookmark originated */
	char *device_id;	/**< Device ID of cloud sync from which the bookmark originated */
	unsigned char *favicon;		/**< Raw data of the icon ( favorites icon ) */
	unsigned char *thumbnail; 	/**< Raw data of the snapshot */
	unsigned char *webicon;		/**< Raw data of the webicon */
	int tag_count;				/**< Deprecated */
	void *tags;					/**< Deprecated */
	char *sync;					/**< Extra property to sync */
} bp_bookmark_info_fmt;


/**
 * @brief The structure type that includes the basic conditions of the bookmark.
 * @since_tizen 2.3
 */
typedef struct {
	int parent;			/**< Parent ID */
	int type;			/**< Type @c 0: bookmark @c 1: folder */
	int is_operator;	/**< Is made by the operator, @c 0: made by the user @c 1: by the operator */
	int is_editable;	/**< Is editable @c 0: editable, @c 1: read only */
} bp_bookmark_property_cond_fmt;


/**
 * @brief The structure type that includes some conditions that are related to the period, the ordering, and the number of searched rows.
 * @since_tizen 2.3
 * @remarks The default offset is #BP_BOOKMARK_O_DATE_CREATED.
 */
typedef struct {
	int limit;							/**< The maximum number of rows to get, negative means no limitation */
	int offset;							/**< Starting index to get among entire rows */
	bp_bookmark_offset order_offset;	/**< The property to be sorted, default is #BP_BOOKMARK_O_DATE_CREATED */
	int ordering;						/**< The way of ordering, @c 0: ASC(default) @c 1: DESC */
	bp_bookmark_offset period_offset;	/**< A property to be searched by the period_type, default is #BP_BOOKMARK_O_DATE_CREATED */
	bp_bookmark_date_defs period_type;	/**< The period to be searched, default is #BP_BOOKMARK_DATE_ALL */
} bp_bookmark_rows_cond_fmt;


/**
 * @brief Called when another client changes some bookmarks.
 * @since_tizen 2.3
 * @param[in] user_data The user data to be passed to the callback function
 * @see bp_bookmark_adaptor_set_data_changed_cb()
 * @see bp_bookmark_adaptor_unset_data_changed_cb()
 */
typedef void (*bp_bookmark_adaptor_data_changed_cb)(void *user_data);


/**
 * @brief Registers the callback function called when another client changes some bookmarks.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] callback  The function address of the callback function to be called
 * @param[in] user_data  The user data to be passed to the callback function
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 * @see bp_bookmark_adaptor_is_setted_data_changed_cb()
 * @see bp_bookmark_adaptor_unset_data_changed_cb()
 */
EXPORT_API int bp_bookmark_adaptor_set_data_changed_cb
	(bp_bookmark_adaptor_data_changed_cb callback, void *user_data);


/**
 * @brief Unsets the callback function passed to bp_bookmark_adaptor_set_data_changed_cb().
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] callback  The function address of the callback function to be called
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 * @see bp_bookmark_adaptor_is_setted_data_changed_cb()
 * @see bp_bookmark_adaptor_set_data_changed_cb()
 */
EXPORT_API int bp_bookmark_adaptor_unset_data_changed_cb
	(bp_bookmark_adaptor_data_changed_cb callback);


/**
 * @brief Checks whether the callback function is already registered.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 * @see bp_bookmark_adaptor_set_data_changed_cb()
 * @see bp_bookmark_adaptor_unset_data_changed_cb()
 */
EXPORT_API int bp_bookmark_adaptor_is_setted_data_changed_cb(void);


/**
 * @brief Initializes the resources in order to use bookmark functions.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_initialize(void);


/**
 * @brief Frees all the resources allocated by bp_bookmark_adaptor_initialize().
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 * @see bp_bookmark_adaptor_initialize()
 */
EXPORT_API int bp_bookmark_adaptor_deinitialize(void);


/**
 * @brief Gets the ID of the root level folder.
 * @since_tizen 2.3
 * @param[out] id  The integer ID value of the root level folder
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned if the address of the parameter is @c NULL
 */
EXPORT_API int bp_bookmark_adaptor_get_root(int *id);


/**
 * @brief Marks an item with the specified ID as updated.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[out] id  The ID of the item
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_set_dirty(const int id);


/**
 * @brief Sends the notification to other clients.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @details The callback function is invoked if already registered.
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned
 * @see bp_bookmark_adaptor_set_data_changed_cb()
 */
EXPORT_API int bp_bookmark_adaptor_publish_notification(void);

/**
 * @brief Gets the type of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id  The ID of the item
 * @param[out] value If @c 0 it means bookmark, 
 *                   otherwise @c 1 means folder
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_get_type(const int id, int *value);


/**
 * @brief Gets the parent ID of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id  The ID of the item
 * @param[out] value  The integer ID of the parent folder
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_get_parent_id(const int id, int *value);


/**
 * @brief Gets the URL string of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id  The ID of the item
 * @param[out] value  The string array
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_get_url(const int id, char **value);


/**
 * @brief Gets the title string of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id  The ID of the item
 * @param[out] value The string array
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_get_title(const int id, char **value);


/**
 * @brief Gets the index of ordering of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id  The ID of the item
 * @param[out] value  The integer pointer
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_get_sequence(const int id, int *value);


/**
 * @brief Gets the editable permission of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id  The ID of the item
 * @param[out] value  @c 0 means editable, 
 *                    otherwise @c 1 means read only
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_get_is_editable(const int id, int *value);


/**
 * @brief Gets who made the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id  The ID of the item
 * @param[out] value  @c 0 means it is made by the user, 
 *                    otherwise @c 1 means it is made by the operator
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_get_is_operator(const int id, int *value);


/**
 * @brief Gets the frequency of use of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id  The ID of the item
 * @param[out] value  The integer, frequency of use
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_get_access_count(const int id, int *value);


/**
 * @brief Gets the time when the item with the given ID is made.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id  The ID of the item
 * @param[out] value  The time stamp, integer
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_get_date_created(const int id, int *value);


/**
 * @brief Gets the time when the item with the given ID is last modified.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id  The ID of the item
 * @param[out] value  The time stamp, integer
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_get_date_modified(const int id, int *value);


/**
 * @brief Gets the time when the item with the given ID is last accessed.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id The ID of the item
 * @param[out] value  The time stamp, integer
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_get_date_visited(const int id, int *value);


/**
 * @brief Gets the account name of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @remarks The allocated string memory has to be released by the caller.
 * @param[in] id  The ID of the item
 * @param[out] value  The string
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_get_account_name(const int id, char **value);


/**
 * @brief Gets the account type of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @remarks The allocated string memory has to be released by the caller.
 * @param[in] id  The ID of the item
 * @param[out] value  The string
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_get_account_type(const int id, char **value);


/**
 * @brief Gets the device name of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @remarks The allocated string memory has to be released by the caller.
 * @param[in] id  The ID of the item
 * @param[out] value  The string
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_get_device_name(const int id, char **value);


/**
 * @brief Gets a device id of an item with the given id.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @remarks The allocated string memory has to be released by the caller.
 * @param[in] id  The ID of the item
 * @param[out] value  The string
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_get_device_id(const int id, char **value);


/**
 * @brief Gets the favicon of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @details The color space of the image is ARGB8888.
 * @remarks The image data value will be stored in the shared memory allocated by the browser-provider.
 *          Therefore, if this API is called multiple times, image contents may be changed, but the pointer will be the same.
 *          PLEASE DO NOT free the 'value' parameter directly after it is assigned.
 *          If you want to use the image data permanently, allocate memory and make a deep copy of the data.
 * @param[in] id  The ID of the item
 * @param[out] width  The width of the image
 * @param[out] height  The height of the image
 * @param[out] value A pointer to the raw image data; this memory will be released automatically when the browser provider is deinitialized
 * @param[out] length  The size of the raw data
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 * @see bp_bookmark_adaptor_get_icon_png()
 */
EXPORT_API int bp_bookmark_adaptor_get_icon(const int id, int *width, int *height, unsigned char **value, int *length);


/**
 * @brief Gets a snapshot of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @details The color space of the image is ARGB8888.
 * @remarks The image data value will be stored in the shared memory allocated by the browser-provider.
 *          Therefore, if this API is called multiple times, image contents may be changed, but the pointer will be the same.
 *          PLEASE DO NOT free the 'value' parameter directly after it is assigned.
 *          If you want to use the image data permanently, allocate memory and make a deep copy of the data.
 * @param[in] id  The ID of the item
 * @param[out] width  The width of the image
 * @param[out] height  The height of the image
 * @param[out] value A pointer to the raw image data; this memory will be released automatically when the browser provider is deinitialized
 * @param[out] length  The size of the raw data
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_get_snapshot(const int id, int *width, int *height, unsigned char **value, int *length);


/**
 * @brief Gets the webicon of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @details The color space of the image is ARGB8888.
 * @remarks The image data value will be stored in the shared memory allocated by the browser-provider.
 *          Therefore, if this API is called multiple times, image contents may be changed, but the pointer will be the same.
 *          PLEASE DO NOT free the 'value' parameter directly after it is assigned.
 *          If you want to use the image data permanently, allocate memory and make a deep copy of the data.
 * @param[in] id  The ID of the item
 * @param[out] width  The width of the image
 * @param[out] height  The height of the image
 * @param[out] value A pointer to the raw image data; this memory will be released automatically when the browser-provider is deinitialized
 * @param[out] length  The size of the raw data
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_get_webicon(const int id, int *width, int *height, unsigned char **value, int *length);


/**
 * @brief Gets a png format favicon of the bookmark with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @details The color space of the image is 32bit RGBA.
 * @remarks The allocated memory for rows data has to be released by the caller.
 * @param[in] id  The ID of the item
 * @param[out] value  The binary of the image
 * @param[out] length  The size of the binary
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 * @see bp_bookmark_adaptor_get_icon_png()
 */
EXPORT_API int bp_bookmark_adaptor_get_icon_png(const int id, unsigned char **value, int *length);


/**
 * @brief Sets the type of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id  The ID of the item
 * @param[in] value  If @c 0 it means bookmark, 
 *                   otherwise @c 1 means folder
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_set_type(const int id, const int value);


/**
 * @brief Sets the parent ID of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id  The ID of the item
 * @param[in] value  The integer ID of the parent folder
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_set_parent_id(const int id, const int value);


/**
 * @brief Sets the URL string of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id  The ID of the item
 * @param[in] value  The URI address
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_set_url(const int id, const char *value);


/**
 * @brief Sets the title string of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id  The ID of the item
 * @param[in] value  The title string
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_set_title(const int id, const char *value);


/**
 * @brief Sets the index of ordering of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id  The ID of the item
 * @param[in] value  The index of ordering, 
 *                   otherwise if negative, update to max index automatically
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_set_sequence(const int id, const int value);


/**
 * @brief Sets the frequency of use of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id  The ID of the item
 * @param[in] value  The integer, frequency of use
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_set_access_count(const int id, const int value);


/**
 * @brief Changes the creation time of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id  The ID of the item
 * @param[in] value  The time stamp, 
 *                   otherwise if less than or equal to @c 0, update to present time automatically
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_set_date_created(const int id, const int value);


/**
 * @brief Changes the time when the item with the given ID is last modified.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id  The ID of the item
 * @param[in] value  The time stamp, 
 *                   otherwise if less than or equal to @c 0, update to present time automatically
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_set_date_modified(const int id, const int value);


/**
 * @brief Changes the time when the item with the given ID is last accessed.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id  The ID of the item
 * @param[in] value  The time stamp, 
 *                   otherwise if less than or equal to @c 0, update to present time automatically
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_set_date_visited(const int id, const int value);


/**
 * @brief Sets the account name of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id  The ID of the item
 * @param[in] value  The account name, string
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_set_account_name(const int id, const char *value);


/**
 * @brief Sets the account type of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id  The ID of the item
 * @param[in] value  The account type, string
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_set_account_type(const int id, const char *value);


/**
 * @brief Sets the device name of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id  The ID of the item
 * @param[in] value  The device name, string
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_set_device_name(const int id, const char *value);


/**
 * @brief Sets the device ID of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id  The ID of the item
 * @param[in] value The device ID, string
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_set_device_id(const int id, const char *value);

/**
 * @brief Sets the favicon of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @details The color space of the image is ARGB8888. 
 * @param[in] id  The ID of the item
 * @param[in] width  The width of the image
 * @param[in] height  The height of the image
 * @param[in] value  The raw data of the image
 * @param[in] length  The size of the raw data
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 * @see bp_bookmark_adaptor_set_icon_png()
 */
EXPORT_API int bp_bookmark_adaptor_set_icon(const int id, const int width, const int height, const unsigned char *value, const int length);


/**
 * @brief Sets a snapshot of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @details The color space of the image is ARGB8888. 
 * @param[in] id  The ID of the item
 * @param[in] width  The width of the image
 * @param[in] height The height of the image
 * @param[in] value  The raw data of the image
 * @param[in] length  The size of the raw data
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_set_snapshot(const int id, const int width, const int height, const unsigned char *value, const int length);


/**
 * @brief Sets the webicon of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @details The color space of the image is ARGB8888. 
 * @param[in] id  The ID of the item
 * @param[in] width  The width of the image
 * @param[in] height  The height of the image
 * @param[in] value  The raw data of the image
 * @param[in] length  The size of the raw data
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 */
EXPORT_API int bp_bookmark_adaptor_set_webicon(const int id, const int width, const int height, const unsigned char *value, const int length);


/**
 * @brief Sets a png format favicon of the bookmark with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] id  The ID of the item
 * @param[in] value  The binary of the image
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 * @see bp_bookmark_adaptor_set_icon()
 */
EXPORT_API int bp_bookmark_adaptor_set_icon_png(const int id, const unsigned char *value);


/**
 * @brief Creates a new item.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @details The creation time is set to the present time automatically. 
 * @param[out] id  The ID before calling returns the ID of an item created newly if the ID is initialized to @c -1,
 * 				   otherwise a new item is created with the ID if the ID is a positive unique value
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 * @see bp_bookmark_adaptor_easy_create()
 */
EXPORT_API int bp_bookmark_adaptor_create(int *id);


/**
 * @brief Deletes an item.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @details If cloud is on, the item is not deleted, but the is_deleted property is set.
 * 			However, the caller may think it has been deleted because all getting IDs functions will ignore it.
 * 			It will be deleted from the storage later by calling bp_bookmark_adaptor_clear_deleted_ids() in the cloud module. 
 * @remarks If this item is a folder, all child items are deleted too.
 * @param[in] id  The ID of the item
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 * @see bp_bookmark_adaptor_clear_deleted_ids()
 */
EXPORT_API int bp_bookmark_adaptor_delete(const int id);


/**
 * @brief Gets the error code to indicate the error when the function returns @c -1.
 * @since_tizen 2.3
 * @return The error code that indicates the error that is set in the end
 */
EXPORT_API int bp_bookmark_adaptor_get_errorcode(void);


/**
 * @brief Creates or updates an item using the structure.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[out] id  The ID before calling returns the ID of an item created newly if the ID is initialized to @c -1,
 * 				   otherwise a new item is created with the ID if the ID is a positive unique value
 * @param[in] info  The structure includes all the properties to set
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 * @pre First the structure has to be initialized to @c 0, then set the variables to change.
 * @see #bp_bookmark_info_fmt
 * @see bp_bookmark_adaptor_create()
 */
EXPORT_API int bp_bookmark_adaptor_easy_create(int *id, bp_bookmark_info_fmt *info);


/**
 * @brief Gets all the properties of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @remarks The allocated memory of strings in the structure has to be released by the caller.
 * @param[in] id  The ID of the item
 * @param[in] info  The structure including all the properties
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 * @see #bp_bookmark_info_fmt
 * @see bp_bookmark_adaptor_get_info()
 */
EXPORT_API int bp_bookmark_adaptor_get_easy_all(int id, bp_bookmark_info_fmt *info);


/**
 * @brief Gets one or more properties indicated by the offset mask of the item with the given ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @remarks The allocated memory of strings in the structure has to be released by the caller.
 * @param[in] id  The ID of the item
 * @param[in] offset  The value that indicates one or more properties by '|' bit masking
 * @param[in] info  The structure that includes all properties
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 * @pre First the structure has to be initialized to @c 0, then set the variables to change.
 * @see #bp_bookmark_info_fmt
 * @see bp_bookmark_adaptor_get_easy_all()
 */
EXPORT_API int bp_bookmark_adaptor_get_info(const int id, const bp_bookmark_offset offset, bp_bookmark_info_fmt *info);


/**
 * @brief Clears the allocated memory in the structure.
 * @since_tizen 2.3
 * @param[in] info  The structure to clear
 * @return @c 0 on success
 * @see #bp_bookmark_info_fmt
 * @see bp_bookmark_adaptor_get_info()
 */
EXPORT_API int bp_bookmark_adaptor_easy_free(bp_bookmark_info_fmt *info);


/**
 * @brief Creates a backup file at the path.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] value  The absolute path to store
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 * @see bp_bookmark_adaptor_restore()
 */
EXPORT_API int bp_bookmark_adaptor_backup(char *value);


/**
 * @brief Restores the backup file.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @param[in] value  The absolute path to restore
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 * @see bp_bookmark_adaptor_backup()
 */
EXPORT_API int bp_bookmark_adaptor_restore(char *value);


/**
 * @brief Gets an ID array and the number of rows of items searched from the storage.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @remarks The allocated memory (IDs) has to be released by the caller.
 * @param[out] ids  The ID array
 * @param[out] count  The size of the array
 * @param[in] limit  The maximum number of rows to get, negative means no limitation
 * @param[in] offset  The starting index to get among entire rows
 * @param[in] parent  The ID of the parent folder, negative means searching all folders
 * @param[in] type  The kind of items to be searched, @c 0 means bookmark, @c 1 means folder, and negative means both
 * @param[in] is_operator  @c 0: made by the user @c 1: by the operator, negative means both
 * @param[in] is_editable  @c 0 means read only, @c 1 means editable, negative means both
 * @param[in] order_offset  A property to be sorted, default is #BP_BOOKMARK_O_DATE_CREATED
 * @param[in] ordering  The way of ordering, @c 0: ASC(default) @c 1: DESC
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 * @see #bp_bookmark_offset
 * @see bp_bookmark_adaptor_get_cond_ids_p()
 */
EXPORT_API int bp_bookmark_adaptor_get_ids_p
	(int **ids, int *count, const int limit, const int offset,
	const int parent, const int type,
	const int is_operator, const int is_editable,
	const bp_bookmark_offset order_offset, const int ordering);

/**
 * @brief Gets an ID array and the number of rows of items searched by keyword and some structure from the storage.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @remarks The allocated memory (IDs) has to be released by the caller.\n
 * 			If the caller wants to do LIKE searching, the keyword has to have some wild card like _aa, a_a, %aa, aa%, or %aa%.
 * @param[out] ids  The ID array
 * @param[out] count  The size of the array
 * @param[in] properties  The structure including basic conditions of the bookmark
 * @param[in] conds The structure including some conditions that are related to the period, the ordering, and the number of searched rows
 * @param[in] check_offset  A property to be searched by keywords, #BP_BOOKMARK_O_TITLE, #BP_BOOKMARK_O_URL, or (#BP_BOOKMARK_O_TITLE | #BP_BOOKMARK_O_URL)
 * @param[in] keyword  The string to search, 
 *                     otherwise @c NULL to ignore searching by keyword
 * @param[in] is_like  The way of searching, @c 1 means LIKE, @c 0 means EQUAL, and negative means NOT EQUAL
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 * @pre First the structure has to be initialized to @c 0, then set the variables to change.
 * @see #bp_bookmark_property_cond_fmt
 * @see #bp_bookmark_rows_cond_fmt
 * @see #bp_bookmark_offset
 * @see bp_bookmark_adaptor_get_ids_p()
 * @see bp_bookmark_adaptor_get_timestamp_ids_p()
 */
EXPORT_API int bp_bookmark_adaptor_get_cond_ids_p
	(int **ids, int *count,
	bp_bookmark_property_cond_fmt *properties,
	bp_bookmark_rows_cond_fmt *conds,
	const bp_bookmark_offset check_offset,
	const char *keyword,
	const int is_like);


/**
 * @brief Gets an ID array and the number of rows of items searched by keyword, time stamp, and some structure from the storage.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @remarks The allocated memory (IDs) has to be released by the caller.\n
 * 			If the caller wants to do LIKE searching, the keyword has to have some wild card like _aa, a_a, %aa, aa%, or %aa%.
 * @param[out] ids  The ID array
 * @param[out] count  The size of the array
 * @param[in] properties  The structure including basic conditions of the bookmark
 * @param[in] limits  The structure including some conditions that are related to the period, the ordering, and the number of searched rows
 * @param[in] times  The structure including some conditions for searching by time stamp
 * @param[in] times_count  The size of the times array
 * @param[in] check_offset  A property to be searched by keywords, #BP_BOOKMARK_O_TITLE, #BP_BOOKMARK_O_URL or (#BP_BOOKMARK_O_TITLE | #BP_BOOKMARK_O_URL)
 * @param[in] keyword  The string to search, 
 *                     otherwise @c NULL to ignore searching by keyword
 * @param[in] is_like  The way of searching, @c 1 means LIKE, @c 0 means EQUAL, and negative means NOT EQUAL
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 * @pre First the structure has to be initialized to @c 0, then set the variables to change.
 * @see #bp_bookmark_property_cond_fmt
 * @see #bp_bookmark_rows_cond_fmt
 * @see #bp_bookmark_timestamp_fmt
 * @see #bp_bookmark_offset
 * @see bp_bookmark_adaptor_get_ids_p()
 * @see bp_bookmark_adaptor_get_cond_ids_p()
 */
EXPORT_API int bp_bookmark_adaptor_get_timestamp_ids_p
	(int **ids, int *count,
	const bp_bookmark_property_cond_fmt *properties,
	const bp_bookmark_rows_fmt *limits,
	const bp_bookmark_timestamp_fmt times[], const int times_count,
	const bp_bookmark_offset check_offset,
	const char *keyword,
	const int is_like);

/**
 * @brief Gets an ID array and the number of rows of items searched by keyword and some structure from the storage.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @remarks The allocated memory (IDs) has to be released by the caller.\n
 * 			If the caller wants to do LIKE searching, the keyword has to have some wild card like _aa, a_a, %aa, aa%, or %aa%.\n
 * 			Although URI addresses have a protocol or the www prefix, search engines does not exclude it.
 * @param[out] ids  The ID array
 * @param[out] count  The size of the array
 * @param[in] properties  The structure including basic conditions of the bookmark
 * @param[in] conds  The structure including some conditions that are related to the period, the ordering, and the number of searched rows
 * @param[in] check_offset  A property to be searched by keywords, #BP_BOOKMARK_O_TITLE, #BP_BOOKMARK_O_URL or (#BP_BOOKMARK_O_TITLE | #BP_BOOKMARK_O_URL)
 * @param[in] keyword  The string to search, 
 *                     otherwise @c NULL to ignore searching by keyword
 * @param[in] is_like  The way of searching, @c 1 means LIKE, @c 0 means EQUAL, and negative means NOT EQUAL
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 * @pre First the structure has to be initialized to @c 0, then set the variables to change.
 * @see #bp_bookmark_property_cond_fmt
 * @see #bp_bookmark_rows_cond_fmt
 * @see #bp_bookmark_offset
 */
EXPORT_API int bp_bookmark_adaptor_get_raw_retrieved_ids_p
	(int **ids, int *count,
	bp_bookmark_property_cond_fmt *properties,
	bp_bookmark_rows_cond_fmt *conds,
	const bp_bookmark_offset check_offset,
	const char *keyword,
	const int is_like);

/**
 * @brief Clears all items.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/bookmark.admin
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
*/
EXPORT_API int bp_bookmark_adaptor_reset(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __TIZEN_WEB_BOOKMARK_H__ */
