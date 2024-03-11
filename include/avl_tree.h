#ifndef AVL_TREE_H
#define AVL_TREE_H
#include <unistd.h>

/* DATA */

/**
 * @brief A pointer to a user-defined free function.  This is used to free
 *        memory allocated for tree data.  For simple data types, this
 * 	      is just a pointer to the standard free function.  More complex
 *        structs stored in lists may require a function that calls free on
 *        multiple components.
 */
typedef void (*FREE_F)(void *data);

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
 * @brief A user-defined function that gets called in the foreach function.
 *
 * The data pointer can be use to pass in any additional data that the user may
 * need. If no additional data is needed, then the data pointer can be NULL.
 * The return value of the function should be 0 on success and non-zero on
 * failure. If the function returns non-zero, then the foreach_call will stop
 * and return the error code.
 *
 */
typedef int (*ACT_F)(void *node_data, void *addl_data);

typedef struct tree_t tree_t;

/* FUNCTIONS */

/**
 * @brief Create a new tree.
 *
 * The free_func is used to free the memory allocated for the tree data. If
 * the tree is not meant to manage the memory of the data, then the free_func
 * can be NULL. The cmp_func is used to compare the data in the tree to the
 * data being searched for and to order nodes. The cmp_func is required and
 * cannot be NULL.
 *
 * If memory allocation fails, then the function will return NULL and set errno
 * to ENOMEM. If cmp_func is NULL, then the function will return NULL and set
 * errno to EINVAL.
 *
 * @param free_func A user-defined free function.
 * @param cmp_func A user-defined compare function.
 * @return tree_t* A pointer to the new tree.
 */
tree_t *tree_new(FREE_F free_func, CMP_F cmp_func);

/**
 * @brief Check if the tree is empty.
 *
 * If tree is NULL, then the function will return -1 and set errno to EINVAL.
 *
 * @param tree A pointer to the tree.
 * @return int non-zero if the tree is empty, 0 if the tree is not empty, and -1
 * if the tree is NULL.
 */
int tree_is_empty(tree_t *tree);

/**
 * @brief Get the size of the tree.
 *
 * If tree is NULL, then the function will return -1 and set errno to EINVAL.
 *
 * @param tree A pointer to the tree.
 * @return ssize_t The size of the tree or -1 if the tree is NULL.
 */
ssize_t tree_size(tree_t *tree);

/**
 * @brief Add a new node to the tree.
 *
 * NULL data is allowed.
 *
 * If tree is NULL, then the function will return EINVAL. If the tree fails to
 * allocate memory for the new node, then the function will return ENOMEM.
 *
 *
 * @param tree A pointer to the tree.
 * @param data A pointer to the data to be added to the tree.
 * @return int 0 on success, non-zero on failure.
 */
int tree_add(tree_t *tree, void *data);

/**
 * @brief Remove a node from the tree.
 *
 * The data pointer is used to find the node to be removed; the tree's cmp_func
 * will be used in comparison. The data does not need to be the same pointer
 * that was added to the tree, but should be inside the tree according to the
 * cmp_func.
 *
 * If tree is NULL, then the function will return NULL and set errno to EINVAL.
 *
 * @param tree A pointer to the tree.
 * @param data A pointer to the data to be removed from the tree.
 * @return void* A pointer to the data that was removed from the tree or NULL if
 * the data was not found.
 */
void *tree_remove(tree_t *tree, void *data);

/**
 * @brief Remove all nodes from the tree.
 *
 * The data pointer is used to find the nodes to be removed; the tree's
 * cmp_func will be used in comparison. The data does not need to be the same
 * pointer that was added to the tree, but should be inside the tree according
 * to the cmp_func. This function will free the data in the tree if the tree was
 * created with a free_func. If the user does not want the memory to be freed,
 * then tree_remove() should be used directly.
 *
 * If tree is NULL, then the function will return -1 and set errno to EINVAL.
 *
 * @param tree A pointer to the tree.
 * @param data A pointer to the data to be removed from the tree.
 * @return int The number of nodes removed from the tree or -1 if the tree is
 * NULL.
 */
ssize_t tree_remove_all(tree_t *tree, void *data);

/**
 * @brief Check if the tree contains a node with the given data.
 *
 * The data pointer is used to find the node; the tree's cmp_func will be used
 * in comparison. The data does not need to be the same pointer that was added
 * to the tree, but should be inside the tree according to the cmp_func.
 *
 * If tree is NULL, then the function will return -1 and set errno to EINVAL.
 *
 * @param tree A pointer to the tree.
 * @param data A pointer to the data to be searched for in the tree.
 * @return int 0 if the tree contains the data, non-zero if the tree does not
 * contain the data, and -1 if the tree is NULL.
 */
