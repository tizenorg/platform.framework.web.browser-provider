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

#ifndef __TIZEN_WEB_HISTORY_H__
#define __TIZEN_WEB_HISTORY_H__

#ifndef EXPORT_API
#define EXPORT_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @file web_history.h
 */

/**
 * @addtogroup CAPI_WEB_HISTORY_MODULE
 * @{
 */

/**
 * @brief Each offset mask corresponds to each property of the storage, respectively.
 * @remarks A user can set one or more properties by '|' bit masking.
 */
typedef unsigned short bp_history_offset;
/**
 * @brief Definition for the frequency of use.
 * @since_tizen 2.3
 */
#define BP_HISTORY_O_FREQUENCY 0x0001
/**
 * @brief Definition for the creation time.
 * @since_tizen 2.3
 */
#define BP_HISTORY_O_DATE_CREATED 0x0002
/**
 * @brief Definition for the time of the last modification.
 * @since_tizen 2.3
 */
#define BP_HISTORY_O_DATE_MODIFIED 0x0004
/**
 * @brief Definition for the time of the last use.
 * @since_tizen 2.3
 */
#define BP_HISTORY_O_DATE_VISITED 0x0008
/**
 * @brief Definition for the URI address of the website.
 * @since_tizen 2.3
 */
#define BP_HISTORY_O_URL 0x0010
/**
 * @brief Definition for the title of the website.
 * @since_tizen 2.3
 */
#define BP_HISTORY_O_TITLE 0x0020
/**
 * @brief Definition for the snapshot (thumbnail).
 * @since_tizen 2.3
 */
#define BP_HISTORY_O_SNAPSHOT 0x0400
/**
 * @brief Definition for the snapshot (thumbnail).
 * @since_tizen 2.3
 */
#define BP_HISTORY_O_THUMBNAIL 0x0400
/**
 * @brief Definition for the favicon.
 * @since_tizen 2.3
 */
#define BP_HISTORY_O_ICON 0x0800
/**
 * @brief Definition for the favicon.
 * @since_tizen 2.3
 */
#define BP_HISTORY_O_FAVICON 0x0800
/**
 * @brief Definition for the webicon.
 * @since_tizen 2.3
 */
#define BP_HISTORY_O_WEBICON 0x1000
/**
 * @brief Definition for all properties.
 * @since_tizen 2.3
 */
#define BP_HISTORY_O_ALL 0xffff


/**
 * @brief Enumeration for the search period.
 * @since_tizen 2.3
 */
typedef enum {
	BP_HISTORY_DATE_ALL,			/**< Unlimited */
	BP_HISTORY_DATE_TODAY,			/**< Only today */
	BP_HISTORY_DATE_YESTERDAY,		/**< Only yesterday */
	BP_HISTORY_DATE_LAST_7_DAYS,	/**< From 7 days ago, to the day before yesterday */
	BP_HISTORY_DATE_LAST_MONTH,		/**< This month */
	BP_HISTORY_DATE_OLDER			/**< Until last month */
} bp_history_date_defs;


/**
 * @brief Enumeration for the operators used to search the date property.
 * @details Offset GREATER timestamp AND offset2 LESS_EQUAL timestamp.
 * @since_tizen 2.3
 */
typedef enum {
	BP_HISTORY_OP_NONE = 0,			/**< No operater */
	BP_HISTORY_OP_EQUAL,			/**< Equal ( == ) */
	BP_HISTORY_OP_NOT_EQUAL,		/**< Not equal ( != ) */
	BP_HISTORY_OP_GREATER,			/**< Greater ( > ) */
	BP_HISTORY_OP_LESS,			    /**< Less than ( < ) */
	BP_HISTORY_OP_GREATER_QUEAL,	/**< Greater or equal ( >= ) */
	BP_HISTORY_OP_LESS_QUEAL		/**< Less or equal ( <= ) */
} bp_history_timestamp_op_defs;


