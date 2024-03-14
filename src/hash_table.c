#include "hash_table.h"
#include "linked_list.h"
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

/* DATA */

#define GROWTH_FACTOR 2 // amount to grow table by when resizing
#define MAX_LOAD 75     // max load factor before resizing
#define PRIME 6967      // prime number for hash algo
#define CONTINUE 0      // return value keep iterating
#define STOP 1          // return value to stop iterating
#define SUCCESS 0       // return value for success
#define INVALID -1      // return value for invalid arguments

/**
 * @brief structure of a table_node_t object
 *
 * @param key       pointer to the saved keyvalue string
 * @param data      saved data pointer
 * @param compare   pointer to the user defined key compare function
 */
typedef struct table_node_t {
    const void *key;
    void *data;
    CMP_F compare;
} table_node_t;

/**
 * @brief structure of a hash_table_t object
 *
 * Table will have N slots, with each slot holding a table_node_t
 * Upon table insertion, hash algo will determine which slot to store data
 * If that slot is null, insert new table_node_t there
 * If not null (collision), traverse to end of list and append new table_node_t
 *
 * @param capacity      number of positions supported by table
 * @param size          number of elements in table
 * @param buckets       the buckets of table_node_t
 * @param customfree    pointer to the user defined free function
 * @param compare       pointer to the user defined key compare function
 */
struct hash_table_t {
    size_t capacity;
    size_t size;
    list_t **buckets;
    FREE_F customfree;
    CMP_F compare;
};

/**
 * @brief structure used to pass data to list_foreach_call
 *
 * @param cmp_key       key to compare against
 * @param data          data to be stored at that key value
 * @param customfree    pointer to the user defined free function
 */
struct lookup_data_t {
    const void *cmp_key;
    void *data;
    FREE_F customfree;
};

/**
 * @brief Structure used to pass data to list_foreach_call
 *
 * @param action        action function to be called
 * @param addl_data     additional data to be passed to action function
 */
struct action_data_t {
    ACT_TABLE_F action;
    void *addl_data;
};

/* PRIVATE FUNCTIONS */

/**
 * @brief Comparison function to pass to list_new.
 *
 * @param to_find the key to find
 * @param node_data the node to compare to
 * @return int 0 if equal, negative if less than, positive if greater than
 */
static int map_node_cmp(const void *to_find, const void *node_data) {
    if (node_data == NULL) {
        // if somehow node_data is NULL, return 0 so order is not changed
        return 0;
    }
    const struct table_node_t *table_node = node_data;
    return table_node->compare(to_find, table_node->key);
}

/**
 * @brief Hash function to determine which slot to store data.
 *
 * @param data the key to hash
 * @param capacity the capacity of the table
 * @return size_t the index of the slot to store data
 */
static size_t hash(const void *data, size_t capacity) {
    size_t hash = PRIME;
    const uint8_t *bytes = data;
    while (*(bytes++)) {
        hash += *bytes;
    }
    return hash % capacity;
}

/**
 * @brief Conversion function to pass to list_foreach_call.
 *
 * @param node the node to be passed to the action function
 * @param addl_data the action_data_t struct
 * @return int
 */
static int action_wrapper(void **node, void *addl_data) {
    if (node == NULL || *node == NULL || addl_data == NULL) {
        return EINVAL;
    }
    table_node_t *table_node = *node;
    struct action_data_t *action_data = addl_data;
    return action_data->action(table_node->key, &table_node->data,
                               action_data->addl_data);
}

/**
 * @brief Copy the node into a new position into the table.
 *
 * @param node_data the node to be copied
 * @param addl_data the table to copy to
 * @return int 0 on success, non-zero on failure
 */
static int copy_node(void **node_data, void *addl_data) {
    if (node_data == NULL || *node_data == NULL || addl_data == NULL) {
        return EINVAL;
    }
    table_node_t *table_node = *node_data;
    hash_table_t *table = addl_data;

    size_t idx = hash(table_node->key, table->capacity);
    if (table->buckets[idx] == NULL) {
        // list will not manage memory of table_node
        table->buckets[idx] = list_new(NULL, map_node_cmp);
        if (table->buckets[idx] == NULL) {
            return ENOMEM;
        }
    }
    return list_push_head(table->buckets[idx], table_node);
}

