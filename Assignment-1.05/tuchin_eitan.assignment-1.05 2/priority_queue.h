//
//  priority_queue.h
//  Dungeon_Game_327
//
//  Created by Eitan Tuchin on 1/30/25.
//

#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <stdint.h>
#include <stdbool.h>

// Define the node in the priority queue
typedef struct pq_node {
    int x, y;             // Coordinates in the dungeon
    int priority;         // Priority value, lower is higher priority
} pq_node_t;

// Define the priority queue structure
typedef struct priority_queue {
    pq_node_t *nodes;       // Array of nodes
    int capacity;           // Total capacity of the queue
    int size;               // Number of elements in the queue
} priority_queue_t;

// Function prototypes
priority_queue_t* pq_create(int capacity);
void pq_destroy(priority_queue_t* pq);
bool pq_is_empty(priority_queue_t* pq);
void pq_insert(priority_queue_t* pq, int x, int y, int priority);
pq_node_t pq_extract_min(priority_queue_t* pq);
void pq_decrease_priority(priority_queue_t* pq, int x, int y, int new_priority);
void pq_swap(pq_node_t* a, pq_node_t* b);
void pq_heapify(priority_queue_t* pq, int index);

#endif // PRIORITY_QUEUE_H