/**
 * @brief The structure type containing conditions for searching by timestamp.
 * @since_tizen 2.3
 * @remarks Default offset is BP_HISTORY_O_DATE_CREATED.
 */
typedef struct {
	bp_history_offset offset;			/**< A property to search */
	long timestamp;					    /**< The timestamp to search */
	bp_history_timestamp_op_defs cmp_operator;	/**< The operater between offset and timestamp */
	unsigned conds_operator;			/**< The relation with a previous condition (@c 0: AND, @c 1: OR) */
} bp_history_timestamp_fmt;


/**
 * @brief The structure type containing conditions associated with the ordering and the number of searched rows.
 * @since_tizen 2.3
 * @remarks The default offset is #BP_HISTORY_O_DATE_CREATED.
 */
typedef struct {
	int limit;				           /**< The maximum number of rows to get, negative means no limitaion */
	int offset;				           /**< The first row's index */
	bp_history_offset order_offset;	   /**< A property to sort, default is #BP_HISTORY_O_DATE_CREATED */
	int ordering;				       /**< The way of ordering (@c 0: ASC (default), @c 1: DESC) */
} bp_history_rows_fmt;


/**
 * @brief The structure type containing conditions associated with the period, the ordering, and the number of searched rows.
 * @since_tizen 2.3
 * @remarks The default offset is #BP_HISTORY_O_DATE_CREATED.
 */
typedef struct {
	int limit;				            /**< The maximum number of rows to get, negative means no limitation */
	int offset;				            /**< The first row's index */
	bp_history_offset order_offset;		/**< The property to sort, default is #BP_HISTORY_O_DATE_CREATED */
	int ordering;				        /**< The way of ordering (@c 0: ASC (default), @c 1: DESC) */
	bp_history_offset period_offset;	/**< A property to search by @a period_type, default is #BP_HISTORY_O_DATE_CREATED */
	bp_history_date_defs period_type;	/**< The period to search, default is #BP_HISTORY_DATE_ALL */
} bp_history_rows_cond_fmt;


/**
 * @brief Enumeration for error codes.
 * @details The caller can know why the function failed.
 * @since_tizen 2.3
 */
typedef enum {
	BP_HISTORY_ERROR_NONE = 10,		    /**< Default, no error */
	BP_HISTORY_ERROR_INVALID_PARAMETER,	/**< Passed wrong parameter */
	BP_HISTORY_ERROR_OUT_OF_MEMORY,		/**< Failed to allocate memory */
	BP_HISTORY_ERROR_IO_ERROR,		    /**< I/O error in communication with provider process */
	BP_HISTORY_ERROR_NO_DATA,		    /**< No data is retrieved */
	BP_HISTORY_ERROR_ID_NOT_FOUND,		/**< Passed ID does not exist in the storage */
	BP_HISTORY_ERROR_DUPLICATED_ID,		/**< Try to create an ID that already exists */
	BP_HISTORY_ERROR_PERMISSION_DENY,	/**< Denied by the user space smack rule */
	BP_HISTORY_ERROR_DISK_BUSY,		    /**< Failed to write or read (make cache) due to the disk busy error */
	BP_HISTORY_ERROR_DISK_FULL,		    /**< Failed to write due to the disk full error */
	BP_HISTORY_ERROR_TOO_BIG_DATA,		/**< Tried to write too much data, the maximum length of the string is 4K */
	BP_HISTORY_ERROR_UNKNOWN		    /**< Unknown error */
} bp_history_error_defs;


/**
 * @brief The structure type containing variables for all properties.
 * @since_tizen 2.3
 * @remarks The caller only has access to the variables which are associated with #bp_history_offset. \n
 * 			The caller has to initialize the structure before using it.\n
 * 			The caller has to free the memory of the structure allocated by bp_history_adaptor_get_info().
 */
