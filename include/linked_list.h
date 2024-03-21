#ifndef LINKED_LIST_H
#define LINKED_LIST_H

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
 * @brief A user-defined function that gets called in the foreach_call.
 *
 * The data pointer can be use to pass in any additional data that the user may
 * need. If no additional data is needed, then the data pointer can be NULL.
 * The return value of the function should be 0 on success and non-zero on
 * failure. If the function returns non-zero, then the foreach_call will stop
 * and return the error code.
 *
 */
typedef int (*ACT_F)(void **node_data, void *addl_data);

typedef struct list_t list_t;
/**
 * @brief Create a new list.
 *
 * If free_f is NULL, then data stored in the list will not be free'd. If cmp_f
 * is NULL, then certain functions that require a compare function will not be
 * available. If any of these functions are called, they will result in an
 * ENOTSUP error.
 *
 * If an error occurs, then NULL will be returned.
 * Possible error codes are:
 * - ENOMEM: memory allocation failed
 *
 * @param free_f pointer to the free function to be used with that list
 * @param cmp_f pointer to the compare function to be used with that list
 * @param err pointer to the error code
 * @returns pointer to allocated list on success or NULL on failure
 */
list_t *list_new(FREE_F free_f, CMP_F cmp_f, int *err);

/**
 * @brief Query the list.
 *
 * The query command is used to get information about the list. The result
 * pointer is used to store the result of the query.
 *
 * Possible queries:
 * - QUERY_SIZE: Get the number of nodes in the list.
 * - QUERY_IS_EMPTY: Check if the list is empty.
 *
 * Possible results:
 * - QUERY_SIZE: The number of nodes in the list.
 * - QUERY_IS_EMPTY: 0 if the list is not empty, non-zero if the list is
 * empty.
 *
 * Possible errors:
 * - EINVAL: The list or result pointers are NULL.
 * - ENOTSUP: The query command is invalid.
 *
 * @param list A pointer to the list.
 * @param query The query command.
 * @param result A pointer to the result of the query.
 * @return int 0 on success, non-zero on failure.
 */
int list_query(const list_t *list, int query, ssize_t *result);

/**
 * @brief Push a new node onto the head of list.
 *
 * Possible error codes are:
 * - EINVAL: list is NULL
 * - ENOMEM: memory allocation failed
 *
 * @param list list to push the node into
 * @param data data to be pushed into node
 * @returns 0 on success, EINVAL if list is NULL, ENOMEM if malloc fails
 */
int list_push_head(list_t *list, void *data);

/**
 * @brief Push a new node onto the tail of list.
 *
 * Possible error codes are:
 * - EINVAL: list is NULL
 * - ENOMEM: memory allocation failed
 *
 * @param list list to push the node into
 * @param data data to be pushed into node
 * @return 0 on success, EINVAL if list is NULL, ENOMEM if malloc fails
 */
int list_push_tail(list_t *list, void *data);

/**
 * @brief Insert a new node into the list at a specific position.
 *
 * Possible error codes are:
 * - EINVAL: list is NULL or position is invalid
 * - ENOMEM: memory allocation failed
 *
 * @param list list to insert the node into
 * @param data data to be inserted into node
 * @param position position in the list to insert the node
 * @return 0 on success, EINVAL if list is NULL or position is invalid, ENOMEM
 * if malloc fails
 */
int list_insert(list_t *list, void *data, size_t position);

/**
 * @brief Get the data at a specific position in the list.
 *
 * If list is NULL or position is invalid, then NULL will be returned. Note that
 * if the list allows NULL values or is empty, then this function will also
 * return NULL.
 *
 * @param list list to get the node from
 * @param position position in the list to get the node from
 * @return the data in the node on success, NULL on failure
 */
void *list_get(const list_t *list, size_t position);

/**
 * @brief Get the size of the list.
 *
 * If list is NULL, then -1 will be returned.
 *
 * @param list list to get the size of
 * @return the size of the list on success, -1 on failure
 */
ssize_t list_size(const list_t *list);

/**
 * @brief Check if the list object is empty.
 *
 * If list is NULL, then -1 will be returned.
 *
 * @param list pointer to linked list object to be checked
 * @returns 0 if list is not empty or list is NULL, non-zero if list is empty,
 * -1 on error
 */
int list_is_empty(const list_t *list);

/**
 * @brief Pop the head out of the list.
 *
 * If list is NULL, then NULL will be returned. Note that if the list allows
 * NULL values or is empty, then this function will also return NULL.
 *
 * @param list list to pop out of
 * @return the popped data on success, NULL on failure
 */
void *list_pop_head(list_t *list);

/**
 * @brief Pop the tail out of the list.
 *
 * If list is NULL, then NULL will be returned. Note that if the list allows
 * NULL values or is empty, then this function will also return NULL.
 *
 * @param list list to pop out of
 * @return the popped data on success, NULL on failure
 */
void *list_pop_tail(list_t *list);

