#include "graph_vector.hpp"

void GraphVector::add_node(char router, unsigned short port) {
    temp_node.router = router;
    temp_node.port = port;
    temp_node.dist_start = UINT_MAX / 2;
    temp_node.prev = -1;

    node_list.push_back(temp_node);
}

void GraphVector::add_vertex(char vertex1, char vertex2, unsigned int weight) {
    int vertex1_dest = find_location(vertex1);
    int vertex2_dest = find_location(vertex2);
    temp_vertex.weight = weight;
    temp_vertex.searched = false;

    temp_vertex.destination = vertex2_dest;
    node_list[vertex1_dest].vertex_list.push_back(temp_vertex);

    temp_vertex.destination = vertex1_dest;
    node_list[vertex2_dest].vertex_list.push_back(temp_vertex);
}

// Finds the position in the vector array marked by the router id
int GraphVector::find_location(char id) {
    for (int i = 0; i < node_list.size(); i++) {
        if (node_list[i].router == id) {
            return i;
        }
    }

    return -1;
}

// Finds the vector array reference for the vertex from node_src to node_dest
int GraphVector::find_vertex_ref(int node_dest, int node_src) {
    for (int i = 0; i < node_list[node_src].vertex_list.size(); i++) {
        if (node_list[node_src].vertex_list[i].destination == node_dest) {
            return i;
        }
    }

    return -1;
}

void GraphVector::add_to_graph(char router, char edge, unsigned short port,
                                unsigned int weight) {
    int location = find_location(router);
    if (location == -1) {
        add_node(router, 0);
        location = find_location(router);
        if (location == -1) {
            std::cout << "Error: Failed to insert node\n";
            exit(1);
        }
    }
    temp_queue.router = router;
    temp_queue.edge = edge;
    temp_queue.weight = weight;
    temp_queue.port = port;
    queue.push_back(temp_queue);
}

void GraphVector::flush_queue() {
    for (int i = 0; i < queue.size(); i++) {
        add_vertex(queue[i].router, queue[i].edge, queue[i].weight);
    }
    add_ports();
}

void GraphVector::add_ports() {
    int l;
    for (int i = 0; i < queue.size(); i++) {
        l = find_location(queue[i].edge);
        if (node_list[l].port == 0) {
            node_list[l].port = queue[i].port;
        }
    }
}