typedef struct {
	int frequency;			    /**< Frequency of use */
	int date_created;		    /**< Ignore this variable by setting it to zero */
	int date_modified;		    /**< Ignore this variable by setting it to zero */
	int date_visited;		    /**< Ignore this variable by setting it to zero */
	int favicon_length;		    /**< Ignore this favicon by setting it to zero */
	int favicon_width;	        /**< Favicon image's width. Ignore this variable by setting it to zero */
	int favicon_height;	        /**< Favicon image's height. Ignore this variable by setting it to zero */
	int thumbnail_length;		/**< Ignore this thumbnail by setting it to zero */
	int thumbnail_width;	    /**< Thumbnail image's width. Ignore this variable by setting it to zero */
	int thumbnail_height;	    /**< Thumbnail image's height. Ignore this variable by setting it to zero */
	int webicon_length;		    /**< Ignore this webicon by setting it to zero */
	int webicon_width;	        /**< Web icon image's width. Ignore this variable by setting it to zero */
	int webicon_height;	        /**< Web icon image's height. Ignore this variable by setting it to zero */
	char *url;	                /**< Address of the visited site */
	char *title;	            /**< Title of the visited site */
	unsigned char *favicon;		/**< Row data of the icon ( favorites icon ) */
	unsigned char *thumbnail; 	/**< Row data of the snapshot */
	unsigned char *webicon;		/**< Row data of the webicon */
	char *sync;			        /**< Extra property to sync */
} bp_history_info_fmt;

/**
 * @brief Called when another client changes some web histories.
 * @since_tizen 2.3
 * @param[in] user_data The user data to be passed to the callback function
 * @see bp_history_adaptor_set_data_changed_cb()
 * @see bp_history_adaptor_unset_data_changed_cb()
 */
typedef void (*bp_history_adaptor_data_changed_cb)(void *user_data);


/**
 * @brief Initializes the resources in order to use history functions.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @return @c 0 on success, 
 *         otherwise @c -1
 */
EXPORT_API int bp_history_adaptor_initialize(void);


/**
 * @brief Frees all the resources allocated by bp_history_adaptor_initialize().
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @return @c 0 on success, 
 *         otherwise @c -1
 * @see bp_history_adaptor_initialize()
 */
EXPORT_API int bp_history_adaptor_deinitialize(void);


/**
 * @brief Gets the URL string of the item with the specified ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @param[in] id The ID of the item
 * @param[out] value The array containing the URI string value
 * @return @c 0 on success, 
 *         otherwise @c -1
 */
EXPORT_API int bp_history_adaptor_get_url(const long long int id, char **value);


/**
 * @brief Gets the title string of the item with the specified ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @param[in] id The id of the item
 * @param[out] value The array containing the title string value
 * @return @c 0 on success, 
 *         otherwise @c -1
 */
EXPORT_API int bp_history_adaptor_get_title(const long long int id, char **value);


/**
 * @brief Gets the frequency of use of the item with the specified ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @param[in] id The ID of the item
 * @param[out] value The frequency of use
 * @return @c 0 on success, 
 *         otherwise @c -1
 */
EXPORT_API int bp_history_adaptor_get_frequency(const long long int id, int *value);


/**
 * @brief Gets the creation time of the item with the specified ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @param[in] id The ID of the item
 * @param[out] value The timestamp value
 * @return @c 0 on success, 
 *         otherwise @c -1
 */
EXPORT_API int bp_history_adaptor_get_date_created(const long long int id, int *value);


/**
 * @brief Gets the time of the last modification of the item with the specified ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @param[in] id The ID of the item
 * @param[out] value The timestamp value
 * @return @c 0 on success, 
 *         otherwise @c -1
 */
EXPORT_API int bp_history_adaptor_get_date_modified(const long long int id, int *value);


/**
 * @brief Gets the time of the last use of the item with the specified ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @param[in] id The ID of the item
 * @param[out] value The timestamp value
 * @return @c 0 on success, 
 *         otherwise @c -1
 */
EXPORT_API int bp_history_adaptor_get_date_visited(const long long int id, int *value);


