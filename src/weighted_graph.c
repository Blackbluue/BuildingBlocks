#include "weighted_graph.h"
#include "hash_table.h"
#include "queue_p.h"
#include <errno.h>
#include <math.h>
#include <stdlib.h>

/* DATA */

#define SUCCESS 0
#define FAILURE -1

union double_pointer {
    double dbl;
    void *ptr;
};

struct edge {
    struct node *from;
    struct node *to;
    double weight;
};

struct node {
    void *data;
    list_t *edges;
    CMP_F cmp;
    FREE_F destroy;
};

struct weighted_graph_t {
    list_t *nodes;
    CMP_F cmp;
    FREE_F destroy;
    queue_p_t *to_process;
    hash_table_t *previous;
    hash_table_t *distance_from_origin;
    struct queue_p_node_t *curr_item;
};

/* PRIVATE FUNCTIONS */

/**
 * @brief Sets the error code.
 *
 * @param err The error code.
 * @param value The value to set.
 */
static void set_err(int *err, int value) {
    if (err != NULL) {
        *err = value;
    }
}

/**
 * @brief Create a new node
 *
 * @param graph pointer to the graph
 * @param data data to be stored in the node
 * @return pointer to the new node on success, NULL on failure
 */
static struct node *node_new(const weighted_graph_t *graph, void *data) {
    if (graph == NULL) {
        return NULL;
    }
    struct node *new = malloc(sizeof(*new));
    if (new == NULL) {
        return NULL;
    }
    new->data = data;
    new->edges = NULL;
    new->cmp = graph->cmp;
    new->destroy = graph->destroy;
    return new;
}

/**
 * @brief Compare two nodes
 *
 * @param value_to_find pointer to the node to be found
 * @param node_data pointer to the node to be compared
 * @return int 0 if the nodes are equal, non-zero if the nodes are not equal
 */
static int node_cmp(const void *value_to_find, const void *node_data) {
    if (value_to_find == NULL || node_data == NULL) {
        return 0;
    }
    const struct node *node_1 = value_to_find;
    const struct node *node_2 = node_data;
    return node_1->cmp(node_1->data, node_2->data);
}

/**
 * @brief Free a node
 *
 * @param data pointer to the node to be freed
 */
static void node_free(void *data) {
    if (data == NULL) {
        return;
    }
    struct node *node = data;
    list_delete(&node->edges);
    if (node->destroy != NULL) {
        node->destroy(node->data);
    }
    free(node);
}

/**
 * @brief Create a new edge
 *
 * @param to pointer to the node that the edge points to
 * @param weight weight of the edge
 * @return pointer to the new edge on success, NULL on failure
 */
static struct edge *edge_new(struct node *from, struct node *to,
                             double weight) {
    struct edge *new = malloc(sizeof(*new));
    if (new == NULL) {
        return NULL;
    }
    new->from = from;
    new->to = to;
    new->weight = weight;
    return new;
}

/**
 * @brief Find an edge
 *
 * Specifically used to find which node an edge is connected to. Cannot be used
 * for ordering/sorting.
 *
 * @param value_to_find pointer to the edge to be found
 * @param to_node pointer to the node to be compared
 * @return int 0 if edge points to node, non-zero otherwise
 */
static int edge_cmp(const void *value_to_find, const void *to_node) {
    if (value_to_find == NULL) {
        return 0;
    }
    const struct edge *edge = value_to_find;
    const struct node *node = to_node;
    // only care about equality, not ordering
    return edge->to == node;
}

static int add_to_pqueue_if_faster(void **neighbor, void *addl_data) {
    if (neighbor == NULL || addl_data == NULL) {
        return EINVAL;
    }
    weighted_graph_t *graph = addl_data;
    double weight =
        graph_get_edge_weight(graph, graph->curr_item->data, *neighbor);
    double distance = weight + graph->curr_item->priority;

    union double_pointer current_best = {
        .ptr = hash_table_lookup(graph->distance_from_origin, *neighbor)};

    // if the node has not been visited yet, or if the distance is shorter
    // than the current best, then update the distance and add to the queue
    if (hash_table_lookup(graph->previous, *neighbor) == NULL ||
        distance < current_best.dbl) {
        int err =
            hash_table_set(graph->previous, graph->curr_item->data, *neighbor);
        if (err) {
            return err;
        }
        struct node *new = node_new(graph, *neighbor);
        if (new == NULL) {
            return ENOMEM;
        }
        err = queue_p_enqueue(graph->to_process, new, distance);
        if (err) {
            node_free(new);
            return err;
        }

        current_best.dbl = distance;
        err = hash_table_set(graph->distance_from_origin, current_best.ptr,
                             *neighbor);
        if (err) {
            return err;
        }
    }
    return SUCCESS;
}

/* PUBLIC FUNCTIONS */

