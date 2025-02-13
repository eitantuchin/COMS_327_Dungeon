// priority_queue.h
#ifndef PRIORITY_QUEUE_HPP
#define PRIORITY_QUEUE_HPP

#include <vector>
#include <limits>
#include <iostream> 

typedef struct pq_node {
    int x, y;
    int priority;
    pq_node(int _x, int _y, int _priority) : x(_x), y(_y), priority(_priority) {} // Constructor
    bool operator>(const pq_node& other) const;
} pq_node_t;

class priority_queue {
    private:
        vector<pq_node_t> nodes;
        void swap(pq_node_t& a, pq_node_t& b);
        void heapify(int index);

    public:
        priority_queue();
        bool is_empty();
        void insert(int x, int y, int priority);
        pq_node_t extract_min();
        void decrease_priority(int x, int y, int new_priority);
        void clear();
};

#endif // PRIORITY_QUEUE_H
