#ifndef NODE_HPP
#define NODE_HPP

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "bmf.hpp"
#include "connection.hpp"
#include "parser.hpp"
#include <cstdio>
#include <cstdlib>
#include <pthread.h>

#define SEARCH_LIM 100
#define HOME_ADDR (char *)"127.0.0.1"
#define SLEEP_SEC 1 // time to sleep between each update
#define TOPOLOGY_FILE (char *)"topology.dat"
#define LINELENGTH 20

extern bool DEBUG;

// Queue of data to be routed
struct Packet {
    char type;        // update dv, data etc.
    char dest_id;     // destination node id
    char src_id;      // src node id
    std::string data; // data, empty when not a data type packet.
};

typedef struct Packet Packet;

struct RouterEntry {
    bool is_neighbor;
    char router_id;
    unsigned int cost; // cost of neighbor
    unsigned int port;
    unsigned int ref_cost; // cost if sending through some other node
    char ref_router_id; // id of neighbor router through this distant node has
                        // min cost
    long last_update; // timestamp of when last DV update came from neighbor.
                      // used for checking if it's alive

    bool is_dead; // used for printing in debug mode only
};

typedef struct RouterEntry RouterEntry;

#define HEADER_FIELD_TYPE_UPDATE_DV '1'
#define HEADER_FIELD_TYPE_MSG '2'

class Router {
  private:
    std::vector<Packet> packet_queue;

    void handle_packet(Packet &, std::string);
    void build_table(char *filename);
    void update_dv_in_table(Packet *);
    void forward_message(Packet &);

    pthread_t adv_thread;
    void run_advertisement_thread();

  public:
    Connection connection;
    char router_id;
    int port;

    pthread_mutex_t mutex_routing_table = PTHREAD_MUTEX_INITIALIZER;
    std::vector<RouterEntry> routing_table;

    unsigned int get_link_cost(RouterEntry &router);
    RouterEntry *get_router_by_id(char id);
    bool is_router_dead(RouterEntry &router);

    void print_routing_table();
    std::string serialize_packet(Packet *);
    void run_router(); // start routing receiving/sending.

    Router(char);
    ~Router();
};

#endif