weighted_graph_t *graph_create(CMP_F cmp, FREE_F free_f, int *err) {
    if (cmp == NULL) {
        set_err(err, EINVAL);
        return NULL;
    }
    weighted_graph_t *graph = malloc(sizeof(*graph));
    if (graph == NULL) {
        set_err(err, ENOMEM);
        return NULL;
    }
    graph->nodes = list_new(node_free, node_cmp, err);
    if (graph->nodes == NULL) {
        free(graph);
        return NULL;
    }
    graph->to_process = queue_p_init(0, NULL, node_cmp, err);
    if (graph->to_process == NULL) {
        list_delete(&graph->nodes);
        free(graph);
        return NULL;
    }
    graph->previous = hash_table_init(0, NULL, cmp, err);
    if (graph->previous == NULL) {
        list_delete(&graph->nodes);
        queue_p_destroy(&graph->to_process);
        free(graph);
        return NULL;
    }
    graph->distance_from_origin = hash_table_init(0, NULL, cmp, err);
    if (graph->distance_from_origin == NULL) {
        list_delete(&graph->nodes);
        queue_p_destroy(&graph->to_process);
        hash_table_destroy(&graph->previous);
        free(graph);
        return NULL;
    }
    graph->cmp = cmp;
    graph->destroy = free_f;
    return graph;
}

ssize_t graph_size(const weighted_graph_t *graph) {
    return graph == NULL ? FAILURE : list_size(graph->nodes);
}

int graph_add_node(weighted_graph_t *graph, void *data) {
    if (graph == NULL || data == NULL) {
        return EINVAL;
    }

    // If the data already exist in a node, then bail out
    if (graph_contains(graph, data)) {
        return SUCCESS;
    }

    struct node *new = node_new(graph, data);
    if (new == NULL) {
        return ENOMEM;
    }
    if (list_push_tail(graph->nodes, new) == ENOMEM) {
        free(new);
        return ENOMEM;
    } else {
        return SUCCESS;
    }
}

void *graph_remove_node(weighted_graph_t *graph, void *data) {
    if (graph == NULL || data == NULL) {
        errno = EINVAL;
        return NULL;
    }

    struct node *removed = list_remove(graph->nodes, data, NULL);
    if (removed == NULL) {
        errno = ENOENT; // item not found in graph
        return NULL;
    }

    list_iterator_reset(graph->nodes);
    struct node *curr_node = list_iterator_next(graph->nodes, NULL);
    while (curr_node) {
        // will skip nodes with no edges
        while (list_remove(curr_node->edges, removed, NULL)) {
            // removes all edges that point to the removed node
        }
        if (list_size(curr_node->edges) == 0) {
            list_delete(&curr_node->edges);
        }
        curr_node = list_iterator_next(graph->nodes, NULL);
    }

    list_delete(&removed->edges);
    return removed->data;
}

int graph_iterate_nodes(weighted_graph_t *graph, ACT_F func, void *obj) {
    if (graph == NULL || func == NULL) {
        return EINVAL;
    }

    list_iterator_reset(graph->nodes);
    struct node *curr = list_iterator_next(graph->nodes, NULL);
    while (curr) {
        int err = func(&curr->data, obj);
        if (err) {
            return err;
        }
        curr = list_iterator_next(graph->nodes, NULL);
    }
    return SUCCESS;
}

int graph_iterate_neighbors(weighted_graph_t *graph, void *center, ACT_F func,
                            void *obj) {
    if (graph == NULL || obj == NULL || func == NULL) {
        return EINVAL;
    }

    struct node *curr_node = list_find_first(graph->nodes, center, NULL);
    if (curr_node == NULL) {
        return ENOENT;
    }

    if (curr_node->edges == NULL) {
        return SUCCESS;
    }
    list_iterator_reset(curr_node->edges);
    struct edge *curr_edge = list_iterator_next(curr_node->edges, NULL);
    while (curr_edge) {
        int err = func(&curr_edge->to->data, obj);
        if (err) {
            return err;
        }
        curr_edge = list_iterator_next(curr_node->edges, NULL);
    }
    return SUCCESS;
}

