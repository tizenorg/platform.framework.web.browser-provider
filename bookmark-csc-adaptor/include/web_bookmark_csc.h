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

#ifndef __TIZEN_WEB_BOOKMARK_CSC_H__
#define __TIZEN_WEB_BOOKMARK_CSC_H__

#ifndef EXPORT_API
#define EXPORT_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @file web_bookmark_csc.h
 */

/**
 * @internal
 * @addtogroup CAPI_WEB_BOOKMARK_CSC_MODULE
 * @{
 */

/**
 * @brief Enumeration for error codes of Bookmark CSC.
 * @details Caller can check why function has failed.
 */
typedef enum {
	BOOKMARK_CSC_ERROR_NONE = 10,			/**< Default, no error */
	BOOKMARK_CSC_ERROR_INVALID_PARAMETER,	/**< Passed wrong parameter */
	BOOKMARK_CSC_ERROR_OUT_OF_MEMORY,		/**< Failed to allocate the memory */
	BOOKMARK_CSC_ERROR_IO_ERROR,			/**< Some I/O error in communication with provider process */
	BOOKMARK_CSC_ERROR_NO_DATA,				/**< No data retrieved */
	BOOKMARK_CSC_ERROR_ID_NOT_FOUND,		/**< Passed id does not exist in the storage */
	BOOKMARK_CSC_ERROR_DUPLICATED_ID,		/**< Passed id already exist in the storage */
	BOOKMARK_CSC_ERROR_PERMISSION_DENY,		/**< Denied by user space smack rule */
	BOOKMARK_CSC_ERROR_DISK_BUSY,			/**< Failed to write or read (make cache), disk busy error */
	BOOKMARK_CSC_ERROR_DISK_FULL,			/**< Failed to write, disk full error */
	BOOKMARK_CSC_ERROR_TOO_BIG_DATA,		/**< Tried to write too big data, the maximum length of string is 4K */
	BOOKMARK_CSC_ERROR_UNKNOWN				/**< Unknown error */
} bookmark_csc_error_defs;


/**
 * @brief Enumeration for the kinds of item.
 */
typedef enum {
	BOOKMARK_CSC_TYPE_BOOKMARK = 0,	/**< Bookmark type data for embedded  */
	BOOKMARK_CSC_TYPE_FOLDER = 1	/**< Folder type data */
} bookmark_csc_type_defs;


/**
 * @brief The structure including the variables for all properties.
 * @remarks Caller has to initialize the structure before using it.\n
 * 			Caller has to free the memory allocated for the structure by bookmark_csc_get_info().
 */
typedef struct {
	bookmark_csc_type_defs type;	/**< Type 0:bookmark, 1:folder */
	unsigned parent;				/**< Parent id */
	unsigned editable;				/**< Is editable 0:editable, 1:read only */
	char *title;	/**< Title of bookmark or folder */
	char *url;	/**< Address of bookmark. NULL if the type is folder. */
} bookmark_csc_info_fmt;



/**
 * @brief Initializes resources in order to use bookmark functions.
 * @return 0 on success, otherwise, -1 is returned and error code is set to indicate the error
 * @see bookmark_csc_deinitialize()
 */
EXPORT_API int bookmark_csc_initialize(void);



/**
 * @brief Frees all resources allocated by bp_bookmark_adaptor_initialize.
 * @return 0 on success, otherwise, -1 is returned and error code is set to indicate the error
 * @see bookmark_csc_initialize()
 */
EXPORT_API int bookmark_csc_deinitialize(void);


/**
 * @brief Gets the id of the root level folder.
 * @param[out] id Integer id value of root level folder
 * @return 0 on success, otherwise, -1 is returned if the address of param is NULL
 */
EXPORT_API int bookmark_csc_get_root(int *id);


/**
 * @brief Creates or updates item using the structure.
 * @remarks If type in the info structure is 1 (folder), ignore URL.
 * @param[out] id before calling, returned id of an item created newly if id is initialized to -1
 * 				otherwise, new item is created with id if id is positive unique value
 * @param[in] info The structure that includes all properties that caller want to set
 * @return 0 on success, otherwise, -1 is returned and error code is set to indicate the error
 * @pre The structure have to be initialized to 0, then use the variables that caller want to change.
 * @see bookmark_csc_info_fmt
 */