/**
 * @brief Gets the favicon of the item with the specified ID.
 * @details The color space used is ARGB8888.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
* @remarks The image data value will be stored in the shared memory allocated by the browser-provider.
 *          Therefore, if this API is called multiple times, image contents may be changed, but the pointer will be the same.
 *          PLEASE DO NOT free the 'value' parameter directly after it is assigned.
 *          If you want to use the image data permanently, allocate memory and make a deep copy of the data.
 * @param[in] id The ID of the item
 * @param[out] width The width of the image
 * @param[out] height The height of the image
 * @param[out] value A pointer to the raw image data; this memory will be released automatically when the browser-provider is deinitialized
 * @param[out] length The size of the raw data
 * @return @c 0 on success, 
 *         otherwise @c -1
 */
EXPORT_API int bp_history_adaptor_get_icon(const long long int id, int *width, int *height, unsigned char **value, int *length);


/**
 * @brief Gets a snapshot of the item with the specified ID.
 * @details The color space used is ARGB8888.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @remarks The image data value will be stored in the shared memory allocated by the browser-provider.
 *          Therefore, if this API is called multiple times, image contents may be changed, but the pointer will be the same.
 *          PLEASE DO NOT free the 'value' parameter directly after it is assigned.
 *          If you want to use the image data permanently, allocate memory and make a deep copy of the data.
 * @param[in] id The ID of the item
 * @param[out] width The width of the image
 * @param[out] height The height of the image
 * @param[out] value A pointer to the raw image data; this memory will be released automatically when the browser provider is deinitialized
 * @param[out] length The size of the raw data
 * @return @c 0 on success, 
 *         otherwise @c -1
 */
EXPORT_API int bp_history_adaptor_get_snapshot(const long long int id, int *width, int *height, unsigned char **value, int *length);


/**
 * @brief Gets the webicon of the item with the specified ID.
 * @details The color space used is ARGB8888.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @remarks The image data value will be stored in the shared memory allocated by the browser-provider.
 *          Therefore, if this API is called multiple times, image contents may be changed but the pointer will be the same.
 *          PLEASE DO NOT free the 'value' parameter directly after it is assigned.
 *          If you want to use the image data permanently, allocate memory and make a deep copy of the data.
 * @param[in] id The ID of the item
 * @param[out] width The width of the image
 * @param[out] height The height of the image
 * @param[out] value A pointer to the raw image data; this memory will be released automatically when the browser-provider is deinitialized
 * @param[out] length The size of the raw data
 * @return @c 0 on success, 
 *         otherwise @c -1
 */
EXPORT_API int bp_history_adaptor_get_webicon(const long long int id, int *width, int *height, unsigned char **value, int *length);


/**
 * @brief Sets the URL string of the item with the specified ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @param[in] id The ID of the item
 * @param[in] value The URI value
 * @return @c 0 on success, 
 *         otherwise @c -1
 */
EXPORT_API int bp_history_adaptor_set_url(const long long int id, const char *value);


/**
 * @brief Sets the title string of the item with the specified ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @param[in] id The ID of the item
 * @param[in] value The title value
 * @return @c 0 on success, 
 *         otherwise @c -1
 */
EXPORT_API int bp_history_adaptor_set_title(const long long int id, const char *value);


/**
 * @brief Sets the frequency of use of the item with the specified ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @param[in] id The ID of the item
 * @param[in] value The frequency of use
 * @return @c 0 on success, 
 *         otherwise @c -1
 */
EXPORT_API int bp_history_adaptor_set_frequency(const long long int id, const int value);


/**
 * @brief Changes the creation time of the item with the specified ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @param[in] id The ID of the item
 * @param[in] value The timestamp (if less than or equal to @c 0, update to current time automatically)
 * @return @c 0 on success, 
 *         otherwise @c -1
 */
EXPORT_API int bp_history_adaptor_set_date_created(const long long int id, const int value);


/**
 * @brief Changes the time of the last modification of the item with the specified ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @param[in] id The ID of the item
 * @param[in] value The timestamp (if less than or equal to @c 0, update to current time automatically)
 * @return @c 0 on success, 
 *         otherwise @c -1
 */