list_t *graph_find_path(weighted_graph_t *graph, const void *start,
                        const void *end) {
    if (graph == NULL || start == NULL || end == NULL) {
        errno = EINVAL;
        return NULL;
    }

    // Nothing in Dijkstra's changes these items or neighbors; casting is safe
    struct node *new = node_new(graph, (void *)start);
    if (new == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    int err = queue_p_enqueue(graph->to_process, new, 0);
    if (err) {
        node_free(new);
        errno = err;
        return NULL;
    }
    err = hash_table_set(graph->previous, NULL, start);
    if (err) {
        queue_p_clear(graph->to_process);
        node_free(new);
        errno = err;
        return NULL;
    }

    while (!queue_p_is_empty(graph->to_process)) {
        graph->curr_item = queue_p_dequeue(graph->to_process);
        if (graph->cmp(graph->curr_item->data, end) == 0) {
            break;
        }

        err = graph_iterate_neighbors(
            (weighted_graph_t *)graph, graph->curr_item->data,
            add_to_pqueue_if_faster, (weighted_graph_t *)graph);
        node_free(graph->curr_item->data);
        free(graph->curr_item);
        if (err) {
            queue_p_clear(graph->to_process);
            hash_table_clear(graph->previous);
            hash_table_clear(graph->distance_from_origin);
            errno = err;
            return NULL;
        }
    }
    queue_p_clear(graph->to_process);
    hash_table_clear(graph->distance_from_origin);

    list_t *results = list_new(NULL, graph->cmp, NULL);
    if (results == NULL) {
        hash_table_clear(graph->previous);
        errno = ENOMEM;
        return NULL;
    }
    void *curr = (void *)end;
    while (curr) {
        err = list_push_head(results, curr);
        if (err) {
            hash_table_clear(graph->previous);
            list_delete(&results);
            errno = err;
            return NULL;
        }

        curr = hash_table_lookup(graph->previous, curr);
    }

    hash_table_clear(graph->previous);
    return results;
}

int graph_contains(const weighted_graph_t *graph, const void *data) {
    if (graph == NULL || data == NULL) {
        return FAILURE;
    }
    return list_find_first(graph->nodes, data, NULL) == NULL;
}

int graph_add_edge(weighted_graph_t *graph, void *src, void *dst,
                   double weight) {
    if (graph == NULL || src == NULL || dst == NULL) {
        return EINVAL;
    }

    struct node *from = list_find_first(graph->nodes, src, NULL);
    if (from == NULL) {
        return ENOENT;
    }
    struct node *to = list_find_first(graph->nodes, dst, NULL);
    if (to == NULL) {
        return ENOENT;
    }

    // check if the edge already exists, if so, update the weight
    if (from->edges == NULL) {
        int err;
        from->edges = list_new(free, edge_cmp, &err);
        if (from->edges == NULL) {
            return err;
        }
    } else {
        struct edge *checker = list_find_first(from->edges, to, NULL);
        if (checker) {
            checker->weight = weight;
            return SUCCESS;
        }
    }

    // edge not found, create new one
    struct edge *new = edge_new(from, to, weight);
    if (new == NULL) {
        return ENOMEM;
    }
    return list_push_tail(from->edges, new);
}

double graph_get_edge_weight(const weighted_graph_t *graph, const void *src,
                             const void *dst) {
    if (graph == NULL || src == NULL || dst == NULL) {
        errno = EINVAL;
        return NAN;
    }

    struct node *from = list_find_first(graph->nodes, src, NULL);
    if (from == NULL) {
        errno = ENOENT;
        return NAN;
    }
    struct node *to = list_find_first(graph->nodes, dst, NULL);
    if (to == NULL) {
        errno = ENOENT;
        return NAN;
    }

    struct edge *checker = list_find_first(from->edges, to, NULL);
    if (checker == NULL) {
        errno = ENOENT;
        return NAN;
    }
    return checker->weight;
}

int graph_remove_edge(weighted_graph_t *graph, const void *src,
                      const void *dst) {
    if (graph == NULL || src == NULL || dst == NULL) {
        return EINVAL;
    }

    struct node *from = list_find_first(graph->nodes, src, NULL);
    if (from == NULL) {
        return ENOENT;
    }
    struct node *to = list_find_first(graph->nodes, dst, NULL);
    if (to == NULL) {
        return ENOENT;
    }

    struct edge *checker = list_remove(from->edges, to, NULL);
    if (checker) {
        free(checker);
        return SUCCESS;
    }
    return ENOENT;
}

ssize_t graph_out_degree_size(const weighted_graph_t *graph, const void *src) {
    if (graph == NULL || src == NULL) {
        errno = EINVAL;
        return FAILURE;
    }

    struct node *from = list_find_first(graph->nodes, src, NULL);
    if (from == NULL) {
        errno = ENOENT;
        return FAILURE;
    }
    // node might not have any edges yet
    return from->edges == NULL ? 0 : list_size(from->edges);
}

ssize_t graph_in_degree_size(const weighted_graph_t *graph, const void *dst) {
    if (graph == NULL || dst == NULL) {
        errno = EINVAL;
        return FAILURE;
    }

    struct node *to = list_find_first(graph->nodes, dst, NULL);
    if (dst == NULL) {
        errno = ENOENT;
        return FAILURE;
    }

    size_t count = 0;
    list_iterator_reset(graph->nodes);
    struct node *curr_node = list_iterator_next(graph->nodes, NULL);
    while (curr_node) {
        // skip any nodes with no outgoing edges
        if (curr_node->edges == NULL) {
            curr_node = list_iterator_next(graph->nodes, NULL);
            continue;
        }
        list_iterator_reset(curr_node->edges);
        struct edge *curr_edge = list_iterator_next(curr_node->edges, NULL);
        while (curr_edge) {
            if (curr_edge->to == to) {
                ++count;
            }

            curr_edge = list_iterator_next(curr_node->edges, NULL);
        }

        curr_node = list_iterator_next(graph->nodes, NULL);
    }

    return count;
}

int graph_destroy(weighted_graph_t *graph) {
    if (graph == NULL) {
        return EINVAL;
    }

    list_delete(&graph->nodes);
    queue_p_destroy(&graph->to_process);
    hash_table_destroy(&graph->previous);
    hash_table_destroy(&graph->distance_from_origin);
    free(graph);
    return SUCCESS;
}
