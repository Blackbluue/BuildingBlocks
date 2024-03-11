#include "linked_list.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* DATA */

#define SUCCESS 0
#define INVALID -1

/**
 * @brief structure of a list node
 *
 * @param position is the position in the list relevant to the head (1)
 * @param data void pointer to whatever data that list points to
 * @param prev pointer to the node before it
 * @param next pointer to the node after it
 */
typedef struct list_node_t {
    size_t position;
    void *data;
    struct list_node_t *next;
} list_node_t;

/**
 * @brief Structure of a list object
 *
 * @param size is the number of nodes the list is currently storing
 * @param head pointer to the head node
 * @param tail pointer to the tail node
 * @param current pointer to the current node for iteration
 * @param customfree pointer to the user defined free function
 * @param compare_function pointer to the user defined compare function
 */
struct list_t {
    size_t size;
    list_node_t *head;
    list_node_t *tail;
    list_node_t *current;
    FREE_F customfree;
    CMP_F compare_function;
};

typedef int (*LOCAL_ACT_F)(list_node_t *node_data, void *addl_data);

/* PRIVATE FUNCTIONS*/

/**
 * @brief Create a new list node.
 *
 * @param data data to be stored in the node
 * @param next pointer to the next node in the list
 * @param position position of the node in the list
 * @return list_node_t* pointer to the new node
 */
static list_node_t *create_node(void *data, list_node_t *next,
                                size_t position) {
    list_node_t *new_node = malloc(sizeof(*new_node));
    if (new_node == NULL) {
        return NULL;
    }
    new_node->position = position;
    new_node->data = data;
    new_node->next = next;
    return new_node;
}

/**
 * @brief Increment the position of the node
 *
 * @param node The node to be incremented
 * @param data unused
 * @return int 0 on success, EINVAL if node is NULL
 */
static int inc_pos(list_node_t *node, void *data) {
    if (node == NULL) {
        return EINVAL;
    }
    node->position++;
    return SUCCESS;
}

/**
 * @brief Decrement the position of the node
 *
 * @param node The node to be decremented
 * @param data unused
 * @return int 0 on success, EINVAL if node is NULL
 */
static int dec_pos(list_node_t *node, void *data) {
    if (node == NULL) {
        return EINVAL;
    }
    if (node->position > 0) {
        node->position--;
    }
    return SUCCESS;
}

/**
 * @brief Merge two sorted lists into one sorted list
 *
 * @param firstNode The first list
 * @param secondNode The second list
 * @param cmp_f The compare function to be used
 * @return list_node_t* The head of the sorted list
 */
static list_node_t *merge(list_node_t *firstNode, list_node_t *secondNode,
                          CMP_F cmp_f) {
    if (firstNode == NULL && secondNode == NULL) {
        return NULL;
    } else if (cmp_f == NULL) {
        return NULL;
    } else if (firstNode == NULL) {
        return secondNode;
    } else if (secondNode == NULL) {
        return firstNode;
    }
    list_node_t *merged;
    if (cmp_f(firstNode->data, secondNode->data) <= 0) {
        merged = firstNode;
        firstNode = firstNode->next;
    } else {
        merged = secondNode;
        secondNode = secondNode->next;
    }

    list_node_t **current_node = &merged->next;
    // determine order of nodes
    while (firstNode != NULL && secondNode != NULL) {
        if (cmp_f(firstNode->data, secondNode->data) <= 0) {
            *current_node = firstNode;
            firstNode = firstNode->next;
        } else {
            *current_node = secondNode;
            secondNode = secondNode->next;
        }
        current_node = &(*current_node)->next;
    }

    // any remaining nodes gets inserted in the list
    while (firstNode != NULL) {
        *current_node = firstNode;
        firstNode = firstNode->next;
        current_node = &(*current_node)->next;
    }

    while (secondNode != NULL) {
        *current_node = secondNode;
        secondNode = secondNode->next;
        current_node = &(*current_node)->next;
    }
    // return the head of the sorted list
    return merged;
}

/**
 * @brief Find the middle node of the list
 *
 * @param head The head of the list
 * @return list_node_t* The middle node
 */
static list_node_t *middle(const list_node_t *head) {
    if (head == NULL) {
        return NULL;
    }
    list_node_t *slow = (list_node_t *)head;
    list_node_t *fast = head->next;

    while (slow->next && (fast && fast->next)) {
        slow = slow->next;
        fast = fast->next->next;
    }
    return slow;
}

/**
 * @brief Sort the list using merge sort
 *
 * @param head The head of the list
 * @param cmp_f The compare function to be used
 * @return list_node_t* The head of the sorted list
 */
static list_node_t *merge_sort(list_node_t *head, CMP_F cmp_f) {
    if (head == NULL || cmp_f == NULL) {
        return NULL;
    }
    // merge sort algorithm taken from
    // https://www.geeksforgeeks.org/merge-sort-for-linked-list/
    if (head->next == NULL) {
        return head;
    }

    list_node_t *mid = middle(head);
    list_node_t *head2 = mid->next;
    mid->next = NULL;
    // recursive call to sort() hence dividing our problem,
    // and then merging the solution
    list_node_t *final_head =
        merge(merge_sort(head, cmp_f), merge_sort(head2, cmp_f), cmp_f);
    return final_head;
}