EXPORT_API int bp_history_adaptor_set_date_modified(const long long int id, const int value);


/**
 * @brief Changes the time of the last use of the item with the specified ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @param[in] id The ID of the item
 * @param[in] value The timestamp (if less than or equal to @c 0, update to current time automatically)
 * @return @c 0 on success, 
 *         otherwise @c -1
 */
EXPORT_API int bp_history_adaptor_set_date_visited(const long long int id, const int value);


/**
 * @brief Sets the favicon of the item with the specified ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @remarks The color space used is ARGB8888.
 * @param[in] id The ID of the item
 * @param[in] width The width of the image
 * @param[in] height The height of the image
 * @param[in] value The raw data of the image
 * @param[in] length The size of the raw data
 * @return @c 0 on success, 
 *         otherwise @c -1
 */
EXPORT_API int bp_history_adaptor_set_icon(const long long int id, const int width, const int height, const unsigned char *value, const int length);


/**
 * @brief Sets the snapshot of the item with the specified ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @remarks The color space used is ARGB8888.
 * @param[in] id The ID of the item
 * @param[in] width The width of the image
 * @param[in] height The height of the image
 * @param[in] value The raw data of the image
 * @param[in] length The size of the raw data
 * @return @c 0 on success, 
 *         otherwise @c -1
 */
EXPORT_API int bp_history_adaptor_set_snapshot(const long long int id, const int width, const int height, const unsigned char *value, const int length);


/**
 * @brief Sets the webicon of the item with the specified ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @remarks The color space used is ARGB8888.
 * @param[in] id The ID of the item
 * @param[in] width The width of the image
 * @param[in] height The height of the image
 * @param[in] value The raw data of the image
 * @param[in] length The size of the raw data
 * @return @c 0 on success, 
 *          otherwise @c -1
 */
EXPORT_API int bp_history_adaptor_set_webicon(const long long int id, const int width, const int height, const unsigned char *value, const int length);



/**
 * @brief Creates a new item.
 * @details The creation time is set to the current time automatically.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @param[out] id The ID of the new item (should be a positive unique value)
 * @return @c 0 on success, 
 *         otherwise @c -1
 * @see bp_history_adaptor_delete()
 */
EXPORT_API int bp_history_adaptor_create(int *id);


/**
 * @brief Deletes an item.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @param[in] id The ID of the item
 * @return @c 0 on success,
 *         otherwise @c -1
 * @see bp_history_adaptor_create()
 */
EXPORT_API int bp_history_adaptor_delete(const long long int id);


/**
 * @brief Increases the frequency of use and updates the time of the last use.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @param[in] id The ID of the item
 * @return @c 0 on success, 
 *         otherwise @c -1
 * @see bp_history_adaptor_set_frequency()
 * @see bp_history_adaptor_set_date_visited()
 */
EXPORT_API int bp_history_adaptor_visit(const long long int id);


/**
 * @brief Maintains the size of the rows with the @a size parameter after sorting.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @param [in] size The size
 * @param [in] order_offset A property to be sorted, default is #BP_HISTORY_O_DATE_CREATED
 * @param [in] ordering The way of ordering, @c 0: ASC(default) @c 1: DESC
 * @return @c 0 on success, 
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 * @see #bp_history_offset
 */
EXPORT_API int bp_history_adaptor_limit_size(const int size, const bp_history_offset order_column_offset, const int ordering);


/**
 * @brief Gets the error code indicating the error when the function returns @c -1.
 * @since_tizen 2.3
 * @return The error code that indicates the error that is set in the end
 */
EXPORT_API int bp_history_adaptor_get_errorcode(void);


/**
 * @brief Creates the new item or updates the existing one.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @param[out] id The ID of the new item (should be a positive unique value)
 * @param[in] info The structure contains all properties to set
 * @return @c 0 on success, 
 *         otherwise @c -1
 * @pre First the structure has to be initialized to @c 0, then set the variables to change.
 * @see #bp_history_info_fmt
 * @see bp_history_adaptor_create()
 */
