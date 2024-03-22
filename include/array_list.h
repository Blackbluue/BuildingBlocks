#ifndef ARRAY_LIST_H
#define ARRAY_LIST_H

#include "buildingblocks.h"
#include <unistd.h>

/* DATA */

/**
 * @brief A pointer to a user-defined free function.
 *
 * This is used to free memory allocated for list data. For simple data types,
 * this is just a pointer to the standard free function. More complex structs
 * stored in lists may require a function that calls free on multiple
 * components.
 */
typedef void (*FREE_F)(void *to_free);

/**
 * @brief A user-defined compare function.
 *
 * Positive return value indicates that the first argument is greater than the
 * second argument. Negative return value indicates that the first argument is
 * less than the second argument. Zero return value indicates that the first
 * argument is equal to the second argument.
 */
typedef int (*CMP_F)(const void *value_to_find, const void *node_data);

/**
 * @brief A user-defined function that gets called in the foreach.
 *
 * The data pointer can be use to pass in any additional data that the user may
 * need. If no additional data is needed, then the data pointer can be NULL.
 * The return value of the function should be 0 on success and non-zero on
 * failure. If the function returns non-zero, then the foreach will stop
 * and return the error code.
 *
 */
typedef int (*ACT_F)(void *node_data, void *addl_data);

typedef struct arr_list_t arr_list_t;

/**
 * @brief Create a new array list.
 *
 * The newly created array list will use the given size and nmemb values to
 * allocate an array. The free_f and cmp_f functions will be used to free and
 * compare data stored in the array.
 *
 * If free_f is NULL, then data stored in the list will not be free'd when the
 * list is cleared. If cmp_f is NULL, then certain functions that require a
 * compare function will not be available. If any of these functions are called,
 * then they will result in an ENOTSUP error.
 *
 * In case of an error, this function will return NULL and set the error
 * pointer if given. The error pointer may be NULL, in which case the error
 * will not be stored.
 * Possible errors:
 * - ENOMEM: Memory allocation error
 * - EINVAL: Invalid size or nmemb value
 *
 * @param free_f pointer to the free function to be used with that list
 * @param cmp_f pointer to the compare function to be used with that list
 * @param nmemb number of elements to allocate for the list
 * @param size size of each element in the list
 * @param err where to store the error code
 * @returns pointer to allocated list on success or NULL on failure
 */
arr_list_t *arr_list_new(FREE_F free_f, CMP_F cmp_f, size_t nmemb, size_t size,
                         int *err);

/**
 * @brief Wrap an array in a list.
 *
 * Identical to arr_list_new, except that the array is provided by the user
 * instead of being allocated by the function. The array must be at least
 * nmemb * size bytes in size and must not be freed by the user until the list
 * is deleted when arr_list_delete is called on the list object (i.e. the list
 * object is deleted). The array must either be pointing to NULL or be
 * initialized. If arr is pointing to NULL, then the list will allocate
 * memory for the array and store it at the address pointed to by arr. If the
 * array is initialized, then the list will use the array as is. If arr itself
 * is NULL, then the behavior is identical to arr_list_new; the user will have
 * no way of accessing the array that the list is using. If the array is
 * not initialized, then the behavior is undefined. A static array may be used,
 * but any attempts to resize the list later will result in undefined behavior.
 *
 * Note that this function only cares about the capacity of the initialized
 * array, not its contents. The size of the list will be set to 0, and the
 * capacity will be set to nmemb. If an existing array is used, it is advised
 * that the array is empty.
 *
 *
 * If free_f is NULL, then data stored in the list will not be free'd when the
 * list is cleared. If cmp_f is NULL, then certain functions that require a
 * compare function will not be available. If any of these functions are called,
 * then they will result in an ENOTSUP error.
 *
 * In case of an error, this function will return NULL and set the error
 * pointer if given. The error pointer may be NULL, in which case the error
 * will not be stored.
 * Possible errors:
 * - ENOMEM: Memory allocation error
 * - EINVAL: Invalid size or nmemb value
 *
 * @param free_f pointer to the free function to be used with that list
 * @param cmp_f pointer to the compare function to be used with that list
 * @param nmemb current capacity of the wrapped array
 * @param size size of each element in the wrapped array
 * @param arr pointer to the array to be wrapped
 * @param err where to store the error code
 * @returns pointer to allocated list on success or NULL on failure
 */
