#include "avl_tree.h"
#include "queue.h"
#include <errno.h>
#include <stdlib.h>

/* DATA */

enum {
    FOUND = 42,
};
#define SUCCESS 0  // no error
#define INVALID -1 // invalid input

/**
 * @brief structure of a node in the tree
 *
 * @param data void pointer to whatever data that node points to
 * @param left pointer to the node to the left of it
 * @param right pointer to the node to the right of it
 */
struct node {
    void *data;
    struct node *left;
    struct node *right;
};

/**
 * @brief structure of a tree object
 *
 * @param root pointer to the root node
 * @param iterator in order queue for iterator
 * @param size the number of nodes the tree is currently storing
 * @param customfree pointer to the user defined free function
 * @param compare_function pointer to the user defined compare function
 */
struct tree_t {
    struct node *root;
    queue_t *iterator;
    size_t size;
    FREE_F free_func;
    CMP_F cmp_func;
};

/* PRIVATE FUNCTIONS*/

/**
 * @brief Search for a node in the tree recursively.
 *
 * If the node is not found, the function returns a pointer to the node where
 * the new node should be inserted. If node or cmp are NULL, the function
 * returns NULL.
 *
 * @param node A pointer to the current node
 * @param cmp A pointer to the user-defined compare function
 * @param needle A pointer to the data to be searched for
 * @return struct node** A pointer to the node containing the data or NULL if
 * the data is not found.
 */
static struct node **tree_search(struct node **node, CMP_F cmp, void *needle) {
    if (node == NULL || cmp == NULL) {
        return NULL;
    } else if (*node == NULL) {
        return node;
    }
    int result = cmp(needle, (*node)->data);
    if (result == 0) {
        return node;
    } else if (result < 0) {
        return tree_search(&(*node)->left, cmp, needle);
    } else {
        return tree_search(&(*node)->right, cmp, needle);
    }
}

/**
 * @brief Create a new node.
 *
 * @param data A pointer to the data to be stored in the node
 * @return struct node* A pointer to the new node
 */
static struct node *create_node(void *data) {
    struct node *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    node->data = data;
    node->left = NULL;
    node->right = NULL;
    return node;
}

/**
 * @brief Get the height of the tree.
 *
 * @param node A pointer to the current node
 * @return size_t The height of the tree
 */
static size_t tree_height(const struct node *node) {
    if (node == NULL) {
        return 0;
    }

    size_t left = node->left ? tree_height(node->left) : 0;
    size_t right = node->right ? tree_height(node->right) : 0;

    return 1 + (left > right ? left : right);
}

/**
 * @brief Get the balance factor of the tree.
 *
 * @param node A pointer to the current node
 * @return int The balance factor of the tree
 */
static int balance_factor(struct node *node) {
    if (node == NULL) {
        return -1;
    }
    return tree_height(node->left) - tree_height(node->right);
}

/**
 * @brief Check if the tree is balanced.
 *
 * @param balance The balance factor of the tree
 * @return int non-zero if the tree is balanced, 0 if the tree is not
 */
static inline int balanced(int balance) {
    return balance >= -1 && balance <= 1;
}

/**
 * @brief Check if the tree is heavy on the left.
 *
 * @param balance The balance factor of the tree
 * @return int non-zero if the tree is heavy on the left, 0 if the tree
 * is not
 */
static inline int left_heavy(int balance) { return balance > 0; }

/**
 * @brief Check if the tree is heavy on the right.
 *
 * @param balance The balance factor of the tree
 * @return int non-zero if the tree is heavy on the right, 0 if the tree
 * is not
 */
static inline int right_heavy(int balance) { return balance < 0; }

/**
 * @brief Get the maximum value in the tree.
 *
 * If node is NULL, the function returns NULL.
 *
 * @param node A pointer to the current node
 * @return struct node** A pointer to the node containing the maximum
 * value, or NULL.
 */
static struct node **tree_max(struct node **node) {
    if (node == NULL) {
        return NULL;
    } else if ((*node)->right) {
        return tree_max(&(*node)->right);
    } else {
        return node;
    }
}

/**
 * @brief Rotate the tree to the left.
 *
 * @param node A pointer to the current node
 */
static void rotate_left(struct node **node) {
    if (node == NULL || *node == NULL) {
        return;
    }
    struct node *new_root = (*node)->right;

    (*node)->right = new_root->left;
    new_root->left = *node;
    *node = new_root;
}

/**
 * @brief Rotate the tree to the right.
 *
 * @param node A pointer to the current node
 */
static void rotate_right(struct node **node) {
    if (node == NULL || *node == NULL) {
        return;
    }
    struct node *new_root = (*node)->left;

    (*node)->left = new_root->right;
    new_root->right = *node;
    *node = new_root;
}

/**
 * @brief Balance the tree.
 *
 * @param node A pointer to the current node
 */