EXPORT_API int bp_history_adaptor_easy_create(int *id, bp_history_info_fmt *info);


/**
 * @brief Gets one or more properties indicated by the offset mask of the item with the specified ID.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @remarks The allocated memory of strings in the structure has to be released by the caller.
 * @param[in] id The ID of the item
 * @param[in] offset The value that indicates one or more properties by '|' bit masking
 * @param[in] info The structure containing all properties
 * @return @c 0 on success, 
 *         otherwise @c -1
 * @pre First the structure has to be initialized to @c 0, then set the variables to change.
 * @see #bp_history_offset
 * @see #bp_history_info_fmt
 */
EXPORT_API int bp_history_adaptor_get_info(const long long int id, const bp_history_offset offset, bp_history_info_fmt *info);


/**
 * @brief Frees the memory allocated in the structure.
 * @since_tizen 2.3
 * @param[in] info The structure to clear
 * @return @c 0 on success
 * @see #bp_history_info_fmt
 * @see bp_history_adaptor_get_info()
 */
EXPORT_API int bp_history_adaptor_easy_free(bp_history_info_fmt *info);


/**
 * @brief Clears the entire history.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @return @c 0 on success, 
 *         otherwise @c -1
*/
EXPORT_API int bp_history_adaptor_reset(void);


/**
 * @brief Gets the number of rows of items specified with the given @a date_type.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @param[out] count The number of rows of items
 * @param[in] date_column_offset The property to search by the given @a date_type
 * @param[in] date_type The period to search
 * @return @c 0 on success,
 *         otherwise @c -1
 * @see #bp_history_offset
 * @see #bp_history_date_defs
 */
EXPORT_API int bp_history_adaptor_get_date_count
	(int *count, const bp_history_offset date_column_offset, const bp_history_date_defs date_type);

/**
 * @brief Gets an ID array and the number of rows of items searched by the given keyword and structure.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @remarks The allocated memory (IDs) has to be released by the caller.\n
 * 		    If a caller would like to do LIKE searching, the keyword has to have a wild card like _aa, a_a, %aa, aa%, or %aa%.
 * @param[out] ids The ID array
 * @param[out] count The size of the array
 * @param[in] conds The structure containing conditions bound with the period, the ordering, and the number of searched rows
 * @param[in] check_offset The property to search by keyword (#BP_HISTORY_O_TITLE, #BP_HISTORY_O_URL, or (#BP_HISTORY_O_TITLE | #BP_HISTORY_O_URL))
 * @param[in] keyword The string to search (@c NULL value ignores searching by the keyword)
 * @param[in] is_like The way of searching (@c 1 means LIKE, @c 0 means EQUAL, and negative means NOT EQUAL)
 * @return @c 0 on success, 
 *         otherwise @c -1
 * @pre First the structure has to be initialized to @c 0, then set the variables to change.
 * @see #bp_history_offset
 * @see #bp_history_rows_cond_fmt
 * @see bp_history_adaptor_get_timestamp_ids_p()
 */
EXPORT_API int bp_history_adaptor_get_cond_ids_p
	(int **ids, int *count,
	bp_history_rows_cond_fmt *conds,
	const bp_history_offset check_offset,
	const char *keyword,
	const int is_like);


