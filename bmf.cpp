#include "bmf.hpp"

/*void BellmanFordSearch::bmf_search(char source) {
    int l = find_location(source);
    node_list[l].dist_start = 0;
    node_list[l].prev = -2;
    search(source);
}

void BellmanFordSearch::search(char router) {
    // The algorithm isn't fully implemented
    int l = find_location(router); // location: in std::vector list

    for (int i = 0; i < node_list[l].vertex_list.size(); i++) {
        if (node_list[l].vertex_list[i].searched == false) {
            if (node_list[node_list[l].vertex_list[i].destination].dist_start >
                (node_list[l].dist_start +
                 node_list[l].vertex_list[i].weight)) {
                // Set new dist_start for next node
                node_list[node_list[l].vertex_list[i].destination].dist_start =
                    (node_list[l].dist_start +
                     node_list[l].vertex_list[i].weight);

                // Update two vertices (this is a directed graph)
                // Searching in node l, vertex i, the vertex in the
                // other direction must be updated too
                node_list[l].vertex_list[i].searched = true;
                node_list[node_list[l].vertex_list[i].destination]
                    .vertex_list[find_vertex_ref(
                        l, node_list[l].vertex_list[i].destination)]
                    .searched = true;

                // Set previous node
                node_list[node_list[l].vertex_list[i].destination].prev = l;
                search(
                    node_list[node_list[l].vertex_list[i].destination].router);
            }
        }
    }
}*/