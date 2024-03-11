#define _DEFAULT_SOURCE
#include "array_list.h"
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* DATA */

#define SUCCESS 0  // no error
#define INVALID -1 // invalid input

/**
 * @brief The array list structure.
 *
 * @param wrap The pointer to the array, if the array is wrapped.
 * @param array The array of elements.
 * @param free_f The function to free the elements.
 * @param cmp_f The function to compare the elements.
 * @param size The number of elements in the array.
 * @param mem_sz The size of each element.
 * @param capacity The capacity of the array.
 */
struct arr_list_t {
    void **wrap;
    void *array;
    FREE_F free_f;
    CMP_F cmp_f;
    size_t size;
    size_t mem_sz;
    size_t capacity;
    size_t iter_pos;
};

/* PRIVATE FUNCTIONS */

/**
 * @brief Adjusts the size of the array.
 *
 * Possible error codes:
 * - EINVAL: The array list is NULL.
 * - ENOMEM: The memory allocation failed.
 *
 * @param list The array list.
 * @param new_size The new size of the array.
 * @return int 0 if successful, non-zero error code on error.
 */
static int adjust_size(arr_list_t *list, size_t new_size) {
    if (list == NULL) {
        return EINVAL;
    }
    void *new_array = reallocarray(list->array, new_size, list->mem_sz);
    if (new_array == NULL) {
        return ENOMEM;
    }
    list->array = new_array;
    if (list->wrap != NULL) {
        *list->wrap = new_array;
    }
    list->capacity = new_size;
    return SUCCESS;
}

/**
 * @brief Shifts the elements in the array forward.
 *
 * Possible error codes:
 * - EINVAL: The array list is NULL or the position is out of bounds.
 *
 * This function shifts the elements in the array forward one index, starting at
 * the given position. The opened spot at the end of the array is not cleared.
 *
 * @param list The array list.
 * @param position The position to shift forward.
 * @return int 0 if successful, non-zero error code on error.
 */
static int shift_forward(arr_list_t *list, size_t position) {
    if (list == NULL || position >= list->size) {
        return EINVAL;
    }
    uint8_t *dest = (uint8_t *)list->array + (position * list->mem_sz);
    memmove(dest, dest + list->mem_sz,
            (list->size - position - 1) * list->mem_sz);
    return SUCCESS;
}

/**
 * @brief Shifts the elements in the array back.
 *
 * This function shifts the elements in the array back, starting at the given
 * position. The opened spot is not cleared. If the position is out of bounds,
 * it is set to the end of the array. If the array list is NULL, this function
 * returns NULL.
 *
 * @param list The array list.
 * @param position The position to shift back.
 * @return void* The pointer to the position, or NULL.
 */
static void *shift_back(arr_list_t *list, size_t position) {
    if (list == NULL) {
        return NULL;
    } else if (position > list->size) {
        position = list->size;
    }
    uint8_t *dest = (uint8_t *)list->array + (position * list->mem_sz);
    memmove(dest + list->mem_sz, dest, (list->size - position) * list->mem_sz);
    return dest;
}

/* PUBLIC FUNCTIONS */