arr_list_t *arr_list_wrap(FREE_F free_f, CMP_F cmp_f, size_t nmemb, size_t size,
                          void **arr, int *err);

/**
 * @brief Query the array.
 *
 * The query command is used to get information about the array. The result
 * pointer is used to store the result of the query.
 *
 * Possible queries:
 * - QUERY_SIZE: Get the number of nodes in the array.
 * - QUERY_CAPACITY: Get the capacity of the array.
 * - QUERY_IS_EMPTY: Check if the array is empty.
 * - QUERY_IS_FULL: Check if the array is full.
 *
 * Possible results:
 * - QUERY_SIZE: The number of nodes in the array.
 * - QUERY_CAPACITY: The capacity of the array.
 * - QUERY_IS_EMPTY: 0 if the array is not empty, non-zero if the array is
 * - QUERY_IS_FULL: 0 if the array is not full, non-zero if the array is full
 * empty.
 *
 * Possible errors:
 * - EINVAL: The array or result pointers are NULL.
 * - ENOTSUP: The query command is invalid.
 *
 * @param list A pointer to the array.
 * @param query The query command.
 * @param result A pointer to the result of the query.
 * @return int 0 on success, non-zero on failure.
 */
int arr_list_query(const arr_list_t *list, int query, ssize_t *result);

/**
 * @brief Resize the list to the new capacity.
 *
 * If the new capacity is greater than the current capacity of the list, then
 * the list will be expanded. Otherwise, no action will be taken.
 *
 * Note that if the list wraps a static, then this function will have
 * undefined behavior.
 *
 * If the list is NULL, then EINVAL is returned. If a memory allocation error
 * occurs, then ENOMEM is returned.
 *
 * @param list pointer to list object to be checked
 * @param new_capacity new capacity of the list
 * @return 0 on success, non-zero on failure
 */
int arr_list_resize(arr_list_t *list, size_t new_capacity);

/**
 * @brief Trim the list capacity to the current size.
 *
 * If the list is NULL, then EINVAL is returned. If a memory allocation error
 * occurs, then ENOMEM is returned.
 *
 * @param list pointer to list object to be checked
 * @return 0 on success, non-zero on failure
 */
int arr_list_trim(arr_list_t *list);

/**
 * @brief Insert a new item into the list at a specific position.
 *
 * This will insert the item into the given position; all other elements after
 * will be shifted back. The list will be resized as needed.
 *
 * Note that if the list wraps a static array, then this function will have
 * undefined behavior if the list needs to be resized.
 *
 * If the list is NULL or position is outside the range of the list size, EINVAL
 * is returned. If a memory allocation error occurs, then ENOMEM is returned.
 *
 * @param list list to push the node into
 * @param data data to be pushed into node
 * @param position position in the list to push the node into
 * @returns 0 on success, non-zero on failure
 */
int arr_list_insert(arr_list_t *list, void *data, size_t position);

/**
 * @brief Set the item at a specific position in the list.
 *
 * Replaces the item at the given position with the given data.
 *
 * If the list is NULL or position is outside the range of the list capacity,
 * EINVAL is returned.
 *
 * @param list list to set the node in
 * @param data item to be set in node
 * @param position position in the list to set the node in
 * @param old where to store the old item
 * @return int 0 on success, non-zero on failure
 */
int arr_list_set(arr_list_t *list, void *data, size_t position, void *old);

/**
 * @brief Get the item at a specific position in the list.
 *
 * If the list is NULL or position is outside the range of the list, NULL is
 * returned. Note that if the list allows NULL values, then it is up to the user
 * to differentiate between a NULL value in the list and an error.
 *
 * @param list list to get the node from
 * @param position position in the list to get the node from
 * @return the item at the position on success, NULL on failure
 */
void *arr_list_get(const arr_list_t *list, size_t position);

/**
 * @brief Remove an item from the list at the given index.
 *
 * This will remove the item at the given position; all other elements after
 * will be shifted forward. The removed item will be stored in the old pointer.
 *
 * In the event of error, an appropriate error code is returned.
 * Possible errors:
 * - EINVAL: list is NULL or position is outside the range of the list
 *
 * @param list list to remove the node from
 * @param position position in the list to remove
 * @param old where to store the removed item
 * @return 0 on success, non-zero on failure
 */