static void balance_tree(struct node **node) {
    if (node == NULL || *node == NULL) {
        return;
    }

    int balance = balance_factor(*node);
    if (balanced(balance)) {
        // tree is balanced
    } else if (left_heavy(balance)) {
        // left subtree is too tall
        balance = balance_factor((*node)->left);
        if (left_heavy(balance)) {
            rotate_right(node);
        } else {
            rotate_left(&(*node)->left);
            rotate_right(node);
        }
    } else {
        // right subtree is too tall
        balance = balance_factor((*node)->right);
        if (right_heavy(balance)) {
            rotate_left(node);
        } else {
            rotate_right(&(*node)->right);
            rotate_left(node);
        }
    }
}

/**
 * @brief Insert a new node into the tree.
 *
 * @param node A pointer to the current node
 * @param new A pointer to the new node to be inserted
 * @param cmp A pointer to the user-defined compare function
 */
static void insert_node(struct node **node, struct node *new, CMP_F cmp) {
    if (node == NULL || new == NULL || cmp == NULL) {
        return;
    } else if (*node == NULL) {
        *node = new;
        return;
    }

    int result = cmp(new->data, (*node)->data);
    if (result < 0) {
        // new node is less than current node
        insert_node(&(*node)->left, new, cmp);
    } else {
        // new node is greater than or equal to current node
        insert_node(&(*node)->right, new, cmp);
    }
    balance_tree(node);
}

static int tree_in_order(struct node *node, ACT_F func, void *addl_data) {
    if (node == NULL) {
        return SUCCESS;
    }

    // return early if action function returns non-zero
    int result = tree_in_order(node->left, func, addl_data);
    if (result != SUCCESS) {
        return result;
    }
    result = func(node->data, addl_data);
    if (result != SUCCESS) {
        return result;
    }
    result = tree_in_order(node->right, func, addl_data);
    if (result != SUCCESS) {
        return result;
    }
    return SUCCESS;
}

/**
 * @brief Add each node to the tree.
 *
 * Wrapper function needed because the arguments for tree_add() are in a
 * different order than the arguments for the action function.
 *
 * @param node_data A pointer to the data in the current node
 * @param addl_data A pointer to the tree to be built
 */
static int find_each(void *node_data, void *addl_data) {
    if (addl_data == NULL) {
        return EINVAL;
    }
    tree_t *found = (tree_t *)addl_data;
    // must compare the memory addresses of the data pointers to avoid adding
    // duplicate nodes
    if ((node_data != found->root->data) &&
        (found->cmp_func(node_data, found->root->data) == 0)) {
        return tree_add(found, node_data);
    }
    return SUCCESS;
}

/**
 * @brief Build an in-order queue for the iterator.
 *
 * Wrapper function needed because the arguments for queue_enqueue() are
 * in a different order than the arguments for the action function.
 *
 * @param node_data A pointer to the data in the current node
 * @param addl_data A pointer to the queue to be built
 */
static int build_iter(void *node_data, void *addl_data) {
    return queue_enqueue((queue_t *)addl_data, node_data);
}

/**
 * @brief Traverse the tree in post-order to remove nodes.
 *
 * @param node A pointer to the current node
 * @param free_func A pointer to the user-defined free function
 * @param addl_data A pointer to additional data to be passed to the action
 * function
 */
static void clear_nodes(struct node *node, FREE_F free_func) {
    if (node == NULL) {
        return;
    }

    clear_nodes(node->left, free_func);
    clear_nodes(node->right, free_func);
    if (free_func != NULL) {
        free_func(node->data);
    }
    free(node);
}

/* PUBLIC FUNCTIONS */

