#ifndef THREADPOOL_TYPES_H
#define THREADPOOL_TYPES_H

/* DATA */

#define THREADPOOL_ATTR_SIZE 28

typedef union {
    char __size[THREADPOOL_ATTR_SIZE];
} threadpool_attr_t;

#endif /* THREADPOOL_TYPES_H */