/**
 * @brief Get the data from the head of the list without popping.
 *
 * If list is NULL, then NULL will be returned. Note that if the list allows
 * NULL values or is empty, then this function will also return NULL.
 *
 * @param list list to peeked out of
 * @return the head on success, NULL on failure
 */
void *list_peek_head(const list_t *list);

/**
 * @brief Get the data from the tail of the list without popping.
 *
 * If list is NULL, then NULL will be returned. Note that if the list allows
 * NULL values or is empty, then this function will also return NULL.
 *
 * @param list list to peeked out of
 * @return the tail on success, NULL on failure
 */
void *list_peek_tail(const list_t *list);

/**
 * @brief Remove a specific node from the list.
 *
 * If the list allows duplicates, then the first node found with the
 * item_to_remove will be removed. If NULL values are allowed in the list,
 * it is up to the user to differentiate between a NULL value in the list
 * and an error.
 *
 * If an error occurs, then NULL will be returned.
 * Possible error codes are:
 * - EINVAL: list is NULL
 * - ENOTSUP: list does not support comparisons
 *
 * @param list list to remove the node from
 * @param item_to_remove the data object to be searched for
 * @param err pointer to the error code
 * @return the value in the removed node on success, NULL on failure
 */
void *list_remove(list_t *list, void *item_to_remove, int *err);

/**
 * @brief Perform a user defined action on the data in list.
 *
 * If the action function returns non-zero, then the foreach_call will stop and
 * return the error code. The data pointer can be used to pass in any additional
 * data that the user may need. If no additional data is needed, then the data
 * pointer can be NULL. If the list is empty, then the action function will not
 * be called and 0 will be returned.
 *
 * Possible error codes are:
 * - EINVAL: list or action_function are NULL
 *
 * @param list list to perform actions on
 * @param action_function pointer to user defined action function
 * @return the last returned value from the action function, or EINVAL if list
 * or action are NULL
 */
int list_foreach_call(list_t *list, ACT_F action_function, void *data);

/**
 * @brief Reset the list iterator to the head of the list.
 *
 * Possible error codes are:
 * - EINVAL: list is NULL
 *
 * @param list list to reset the iterator of
 * @return 0 on success, non-zero on failure
 */
int list_iterator_reset(list_t *list);

/**
 * @brief Get the next node in the list.
 *
 * The iterator must be reset before the first call to this function. This
 * function will have undefined behavior if the list is modified while the
 * iterator is in use without resetting it.
 *
 * Possible error codes are:
 * - EINVAL: list is NULL
 * - ENOTSUP: reached the end of the list
 *
 * @param list list to iterate through
 * @param err pointer to the error code
 * @return the data in the next node on success, or NULL on failure
 */
void *list_iterator_next(list_t *list, int *err);

/**
 * @brief Find the first occurrence of a node containing the search_data.
 *
 * If an error occurs, then NULL will be returned.
 * Possible error codes are:
 * - EINVAL: list is NULL
 * - ENOTSUP: list does not support comparisons
 *
 * @param list list to search through
 * @param search_data pointer to the data to be searched for
 * @param err pointer to the error code
 * @return the data found on success, NULL on failure
 */
void *list_find_first(const list_t *list, const void *search_data, int *err);

/**
 * @brief Find all occurrences of a node containing the search_data.
 *
 * The returned list will contain pointers to the data in the nodes. If the
 * list allows duplicates, then the returned list will contain pointers to
 * all nodes containing the search_data. Modifying the structure of the returned
 * list will not modify the original list, but modifying the data in the
 * returned list will modify the data in the original list. The user must delete
 * the returned list; however, deleting/clearing the returned list will not
 * affect the original list.
 *
 * If an error occurs, then NULL will be returned.
 * Possible error codes are:
 * - EINVAL: list is NULL
 * - ENOTSUP: list does not support comparisons
 * - ENOMEM: memory allocation failed
 *
 * @param list list to search through
 * @param search_data is the pointer to the address of the data to be searched
 *                    for
 * @param err pointer to the error code
 * @return pointer to list of all found occurrences on success, NULL on failure
 */
list_t *list_find_all(const list_t *list, const void *search_data, int *err);

/**
 * @brief Sort list as per user defined compare function.
 *
 * Possible error codes are:
 * - EINVAL: list is NULL
 * - ENOTSUP: list does not support comparisons
 *
 * @param list pointer to list to be sorted
 * @return 0 on success, EINVAL if list is NULL
 */
int list_sort(list_t *list);

/**
 * @brief Clear all nodes out of a list.
 *
 * Possible error codes are:
 * - EINVAL: list is NULL
 *
 * @param list list to clear out
 * @return 0 on success, EINVAL if list is NULL
 */
int list_clear(list_t *list);

/**
 * @brief Delete a list.
 *
 * Possible error codes are:
 * - EINVAL: list is NULL
 *
 * @param list_address pointer to list pointer
 * @return 0 on success, EINVAL if list is NULL
 */
int list_delete(list_t **list_address);

#endif /* LINKED_LIST_H */
