
//
//  priority_queue.c
//  Dungeon_Game_327
//
//  Created by Eitan Tuchin on 1/30/25.
//

#include "priority_queue.h"
#include <stdlib.h>
#include <stdio.h>

// Create a new priority queue with the specified capacity
priority_queue_t* pq_create(int capacity) {
    priority_queue_t* pq = (priority_queue_t*)malloc(sizeof(priority_queue_t));
    pq->capacity = capacity;
    pq->size = 0;
    pq->nodes = (pq_node_t*)malloc(sizeof(pq_node_t) * capacity);
    return pq;
}

// Destroy the priority queue and free the memory
void pq_destroy(priority_queue_t* pq) {
    free(pq->nodes);
    free(pq);
}

// Check if the priority queue is empty
bool pq_is_empty(priority_queue_t* pq) {
    return pq->size == 0;
}

// Insert a new node into the priority queue
void pq_insert(priority_queue_t* pq, int x, int y, int priority) {
    if (pq->size >= pq->capacity) {
        printf("Priority Queue is full!\n");
        return;
    }

    // Insert the new node at the end of the queue
    int index = pq->size++;
    pq->nodes[index].x = x;
    pq->nodes[index].y = y;
    pq->nodes[index].priority = priority;

    // Bubble up to maintain heap property
    while (index > 0 && pq->nodes[index].priority < pq->nodes[(index - 1) / 2].priority) {
        pq_swap(&pq->nodes[index], &pq->nodes[(index - 1) / 2]);
        index = (index - 1) / 2;
    }
}

// Extract the node with the minimum priority (highest priority)
pq_node_t pq_extract_min(priority_queue_t* pq) {
    if (pq_is_empty(pq)) {
        printf("Priority Queue is empty!\n");
        exit(1);
    }

    pq_node_t min_node = pq->nodes[0];

    // Replace the root with the last node and reduce the size
    pq->nodes[0] = pq->nodes[--pq->size];
    
    // Heapify the root to maintain the heap property
    pq_heapify(pq, 0);

    return min_node;
}

// Decrease the priority of a node
void pq_decrease_priority(priority_queue_t* pq, int x, int y, int new_priority) {
    for (int i = 0; i < pq->size; i++) {
        if (pq->nodes[i].x == x && pq->nodes[i].y == y) {
            if (new_priority >= pq->nodes[i].priority) {
                return;
            }
            pq->nodes[i].priority = new_priority;

            // Bubble up to maintain heap property
            while (i > 0 && pq->nodes[i].priority < pq->nodes[(i - 1) / 2].priority) {
                pq_swap(&pq->nodes[i], &pq->nodes[(i - 1) / 2]);
                i = (i - 1) / 2;
            }
            return;
        }
    }
    printf("Node not found!\n");
}

// Swap two nodes in the queue
void pq_swap(pq_node_t* a, pq_node_t* b) {
    pq_node_t temp = *a;
    *a = *b;
    *b = temp;
}

// Heapify the queue to maintain the heap property
void pq_heapify(priority_queue_t* pq, int index) {
    int left = 2 * index + 1;
    int right = 2 * index + 2;
    int smallest = index;

    if (left < pq->size && pq->nodes[left].priority < pq->nodes[smallest].priority) {
        smallest = left;
    }
    if (right < pq->size && pq->nodes[right].priority < pq->nodes[smallest].priority) {
        smallest = right;
    }

    if (smallest != index) {
        pq_swap(&pq->nodes[index], &pq->nodes[smallest]);
        pq_heapify(pq, smallest);
    }
}