tree_t *tree_new(FREE_F free_func, CMP_F cmp_func) {
    if (cmp_func == NULL) {
        errno = EINVAL;
        return NULL;
    }
    tree_t *tree = malloc(sizeof(*tree));
    if (tree == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    tree->iterator = NULL;
    tree->root = NULL;
    tree->size = 0;
    tree->free_func = free_func;
    tree->cmp_func = cmp_func;
    return tree;
}

int tree_is_empty(tree_t *tree) {
    if (tree == NULL) {
        errno = EINVAL;
        return INVALID;
    }
    return tree->size == 0;
}

ssize_t tree_size(tree_t *tree) {
    if (tree == NULL) {
        errno = EINVAL;
        return INVALID;
    }
    return tree->size;
}

int tree_add(tree_t *tree, void *data) {
    if (tree == NULL) {
        return EINVAL;
    }
    struct node *node = create_node(data);
    if (node == NULL) {
        return ENOMEM;
    }
    insert_node(&tree->root, node, tree->cmp_func);
    tree->size++;
    return SUCCESS;
}

void *tree_remove(tree_t *tree, void *data) {
    if (tree == NULL) {
        errno = EINVAL;
        return NULL;
    } else if (tree->size == 0) {
        return NULL;
    }
    struct node **to_remove = tree_search(&tree->root, tree->cmp_func, data);
    if (*to_remove == NULL) {
        return NULL;
    }

    struct node *to_promote = NULL;

    if (!((*to_remove)->left && (*to_remove)->right)) {
        // node has 0 or 1 children
        if ((*to_remove)->left) {
            to_promote = (*to_remove)->left;
        } else {
            to_promote = (*to_remove)->right;
        }
    } else {
        // node has 2 children
        struct node **max_left = tree_max(&(*to_remove)->left);
        to_promote = *max_left;
        *max_left = (*max_left)->left;
        to_promote->left = (*to_remove)->left;
        to_promote->right = (*to_remove)->right;
    }

    void *removed = (*to_remove)->data;
    free(*to_remove);

    *to_remove = to_promote;
    balance_tree(to_remove);
    tree->size--;
    return removed;
}

ssize_t tree_remove_all(tree_t *tree, void *data) {
    if (tree == NULL) {
        errno = EINVAL;
        return INVALID;
    } else if (tree->size == 0) {
        return 0;
    }
    size_t count = 0;
    // must use tree_search() instead of tree_remove() in case NULL data is
    // allowed
    struct node **node = tree_search(&tree->root, tree->cmp_func, data);
    while (*node != NULL) {
        void *to_remove = tree_remove(tree, data);
        if (tree->free_func != NULL) {
            tree->free_func(to_remove);
        }
        count++;
        node = tree_search(&tree->root, tree->cmp_func, data);
    }
    // TODO: How to full rebalance the tree?
    balance_tree(&tree->root);
    return count;
}

int tree_contains(tree_t *tree, void *data) {
    if (tree == NULL) {
        errno = EINVAL;
        return INVALID;
    }
    return *tree_search(&tree->root, tree->cmp_func, data) != NULL;
}

void *tree_find_first(tree_t *tree, void *data) {
    if (tree == NULL) {
        errno = EINVAL;
        return NULL;
    }
    struct node **node = tree_search(&tree->root, tree->cmp_func, data);
    if (*node == NULL) {
        return NULL;
    }
    return (*node)->data;
}

tree_t *tree_find_all(tree_t *tree, void *data) {
    if (tree == NULL) {
        errno = EINVAL;
        return NULL;
    }

    tree_t *found = tree_new(NULL, tree->cmp_func);
    if (found == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    struct node **node = tree_search(&tree->root, tree->cmp_func, data);
    if (*node == NULL) {
        // tree is empty, or node not found
        return found;
    }

    // add the first node to the new tree
    int results = tree_add(found, (*node)->data);
    if (results != SUCCESS) {
        tree_delete(&found);
        errno = results;
        return NULL;
    }
    results = tree_in_order(tree->root, find_each, found);
    if (results != SUCCESS) {
        tree_delete(&found);
        errno = results;
        return NULL;
    }
    return found;
}

int tree_foreach(tree_t *tree, ACT_F act_func, void *addl_data) {
    if (tree == NULL || act_func == NULL) {
        // set errno instead of returning EINVAL to differentiate between
        // tree_foreach() failing and act_func() failing
        errno = EINVAL;
        return INVALID;
    }
    return tree_in_order(tree->root, act_func, addl_data);
}

int tree_iterator_reset(tree_t *tree) {
    if (tree == NULL) {
        return EINVAL;
    } else if (!tree_is_empty(tree)) {
        // build a new iterator
        queue_destroy(&tree->iterator);
        tree->iterator = queue_init(tree->size, NULL, tree->cmp_func);
        if (tree->iterator == NULL) {
            return ENOMEM;
        }
        int results = tree_in_order(tree->root, build_iter, tree->iterator);
        if (results != SUCCESS) {
            queue_destroy(&tree->iterator);
        }
        return results;
    }
    return SUCCESS;
}

void *tree_iterator_next(tree_t *tree) {
    if (tree == NULL) {
        errno = EINVAL;
        return NULL;
    } else if (tree->iterator != NULL) {
        return queue_dequeue(tree->iterator);
    }
    return NULL;
}

int tree_clear(tree_t *tree) {
    if (tree == NULL) {
        return EINVAL;
    } else if (tree->size == 0) {
        return SUCCESS;
    }
    clear_nodes(tree->root, tree->free_func);
    tree->root = NULL;
    tree->size = 0;
    queue_destroy(&tree->iterator);
    return SUCCESS;
}

int tree_delete(tree_t **tree) {
    if (tree == NULL || *tree == NULL) {
        return EINVAL;
    }
    tree_clear(*tree);
    queue_destroy(&(*tree)->iterator);
    free(*tree);
    *tree = NULL;
    return SUCCESS;
}
