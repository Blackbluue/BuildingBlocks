#ifndef WEIGHTED_GRAPH_H
#define WEIGHTED_GRAPH_H
#include "linked_list.h"
#include <unistd.h>

/* DATA */

/**
 * @brief A pointer to a user-defined free function.  This is used to free
 *        memory allocated for queue data.  For simple data types, this is
 *        just a pointer to the standard free function.  More complex structs
 *        stored in queues may require a function that calls free on multiple
 *        components.
 */
typedef void (*FREE_F)(void *);

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

typedef struct weighted_graph_t weighted_graph_t;

/* FUNCTIONS */

/**
 * @brief Create a weighted graph.
 *
 * If free_f is NULL, then data stored in the queue will not be free'd.
 *
 * On error, the function will return NULL and set errno. Possible error codes
 * are:
 * - EINVAL: compare function is NULL
 * - ENOMEM: memory allocation failed
 *
 * @param compare pointer to user defined compare function
 * @param free_f pointer to a function to free queue data
 * @return A pointer to a weighted graph.
 */
weighted_graph_t *graph_create(CMP_F cmp, FREE_F free_f);

/**
 * @brief Return the number of nodes in the graph.
 *
 * If graph is NULL, then the function will return -1 and set errno. Possible
 * error codes are:
 * - EINVAL: graph is NULL
 *
 * @param graph pointer to a weighted graph
 * @return The number of nodes in the graph, or non-zero on error.
 */
ssize_t graph_size(const weighted_graph_t *graph);

/**
 * @brief Add a node to the graph.
 *
 * Duplicate nodes are not allowed in the graph. If the graph already contains
 * the node, then the insertion will be ignored.
 *
 * Possible error codes are:
 * - EINVAL: graph or data are NULL
 * - ENOMEM: memory allocation failed
 *
 * @param graph pointer to a weighted graph
 * @param data pointer to data to be added to the graph
 * @return 0 on success, non-zero on failure
 */
int graph_add_node(weighted_graph_t *graph, void *data);

/**
 * @brief Remove a node from the graph.
 *
 * The function will remove the node that matches the data pointer,and
 * all edges connected to that node.
 *
 * Possible error codes are:
 * - EINVAL: graph or data are NULL
 * - ENOENT: data is not in the graph
 *
 * @param graph pointer to a weighted graph
 * @param data pointer to data to be removed from the graph
 * @return the removed item on success, NULL on failure
 */
void *graph_remove_node(weighted_graph_t *graph, void *data);

/**
 * @brief Iterate over all nodes in the graph.
 *
 * Perform the function func on each node in the graph. If func returns
 * non-zero, then the iteration will stop and return the error code.
 *
 * If an error occurs, then the function will return non-zero.
 * Possible error codes are:
 * - EINVAL: graph or func are NULL
 *
 * @param graph pointer to a weighted graph
 * @param func pointer to a function to be called on each node
 * @param obj pointer to additional data to be passed to func
 * @return 0 on success, non-zero on failure or if func returns non-zero
 */
int graph_iterate_nodes(weighted_graph_t *graph, ACT_F func, void *obj);

/**
 * @brief Iterate over all neighbors of a node in the graph.
 *
 * Perform the function func on each node surrounding a center node. If func
 * returns non-zero, then the iteration will stop and return the error code.
 *
 * If an error occurs, then the function will return non-zero.
 * Possible error codes are:
 * - EINVAL: graph, center, or func are NULL
 * - ENOENT: center is not in the graph
 *
 * @param graph pointer to a weighted graph
 * @param center pointer to a node in the graph
 * @param func pointer to a function to be called on each neighbor
 * @param obj pointer to additional data to be passed to func
 * @return 0 on success, non-zero on failure or if func returns non-zero
 */
int graph_iterate_neighbors(weighted_graph_t *graph, void *center, ACT_F func,
                            void *obj);

/**
 * @brief Find the shortest path between 2 nodes in the graph.
 *
 * The function will return a list of nodes that represent the shortest path
 * between the 2 given nodes. The list will be ordered from the start node to
 * the end node. If no path exists, then the function will return an empty list.
 *
 * The list will contain references to the user provided data contained within
 * the graph. The list itself must be freed by the user, but freeing the data
 * within the list will affect the graph. Sorting/lookup is provided via the
 * compare function given to the graph.
 *
 * If an error occurs, then the function will return NULL and set errno.
 * Possible error codes are:
 * - EINVAL: graph, start, or end are NULL
 * - ENOENT: start or end are not in the graph
 * - ENOMEM: memory allocation failed
 *
 * @param graph pointer to a weighted graph
 * @param start pointer to the start node
 * @param end pointer to the end node
 * @return list_t* a list of nodes representing the shortest path, NULL on error
 */
