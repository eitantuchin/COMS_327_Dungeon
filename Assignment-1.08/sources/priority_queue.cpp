#include "../headers/priority_queue.h"

bool pq_node_t::operator>(const pq_node_t& other) const {  // Definition of the comparison operator
     return priority > other.priority;
}

my_priority_queue::my_priority_queue() {}

std::vector<pq_node_t> nodes;

void my_priority_queue::swap(pq_node_t& a, pq_node_t& b) {
    pq_node temp = a;
    a = b;
    b = temp;
}

void my_priority_queue::heapify(size_t index) {
    size_t left = 2 * index + 1;
    size_t right = 2 * index + 2;
    size_t smallest = index;

    if (left < nodes.size() && nodes[left].priority < nodes[smallest].priority) {
        smallest = left;
    }
    if (right < nodes.size() && nodes[right].priority < nodes[smallest].priority) {
        smallest = right;
    }

    if (smallest != index) {
        swap(nodes[index], nodes[smallest]);
        heapify(smallest);
    }
}

bool my_priority_queue::is_empty()  {
    return nodes.empty();
}

void my_priority_queue::insert(int x, int y, int priority) {
    nodes.push_back({x, y, priority});
    size_t index = nodes.size() - 1;

    while (index > 0 && nodes[index].priority < nodes[(index - 1) / 2].priority) {
        swap(nodes[index], nodes[(index - 1) / 2]);
        index = (index - 1) / 2;
    }
}

void my_priority_queue::clear() {
    nodes.clear(); // This removes all elements from the vector.
                  // The vector's destructor will handle the memory.
}

pq_node_t my_priority_queue::extract_min() {
    if (is_empty()) {
        // Return a "dummy" node with max priority to indicate empty.  Handle this in calling code.
        return {0, 0, std::numeric_limits<int>::max()}; // Or throw an exception
    }

    pq_node_t min_node = nodes[0];
    nodes[0] = nodes.back();
    nodes.pop_back();
    heapify(0);
    return min_node;
}

void my_priority_queue::decrease_priority(int x, int y, int new_priority) {
    for (size_t i = 0; i < nodes.size(); i++) {
        if (nodes[i].x == x && nodes[i].y == y) {
            if (new_priority >= nodes[i].priority) {
                return;
            }
            nodes[i].priority = new_priority;

            size_t index = i;
            while (index > 0 && nodes[index].priority < nodes[(index - 1) / 2].priority) {
                swap(nodes[index], nodes[(index - 1) / 2]);
                index = (index - 1) / 2;
            }
            return;
        }
    }
}

