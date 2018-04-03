#ifndef BF_HPP
#define BF_HPP

#include "graph_vector.hpp"

#define SEARCH_P 300
#define SEARCH_LIM 100

// This inherits from Graph_Vector, it performs a Bellman-Ford search on
// the graph and updates dist_start and prev appropriately

class BF_Search : public Graph_Vector {
  private:
    void search(char id);

  protected:
    void bf_search(char source);
};

#endif