arr_list_t *arr_list_new(FREE_F free_f, CMP_F cmp_f, size_t nmemb,
                         size_t size) {
    if (nmemb == 0 || size == 0) {
        errno = EINVAL;
        return NULL;
    }
    arr_list_t *list = malloc(sizeof(*list));
    if (list == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    list->wrap = NULL;
    list->array = calloc(nmemb, size);
    if (list->array == NULL) {
        free(list);
        errno = ENOMEM;
        return NULL;
    }
    list->free_f = free_f;
    list->cmp_f = cmp_f;
    list->size = 0;
    list->mem_sz = size;
    list->capacity = nmemb;
    list->iter_pos = 0;
    return list;
}

arr_list_t *arr_list_wrap(FREE_F free_f, CMP_F cmp_f, size_t nmemb, size_t size,
                          void **arr) {
    if (nmemb == 0 || size == 0) {
        errno = EINVAL;
        return NULL;
    } else if (arr == NULL) {
        return arr_list_new(free_f, cmp_f, nmemb, size);
    }
    arr_list_t *list = malloc(sizeof(*list));
    if (list == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    list->wrap = arr;
    if (*list->wrap == NULL) {
        *list->wrap = calloc(nmemb, size);
        if (*list->wrap == NULL) {
            free(list);
            errno = ENOMEM;
            return NULL;
        }
    }
    list->array = *list->wrap;
    list->free_f = free_f;
    list->cmp_f = cmp_f;
    list->size = 0;
    list->mem_sz = size;
    list->capacity = nmemb;
    list->iter_pos = 0;
    return list;
}

ssize_t arr_list_size(const arr_list_t *list) {
    if (list == NULL) {
        return INVALID;
    }
    return list->size;
}

ssize_t arr_list_capacity(const arr_list_t *list) {
    if (list == NULL) {
        return INVALID;
    }
    return list->capacity;
}

int arr_list_resize(arr_list_t *list, size_t new_capacity) {
    if (list == NULL) {
        return EINVAL;
    } else if (new_capacity <= list->capacity) {
        return SUCCESS;
    }
    return adjust_size(list, new_capacity);
}

int arr_list_trim(arr_list_t *list) {
    if (list == NULL) {
        return EINVAL;
    } else if (list->size == list->capacity) {
        return SUCCESS;
    }
    return adjust_size(list, list->size);
}

int arr_list_is_empty(const arr_list_t *list) {
    if (list == NULL) {
        return INVALID;
    }
    return list->size == 0;
}

int arr_list_is_full(const arr_list_t *list) {
    if (list == NULL) {
        return INVALID;
    }
    return list->size == list->capacity;
}

int arr_list_insert(arr_list_t *list, void *data, size_t position) {
    if (list == NULL || position > list->size) {
        return EINVAL;
    }
    if (list->size == list->capacity) {
        if (adjust_size(list, list->capacity * 2) != SUCCESS) {
            return ENOMEM;
        }
    }
    void *dest = shift_back(list, position);
    memcpy(dest, data, list->mem_sz);
    list->size++;
    return SUCCESS;
}

int arr_list_set(arr_list_t *list, void *data, size_t position, void *old) {
    if (list == NULL || position >= list->size) {
        return EINVAL;
    }
    void *elem = (uint8_t *)list->array + (position * list->mem_sz);
    if (old != NULL) {
        memcpy(old, elem, list->mem_sz);
    }
    memcpy(elem, data, list->mem_sz);
    return SUCCESS;
}

void *arr_list_get(const arr_list_t *list, size_t position) {
    if (list == NULL || position >= list->size) {
        errno = EINVAL;
        return NULL;
    }
    return (uint8_t *)list->array + (position * list->mem_sz);
}

void *arr_list_pop(arr_list_t *list, size_t position) {
    if (list == NULL || position >= list->size) {
        errno = EINVAL;
        return NULL;
    }
    void *old = malloc(list->mem_sz);
    if (old == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    void *element = (uint8_t *)list->array + (position * list->mem_sz);
    memcpy(old, element, list->mem_sz);
    shift_forward(list, position);
    list->size--;
    return old;
}

int arr_list_remove(arr_list_t *list, void *item_to_remove) {
    if (list == NULL) {
        errno = EINVAL;
        return INVALID;
    } else if (list->cmp_f == NULL) {
        errno = ENOTSUP;
        return INVALID;
    } else if (list->size == 0) {
        return false;
    }

    for (size_t i = 0; i < list->size; i++) {
        void *element = (uint8_t *)list->array + (i * list->mem_sz);
        if (list->cmp_f(item_to_remove, element) == 0) {
            shift_forward(list, i);
            list->size--;
            return true;
        }
    }
    return false;
}

int arr_list_foreach(arr_list_t *list, ACT_F action_function, void *addl_data) {
    if (list == NULL || action_function == NULL) {
        return EINVAL;
    }
    for (size_t i = 0; i < list->size; i++) {
        void *element = (uint8_t *)list->array + (i * list->mem_sz);
        int err = action_function(element, addl_data);
        if (err != SUCCESS) {
            return err;
        }
    }
    return SUCCESS;
}

int arr_list_iterator_reset(arr_list_t *list) {
    if (list == NULL) {
        return EINVAL;
    }
    list->iter_pos = 0;
    return SUCCESS;
}

void *arr_list_iterator_next(arr_list_t *list) {
    if (list == NULL) {
        errno = EINVAL;
        return NULL;
    } else if (list->iter_pos >= list->size) {
        errno = ENOTSUP;
        return NULL;
    }
    return (uint8_t *)list->array + (list->iter_pos++ * list->mem_sz);
}

int arr_list_index_of(const arr_list_t *list, const void *data, ssize_t *idx) {
    if (list == NULL) {
        return EINVAL;
    } else if (list->cmp_f == NULL) {
        return ENOTSUP;
    }
    *idx = INVALID;

    for (size_t i = 0; i < list->size; i++) {
        void *element = (uint8_t *)list->array + (i * list->mem_sz);
        if (list->cmp_f(data, element) == 0) {
            *idx = i;
            return SUCCESS;
        }
    }
    return SUCCESS;
}

int arr_list_sort(arr_list_t *list) {
    if (list == NULL) {
        return EINVAL;
    } else if (list->cmp_f == NULL) {
        return ENOTSUP;
    } else if (list->size == 0 || list->size == 1) {
        return SUCCESS;
    }
    qsort(list->array, list->size, list->mem_sz, list->cmp_f);
    return SUCCESS;
}

int arr_list_clear(arr_list_t *list) {
    if (list == NULL) {
        return EINVAL;
    }
    if (list->free_f != NULL) {
        for (size_t i = 0; i < list->size; i++) {
            list->free_f((uint8_t *)list->array + (i * list->mem_sz));
        }
    }
    memset(list->array, 0, list->size * list->mem_sz);
    list->size = 0;
    return SUCCESS;
}

int arr_list_delete(arr_list_t *list) {
    if (list == NULL) {
        return EINVAL;
    }
    // only clear list if its not wrapping something else
    if (list->wrap == NULL) {
        arr_list_clear(list);
        free(list->array);
    }
    free(list);
    return SUCCESS;
}