list_t *graph_find_path(weighted_graph_t *graph, const void *start,
                        const void *end);

/**
 * @brief Check if the graph contains a node.
 *
 * If an error occurs, then the function will return -1 and errno is set.
 * Possible error codes are:
 * - EINVAL: graph or data are NULL
 *
 * @param graph pointer to a weighted graph
 * @param data pointer to data to be searched for in the graph
 * @return 0 if the graph does not contain the node, non-zero if the graph
 * contain the node, -1 on error
 */
int graph_contains(const weighted_graph_t *graph, const void *data);

/**
 * @brief Add an edge to the graph.
 *
 * Creates an edge between the 2 given nodes in the graph. If the edge already
 * exists, then the weight will be updated.
 *
 * Possible error codes are:
 * - EINVAL: graph, src, or dst are NULL
 * - ENOENT: src or dst are not in the graph
 * - ENOMEM: memory allocation failed
 *
 * @param graph pointer to a weighted graph
 * @param src pointer to the source node
 * @param dst pointer to the destination node
 * @param weight the weight of the edge
 * @return 0 on success, non-zero on failure
 */
int graph_add_edge(weighted_graph_t *graph, void *src, void *dst,
                   double weight);

/**
 * @brief Get the weight of an edge.
 *
 * Return the weight of the edge between the 2 given nodes in the graph.
 *
 * If an error occurs, then the function will return NAN and errno is set.
 * Possible error codes are:
 * - EINVAL: graph, src, or dst are NULL
 * - ENOENT: src or dst are not in the graph, or src not pointing to dst
 *
 * @param graph pointer to a weighted graph
 * @param src pointer to the source node
 * @param dst pointer to the destination node
 * @return the weight of the edge, NAN on error
 */
double graph_get_edge_weight(const weighted_graph_t *graph, const void *src,
                             const void *dst);

/**
 * @brief Remove an edge from the graph.
 *
 * Remove the edge between the 2 given nodes in the graph.
 *
 * If an error occurs, then the function will return non-zero.
 * Possible error codes are:
 * - EINVAL: graph, src, or dst are NULL
 * - ENOENT: src or dst are not in the graph, or src not pointing to dst
 *
 * @param graph pointer to a weighted graph
 * @param src pointer to the source node
 * @param dst pointer to the destination node
 * @return int 0 on success, non-zero on failure
 */
int graph_remove_edge(weighted_graph_t *graph, const void *src,
                      const void *dst);

/**
 * @brief Get the out degree of a node.
 *
 * Return the number of edges that start at the given node.
 *
 * If an erro occurs, then the function will return -1.
 * Possible error codes are:
 * - EINVAL: graph or src are NULL
 * - ENOENT: src is not in the graph
 *
 * @param graph pointer to a weighted graph
 * @param src pointer to the node
 * @return ssize_t the out degree of the node, -1 on error
 */
ssize_t graph_out_degree_size(const weighted_graph_t *graph, const void *src);

/**
 * @brief Get the in degree of a node.
 *
 * Return the number of edges that point to the given node.
 *
 * If an erro occurs, then the function will return -1.
 * Possible error codes are:
 * - EINVAL: graph or dst are NULL
 * - ENOENT: dst is not in the graph
 *
 * @param graph pointer to a weighted graph
 * @param dst pointer to the node
 * @return ssize_t the in degree of the node, -1 on error
 */
ssize_t graph_in_degree_size(const weighted_graph_t *graph, const void *dst);

/**
 * @brief Destroy the graph.
 *
 * Destroy the given graph. If a free function was provided at creation, then
 * the data in the graph will be free'd.
 *
 * If an error occurs, then the function will return non-zero.
 * Possible error codes are:
 * - EINVAL: graph is NULL
 *
 * @param graph pointer to a weighted graph
 * @return int 0 on success, non-zero on failure
 */
int graph_destroy(weighted_graph_t *graph);
#endif /* WEIGHTED_GRAPH_H */
