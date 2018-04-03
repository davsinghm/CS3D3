#ifndef PARSER_HPP
#define PARSER_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "graph_vector.hpp"

#define TOPOLOGY_FILE "topology.dat"
#define LINELENGTH 20
#define SECTIONLENGTH 3

class Parser
{
public:
    void parse(Graph_Vector& graph);
};

#endif