/**
 * @brief Resize the table.
 *
 * @param table the table to resize
 * @return int 0 on success, non-zero on failure
 */
static int resize_table(hash_table_t *table) {
    if (table == NULL) {
        return EINVAL;
    }
    list_t **copy_arr =
        calloc(GROWTH_FACTOR * table->capacity, sizeof(*copy_arr));
    if (copy_arr == NULL) {
        return ENOMEM;
    }
    // Adjust to new capacity
    hash_table_t copy_table = {
        .capacity = GROWTH_FACTOR * table->capacity,
        .buckets = copy_arr,
    };

    for (size_t n = 0; n < table->capacity; ++n) {
        if (table->buckets[n] == NULL) {
            continue;
        }
        int err = list_foreach_call(table->buckets[n], copy_node, &copy_table);
        if (err != SUCCESS) {
            for (size_t i = 0; i < n; i++) {
                list_delete(&copy_arr[i]);
            }
            free(copy_arr);
            return err;
        }
    }

    // replace table with copy
    for (size_t i = 0; i < table->capacity; i++) {
        list_delete(&table->buckets[i]);
    }
    free(table->buckets);
    table->buckets = copy_arr;
    table->capacity *= GROWTH_FACTOR;

    return SUCCESS;
}

/**
 * @brief Replace the data in an existing node with new data.
 *
 * @param node_data the node to compare to
 * @param addl_data the lookup_data_t struct
 * @return int
 */
static int replace_existing(void **node_data, void *addl_data) {
    if (node_data == NULL || *node_data == NULL || addl_data == NULL) {
        return EINVAL;
    }
    table_node_t *table_node = *node_data;
    struct lookup_data_t *lookup_data = addl_data;

    if (table_node->compare(table_node->key, lookup_data->cmp_key) == 0) {
        if (lookup_data->customfree) {
            lookup_data->customfree(table_node->data);
        }
        table_node->data = lookup_data->data;
        return STOP;
    }
    return CONTINUE;
}

/**
 * @brief Get the bucket object
 *
 * @param table the table to get the bucket from
 * @param key the key to get the bucket for
 * @return list_t* the bucket
 */
static list_t *get_bucket(hash_table_t *table, const void *key) {
    if (table == NULL) {
        return NULL;
    }
    size_t idx = hash(key, table->capacity);
    list_t *bucket = table->buckets[idx];
    // if hash does not exist, create new list
    if (bucket == NULL) {
        // list will not manage memory of table_node
        bucket = list_new(NULL, map_node_cmp);
        if (bucket == NULL) {
            return NULL;
        }
        table->buckets[idx] = bucket;
    }
    return bucket;
}

/**
 * @brief Insert a new node into the bucket.
 *
 * @param key the key to insert
 * @param data the data to insert
 * @param bucket the bucket to insert into
 * @param compare the key compare function to use
 * @return int 0 on success, non-zero on failure
 */
static int insert_into_bucket(const void *key, void *data, list_t *bucket,
                              CMP_F compare) {
    if (compare == NULL || bucket == NULL) {
        return EINVAL;
    }
    table_node_t *new = malloc(sizeof(*new));
    if (new == NULL) {
        return ENOMEM;
    }

    new->key = key;
    new->data = data;
    new->compare = compare;
    int err = list_push_head(bucket, new);
    if (err != SUCCESS) {
        free(new);
        return err;
    }
    return SUCCESS;
}

/* PUBLIC FUNCTIONS */

