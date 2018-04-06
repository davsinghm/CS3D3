#ifndef NODE_HPP
#define NODE_HPP

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "bmf.hpp"
#include "connection.hpp"
#include "parser.hpp"
#include <pthread.h>
#include <cstdio>
#include <cstdlib>

#define SEARCH_LIM 100
#define HOME_ADDR (char *)"127.0.0.1"
#define SLEEP_SEC 1 //time to sleep between each update
#define TOPOLOGY_FILE (char*)"topology.dat"
#define LINELENGTH 20

// Queue of data to be routed
struct Packet {
    char type; //update dv, data etc.
    char dest_id; //destination node id
    char src_id; //src node id
    std::string data; //data, empty when not a data type packet.
};

typedef struct Packet Packet;

struct RoutingTableNode {
    bool is_neighbor;
    char router_id;
    unsigned int cost;
    unsigned int port;
    char ref_router_id; //id of neighbor router through this distant node has min cost.
    //char next_router;
    //unsigned short next_router_port;
};

typedef struct RoutingTableNode RoutingTableNode;

#define HEADER_FIELD_TYPE_UPDATE_DV '1'
#define HEADER_FIELD_TYPE_MSG '2'

class NodeRouter {
  private:
    // Routing table node uses to determine how to route packets

    std::vector<Packet> packet_queue;

    void handle_packet(Packet&, std::string);
    void build_table(char *filename);
    void print_routing_table();
    void update_dv_in_table(Packet *);
    //int get_weight(int l, int dest);
    void forward_message(Packet &);
    //char find_next_router(int l);
    //unsigned short find_port(char arg);

    pthread_t adv_thread;
    void run_advertisement_thread();

  public:
    Connection connection;
    char node_id;
    int port;

    pthread_mutex_t mutex_routing_table = PTHREAD_MUTEX_INITIALIZER;
    std::vector<RoutingTableNode> routing_table;

    std::string serialize_packet(Packet *);
    void run_router(); //start routing receiving/sending.

    NodeRouter(char);
    ~NodeRouter();
    //bool add_to_queue(std::string arg);
};

#endif