int tree_contains(tree_t *tree, void *data);

/**
 * @brief Find the first occurrence of a node with the given data.
 *
 * The data pointer is used to find the node; the tree's cmp_func will be used
 * in comparison. The data does not need to be the same pointer that was added
 * to the tree, but should be inside the tree according to the cmp_func.
 *
 * If tree is NULL, then the function will return NULL and set errno to EINVAL.
 *
 * @param tree A pointer to the tree.
 * @param data A pointer to the data to be searched for in the tree.
 * @return void* A pointer to the data found in the tree or NULL if the data was
 * not found or the tree is NULL.
 */
void *tree_find_first(tree_t *tree, void *data);

/**
 * @brief Find all occurrences of nodes with the given data.
 *
 * The data pointer is used to find the nodes; the tree's cmp_func will be used
 * in comparison. The data does not need to be the same pointer that was added
 * to the tree, but should be inside the tree according to the cmp_func. If
 * no nodes are found, then the function will return an empty tree.
 *
 * The returned tree must be freed by the user by calling tree_delete(). The
 * tree will not free the data in the nodes when the tree is deleted or cleared.
 * The data in this tree is shared with the original tree; modifying one will
 * affect the other.
 *
 * If tree is NULL, then the function will return NULL and set errno to EINVAL.
 * If memory allocation fails, then the function will return NULL and set errno
 * to ENOMEM.
 *
 * @param tree A pointer to the tree.
 * @param data A pointer to the data to be searched for in the tree.
 * @return tree_t* A pointer to the tree containing the data found in the tree
 * or NULL if the tree is NULL.
 */
tree_t *tree_find_all(tree_t *tree, void *data);

/**
 * @brief Perform a user-defined action on the data contained in all of the
 *        nodes in the tree.
 *
 * The act_func is called on each node in the tree. The node data is
 * passed into the act_func as a void pointer. The addl_data pointer can
 * be used to pass in any additional data that the user may need. If no
 * additional data is needed, then the addl_data pointer can be NULL. If the
 * act_func returns non-zero, then the foreach_call will stop and return
 * the status code.
 *
 * If tree is NULL, then the function will return -1 and set errno to EINVAL.
 *
 * @param tree A pointer to the tree.
 * @param act_func A pointer to the user-defined action function.
 * @param addl_data A pointer to any additional data needed by the
 *                  act_func.
 * @return int 0 on success, non-zero on failure.
 */
int tree_foreach(tree_t *tree, ACT_F act_func, void *addl_data);

/**
 * @brief Reset the tree iterator to the first node in the tree.
 *
 * This function must be called before the first call to tree_iterator_next();
 * The iterator is not initialized when the tree is created. This function can
 * be called at any time to reset the iterator to the first node in the tree.
 *
 * If tree is NULL, then the function will return EINVAL.
 *
 * @param tree A pointer to the tree.
 * @return int 0 on success, non-zero on failure.
 */
int tree_iterator_reset(tree_t *tree);

/**
 * @brief Get the next node in the tree.
 *
 * The tree_iterator_reset() function must be called before the first call to
 * tree_iterator_next(); The iterator is not initialized when the tree is
 * created. This function can be called at any time to get the next node in the
 * tree. The function will return NULL when the end of the tree is reached.
 * This function has undefined behavior if the tree is modified between calls
 * to tree_iterator_reset() and tree_iterator_next().
 *
 * If tree is NULL, then the function will return NULL and set errno to EINVAL.
 *
 * @param tree A pointer to the tree.
 * @return void* A pointer to the data in the next node in the tree or NULL if
 * the end of the tree is reached or the tree is NULL.
 */
void *tree_iterator_next(tree_t *tree);

/**
 * @brief Clear all nodes out of a tree.
 *
 * The free_func is used to free the memory allocated for the tree data if
 * provided at tree creation.
 *
 * If tree is NULL, then the function will return EINVAL.
 *
 * @param tree A pointer to the tree.
 * @return int 0 on success, non-zero on failure.
 */
int tree_clear(tree_t *tree);

/**
 * @brief Delete a tree.
 *
 * The free_func is used to free the memory allocated for the tree data if
 * provided at tree creation.
 *
 * If tree is NULL, then the function will return EINVAL.
 *
 * @param tree A pointer to the tree.
 * @return int 0 on success, non-zero on failure.
 */
int tree_delete(tree_t **tree);
#endif /* AVL_TREE_H */