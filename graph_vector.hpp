#ifndef GRAPH_VECTOR_HPP
#define GRAPH_VECTOR_HPP

#include <climits>
#include <iomanip>
#include <iostream>
#include <vector>

// This class holds the data for the entire graph, edges are held in struct
// vertex,
// and nodes are held in struct node. It is important to note that this
// implementation
// means that the graph is directed, so for this project, which has an
// undirected graph
// two vertices nead to be updates when adding a vertex.
//
// struct vertex_queue exists because of how the data is read in. Looking at the
// topology.dat file, the edges and nodes are supplied in one file, and it is
// not possible
// to add edges if the nodes that they reference have not been added to the
// graph yet.
//
// The class is used as follows. Call add_to_graph() to add each node and edge,
// then call
// flush_queue() to populate the edges from struct_vertex.

class GraphVector {
  protected:
    // Edges held here
    struct vertex {
        char router;
        unsigned int weight;

        int destination;
        bool searched;
    };

    // Nodes held here
    struct node {
        char router;
        unsigned short port;

        unsigned int dist_start;
        int prev;

        std::vector<vertex> vertex_list;
    };

    // Temporary queue for edges
    struct vertex_queue {
        char router;
        char edge;
        unsigned int weight;
        unsigned short port;
    };

    // Individual data added here, then pushed onto vector list.
    struct node temp_node;
    struct vertex temp_vertex;
    struct vertex_queue temp_queue;

    void add_node(char router, unsigned short port);
    void add_vertex(char vertex1, char vertex2, unsigned int weight);
    void add_ports();

    std::vector<node> node_list;
    std::vector<vertex_queue> queue;

  public:
    int find_location(char id);
    int find_vertex_ref(int node_dest, int node_src);
    void add_to_graph(char router, char edge, unsigned short port,
                      unsigned int weight);
    void flush_queue();

    void display();
};

#endif