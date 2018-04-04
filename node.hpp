#ifndef NODE_HPP
#define NODE_HPP

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "bmf.hpp"
#include "connection.hpp"
#include "parser.hpp"
#include<pthread.h>

#define SEARCH_LIM 100
#define HOME_ADDR (char *)"127.0.0.1"

    // Queue of data to be routed
    struct Packet {
        char type; //update dv, data etc.
        char dest_id; //destination node id
        std::string data; //data, empty when not a data type packet.
    };

    typedef struct Packet Packet;


#define HEADER_FIELD_TYPE_UPDATE_DV '1'
#define HEADER_FIELD_TYPE_MSG '2'

class NodeRouter : public BellmanFordSearch, public Connection {
  private:
    // Routing table node uses to determine how to route packets
    struct routing_table_node {
        char router;
        unsigned int cost;
        char next_router;
        unsigned short next_router_port;
    };

    typedef struct routing_table_node RoutingTableNode;


    std::vector<RoutingTableNode> routing_table;
    std::vector<Packet> packet_queue;
    RoutingTableNode temp_routing_table;

    void handle_packet(Packet &, std::string request);
    // Bellman-Ford algorithm needs to be used first before using this
    void build_table();
    int get_weight(int l, int dest);
    char find_next_router(int l);
    unsigned short find_port(char arg);

    pthread_t adv_thread;
    void run_advertisement_thread();

  public:
    char node_id;
        std::string serialize_packet(Packet *);


    NodeRouter(char);
    ~NodeRouter();
    void run_router();
    bool add_to_queue(std::string arg);
};

#endif