EXPORT_API int bookmark_csc_create(int *id, bookmark_csc_info_fmt *info);


/**
 * @brief Deletes an item.
 * @remarks If this item is a folder, all child items are deleted too.
 * @param[in] id Id of an item
 * @return 0 on success, otherwise, -1 is returned and error code is set to indicate the error
 */
EXPORT_API int bookmark_csc_delete(const long long int id);


/**
 * @brief Gets an id array and the number of rows of all bookmarks made by csc from the storage.
 * @remarks Allocated memory (ids) have to be released by caller.
 * @param[in] type The type of item, 0 means bookmark, 1 means folder, negative means both
 * @param[out] ids Id array
 * @param[out] count The size of array
 * @return 0 on success, otherwise, -1 is returned and error code is set to indicate the error
 * @see bookmark_csc_type_defs
 */
EXPORT_API int bookmark_csc_get_full_ids_p(bookmark_csc_type_defs type, int **ids, int *count);


/**
 * @brief Gets an id array and the number of rows of bookmarks that belong to the parent from the storage.
 * @remarks Allocated memory (ids) have to be released by caller.
 * @param[in] parent Id of parent folder, negative means searching in all folders
 * @param[out] ids Id array
 * @param[out] count The size of array
 * @return 0 on success, otherwise, -1 is returned and error code is set to indicate the error
 * @see bookmark_csc_type_defs
 */
EXPORT_API int bookmark_csc_get_ids_p(int parent, int **ids, int *count);


/**
 * @brief Gets all properties of an item with the given id.
 * @remarks Memory allocated for strings in the structure have to be released by caller.
 * @param[in] id Id of an item
 * @param[in] info The structure includes all properties
 * @return 0 on success, otherwise, -1 is returned and error code is set to indicate the error
 * @see bookmark_csc_info_fmt
 */
EXPORT_API int bookmark_csc_get_info(const long long int id, bookmark_csc_info_fmt *info);


/**
 * @brief Clears memory allocated in the structure.
 * @param[in] info The structure to be cleared
 * @return 0 on success
 * @see bookmark_csc_info_fmt
 * @see bookmark_csc_get_info()
 */
EXPORT_API int bookmark_csc_free(bookmark_csc_info_fmt *info);


/**
 * @brief Clears all items besides read only items.
 * @return 0 on success, otherwise, -1 is returned and error code is set to indicate the error
 */
EXPORT_API int bookmark_csc_reset(void);

/**
 * @brief Returns error code indicating the error when function returned -1.
 * @return error code indicating the error which is set in the end
 */
EXPORT_API int bookmark_csc_get_errorcode(void);


/**
 * @brief Gets an id array and the number of rows of items searched by title from the storage.
 * @remarks Allocated memory (ids) have to be released by caller.
 * 			If caller want to do LIKE searching, keyword have to have some wild card like _aa, a_a, %aa, aa% or %aa%.
 * @param[out] ids Id array
 * @param[out] count The size of array
 * @param[in] limit The maximum number of rows wanted to be returned, negative means no limitation
 * @param[in] offset Starting index wanted to get among the entire rows
 * @param[in] parent Id of the parent folder, negative means searching in all folders
 * @param[in] type The type of items wanted to be searched for, 0 means bookmark, 1 means folder and negative means both
 * @param[in] is_operator 0:made by user 1:by operator, negative means both
 * @param[in] is_editable 0 means read only, 1 means editable, negative means both
 * @param[in] ordering The way of ordering, 0:ASC(default) 1:DESC
 * @param[in] title Wanted to be searched for
 * @param[in] is_like Ordering the way of searching, 1 means LIKE, 0 means EQUAL and negative means NOT EQUAL
 * @return 0 on success, otherwise, -1 is returned and error code is set to indicate the error
 */
EXPORT_API int bookmark_csc_get_duplicated_title_ids_p
	(int **ids, int *count, const int limit, const int offset,
	const int parent, const int type,
	const int is_operator, const int is_editable,
	const int ordering,
	const char *title, const int is_like);


/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __TIZEN_WEB_BOOKMARK_CSC_H__ */
