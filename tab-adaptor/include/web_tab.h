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

#ifndef __TIZEN_WEB_TAB_H__
#define __TIZEN_WEB_TAB_H__

#ifndef EXPORT_API
#define EXPORT_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @internal
 * @addtogroup CAPI_WEB_TAB_MODULE
 * @{
 */


/**
 * @brief Each offset mask corresponds to each property of the storage, respectively.
 * @remarks The caller can set one or more properties by '|' bit masking.
 */
typedef unsigned short bp_tab_offset;
/**
 * @brief The tab index
 */
#define BP_TAB_O_INDEX 0x0001
/**
 * @brief The 'is activated' flag
 */
#define BP_TAB_O_IS_ACTIVATED 0x0002
/**
 * @brief The 'is incognito' flag
 */
#define BP_TAB_O_IS_INCOGNITO 0x0004
/**
 * @brief The browser instance
 */
#define BP_TAB_O_BROWSER_INSTANCE 0x0008
/**
 * @brief The creation time
 */
#define BP_TAB_O_DATE_CREATED 0x0010
/**
 * @brief The time of the last modification
 */
#define BP_TAB_O_DATE_MODIFIED 0x0020
/**
 * @brief The url address of the web-site
 */
#define BP_TAB_O_URL 0x0040
/**
 * @brief the title of the web-site
 */
#define BP_TAB_O_TITLE 0x0080
/**
 * @brief The account name for sync
 */
#define BP_TAB_O_ACCOUNT_NAME 0x0100
/**
 * @brief The account type for sync
 */
#define BP_TAB_O_ACCOUNT_TYPE 0x0200
/**
 * @brief The device name
 */
#define BP_TAB_O_DEVICE_NAME 0x0400
/**
 * @brief The device id
 */
#define BP_TAB_O_DEVICE_ID 0x0800
/**
 * @brief The usage is not support yet
 */
#define BP_TAB_O_USAGE 0x1000
/**
 * @brief The favicon
 */
#define BP_TAB_O_ICON 0x2000
/**
 * @brief The snapshot (thumbnail)
 */
#define BP_TAB_O_SNAPSHOT 0x4000
/**
 * @brief The extra property for sync
 */
#define BP_TAB_O_SYNC 0x8000
/**
 * @brief The all properties flag
 */
#define BP_TAB_O_ALL 0xffff


/**
 * @brief Enumeration of error codes
 * @details Gives information why function has failed
 */
typedef enum {
	BP_TAB_ERROR_NONE = 10,			/**< success, no error */
	BP_TAB_ERROR_INVALID_PARAMETER,	/**< wrong parameterpassed */
	BP_TAB_ERROR_OUT_OF_MEMORY,		/**< failed to allocate the memory */
	BP_TAB_ERROR_IO_ERROR,			/**< some I/O error in communication with the provider process */
	BP_TAB_ERROR_NO_DATA,			/**< no data is retrieved */
	BP_TAB_ERROR_ID_NOT_FOUND,		/**< passed the id is not exist in the storage */
	BP_TAB_ERROR_DUPLICATED_ID,		/**< try to create the id which already exists */
	BP_TAB_ERROR_PERMISSION_DENY,	/**< denied by user space smack rule */
	BP_TAB_ERROR_DISK_BUSY,			/**< failed to write or read (make cache) by disk busy error */
	BP_TAB_ERROR_DISK_FULL,			/**< failed to write by disk full error */
	BP_TAB_ERROR_TOO_BIG_DATA,		/**< tried to write too big data, the maximum length of the string is 4K */
	BP_TAB_ERROR_UNKNOWN			/**< unknown error */
} bp_tab_error_defs;



/**
 * @brief The structure containing the variables for all properties
 * @remarks The caller has access only to variables which are related to the bp_history_offset. \n
 * 			The caller has to initialize the structure before using it.\n
 * 			The caller has to free the memory of the structure allocated by bp_tab_adaptor_get_info().
 */
typedef struct {
	int index;					/**< The index of the tab, otherwise ignore this variable by setting to negative */
	int is_activated;			/**< 1 means activated and 0 means deactivated, otherwise ignore this variable by setting to negative */
	int is_incognito;			/**< 1 means incognito and 0 - the opposite, ignore this variable by setting to negative */
	int browser_instance;		/**< 0 means default browser, otherwise multi instance browser, ignore this variable by setting to negative */
	int dirty;					/**< deprecated */
	int date_created;			/**< the creation time; ignore this variable by setting to zero */
	int date_modified;			/**< the time of the last modification; ignore this variable by setting to zero */
	int favicon_length;			/**< the favicon length; ignore favicon by setting to zero */
	int favicon_width;          /**< the favicon width */
	int favicon_height;         /**< the favicon height */
	int thumbnail_length;		/**< ignore favicon by setting to zero */
	int thumbnail_width;        /**< the thumbnail width */
	int thumbnail_height;       /**< the tumbnail height */
	char *url;                  /**< the url */
	char *title;                /**< the title */
	char *account_name;         /**< the account name */
	char *account_type;         /**< the account type */
	char *device_name;          /**< the device name */
	char *device_id;            /**< the device id */
	char *usage;				/**< not support yet */
	char *sync;					/**< extra property for sync */
	unsigned char *favicon;		/**< row data of icon ( favorites icon ) */
	unsigned char *thumbnail;	/**< row data of snapshot */
} bp_tab_info_fmt;


/**
 * @brief Initializes some resources in order to use tab functions
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 * @see bp_tab_adaptor_deinitialize()
 */
EXPORT_API int bp_tab_adaptor_initialize(void);


/**
 * @brief Frees all resources allocated by bp_tab_adaptor_initialize()
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 * @see bp_tab_adaptor_initialize()
 */
EXPORT_API int bp_tab_adaptor_deinitialize(void);


/**
 * @brief Called when other client change some tabs
 * @param[in] user_data passed to bp_tab_adaptor_set_data_changed_cb()
 * @see bp_tab_adaptor_set_data_changed_cb()
 * @see bp_tab_adaptor_unset_data_changed_cb()
 */
typedef void (*bp_tab_adaptor_data_changed_cb)(void *user_data);


/**
 * @brief Register the callback function called when other client changes some tabs
 * @param[in] callback The callback function to invoke
 * @param[in] user_data The data passed to the callback function
 * @see bp_tab_adaptor_is_setted_data_changed_cb()
 * @see bp_tab_adaptor_unset_data_changed_cb()
 */
EXPORT_API int bp_tab_adaptor_set_data_changed_cb(bp_tab_adaptor_data_changed_cb callback, void *user_data);


/**
 * @brief Unsets the callback function passed in bp_tab_adaptor_set_data_changed_cb()
 * @param[in] callback The callback function passed in bp_tab_adaptor_set_data_changed_cb()
 * @see bp_tab_adaptor_is_setted_data_changed_cb()
 * @see bp_tab_adaptor_set_data_changed_cb()
 */
EXPORT_API int bp_tab_adaptor_unset_data_changed_cb(bp_tab_adaptor_data_changed_cb callback);


/**
 * @brief Checks whether the callback function is already registered or not
 * @return 0 on success, otherwise a negative error value
 * @see bp_tab_adaptor_set_data_changed_cb()
 * @see bp_tab_adaptor_unset_data_changed_cb()
 */
EXPORT_API int bp_tab_adaptor_is_setted_data_changed_cb(void);


/**
 * @brief Mark an item as updated
 * @param[out] id The id of an item to mark
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_set_dirty(const int id);


/**
 * @brief Mark an item as deleted
 * @remarks if "is_deleted" property for this item is set to true by this function, this item still exists in the storage, but cannot be accessed except bp_tab_adaptor_get_full_with_deleted_ids_p()
 * @param[out] id The id of an item to mark
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 * @see bp_tab_adaptor_get_full_with_deleted_ids_p()
 */
EXPORT_API int bp_tab_adaptor_set_deleted(const int id);


/**
 * @brief Sends the notification to other clients
 * @details The callback function will be invoked if registered
 * @return 0 on success, otherwise -1 is returned
 * @see bp_tab_adaptor_set_data_changed_cb()
 */
EXPORT_API int bp_tab_adaptor_publish_notification(void);


/**
 * @brief Gets the array of ids and the number of rows of all items from the storage
 * @remarks Allocated memory (ids) has to be released by the caller
 * @param[out] ids The array of ids
 * @param[out] count The array size
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_get_full_ids_p(int **ids, int *count);


/**
 * @brief Gets the array of ids and the number of rows of all items with deleted items from the storage
 * @remarks Allocated memory (ids) has to be released by the caller
 * @param[out] ids The array of ids
 * @param[out] count The array size
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_get_full_with_deleted_ids_p(int **ids, int *count);


/**
 * @brief Gets the array of ids and the number of rows of all modified items from the storage
 * @remarks Allocated memory (ids) has to be released by the caller
 * @param[out] ids The array of ids
 * @param[out] count The array size
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_get_dirty_ids_p(int **ids, int *count);


/**
 * @brief Gets the array of ids and the number of rows of all deleted items from the storage
 * @remarks Allocated memory (ids) has to be released by the caller
 * @param[out] ids The array of ids
 * @param[out] count The array size
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_get_deleted_ids_p(int **ids, int *count);


/**
 * @brief Commit all modified tabs
 * @details Clears "is_dirty" property of each item.
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_clear_dirty_ids(void);


/**
 * @brief Ddeletes all tabs having set 'is_deleted' property from the storage
 * @details If cloud is on, "is_dirty" property is off by calling delete function,\n
 * 			if cloud is off, a scrap is deleted really from storage whenever delete function is called.
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_clear_deleted_ids(void);


/**
 * @brief Gets the index of an item matched with the given id
 * @param[in] id The id of the item
 * @param[out] value The index of the tab
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_get_index(const int id, int *value);


/**
 * @brief Gets the url property of an item matched with the given id
 * @param[in] id The id of the item
 * @param[out] value The url string
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_get_url(const int id, char **value);


/**
 * @brief Gets the title property of an item matched with the given id
 * @param[in] id The id of the item
 * @param[out] value The title string
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_get_title(const int id, char **value);


/**
 * @brief Gets the "is_activated" property of an item matched with the given id
 * @param[in] id The id of the item
 * @param[out] value 0 means "activated", 1 means "deactivated"
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_get_activated(const int id, int *value);


/**
 * @brief Gets the "is_incognito" property of an item matched with the given id
 * @param[in] id The id of the item
 * @param[out] value 1 means "incognito", 0 - the opposite
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_get_incognito(const int id, int *value);

/**
 * @brief Gets the "browser_instance" property of an item matched with the given id
 * @param[in] id The id of the item
 * @param[out] instance_id The browser instance ID starting from 0 (which is default)
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_get_browser_instance(const int id, int *instance_id);

/**
 * @brief Gets the time when an item matched with the given id was made
 * @param[in] id The id of the item
 * @param[out] value The time stamp
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_get_date_created(const int id, int *value);


/**
 * @brief Gets the time when an item matched with id was last modified
 * @param[in] id The id of the item
 * @param[out] value The time stamp
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_get_date_modified(const int id, int *value);


/**
 * @brief Gets the account name of an item matched with the given id
 * @remarks Allocated memory has to be released by the caller
 * @param[in] id The id of the item
 * @param[out] value The account name
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_get_account_name(const int id, char **value);


/**
 * @brief Gets the account type of an item matched with the given id
 * @remarks Allocated memory has to be released by the caller
 * @param[in] id The id of the item
 * @param[out] value The account type
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_get_account_type(const int id, char **value);


/**
 * @brief Gets the device name of an item matched with the given id
 * @remarks Allocated memory has to be released by the caller
 * @param[in] id The id of the item
 * @param[out] value The device name
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_get_device_name(const int id, char **value);


/**
 * @brief Gets the device id of an item matched with the given id
 * @remarks Allocated memory has to be released by the caller
 * @param[in] id The id of the item
 * @param[out] value The device id
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_get_device_id(const int id, char **value);


/**
 * @brief Gets the "usage" property of an item matched with the given id
 * @param[in] id The id of the item
 * @param[out] value The value of the "usage" property
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_get_usage(const int id, char **value);


/**
 * @brief Gets extra property for sync of an item matched with the given id
 * @param[in] id The id of the item
 * @param[in] value The value of the extra property for sync
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_get_sync(const int id, char **value);


/**
 * @brief Gets the favicon of an item matched with the given id
 * @details Color space of the image is ARGB8888
 * @remarks The image data value will be stored in shared memory allocated by the browser-provider.
 *			Therefore if this API is called multiple times, image contents may be changed but the pointer will be the same.
 *			PLEASE DO NOT free the 'value' parameter directly after it is assigned.
 *			If you want to use the image data permanently, allocate the memory and make a deep copy of it.
 * @param[in] id The id of the item
 * @param[out] width The width of the image
 * @param[out] height The height of the image
 * @param[out] value A pointer to a raw image data. This memory will be released automatically when the browser provider is deinitialized.
 * @param[out] length The size of the raw data
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_get_icon(const int id, int *width, int *height, unsigned char **value, int *length);


/**
 * @brief Gets the png format favicon of a tab matched with the given id
 * @details Color space of the image is 32bit RGBA
 * @remarks The image data value will be stored in shared memory allocated by the browser-provider.
 *			Therefore if this API is called multiple times, image contents may be changed but the pointer will be the same.
 *			PLEASE DO NOT free the 'value' parameter directly after it is assigned.
 *			If you want to use the image data permanently, allocate the memory and make a deep copy of it.
 * @param[in] id The id of an item
 * @param[out] value A pointer of a raw image data. This memory will be released automatically when the browser provider is deinitialized.
 * @param[out] length The size of the binary
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 * @see bp_tab_adaptor_get_icon()
 */
EXPORT_API int bp_tab_adaptor_get_icon_png(const int id, unsigned char **value, int *length);


/**
 * @brief Gets the snapshot of an item matched with the given id
 * @details Color space of the image is ARGB8888
 * @remarks The image data value will be stored in shared memory allocated by the browser-provider.
 *			Therefore if this API is called multiple times, image contents may be changed but the pointer will be the same.
 *			PLEASE DO NOT free the 'value' parameter directly after it is assigned.
 *			If you want to use the image data permanently, allocate the memory and make a deep copy of it.
 * @param[in] id The id of an item
 * @param[out] width The width of the image
 * @param[out] height The height of the image
 * @param[out] value A pointer of a raw image data. This memory will be released automatically when the browser provider is deinitialized.
 * @param[out] length The size of the raw data
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_get_snapshot(const int id, int *width, int *height, unsigned char **value, int *length);


/**
 * @brief Sets the index of an item matched with the given id
 * @param[in] id The id of the item
 * @param[in] value The index to sort
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_set_index(const int id, const int value);


/**
 * @brief Sets the url string of an item matched with the given id
 * @param[in] id The id of the item
 * @param[in] value The uri address
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_set_url(const int id, const char *value);


/**
 * @brief Sets the title of an item matched with the given id
 * @param[in] id The id of the item
 * @param[in] value The title
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_set_title(const int id, const char *value);


/**
 * @brief Sets the "is_activated" property of an item matched with the given id
 * @param[in] id The id of the item
 * @param[in] value 0 means "activated", otherwise 1 means "deactivated"
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_set_activated(const int id, const int value);


/**
 * @brief Sets the "is_incognito" property of an item matched with the given id
 * @param[in] id The id of an item
 * @param[in] value 1 means "incognito", 0 - the opposite
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_set_incognito(const int id, const int value);

/**
 * @brief Sets the "browser_instance" property of an item matched with the given id
 * @param[in] id The id of the item
 * @param[in] instance_id The browser instance ID starting from 0 (which is default)
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_set_browser_instance(const int id, const int instance_id);

/**
 * @brief Changes creation time of an item matched with the given id
 * @param[in] id The id of the item
 * @param[in] value The time stamp; if @c value <= 0 - update to present time automatically
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_set_date_created(const int id, const int value);


/**
 * @brief Changes the time when an item matched with the given id was last modified
 * @param[in] id The id of the item
 * @param[in] value The time stamp; if @c value <= 0 - update to present time automatically
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_set_date_modified(const int id, const int value);


/**
 * @brief Sets the account name of an item matched with the given id
 * @param[in] id The id of the item
 * @param[in] value The account name
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_set_account_name(const int id, const char *value);


/**
 * @brief Sets the account type of an item matched with the given id
 * @param[in] id The id of the item
 * @param[in] value The account type
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_set_account_type(const int id, const char *value);


/**
 * @brief Sets the device name of an item matched with the given id
 * @param[in] id The id of the item
 * @param[in] value The device name
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_set_device_name(const int id, const char *value);


/**
 * @brief Sets the device id of an item matched with the given id
 * @param[in] id The id of the item
 * @param[in] value The device id
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_set_device_id(const int id, const char *value);


/**
 * @brief Sets the "usage" property of an item matched with the given id
 * @param[in] id The id of the item
 * @param[in] value The device id
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_set_usage(const int id, const char *value);


/**
 * @brief Sets the extra property for sync of an item matched with the given id
 * @param[in] id The id of the item
 * @param[in] value The device id
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_set_sync(const int id, const char *value);


/**
 * @brief Sets the favicon of an item matched with the given id
 * @remarks Color space of the image is ARGB8888
 * @param[in] id The id of the item
 * @param[in] width The width of the image
 * @param[in] height The height of the image
 * @param[in] value The raw data of the image
 * @param[in] length The size of the raw data
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_set_icon(const int id, const int width, const int height, const unsigned char *value, const int length);


/**
 * @brief Sets the png format favicon of a bookmark matched with the given id
 * @param[in] id The id of the item
 * @param[in] value The binary of the image
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 * @see bp_tab_adaptor_set_icon()
 */
EXPORT_API int bp_tab_adaptor_set_icon_png(const int id, const unsigned char *value);


/**
 * @brief Sets the snapshot of an item matched with the given id
 * @remarks Color space of the image is ARGB8888
 * @param[in] id The id of the item
 * @param[in] width The width of the image
 * @param[in] height The height of the image
 * @param[in] value The raw data of the image
 * @param[in] length The size of the raw data
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 */
EXPORT_API int bp_tab_adaptor_set_snapshot(const int id, const int width, const int height, const unsigned char *value, const int length);


/**
 * @brief Creates a new item
 * @details The creation time is set to the present time automatically
 * @param [out] id The id of created item. The caller can handle the way to generate id value with initializing this param before calling.
 * 				If caller pass negative value such as -1, provider will generate id value automatically, otherwise it will try to use passed value as an id of new item.
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 * @see bp_tab_adaptor_delete()
 */
EXPORT_API int bp_tab_adaptor_create(int *id);


/**
 * @brief Deletes an item
 * @param[in] id The id of an item to delete
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 * @see bp_tab_adaptor_create()
 */
EXPORT_API int bp_tab_adaptor_delete(const int id);


/**
 * @brief Gets the error code after function returned -1
 * @return The last error code which occurred
 */
EXPORT_API int bp_tab_adaptor_get_errorcode(void);


/**
 * @brief Creates a new item or update it using the structure
 * @param [out] id The id of created item. The caller can handle the way to generate id value with initializing this param before calling.
 * 				If caller pass negative value such as -1, provider will generate id value automatically, otherwise it will try to use passed value as an id of new item.
 * @param[in] info The structure contains all properties that caller want to set
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 * @pre The structure has to be initialized to 0, then use the variables call to change them,\n
 * e.g. info->title to set the title
 * @see bp_tab_info_fmt
 * @see bp_tab_adaptor_create()
 */
EXPORT_API int bp_tab_adaptor_easy_create(int *id, bp_tab_info_fmt *info);


/**
 * @brief Gets all properties of an item matched with the given id
 * @remarks Allocated memory of strings in the structure has to be released by the caller
 * @param[in] id The id of the item
 * @param[in] info The structure contains all properties
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 * @see bp_tab_info_fmt
 * @see bp_tab_adaptor_get_info()
 */
EXPORT_API int bp_tab_adaptor_get_easy_all(const int id, bp_tab_info_fmt *info);


/**
 * @brief Gets one or more properties indicated by offset mask of an item matched with the given id
 * @remarks Allocated memory of strings in the structure has to be released by the caller
 * @param[in] id The id of the item
 * @param[in] offset Indicates one or more properties by '|' bit masking.
 * @param[in] info The structure contains all properties
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 * @pre The structure has to be initialized to 0, then use the variables call to change them,\n
 * e.g. info->title to set the title
 * @see bp_tab_offset
 * @see bp_tab_info_fmt
 */
EXPORT_API int bp_tab_adaptor_get_info(const int id, const bp_tab_offset offset, bp_tab_info_fmt *info);


/**
 * @brief Clears memory allocated by the structure
 * @param[in] info the structure wanting to be clear
 * @return 0 on success
 * @see bp_tab_info_fmt
 * @see bp_tab_adaptor_get_info()
 * @see bp_tab_adaptor_get_easy_all()
 */
EXPORT_API int bp_tab_adaptor_easy_free(bp_tab_info_fmt *info);


/**
 * @brief Turns on a tab for the given id and turns off others
 * @param[in] The id of an item
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 * @see bp_tab_adaptor_set_activated()
 */
EXPORT_API int bp_tab_adaptor_activate(const int id);


/**
 * @brief Gets the array of ids and the number of rows of items searched by keyword and some structure from the storage
 * @remarks Allocated memory (ids) has to released by the caller.\n
 * 			If the caller wants to do LIKE searching, keyword has to have some wild card like _aa, a_a, %aa, aa% or %aa%.
 * @param[out] ids The array of ids
 * @param[out] count The array size
 * @param[in] limit The maximum number of rows to get, negative value means no limit
 * @param[in] offset The starting row index
 * @param[in] order_column_offset A property on which the rows will be sorted, default BP_TAB_O_DATE_CREATED
 * @param[in] ordering The way of ordering, 0:ASC(default), 1:DESC
 * @param[in] check_column_offset A property to search by the keyword, BP_TAB_O_DEVICE_ID, BP_TAB_O_URL, BP_TAB_O_TITLE or (BP_TAB_O_TITLE | BP_TAB_O_URL)
 * @param[in] keyword The string to find
 * @param[in] is_like The way of searching, 1 means LIKE, 0 means EQUAL and negative means NOT EQUAL
 * @return 0 on success, otherwise -1 is returned and error code is set to indicate the error.
 * @see bp_tab_offset
 */
EXPORT_API int bp_tab_adaptor_get_duplicated_ids_p
	(int **ids, int *count, const int limit, const int offset,
	const bp_tab_offset order_column_offset, const int ordering,
	const bp_tab_offset check_column_offset,
	const char *keyword, const int is_like);


/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __TIZEN_WEB_TAB_H__ */
