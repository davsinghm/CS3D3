#include "node.hpp"

NodeRouter::NodeRouter(char node) {
    // Nodes are parsed, edges are put into a queue
    Parser parser;
    parser.parse(*this);
    // When nodes are intialised, vertices are added
    flush_queue();
    // Set the home router based on the input to the constructor ^
    node_id = node;
    // Perform Bellman-Ford algorithm on graph
    bmf_search(node_id);
    // Build routing table based on above function results on graph
    build_table();
    // Bind to router's port
    setup_connection(
        HOME_ADDR,
        std::to_string((int)node_list[find_location(node_id)].port));

    for (int i = 0; i < routing_table.size(); i++) {
        std::cout << routing_table[i].router << " "
                  << routing_table[i].cost << " "
                  << routing_table[i].next_router << " "
                  << routing_table[i].next_router_port << std::endl;
    }

    run_advertisement_thread();

    // Start routing
    run_router();
}

NodeRouter::~NodeRouter() {
    pthread_join(adv_thread, NULL);
}

void NodeRouter::run_router() {
    std::string message;
    for (;;) {
        recv_udp(message);

        Packet packet;
        handle_packet(packet, message);
    }
}

#define HEADER_FIELD_TYPE_UPDATE_DV '1'
#define HEADER_FIELD_TYPE_MSG '2'

/**
 * Message Struct:
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * first byte: type (update dv, msg)
 * final destination node
 * src node node
 * rest of it is data, interpreted based on first byte.
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
void NodeRouter::handle_packet(Packet &packet, std::string message) {
    std::cout << "Received: " << message << std::endl;

    if (message.length() < 2) {
        std::cout << "Invalid Packet Received. Message Length(): " << message.length() << std::endl;
        return;
    }
    //deserialize packet.
    packet.type = message[0];
    packet.dest_id = message[1];
    packet.data = message.substr(2);
    std::cout << "type: " << packet.type << std::endl;
    std::cout << "dest_id: " << packet.dest_id << std::endl;
    std::cout << "data: " << packet.data << std::endl;

    if (packet.dest_id != node_id) {
        //message = packet_queue.back().dest_id;
        //message = message + " " + packet_queue.back().message;
        std::cout << "FORWARD MESSSAGE TODO" << std::endl;
        //send_udp(message, HOME_ADDR, &packet_queue.back().port[0]);
    } else {
        std::cout << "Final Destination: " << message << std::endl;

        switch (packet.type) {
            case HEADER_FIELD_TYPE_UPDATE_DV:
                std::cout << "HEADER_FIELD_UPDATE_DV" << std::endl;
                break;
            case HEADER_FIELD_TYPE_MSG:
                std::cout << "HEADER_FIELD_TYPE_MSG" << std::endl;
            default:
                std::cout << "HEADER_FIELD_TYPE_INVALID" << std::endl;
        }
    }
    //packet_queue.pop_back();
}

void *adv_thread_func(void *args) {
    std::cout << "started adv thread" << std::endl;

    NodeRouter *node = (NodeRouter *)args;
    std::cout << "started adv thread: " << node->node_id << std::endl;

    pthread_exit(NULL);
}

void NodeRouter::run_advertisement_thread() {
    pthread_create(&adv_thread, NULL, adv_thread_func, (void *)this);
}

std::string NodeRouter::serialize_packet(Packet *packet) {
    std::string str = "";
    str += packet->type;
    str += packet->dest_id;
    str += packet->data;

    return str;
}

bool NodeRouter::add_to_queue(std::string arg) {
    size_t len = (arg.length() + 1) * sizeof(char);
    char *packet = (char *)malloc(len);
    char *message = (char *)malloc(len);
    char destination;
    std::string str_port;
    char port[6];

    strcpy(packet, arg.c_str());

    destination = *(strtok(packet, (char *)" "));

    strcpy(message, strtok(NULL, (char *)"\0"));
    str_port = std::to_string((int)find_port(destination));
    strcpy(port, str_port.c_str());

    Packet temp_packet_queue;
    temp_packet_queue.dest_id = destination;
    temp_packet_queue.data = message;
//    strcpy(temp_packet_queue.port, port);
    packet_queue.push_back(temp_packet_queue);

    free((void *)packet);
    free((void *)message);
    return destination != node_id;
}

unsigned short NodeRouter::find_port(char arg) {
    for (int i = 0; i < routing_table.size(); i++) {
        if (routing_table[i].router == arg) {
            return routing_table[i].next_router_port;
        }
    }

    return 0;
}

void NodeRouter::build_table() {
    // Add this node as first entry to table
    temp_routing_table.cost = 0;
    temp_routing_table.router = node_id;
    temp_routing_table.next_router = node_id;
    routing_table.push_back(temp_routing_table);

    // Search through rest of the table
    for (int i = 0; i < node_list.size(); i++) {
        // Skip the the home router since this was already added
        if (node_list[i].router == node_id) {
            continue;
        }
        // If the router is a neighbour
        else if (node_list[node_list[i].prev].router == node_id) {
            temp_routing_table.cost = node_list[i].dist_start;
            temp_routing_table.router = node_list[i].router;
            temp_routing_table.next_router = node_list[i].router;
            temp_routing_table.next_router_port = node_list[i].port;
            routing_table.push_back(temp_routing_table);
        }
        // If the router is distant
        else {
            temp_routing_table.cost = node_list[i].dist_start;
            temp_routing_table.router = node_list[i].router;
            temp_routing_table.next_router = find_next_router(i);
            temp_routing_table.next_router_port =
                node_list[find_location(temp_routing_table.next_router)].port;
            routing_table.push_back(temp_routing_table);
        }
    }
}

char NodeRouter::find_next_router(int l) {
    int count = 0;
    while (count < SEARCH_LIM) {
        if (node_list[node_list[l].prev].router == node_id) {
            return node_list[l].router;
        } else {
            l = node_list[l].prev;
        }
        count++;
    }

    std::cout << "Error: find_next_router()\n";
    exit(1);

    return '\0';
}