hash_table_t *hash_table_init(size_t capacity, FREE_F free_f, CMP_F cmp_f) {
    if (cmp_f == NULL) {
        errno = EINVAL;
        return NULL;
    }
    hash_table_t *table = malloc(sizeof(*table));
    if (table == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    table->capacity = capacity == 0 ? HASH_TABLE_DEFAULT_CAPACITY : capacity;
    table->buckets = calloc(table->capacity, sizeof(*(table->buckets)));
    if (table->buckets == NULL) {
        free(table);
        errno = ENOMEM;
        return NULL;
    }
    table->size = 0;
    table->customfree = free_f;
    table->compare = cmp_f;
    return table;
}

int hash_table_query(const hash_table_t *table, int query, ssize_t *result) {
    if (table == NULL || result == NULL) {
        return EINVAL;
    }
    switch (query) {
    case QUERY_SIZE:
        *result = table->size;
        return SUCCESS;
    case QUERY_IS_EMPTY:
        *result = table->size == 0;
        return SUCCESS;
    default:
        return ENOTSUP;
    }
}

int hash_table_set(hash_table_t *table, void *data, const void *key) {
    if (table == NULL || key == NULL) {
        return EINVAL;
    }

    int err;
    // Resize if size is greater than MAX_LOAD percent of capacity
    if (100 * (table->size / table->capacity) > MAX_LOAD) {
        err = resize_table(table);
        if (err != SUCCESS) {
            return err;
        }
    }

    list_t *curr_list = get_bucket(table, key);
    if (curr_list == NULL) {
        return ENOMEM;
    }
    // check if key exists
    struct lookup_data_t lookup_data = {
        .cmp_key = key,
        .data = data,
        .customfree = table->customfree,
    };
    // end function early if key already exists and is replaced
    if (list_foreach_call(curr_list, replace_existing, &lookup_data) == STOP) {
        return SUCCESS;
    } else {
        // Key does not exist, add it
        err = insert_into_bucket(key, data, curr_list, table->compare);
        if (err == SUCCESS) {
            table->size++;
        }
        return err;
    }
}

void *hash_table_lookup(const hash_table_t *table, const void *key) {
    if (table == NULL || key == NULL) {
        errno = EINVAL;
        return NULL;
    } else if (table->size == 0) {
        return NULL;
    }
    size_t idx = hash(key, table->capacity);
    table_node_t *node = list_find_first(table->buckets[idx], key);
    return node != NULL ? node->data : NULL;
}

int hash_table_iterate(hash_table_t *table, ACT_TABLE_F action,
                       void *addl_data) {
    if (table == NULL || action == NULL) {
        return EINVAL;
    } else if (table->size == 0) {
        return SUCCESS;
    }
    int err = SUCCESS;
    struct action_data_t action_data = {
        .action = action,
        .addl_data = addl_data,
    };
    for (size_t i = 0; i < table->capacity; i++) {
        err =
            list_foreach_call(table->buckets[i], action_wrapper, &action_data);
        if (err != CONTINUE) {
            return err;
        }
    }
    return err;
}

void *hash_table_remove(hash_table_t *table, const void *key) {
    if (table == NULL || key == NULL) {
        errno = EINVAL;
        return NULL;
    } else if (table->size == 0) {
        return NULL;
    }
    size_t idx = hash(key, table->capacity);
    if (table->buckets[idx] == NULL) {
        return NULL;
    }
    table_node_t *node = list_remove(table->buckets[idx], (void *)key);
    if (node == NULL) {
        return NULL;
    }
    void *data = node->data;
    free(node);
    return data;
}

int hash_table_clear(hash_table_t *table) {
    if (table == NULL) {
        return EINVAL;
    } else if (table->size == 0) {
        return SUCCESS;
    }
    for (size_t i = 0; i < table->capacity; i++) {
        table_node_t *curr = list_pop_tail(table->buckets[i]);
        while (curr != NULL) {
            if (table->customfree) {
                table->customfree(curr->data);
            }
            free(curr);
            curr = list_pop_tail(table->buckets[i]);
        }
        list_delete(&table->buckets[i]);
    }
    table->size = 0;
    return SUCCESS;
}

int hash_table_destroy(hash_table_t **table_addr) {
    if (table_addr == NULL || *table_addr == NULL) {
        return EINVAL;
    }
    hash_table_clear(*table_addr);
    free((*table_addr)->buckets);
    free(*table_addr);
    *table_addr = NULL;
    return SUCCESS;
}
