#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <unistd.h>

/* DATA */

typedef enum query_cmds {
    QUERY_SIZE,
    QUERY_IS_EMPTY,
} QUERIES;

#define HASH_TABLE_DEFAULT_CAPACITY 16 // default capacity of table

/**
 * @brief A function pointer to a custom-defined delete function
 *        required to support deletion/memory deallocation of
 *        arbitrary data types. For simple data types, this function
 *        pointer can simply point to the free function. For more complex data
 *        types, this function should free all of the sub items that are
 *        allocated in the data.
 */
typedef void (*FREE_F)(void *data);

/**
 * @brief A user-defined compare function.
 *
 * Zero return value indicates that the first argument is equal to the second
 * argument. A non-zero return value indicates that they are no equal
 */
typedef int (*CMP_F)(const void *value_to_find, const void *node_data);

/**
 * @brief A user-defined function that gets called in the iterate call.
 *
 * The data pointer can be use to pass in any additional data that the user may
 * need. If no additional data is needed, then the data pointer can be NULL.
 * If the function returns non-zero, then the iterate call will stop
 * and return the error code.
 *
 * @param key key of the data
 * @param data data stored at that key
 * @param addl_data additional data that the user may need
 * @return int indicates to the iterate call whether to continue or stop
 */
typedef int (*ACT_TABLE_F)(const void *key, void **data, void *addl_data);

typedef struct hash_table_t hash_table_t;
/* FUNCTIONS */

/**
 * @brief Initialize hash table.
 *
 * The hash table is initialized with the given capacity. The customfree
 * function is used to free the data stored in the table. If the customfree
 * function is NULL, the data will not be freed when the table is destroyed. If
 * a capacity of 0 is given, the default capacity will be used. The given
 * compare function is used for looking up keys in the table.
 *
 * If an error occurs, NULL is returned and errno is set appropriately.
 * Possible error codes include:
 * - EINVAL: cmp_f is NULL
 * - ENOMEM: memory allocation fails
 *
 * @param capacity initial capacity of the table
 * @param free_f pointer to the user defined free function
 * @param cmp_f pointer to the user defined key compare function
 * @return hash_table_t pointer to allocated table
 */
hash_table_t *hash_table_init(size_t capacity, FREE_F free_f, CMP_F cmp_f);

/**
 * @brief Query the table.
 *
 * The query command is used to get information about the table. The result
 * pointer is used to store the result of the query.
 *
 * Possible queries:
 * - QUERY_SIZE: Get the number of key/value pairs in the table.
 * - QUERY_IS_EMPTY: Check if the table is empty.
 *
 * Possible results:
 * - QUERY_SIZE: The number of key/value pairs in the table.
 * - QUERY_IS_EMPTY: 0 if the table is not empty, non-zero if the table is
 * empty.
 *
 * Possible errors:
 * - EINVAL: The table or result pointers are NULL.
 * - ENOTSUP: The query command is invalid.
 *
 * @param list A pointer to the table.
 * @param query The query command.
 * @param result A pointer to the result of the query.
 * @return int 0 on success, non-zero on failure.
 */
int hash_table_query(const hash_table_t *table, int query, ssize_t *result);

/**
 * @brief Add an item to the table.
 *
 * If the key already exists in the table, the data will be updated. Otherwise,
 * the data will be added to the table. A reference to the key is stored in the
 * table, so the key must remain valid for the lifetime of the table.
 *
 * If an error occurs, an appropriate error code is returned.
 * Possible error codes include:
 * - EINVAL: table or key are NULL
 * - ENOMEM: memory allocation fails
 *
 * @param table pointer to table address
 * @param data data to be stored at that key value
 * @param key key for data to be stored at
 * @return int 0 on success, non-zero on failure
 */
int hash_table_set(hash_table_t *table, void *data, const void *key);

/**
 * @brief Look up an item in the table by key.
 *
 * If an error occurs, NULL is returned and errno is set appropriately.
 * Possible error codes include:
 * - EINVAL: table or key are NULL
 *
 * @param table pointer to table address
 * @param key key for data being searched for
 * @return void * the data stored at that key, or NULL
 */
void *hash_table_lookup(const hash_table_t *table, const void *key);

/**
 * @brief Iterate over the hash table.
 *
 * Iterates over the hash table and calls the action function on each item.
 * The data pointer can be use to pass in any additional data that the user may
 * need. If no additional data is needed, then the data pointer can be NULL.
 * If the function returns non-zero, then the iterate call will stop
 * and return the error code.
 *
 * If an error occurs, an appropriate error code is returned.
 * Possible error codes include:
 * - EINVAL: table or action are NULL
 *
 * @param table pointer to table address
 * @param action pointer to function to be called on each item
 * @param addl_data additional data that the user may need
 * @return int the return value of the action function, or non-zero on failure
 */
int hash_table_iterate(hash_table_t *table, ACT_TABLE_F action,
                       void *addl_data);

/**
 * @brief Remove an item from the hash table.
 *
 * If an error occurs, NULL is returned and errno is set appropriately.
 * Possible error codes include:
 * - EINVAL: table or key are NULL
 *
 * @param table the table to remove the item from
 * @param key key of data to be removed
 * @return void * data that was removed
 */
void *hash_table_remove(hash_table_t *table, const void *key);

/**
 * @brief Clear all data from hash table.
 *
 * The customfree function is used to free the data stored in the table. If the
 * customfree function is NULL, the data will not be freed when the table is
 * cleared.
 *
 * If an error occurs, an appropriate error code is returned.
 * Possible error codes include:
 * - EINVAL: table is NULL
 *
 * @param table_addr the table to clear
 * @return int 0 on success, non-zero on failure
 */
int hash_table_clear(hash_table_t *table);

/**
 * @brief Destroy hash table.
 *
 * The customfree function is used to free the data stored in the table. If the
 * customfree function is NULL, the data will not be freed when the table is
 * destroyed.
 *
 * If an error occurs, an appropriate error code is returned.
 * Possible error codes include:
 * - EINVAL: table is NULL
 *
 * @param table_addr pointer to table address
 * @return int 0 on success, non-zero on failure
 */
int hash_table_destroy(hash_table_t **table_addr);

#endif /* HASH_TABLE_H */