/**
 * @brief Perform an action on each node in the list
 *
 * @param head The head of the list
 * @param size The size of the list
 * @param action The action to be performed
 * @param addl_data Additional data to be passed to the action function
 * @return int SUCCESS, or the error code returned by the action function
 */
static int local_foreach(list_node_t *head, size_t size, LOCAL_ACT_F action,
                         void *addl_data) {
    if (head == NULL || action == NULL) {
        return EINVAL;
    }
    list_node_t *current_node = head;
    for (size_t i = 0; i < size; i++) {
        int err = action(current_node, addl_data);
        if (err != SUCCESS) {
            return err;
        }
        current_node = current_node->next;
    }
    return SUCCESS;
}

/* PUBLIC FUNCTIONS*/

list_t *list_new(FREE_F free_f, CMP_F cmp_f) {
    list_t *list = malloc(sizeof(*list));
    if (list == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    list->size = 0;
    list->head = NULL;
    list->tail = NULL;
    list->current = NULL;
    list->customfree = free_f;
    list->compare_function = cmp_f;
    return list;
}

int list_push_head(list_t *list, void *data) {
    if (list == NULL) {
        return EINVAL;
    }
    list_node_t *new_node = create_node(data, list->head, 0);
    if (new_node == NULL) {
        return ENOMEM;
    }
    // adjust positions before adding new node
    local_foreach(list->head, list->size, inc_pos, NULL);
    list->head = new_node;
    list->size++;
    if (list->tail == NULL) {
        list->tail = new_node;
    }
    // maintain circular list
    list->tail->next = list->head;
    return SUCCESS;
}

int list_push_tail(list_t *list, void *data) {
    if (list == NULL) {
        return EINVAL;
    }
    // maintain circular list
    list_node_t *new_node = create_node(data, list->head, list->size);
    if (new_node == NULL) {
        return ENOMEM;
    }
    if (list->tail != NULL) {
        list->tail->next = new_node;
    }
    list->tail = new_node;
    list->size++;
    if (list->head == NULL) {
        list->head = new_node;
    }
    return SUCCESS;
}

int list_insert(list_t *list, void *data, size_t position) {
    if (list == NULL || position > list->size) {
        return EINVAL;
    } else if (position == 0) {
        return list_push_head(list, data);
    } else if (position == list->size) {
        return list_push_tail(list, data);
    }

    list_node_t *current_node = list->head;
    for (size_t i = 0; i < position - 1; i++) {
        current_node = current_node->next;
    }
    list_node_t *temp = create_node(data, current_node->next, position);
    if (temp == NULL) {
        return ENOMEM;
    }
    local_foreach(current_node->next, list->size - position, inc_pos, NULL);
    current_node->next = temp;
    list->size++;
    return SUCCESS;
}

void *list_get(const list_t *list, size_t position) {
    if (list == NULL || position >= list->size) {
        errno = EINVAL;
        return NULL;
    }
    list_node_t *current_node = list->head;
    for (size_t i = 0; i < position; i++) {
        current_node = current_node->next;
    }
    return current_node->data;
}

ssize_t list_size(const list_t *list) {
    if (list == NULL) {
        errno = EINVAL;
        return INVALID;
    }
    return list->size;
}

int list_is_empty(const list_t *list) {
    if (list == NULL) {
        errno = EINVAL;
        return INVALID;
    }
    return list->size == 0;
}

void *list_pop_head(list_t *list) {
    if (list == NULL) {
        errno = EINVAL;
        return NULL;
    } else if (list->head == NULL) {
        return NULL;
    }

    list_node_t *to_pop = list->head;
    list->size--;
    if (list->size == 0) {
        list->head = NULL;
        list->tail = NULL;
    } else {
        list->head = list->head->next;
        local_foreach(list->head, list->size, dec_pos, NULL);
        // maintain circular list
        list->tail->next = list->head;
    }
    void *value = to_pop->data;
    free(to_pop);
    return value;
}

void *list_pop_tail(list_t *list) {
    if (list == NULL) {
        errno = EINVAL;
        return NULL;
    } else if (list->tail == NULL) {
        return NULL;
    }

    // find the node before the tail
    list_node_t *current_node;
    list->size--;
    if (list->size == 0) {
        current_node = NULL;
        list->head = NULL;
    } else {
        current_node = list->head;
        while (current_node->next != list->tail) {
            current_node = current_node->next;
        }
        // maintain circular list
        current_node->next = list->head;
    }

    list_node_t *to_pop = list->tail;
    list->tail = current_node;
    void *value = to_pop->data;
    free(to_pop);
    return value;
}

void *list_peek_head(const list_t *list) {
    if (list == NULL) {
        errno = EINVAL;
        return NULL;
    } else if (list->head == NULL) {
        return NULL;
    }
    return list->head->data;
}

void *list_peek_tail(const list_t *list) {
    if (list == NULL) {
        errno = EINVAL;
        return NULL;
    } else if (list->tail == NULL) {
        return NULL;
    }
    return list->tail->data;
}

void *list_remove(list_t *list, void *item_to_remove) {
    if (list == NULL) {
        errno = EINVAL;
        return NULL;
    } else if (list->compare_function == NULL) {
        errno = ENOTSUP;
        return NULL;
    } else if (list->size == 0) {
        return NULL;
    }

    // find node to remove and its previous node
    list_node_t *current_node = list->head;
    list_node_t *prev_node = NULL;
    for (size_t i = 0; i < list->size; i++) {
        if (list->compare_function(item_to_remove, current_node->data) == 0) {
            if (list->size == 1) {
                // list will be empty after removal
                list->head = NULL;
                list->tail = NULL;
            } else if (current_node == list->head) {
                list->head = current_node->next;
                // adjust positions after removal
                local_foreach(list->head, list->size - 1, dec_pos, NULL);
                // maintain circular list
                list->tail->next = list->head;
            } else if (current_node == list->tail) {
                list->tail = prev_node;
                // maintain circular list
                prev_node->next = list->head;
            } else {
                prev_node->next = current_node->next;
                // adjust positions after removal
                size_t size = list->size - current_node->position - 1;
                local_foreach(current_node->next, size, dec_pos, NULL);
            }
            list->size--;
            void *data = current_node->data;
            free(current_node);
            return data;
        }
        prev_node = current_node;
        current_node = current_node->next;
    }
    return NULL;
}

int list_foreach_call(list_t *list, ACT_F action_function, void *addl_data) {
    if (list == NULL || action_function == NULL) {
        return EINVAL;
    }
    int err = SUCCESS;
    list_node_t *current_node = list->head;
    for (size_t i = 0; i < list->size; i++) {
        err = action_function(&current_node->data, addl_data);
        if (err != SUCCESS) {
            return err;
        }
        current_node = current_node->next;
    }
    return err;
}

int list_iterator_reset(list_t *list) {
    if (list == NULL) {
        return EINVAL;
    }
    list->current = list->head;
    return SUCCESS;
}

void *list_iterator_next(list_t *list) {
    if (list == NULL) {
        errno = EINVAL;
        return NULL;
    } else if (list->current == NULL) {
        errno = EOPNOTSUPP;
        return NULL;
    }
    void *data = list->current->data;
    if (list->current == list->tail) {
        // end of list. need to check because list is circular
        list->current = NULL;
    } else {
        list->current = list->current->next;
    }
    return data;
}

void *list_find_first(const list_t *list, const void *search_data) {
    if (list == NULL) {
        errno = EINVAL;
        return NULL;
    } else if (list->compare_function == NULL) {
        errno = ENOTSUP;
        return NULL;
    } else if (list->size == 0) {
        return NULL;
    }

    list_node_t *current_node = list->head;
    for (size_t i = 0; i < list->size; i++) {
        if (list->compare_function(search_data, current_node->data) == 0) {
            return current_node->data;
        }
        current_node = current_node->next;
    }
    return NULL;
}

list_t *list_find_all(const list_t *list, const void *search_data) {
    if (list == NULL) {
        errno = EINVAL;
        return NULL;
    } else if (list->compare_function == NULL) {
        errno = ENOTSUP;
        return NULL;
    } else if (list->size == 0) {
        return NULL;
    }

    list_t *found_list = list_new(NULL, list->compare_function);
    if (found_list == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    list_node_t *current_node = list->head;
    for (size_t i = 0; i < list->size; i++) {
        if (list->compare_function(search_data, current_node->data) == 0) {
            int err = list_push_tail(found_list, current_node->data);
            if (err != SUCCESS) {
                list_delete(&found_list);
                errno = err;
                return NULL;
            }
        }
        current_node = current_node->next;
    }

    return found_list;
}

int list_sort(list_t *list) {
    if (list == NULL) {
        return EINVAL;
    } else if (list->compare_function == NULL) {
        return ENOTSUP;
    } else if (list->size <= 1) {
        return SUCCESS;
    }

    // make list not circular for sorting
    list->tail->next = NULL;
    list->head = merge_sort(list->head, list->compare_function);
    list->tail = list->head;
    size_t position = 0;
    while (list->tail->next != NULL) {
        // adjust positions after removal
        list->tail->position = position++;
        list->tail = list->tail->next;
    }
    list->tail->position = position;
    // make list circular again after sorting
    list->tail->next = list->head;
    return SUCCESS;
}

int list_clear(list_t *list) {
    if (list == NULL) {
        return EINVAL;
    } else if (list->size == 0) {
        return SUCCESS;
    }

    list_node_t *current_node = list->head;
    list_node_t *next_node = NULL;
    for (size_t i = 0; i < list->size; i++) {
        next_node = current_node->next;
        if (list->customfree != NULL) {
            list->customfree(current_node->data);
        }
        free(current_node);
        current_node = next_node;
    }
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    return SUCCESS;
}

int list_delete(list_t **list_address) {
    if (list_address == NULL || *list_address == NULL) {
        return EINVAL;
    }
    list_clear(*list_address);
    free(*list_address);
    *list_address = NULL;
    return SUCCESS;
}