/**
 * @brief Gets an ID array and the number of rows of items searched by the given keyword, timestamp, and structure.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @remarks The allocated memory (IDs) has to be released by the caller.
 * 		    If a caller would like to do LIKE searching, the keyword has to have a wild card like _aa, a_a, %aa, aa%, or %aa%.
 * @param[out] ids The ID array
 * @param[out] count The size of the array
 * @param[in] limits The structure containing conditions bound with the period, the ordering, and the number of searched rows
 * @param[in] times The structure containing conditions for searching by the timestamp
 * @param[in] times_count The size of the times array
 * @param[in] check_offset The property to search by the given keyword (#BP_HISTORY_O_TITLE, #BP_HISTORY_O_URL, or (#BP_HISTORY_O_TITLE | #BP_HISTORY_O_URL))
 * @param[in] keyword The string to search (@c NULL value ignores searching by the keyword)
 * @param[in] is_like The way of searching (@c 1 means LIKE, @c 0 means EQUAL, and negative means NOT EQUAL)
 * @return @c 0 on success, 
 *         otherwise @c -1
 * @pre First the structure has to be initialized to @c 0, then set the variables to change.
 * @see #bp_history_offset
 * @see #bp_history_rows_fmt
 * @see #bp_history_timestamp_fmt
 * @see bp_history_adaptor_get_cond_ids_p()
 */
EXPORT_API int bp_history_adaptor_get_timestamp_ids_p
	(int **ids, int *count,
	const bp_history_rows_fmt *limits,
	const bp_history_timestamp_fmt times[], const int times_count,
	const bp_history_offset check_offset,
	const char *keyword,
	const int is_like);


/**
 * @brief Gets an ID array and the number of rows of items searched by the given keyword and structure.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @remarks The allocated memory (IDs) has to be released by the caller.\n
 * 		    If a caller would like to do LIKE searching, the keyword has to have a wild card like _aa, a_a, %aa, aa%, or %aa%.
 * @param[out] ids The ID array
 * @param[out] count The size of the array
 * @param[in] conds The structure containing conditions bound with the period, the ordering, and the number of searched rows
 * @param[in] check_offset The property to search by the given keyword (#BP_BOOKMARK_O_TITLE, #BP_BOOKMARK_O_URL or (#BP_BOOKMARK_O_TITLE | #BP_BOOKMARK_O_URL))
 * @param[in] keyword The string to search (@c NULL value ignores searching by the keyword)
 * @param[in] is_like The way of searching (@c 1 means LIKE, @c 0 means EQUAL, and negative means NOT EQUAL)
 * @return @c 0 on success, 
 *         otherwise @c -1
 * @pre First the structure has to be initialized to @c 0, then set the variables to change.
 * @see #bp_history_offset
 * @see #bp_history_rows_cond_fmt
 */
EXPORT_API int bp_history_adaptor_get_raw_retrieved_ids_p
	(int **ids, int *count,
	bp_history_rows_cond_fmt *conds,
	const bp_history_offset check_offset,
	const char *keyword,
	const int is_like);


/**
 * @brief Registers the callback function called when another client changes some bookmarks.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @param[in] callback  The function address of the callback function to be called
 * @param[in] user_data  The user data to be passed to the callback function
 * @return @c 0 on success,
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 * @see bp_history_adaptor_is_setted_data_changed_cb()
 * @see bp_history_adaptor_unset_data_changed_cb()
 */
EXPORT_API int bp_history_adaptor_set_data_changed_cb
	(bp_history_adaptor_data_changed_cb callback, void *user_data);


/**
 * @brief Unsets the callback function passed to bp_history_adaptor_set_data_changed_cb().
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @param[in] callback  The function address of the callback function to be called
 * @return @c 0 on success,
 *         otherwise @c -1 is returned and the error code is set to indicate the error
 * @see bp_history_adaptor_is_setted_data_changed_cb()
 * @see bp_history_adaptor_set_data_changed_cb()
 */
EXPORT_API int bp_history_adaptor_unset_data_changed_cb
	(bp_history_adaptor_data_changed_cb callback);


/**
 * @brief Sends the notification to other clients.
 * @since_tizen 2.3
 * @privlevel platform
 * @privilege %http://tizen.org/privilege/web-history.admin
 * @details The callback function is invoked if already registered.
 * @return @c 0 on success,
 *         otherwise @c -1 is returned
 * @see bp_history_adaptor_set_data_changed_cb()
 */
EXPORT_API int bp_history_adaptor_publish_notification(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __TIZEN_WEB_HISTORY_H__ */
