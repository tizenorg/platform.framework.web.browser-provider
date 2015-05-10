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

#ifndef __TIZEN_WEB_SCRAP_H__
#define __TIZEN_WEB_SCRAP_H__

#ifndef EXPORT_API
#define EXPORT_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @internal
 * @addtogroup CAPI_WEB_SCRAP_MODULE
 * @{
 */


/**
 * @brief Each offset mask corresponds to each property of the storage, respectively.
 * @remarks The caller can set one or more properties by '|' bit masking.
 */
typedef unsigned short bp_scrap_offset;
/**
 * @brief The creation time
 */
#define BP_SCRAP_O_DATE_CREATED 0x0001
/**
 * @brief The time of the last modification
 */
#define BP_SCRAP_O_DATE_MODIFIED 0x0002
/**
 * @brief The absolute path of mhtml file
 */
#define BP_SCRAP_O_PAGE 0x0008
/**
 * @brief The url address of the web-site
 */
#define BP_SCRAP_O_URL 0x0010
/**
 * @brief The title of the web-site
 */
#define BP_SCRAP_O_TITLE 0x0020
/**
 * @brief The account name for sync
 */
#define BP_SCRAP_O_ACCOUNT_NAME 0x0040
/**
 * @brief The account type for sync
 */
#define BP_SCRAP_O_ACCOUNT_TYPE 0x0080
/**
 * @brief The device name
 */
#define BP_SCRAP_O_DEVICE_NAME 0x0100
/**
 * @brief The device id
 */
#define BP_SCRAP_O_DEVICE_ID 0x0200
/**
 * @brief The main content id
 */
#define BP_SCRAP_O_MAIN_CONTENT 0x0300
/**
 * @brief The icon
 */
#define BP_SCRAP_O_ICON 0x0400
/**
 * @brief The favicon
 */
#define BP_SCRAP_O_FAVICON 0x0400
/**
 * @brief The snapshot (thumbnail)
 */
#define BP_SCRAP_O_SNAPSHOT 0x0800
/**
 * @brief The 'if is read' flag
 */
#define BP_SCRAP_O_IS_READ 0x1000
/**
 * @brief The 'if is reader' flag
 */
#define BP_SCRAP_O_IS_READER 0x2000
/**
 * @brief The night mode flag
 */
#define BP_SCRAP_O_IS_NIGHT_MODE 0x4000
/**
 * @brief The all properties flag
 */
#define BP_SCRAP_O_ALL 0xffff


/**
 * @brief Enumeration of the periods to be searched
 */
typedef enum {
	BP_SCRAP_DATE_ALL,			/**< unlimited */
	BP_SCRAP_DATE_TODAY,		/**< only today */
	BP_SCRAP_DATE_YESTERDAY,	/**< only yesterday */
	BP_SCRAP_DATE_LAST_7_DAYS,	/**< from 7 days ago, to the day before yesterday */
	BP_SCRAP_DATE_LAST_MONTH,	/**< this month */
	BP_SCRAP_DATE_OLDER			/**< until last month */
} bp_scrap_date_defs;


/**
 * @brief The structure containing some conditions related with the periods
 */
typedef struct {
	int limit;						/**< the maximum number of rows to get, negative means no limitation */
	int offset;						/**< starting index to get among the entire rows */
	bp_scrap_offset order_offset;	/**< the property on which the rows will be sorted, default BP_SCRAP_O_DATE_CREATED */
	int ordering;					/**< the way of ordering, 0:ASC(default) 1:DESC */
	bp_scrap_offset period_offset;	/**< a property which will be searched in given period_type, default BP_SCRAP_O_DATE_CREATED */
	bp_scrap_date_defs period_type;	/**< the period which will be searched, default BP_SCRAP_DATE_ALL */
} bp_scrap_rows_cond_fmt;


/**
 * @brief Enumeration of error codes
 * @details Gives information why function has failed
 */
typedef enum {
	BP_SCRAP_ERROR_NONE = 10,			/**< success, no error */
	BP_SCRAP_ERROR_INVALID_PARAMETER,	/**< wrong parameter passed*/
	BP_SCRAP_ERROR_OUT_OF_MEMORY,		/**< failed to allocate the memory */
	BP_SCRAP_ERROR_IO_ERROR,			/**< some I/O error in communication with provider process occured*/
	BP_SCRAP_ERROR_NO_DATA,				/**< no data is retrieved */
	BP_SCRAP_ERROR_ID_NOT_FOUND,		/**< the passed id does not exist in the storage */
	BP_SCRAP_ERROR_DUPLICATED_ID,		/**< try to create the id which already exists */
	BP_SCRAP_ERROR_PERMISSION_DENY,		/**< denied by user space smack rule */
	BP_SCRAP_ERROR_DISK_BUSY,			/**< failed to write or read (make cache) by disk busy error */
	BP_SCRAP_ERROR_DISK_FULL,			/**< failed to write by disk full error */
	BP_SCRAP_ERROR_TOO_BIG_DATA,		/**< tried to write too big data, the maximum length of string is 4k */
	BP_SCRAP_ERROR_UNKNOWN				/**< unknown error */
} bp_scrap_error_defs;


/**
 * @brief The structure containing variables for all properties
 * @remarks The caller has access only to variables which are related to the bp_history_offset. \n
 * 			The caller has to initialize the structure before using it.\n
 * 			The caller has to free the memory of the structure allocated by bp_scrap_adaptor_get_info().
 */
typedef struct {
	int date_created;			/**< the creation time; ignore this variable by setting to zero */
	int date_modified;			/**< the time of the last modification; ignore this variable by setting to zero */
	int favicon_length;			/**< the favicon length; ignore this variable by setting to zero */
	int favicon_width;          /**< the favicon width */
	int favicon_height;         /**< the favicon height */
	int snapshot_length;		/**< ignore this variable by setting to zero **/
	int snapshot_width;         /**< the snapshot width */
	int snapshot_height;        /**< the snapshot height */
	char *url;                  /**< the url */
	char *title;                /**< the title */
	char *page_path;            /**< the path to the page */
	char *desc;                 /**< the summary of scrapped page */
	char *account_name;         /**< the account name */
	char *account_type;         /**< the account type */
	char *device_name;          /** the device name */
	char *device_id;            /**< the device id */
	char *main_content;         /**< the main content */
	unsigned char *favicon;		/**< row data of icon ( favorites icon ) */
	unsigned char *snapshot;	/**< row data of snapshot */
	int is_read;				/**< one or more time read, 1 means read, otherwise 0 is unread */
	int is_reader;				/**< 1 means reader, otherwise 0 is not reader */
	int is_night_mode;			/**< 1 means night mode, otherwise 0 is normal mode*/
} bp_scrap_info_fmt;


/**
 * @brief Initializes some resources in order to use scrap functions
 * @return 0 on success, otherwise -1 is returned and errorcode is set to indicate the error.
 * @see bp_scrap_adaptor_deinitialize()
 */
EXPORT_API int bp_scrap_adaptor_initialize(void);


/**
 * @brief Frees all resources allocated by bp_scrap_adaptor_initialize()
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 * @see bp_scrap_adaptor_initialize()
 */
EXPORT_API int bp_scrap_adaptor_deinitialize(void);


/**
 * @brief Called when other client changes some scraps
 * @param[in] user_data The data passed to bp_scrap_adaptor_set_data_changed_cb()
 * @see bp_scrap_adaptor_set_data_changed_cb()
 * @see bp_scrap_adaptor_unset_data_changed_cb()
 */
typedef void (*bp_scrap_adaptor_data_changed_cb)(void *user_data);


/**
 * @brief Registers the callback function called when other client changes some scraps
 * @param[in] callback The callback function to invoke
 * @param[in] user_data The data passed to the callback function
 * @return 0 on success, otherwise a negative error value
 * @see bp_scrap_adaptor_is_setted_data_changed_cb()
 * @see bp_scrap_adaptor_unset_data_changed_cb()
 */
EXPORT_API int bp_scrap_adaptor_set_data_changed_cb(bp_scrap_adaptor_data_changed_cb callback, void *user_data);


/**
 * @brief Unsets the callback function passed in bp_scrap_adaptor_set_data_changed_cb()
 * @param[in] callback The callback function passed in bp_scrap_adaptor_set_data_changed_cb()
 * @return 0 on success, otherwise a negative error value
 * @see bp_scrap_adaptor_is_setted_data_changed_cb()
 * @see bp_scrap_adaptor_set_data_changed_cb()
 */
EXPORT_API int bp_scrap_adaptor_unset_data_changed_cb(bp_scrap_adaptor_data_changed_cb callback);


/**
 * @brief Checks whether the callback function is already registered or not
 * @return 0 on success, otherwise a negative error value
 * @see bp_scrap_adaptor_set_data_changed_cb()
 * @see bp_scrap_adaptor_unset_data_changed_cb()
 */
EXPORT_API int bp_scrap_adaptor_is_setted_data_changed_cb(void);


/**
 * @brief Mark an item as updated
 * @param[out] id The id of an item to mark
 * @return 0 on success, otherwise -1 is returned and errorcode is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_set_dirty(const int id);


/**
 * @brief Send the notification to other clients
 * @details The callback function will be invoked if registered
 * @return 0 on success, otherwise -1 is returned
 * @see bp_scrap_adaptor_set_data_changed_cb()
 */
EXPORT_API int bp_scrap_adaptor_publish_notification(void);


/**
 * @brief Gets array ids and the number of rows of all items from the storage
 * @remarks Allocated memory (ids) have to be released by the caller.
 * @param[out] ids The array ids
 * @param[out] count The array size
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_get_full_ids_p(int **ids, int *count);


/**
 * @brief Get array ids and the number of rows of all items with deleted items from the storage
 * @remarks Allocated memory (ids) have to be released by the caller.
 * @param[out] ids The array ids
 * @param[out] count The array size
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_get_full_with_deleted_ids_p(int **ids, int *count);


/**
 * @brief Gets array ids and the number of rows of all modified items from the storage
 * @remarks Allocated memory (ids) have to be released by the caller.
 * @param[out] ids The array ids
 * @param[out] count The array size
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_get_dirty_ids_p(int **ids, int *count);


/**
 * @brief Gets array ids and the number of rows of all deleted items from the storage
 * @remarks Allocated memory (ids) have to be released by the caller.
 * @param[out] ids The array ids
 * @param[out] count The array size
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_get_deleted_ids_p(int **ids, int *count);


/**
 * @brief Commits all modified scraps
 * @details Clears "is_dirty" property of each item.
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_clear_dirty_ids(void);


/**
 * @brief Deletes all scraps which have set 'is_deleted' property from the storage
 * @details If cloud is on, "is_dirty" property is off by calling delete function,\n
 * 			if cloud is off, a scrap is deleted really from storage whenever delete function is called.
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_clear_deleted_ids(void);


/**
 * @brief Gets "is_read" property of an item matched with the given id
 * @param[in] id The id of an item
 * @param[out] value 1 means "is read", 0 - it is not
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_get_is_read(const int id, int *value);

/**
 * @brief Gets "is_reader" property of an item matched with the given id
 * @param[in] id The id of an item
 * @param[out] value 1 means "is reader", 0 - it is not
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_get_is_reader(const int id, int *value);

/**
 * @brief Gets "is_night_mode" property of an item matched with the given id
 * @param[in] id The id of an item
 * @param[out] night_mode_flag 1 means "is night mode", 0 - it is not
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_get_is_night_mode(const int id, int *night_mode_flag);

/**
 * @brief Gets the url property of an item matched with the given id
 * @param[in] id The id of an item
 * @param[out] value The url string
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_get_url(const int id, char **value);


/**
 * @brief Gets the title property of an item matched with the given id
 * @param[in] id The id of an item
 * @param[out] value The title string
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_get_title(const int id, char **value);


/**
 * @brief Gets the absolute path to mhtml file of an item matched with the given id
 * @param[in] id The id of an item
 * @param[out] value The path string
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_get_page_path(const int id, char **value);


/**
 * @brief Gets the time when an item matched with the given id was made
 * @param[in] id The id of an item
 * @param[out] value The time stamp
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_get_date_created(const int id, int *value);


/**
 * @brief Gets the time when an item matched with the given id was last modified
 * @param[in] id The id of an item
 * @param[out] value The time stamp
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_get_date_modified(const int id, int *value);


/**
 * @brief Gets the account name of an item matched with the given id
 * @remarks Allocated memory (@c value) has to be released by the caller
 * @param[in] id The id of an item
 * @param[out] value The account name
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_get_account_name(const int id, char **value);


/**
 * @brief Gets the account type of an item matched with the given id
 * @remarks Allocated memory (@c value) has to be released by the caller
 * @param [in] id The id of an item
 * @param [out] value The account type
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_get_account_type(const int id, char **value);


/**
 * @brief Gets the device name of an item matched with the given id
 * @remarks Allocated memory (@c value) has to be released by the caller
 * @param [in] id The id of an item
 * @param [out] value The device name
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_get_device_name(const int id, char **value);


/**
 * @brief Gets the device id of an item matched with the given id
 * @remarks Allocated memory (@c value) has to be released by the caller
 * @param [in] id The id of an item
 * @param [out] value The device id
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_get_device_id(const int id, char **value);

/**
 * @brief Gets the url of an item matched with the given id
 * @param [in] id The id of an item
 * @param [out] value The url value
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_get_main_content(const int id, char **value);

/**
 * @brief Gets the favicon of an item matched with the given id
 * @details The color space of the image is ARGB8888.
 * @remarks The image data value will be stored in shared memory allocated by the browser-provider.
 *			Therefore if this API is called multiple times, image contents may be changed but the pointer will be the same.
 *			PLEASE DO NOT free the 'value' parameter directly after it is assigned.
 *			If you want to use the image data permanently, allocate the memory and make a deep copy of it.
 * @param [in] id The id of an item
 * @param [out] width The width of the image
 * @param [out] height The height of the image
 * @param [out] value A pointer to a raw image data. This memory will be released automatically when the browser provider is deinitialized.
 * @param [out] length The size of the raw data
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_get_icon(const int id, int *width, int *height, unsigned char **value, int *length);


/**
 * @brief Gets the snapshot of an item matched with the given id
 * @details The color space of the image is ARGB8888.
 * @remarks The image data value will be stored in shared memory allocated by the browser-provider.
 *			Therefore if this API is called multiple times, image contents may be changed but the pointer will be the same.
 *			PLEASE DO NOT free the 'value' parameter directly after it is assigned.
 *			If you want to use the image data permanently, allocate the memory and make a deep copy of it.
 * @param [in] id The id of an item
 * @param [out] width The width of the image
 * @param [out] height The height of the image
 * @param [out] value The pointer of a raw image data. This memory will be released automatically when the browser provider is deinitialized.
 * @param [out] length The size of the raw data
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_get_snapshot(const int id, int *width, int *height, unsigned char **value, int *length);


/**
 * @brief Sets "is_read" property of an item matched with the given id
 * @param [in] id The The id of an item
 * @param [in] value 0 as "unread", 1 as "read"
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_set_is_read(const int id, const int value);

/**
 * @brief Sets "is_reader" property of an item matched with the given id
 * @param [in] id The id of an item
 * @param [in] value 0 as "not reader", 1 as "reader"
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_set_is_reader(const int id, const int value);

/**
 * @brief Sets "is_night_mode" property of an item matched with the given id
 * @param [in] id The id of an item
 * @param [in] night_mode_flag 0 as "normal mode", 1 as "night mode".
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_set_is_night_mode(const int id, const int night_mode_flag);

/**
 * @brief Sets the url string of an item matched with the given id
 * @param [in] id The id of an item
 * @param [in] value The uri address
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_set_url(const int id, const char *value);


/**
 * @brief Sets the title of an item matched with the given id
 * @param [in] id The id of an item
 * @param [in] value The title
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_set_title(const int id, const char *value);


/**
 * @brief Sets the absolute path to mhtml file of an item matched with the given id
 * @param [in] id The id of an item
 * @param [in] value The path
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_set_page_path(const int id, const char *value);


/**
 * @brief Changes the creation time of an item matched with the given id
 * @param [in] id The id of an item
 * @param [in] value The time stamp (if @c value <= 0 - update to present time automatically)
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_set_date_created(const int id, const int value);


/**
 * @brief Changes the time when an item matched with the given id was last modified
 * @param [in] id The id of an item
 * @param [in] value The time stamp (if @c value <= 0 - update to present time automatically)
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_set_date_modified(const int id, const int value);


/**
 * @brief Sets the account name of an item matched with the given id
 * @param [in] id The id of an item
 * @param [in] value The account name
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_set_account_name(const int id, const char *value);


/**
 * @brief Sets the account type of an item matched with the given id
 * @param [in] id The id of an item
 * @param [in] value The account type
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_set_account_type(const int id, const char *value);


/**
 * @brief Sets the device name of an item matched with the given id
 * @param [in] id The id of an item
 * @param [in] value The device name
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_set_device_name(const int id, const char *value);


/**
 * @brief Sets the device id of an item matched with the given id
 * @param [in] id The id of an item
 * @param [in] value The device id
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_set_device_id(const int id, const char *value);

/**
 * @brief Sets the main content of an item matched with the given id
 * @param [in] id The id of an item
 * @param [in] value The main content
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_set_main_content(const int id, const char *value);

/**
 * @brief Sets the favicon of an item matched with the given id
 * @remarks The color space of the image is ARGB8888
 * @param [in] id The id of an item
 * @param [in] width The width of the image
 * @param [in] height The height of the image
 * @param [in] value The raw data of the image
 * @param [in] length The size of the raw data
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_set_icon(const int id, const int width, const int height, const unsigned char *value, const int length);


/**
 * @brief Sets the snapshot of an item matched with the given id
 * @remarks The color space of the image is ARGB8888
 * @param [in] id The id of an item
 * @param [in] width The width of the image
 * @param [in] height The height of the image
 * @param [in] value The raw data of the image
 * @param [in] length The size of the raw data
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_scrap_adaptor_set_snapshot(const int id, const int width, const int height, const unsigned char *value, const int length);


/**
 * @brief Creates a new item
 * @details The creation time is set to the present time automatically
 * @param [out] id The id of created item. The caller can handle the way to generate id value with initializing this param before calling.
 * 				If caller pass negative value such as -1, provider will generate id value automatically, otherwise it will try to use passed value as an id of new item.
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 * @see bp_scrap_adaptor_delete()
 */
EXPORT_API int bp_scrap_adaptor_create(int *id);


/**
 * @brief Deletes an item
 * @param [in] id The id of an item to delete
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 * @see bp_scrap_adaptor_create()
 */
EXPORT_API int bp_scrap_adaptor_delete(const int id);


/**
 * @brief Gets the error code after function returned -1
 * @return The last error code which occurred
 */
EXPORT_API int bp_scrap_adaptor_get_errorcode(void);


/**
 * @brief Creates a new item or updates it using the bp_scrap_info_fmt structure
 * @param [out] id The id of created item. The caller can handle the way to generate id value with initializing this param before calling.
 * 				If caller pass negative value such as -1, provider will generate id value automatically, otherwise it will try to use passed value as an id of new item.
 * @param [in] info The structure contains all properties that caller want to set
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 * @pre The structure has to be initialized to 0, then use the variables call to change them,\n
 * e.g. info->title to set the title
 * @see bp_scrap_info_fmt
 * @see bp_scrap_adaptor_create()
 */
EXPORT_API int bp_scrap_adaptor_easy_create(int *id, bp_scrap_info_fmt *info);


/**
 * @brief Gets one or more properties indicated by offset mask of an item matched with the given id
 * @remarks Allocated memory of strings in the structure has to be released by the caller
 * @param [in] id The id of an item
 * @param [in] offset Indicates one or more properties by '|' bit mask
 * @param [in] info The structure contains all properties
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 * @pre The structure has to be initialized to 0, then use the variables call to change them,\n
 * e.g. info->title to set the title
 * @see bp_scrap_offset
 * @see bp_scrap_info_fmt
 */
EXPORT_API int bp_scrap_adaptor_get_info(const int id, const bp_scrap_offset offset, bp_scrap_info_fmt *info);


/**
 * @brief Clears memory allocated by the structure
 * @param [in] info The structure to clear
 * @return 0 on success
 * @see bp_scrap_info_fmt
 * @see bp_scrap_adaptor_get_info()
 */
EXPORT_API int bp_scrap_adaptor_easy_free(bp_scrap_info_fmt *info);


/**
 * @brief Gets the array of ids and the number of rows of items searched by keyword and some structure from the storage
 * @remarks Allocated memory (ids) has to released by the caller.\n
 * 			If the caller wants to do LIKE searching, keyword has to have some wild card like _aa, a_a, %aa, aa% or %aa%.
 * @param [out] ids The array of ids
 * @param [out] count The array size
 * @param [in] conds The structure contains some conditions related to the period, the order and the number of searched rows (if NULL, ignore)
 * @param [in] check_offset The property to search by the keyword, BP_SCRAP_O_TITLE, BP_SCRAP_O_URL or (BP_SCRAP_O_TITLE | BP_SCRAP_O_URL)
 * @param [in] keyword The string to find (NULL ignore searching by the keyword)
 * @param [in] is_like The way of searching, 1 means LIKE, 0 means EQUAL and negative value means NOT EQUAL
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 * @pre The structure has to be initialized to 0, then use the variables call to change them,\n
 * e.g. info->title to set the title
 * @see bp_scrap_offset
 * @see bp_scrap_rows_cond_fmt
 */
EXPORT_API int bp_scrap_adaptor_get_cond_ids_p
	(int **ids, int *count,
	bp_scrap_rows_cond_fmt *conds,
	const bp_scrap_offset check_offset,
	const char *keyword,
	const int is_like);


/**
 * @brief Gets the array ids and the number of rows of items searched in title or url property by keyword from the storage
 * @remarks Allocated memory (ids) has to be released by the caller.\n
 * 			If the caller want to do LIKE searching, keyword has to have some wild card like _aa, a_a, %aa, aa% or %aa%
 * @param [out] ids The array of ids
 * @param [out] count The array size
 * @param [in] limit The maximum number of rows to get, negative means no limitation
 * @param [in] offset The starting row index
 * @param [in] order_column_offset A property on which the rows will be sorted, default BP_SCRAP_O_DATE_CREATED
 * @param [in] ordering The way of ordering, 0:ASC(default), 1:DESC
 * @param [in] keyword The string to find
 * @param [in] is_like The way of searching, 1 means LIKE, 0 means EQUAL and negative means NOT EQUAL
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 * @see bp_scrap_offset
 * @see bp_scrap_adaptor_get_cond_ids_p()
 */
EXPORT_API int bp_scrap_adaptor_get_inquired_ids_p
	(int **ids, int *count, const int limit, const int offset,
	const bp_scrap_offset order_column_offset, const int ordering,
	const char *keyword, const int is_like);


/**
 * @brief Gets the array ids and the number of rows of items searched by keyword from the storage
 * @remarks allocated memory (ids) have to released by caller\n
 * 			if caller want to do LIKE searching, keyword have to have some wild card like _aa, a_a, %aa, aa% or %aa%
 * @param [out] ids The array of ids
 * @param [out] count The array size
 * @param [in] limit The maximum number of rows to get, negative means no limitation
 * @param [in] offset The starting row index
 * @param [in] order_column_offset A property on which the rows will be sorted, default BP_HISTORY_O_DATE_CREATED
 * @param [in] ordering The way of ordering, 0:ASC(default), 1:DESC
 * @param [in] check_column_offset A property to search by the keyword, BP_HISTORY_O_TITLE, BP_HISTORY_O_URL or (BP_HISTORY_O_TITLE | BP_HISTORY_O_URL)
 * @param [in] keyword The string to find
 * @param [in] is_like The way of searching, 1 means LIKE, 0 means EQUAL and negative means NOT EQUAL
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 * @see bp_scrap_offset
 * @see bp_scrap_adaptor_get_cond_ids_p()
 */
EXPORT_API int bp_scrap_adaptor_get_duplicated_ids_p
	(int **ids, int *count, const int limit, const int offset,
	const bp_scrap_offset order_column_offset, const int ordering,
	const bp_scrap_offset check_column_offset,
	const char *keyword, const int is_like);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __TIZEN_WEB_SCRAP_H__ */
