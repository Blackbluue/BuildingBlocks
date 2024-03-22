#include "stack.h"
#include <errno.h>
#include <stdlib.h>

/* DATA */

#define SUCCESS 0
#define INVALID -1

/**
 * @brief structure of a stack object
 *
 * @param capacity is the number of nodes the stack can hold
 * @param size is the number of nodes the stack is currently storing
 * @param arr is the array containing the stack node pointers
 * @param customfree pointer to the user defined free function
 * @param compare_function pointer to the user defined compare function
 */
typedef struct stack_t {
    size_t capacity;
    size_t size;
    void **arr;
    FREE_F customfree;
} stack_t;

/* PUBLIC FUNCTIONS */

stack_t *stack_init(size_t capacity, FREE_F customfree) {
    if (capacity == 0) {
        return NULL;
    }
    stack_t *stack = malloc(sizeof(*stack));
    if (stack == NULL) {
        return NULL;
    }
    stack->arr = malloc(sizeof(*(stack->arr)) * capacity);
    if (stack->arr == NULL) {
        free(stack);
        return NULL;
    }
    stack->capacity = capacity;
    stack->size = 0;
    stack->customfree = customfree;
    return stack;
}

ssize_t stack_get_capacity(stack_t *stack) {
    if (stack == NULL) {
        return INVALID;
    }
    return stack->capacity;
}

ssize_t stack_get_size(stack_t *stack) {
    if (stack == NULL) {
        return INVALID;
    }
    return stack->size;
}

int stack_is_full(stack_t *stack) {
    if (stack == NULL) {
        return 0;
    }
    return stack->size == stack->capacity;
}

int stack_is_empty(stack_t *stack) {
    if (stack == NULL) {
        return 0;
    }
    return stack->size == 0;
}

int stack_push(stack_t *stack, void *data) {
    if (stack == NULL) {
        return EINVAL;
    } else if (stack->size == stack->capacity) {
        return EOVERFLOW;
    }
    stack->arr[stack->size++] = data;
    return SUCCESS;
}

void *stack_pop(stack_t *stack) {
    if (stack == NULL || stack->size == 0) {
        return NULL;
    }
    // no need to set the popped node to NULL
    return stack->arr[--stack->size];
}

void *stack_peek(stack_t *stack) {
    if (stack == NULL || stack->size == 0) {
        return NULL;
    }
    return stack->arr[stack->size - 1];
}

void *stack_get(stack_t *stack, size_t position) {
    if (stack == NULL || position >= stack->size) {
        return NULL;
    }
    return stack->arr[position];
}

int stack_clear(stack_t *stack) {
    if (stack == NULL) {
        return EINVAL;
    } else if (stack->size == 0) {
        return SUCCESS;
    }
    for (size_t i = 0; i < stack->size; i++) {
        if (stack->customfree != NULL) {
            stack->customfree(stack->arr[i]);
        }
    }
    stack->size = 0;
    return SUCCESS;
}

int stack_destroy(stack_t **stack) {
    if (stack == NULL || *stack == NULL) {
        return EINVAL;
    }
    stack_clear(*stack);
    free((*stack)->arr);
    free(*stack);
    *stack = NULL;
    return SUCCESS;
}
