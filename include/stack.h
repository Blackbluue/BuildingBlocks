#ifndef _STACK_H
#define _STACK_H

#include <unistd.h>

/* DATA */
/**
 * @brief A function pointer to a custom-defined delete function
 *        required to support deletion/memory deallocation of
 *        arbitrary data types. For simple data types, this function
 *        pointer can simply point to the free function. For more complex data
 *        types, this function should free all of the sub items that are
 *        allocated in the data.
 *
 */
typedef void (*FREE_F)(void *);

typedef struct stack_t stack_t;

/* FUNCTIONS */
/**
 * @brief Create a new stack.
 *
 * The capacity of the stack is set to the value passed in. This value is not
 * allowed to be 0. The customfree function is used to free the data in the
 * stack. This function pointer can be NULL if the stack is not intended to
 * free the data.
 *
 *
 * @param capacity max number of nodes the stack will hold
 * @param customfree pointer to the free function to be used with that list
 * @returns pointer to allocated stack on SUCCESS, NULL on failure
 */
stack_t *stack_init(size_t capacity, FREE_F customfree);

/**
 * @brief Get the max capacity of the stack.
 *
 * @param stack pointer stack object
 * @return ssize_t capacity of the stack, -1 if stack is NULL
 */
ssize_t stack_get_capacity(stack_t *stack);

/**
 * @brief Get the current size of the stack.
 *
 * @param stack pointer stack object
 * @return ssize_t size of the stack, -1 if stack is NULL
 */
ssize_t stack_get_size(stack_t *stack);

/**
 * @brief Check if stack is full.
 *
 * @param stack pointer stack object
 * @return 0 if stack is not full or stack is NULL, non-zero if stack is full
 */
int stack_is_full(stack_t *stack);

/**
 * @brief Check if stack is empty.
 *
 * @param stack pointer stack object
 * @return 0 if stack is not empty or stack is NULL, non-zero if stack is empty
 */
int stack_is_empty(stack_t *stack);

/**
 * @brief Push a new node into the stack.
 *
 * @param stack pointer to stack pointer to push the node into
 * @param data data to be pushed into node
 * @return 0 on success, EINVAL if stack is NULL, ENOMEM on memory allocation
 * failure, EOVERFLOW if stack is full
 */
int stack_push(stack_t *stack, void *data);

/**
 * @brief Pop the front node out of the stack.
 *
 * @param stack pointer to stack pointer to pop the node off of
 * @return pointer to popped node on SUCCESS, NULL on failure
 */
void *stack_pop(stack_t *stack);

/**
 * @brief Get the data from the node at the front of the stack without popping.
 *
 * @param stack pointer to stack pointer to peek
 * @return pointer to peeked node on SUCCESS, NULL on failure
 */
void *stack_peek(stack_t *stack);

/**
 * @brief Get the data from the node at the specified position.
 *
 * @param stack pointer to stack pointer to peek
 * @param position position of node to peek
 * @return pointer to peeked node on SUCCESS, NULL on failure
 */
void *stack_get(stack_t *stack, size_t position);

/**
 * @brief Clear all nodes out of a stack.
 *
 * If a custom free function is defined, it will be used to free the data in
 * the stack. Otherwise, the data will not be freed.
 *
 * @param stack pointer to stack pointer to clear out
 * @return 0 on success, EINVAL if stack is NULL
 */
int stack_clear(stack_t *stack);

/**
 * @brief Delete a stack.
 *
 * If a custom free function is defined, it will be used to free the data in
 * the stack. Otherwise, the data will not be freed. The stack will then be
 * freed, and the pointer will be set to NULL.
 *
 * @param stack pointer to stack pointer to be destroyed
 * @return 0 on success, EINVAL if stack is NULL
 */
int stack_destroy(stack_t **stack);

#endif