int arr_list_pop(arr_list_t *list, size_t position, void *old);

/**
 * @brief Search for an item and remove it from the list.
 *
 * If the item is found, it is removed from the list. If the list allows
 * duplicates, then only the first item found will be removed.
 *
 * In the event of error, an appropriate error code is returned.
 * Possible errors:
 * - EINVAL: list is NULL
 * - ENOTSUP: list does not support comparisons
 *
 * @param list list to remove the item from
 * @param item_to_remove the data object to be searched for
 * @return true if successfully removed, false if not, negative on failure
 */
int arr_list_remove(arr_list_t *list, void *item_to_remove);

/**
 * @brief Perform an action on every item in the list.
 *
 * Performs the action function on every item in the list, in sequential order.
 * If the action function returns non-zero, then the foreach will stop and
 * return the error code. The data pointer can be used to pass in any additional
 * data that the user may need. If no additional data is needed, then the data
 * pointer can be NULL. If the list is empty, then the action function will not
 * be called.
 *
 * If the list or action function are NULL, then EINVAL is returned. If the
 * action function returns non-zero, then the error code returned by the action
 * function is returned.
 *
 * @param list list to perform actions on
 * @param action_function pointer to user defined action function
 * @param addl_data pointer to additional data to be passed into action function
 * @return 0 if all actions were successful, non-zero if an action failed
 */
int arr_list_foreach(arr_list_t *list, ACT_F action_function, void *addl_data);

/**
 * @brief Reset the list iterator to the head of the list.
 *
 * @param list list to reset the iterator of
 * @return 0 on success, EINVAL if list is NULL
 */
int arr_list_iterator_reset(arr_list_t *list);

/**
 * @brief Get the next item in the list.
 *
 * If the list is NULL, or the iterator is at the end of the list, then NULL
 * will be returned. Note that this function may also return NULL if the next
 * item in the list is NULL. This is not an error, and the user should check
 * the list size before calling this function.
 *
 * This function will have undefined behavior if the list is modified while the
 * iterator is in use.
 *
 * @param list list to iterate through
 * @return the next item on success, or NULL
 */
void *arr_list_iterator_next(arr_list_t *list);

/**
 * @brief Find the lowest index of an item in the list.
 *
 * This function will find the index of an item in the list. If the list
 * allows duplicates, then the lowest index of the first occurrence of the item
 * will be returned. If the item is not found, then negative will be returned.
 *
 * If this list does not support comparisons, then this function will
 * return ENOTSUP. If the list is NULL, then EINVAL will be returned.
 *
 * @param list list to search through
 * @param data is the pointer to the address of the data to be searched for
 * @param idx where to store the index of the found item
 * @return 0 on success, non-zero on failure
 */
int arr_list_index_of(const arr_list_t *list, const void *data, ssize_t *idx);

/**
 * @brief Sort list as per user defined compare function in ascending order.
 *
 * If this list does not support comparisons, then this function will
 * return ENOTSUP. If the list is NULL, then EINVAL will be returned.
 *
 * @param list pointer to list to be sorted
 * @return 0 on success, non-zero on failure
 */
int arr_list_sort(arr_list_t *list);

/**
 * @brief Clear all items out of a list.
 *
 * This function will zero out all the items in the list. If the list has a free
 * function, then this function will call the free function on all items in the
 * list first.
 *
 * If the list is NULL, then EINVAL will be returned.
 *
 * @param list list to clear out
 * @return 0 on success, non-zero on failure
 */
int arr_list_clear(arr_list_t *list);

/**
 * @brief Delete a list.
 *
 * This function will destroy the list object. If the list is not wrapping
 * a user given array, the array will first be cleared. This allows the user
 * to maintain access to the array after the list is deleted.
 *
 * If the list is NULL, then EINVAL will be returned.
 *
 * @param list list to be deleted
 * @return 0 on success, non-zero on failure
 */
int arr_list_delete(arr_list_t *list);

#endif /* ARRAY_LIST_H */
