#ifndef NODE_HPP
#define NODE_HPP

#include <iostream>
#include <vector>
#include <string>
#include <cstring>

#include "parser.hpp"
#include "bf.hpp"
#include "connection.hpp"

#define SEARCH_LIM 100
#define HOME_ADDR (char*)"127.0.0.1"

class Node_Router : public BF_Search, public Connection
{
private:
    // Routing table node uses to determine how to route packets
    struct routing_table_node
    {
        char router;
        unsigned int cost;
        char next_router;
        unsigned short next_router_port;
    };

    // Queue of data to be routed
    struct packet_queue_node
    {
        char destination;
        char port[6];
        std::string message;
    };

    char this_router;

    std::vector<struct routing_table_node> routing_table;
    std::vector<struct packet_queue_node> packet_queue;
    struct routing_table_node temp_routing_table;
    struct packet_queue_node temp_packet_queue;
    Parser parser;

    // Bellman-Ford algorithm needs to be used first before using this
    void build_table();
    int get_weight(int l, int dest);
    char find_next_router(int l);
    unsigned short find_port(char arg);
    
public:
    Node_Router(char this_router_in);
    ~Node_Router();
    void router();
    bool add_to_queue(std::string arg);
};

#endif