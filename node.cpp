#include "node.hpp"

#include "graph_vector.hpp"

#define SLEEP_SEC 1
#define TOPOLOGY_FILE (char*)"topology.dat"
#define LINELENGTH 20
#define SECTIONLENGTH 3

NodeRouter::NodeRouter(char node) {
    // When nodes are intialised, vertices are added
    //flush_queue();
    // Set the home router based on the input to the constructor ^
    node_id = node;
    // Perform Bellman-Ford algorithm on graph
    //bmf_search(node_id);
    // Build routing table based on above function results on graph
    build_table();
    // Bind to router's port
    setup_connection(
        HOME_ADDR,
        std::to_string(this->port));
/*
    for (int i = 0; i < routing_table.size(); i++) {
        std::cout << routing_table[i].router_id << " "
                  << routing_table[i].cost << " "
                  << routing_table[i].port << " "
                  //<< routing_table[i].next_router_port 
                  << std::endl;
    }*/

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

void print_routing_table(NodeRouter *node) {
    for (int i = 0; i < node->routing_table.size(); i++) {
        std::cout << node->routing_table[i].router_id << " "
                  << node->routing_table[i].cost << " ";

        if (node->routing_table[i].is_neighbor)
            std::cout << std::to_string(node->routing_table[i].port);
        else {
            std::cout << "through "; 
            std::cout << node->routing_table[i].ref_router_id;
        }
        std::cout << std::endl;
    }
}

void update_dv_in_table(NodeRouter *node, Packet *packet) {
    int new_cost;
    char to_router_id;
    bool has_updated_table = false; //if added or updated the entry, used for printing

    to_router_id = packet->data[0];
    std::string cost_str = packet->data.substr(1);
    new_cost = atoi((char *)cost_str.c_str());

    //trying to update routing table.
    pthread_mutex_lock(&node->mutex_routing_table); //lock it

    //TODO can optimize if we sum the cost of this link in the node that's sending the DV. then we don't need to find the src (which is neighbor) in table
    RoutingTableNode *src_rtn = NULL; //src, neighbor from where dv came
    for (int i = 0; i < node->routing_table.size(); i++) {
        RoutingTableNode &rtn = node->routing_table.at(i);
        if (rtn.router_id == packet->src_id) {
            src_rtn = &rtn;
        }
    }

    //std::cout << "new cost to " << to_router_id << " is " << cost_str << " from " << src_rtn->router_id << std::endl;

    bool has_node = false; //table has node info, it's updated instead of adding new one
    for (int i = 0; i < node->routing_table.size(); i++) {
        RoutingTableNode &rtn = node->routing_table.at(i);
        if (rtn.router_id == to_router_id) {
            has_node = true;

            if (src_rtn->cost + new_cost < rtn.cost) {
                std::cout << "updating new cost to " << src_rtn->cost + new_cost << " of " << to_router_id << " in table of " << rtn.router_id << ". Received from " << src_rtn->router_id << std::endl;
                rtn.cost = src_rtn->cost + new_cost;
                rtn.ref_router_id = src_rtn->router_id;
                has_updated_table = true;
                break;
            }
        }
    }

    //add if it's a distant router.
    if (!has_node && to_router_id != node->node_id) {
        RoutingTableNode rtn;
        rtn.is_neighbor = false;
        rtn.router_id = to_router_id;
        rtn.cost = new_cost + src_rtn->cost;
        rtn.ref_router_id = packet->src_id;
        rtn.port = 0;
        node->routing_table.push_back(rtn);

        has_updated_table = true;
    }

    pthread_mutex_unlock(&node->mutex_routing_table); //unlock it

    if (has_updated_table)
        print_routing_table(node);
}

void forward_message(NodeRouter *node, Packet &packet) {
    int index = -1;
    int ref_router_id = -1;
    for (int i = 0; i < node->routing_table.size(); i++) {
        RoutingTableNode &rtn = node->routing_table.at(i);
        if (rtn.is_neighbor) {
            if (rtn.router_id == packet.dest_id) {
                index = i;
                goto goto_forward;
            }
        } else {
            if (rtn.router_id == packet.dest_id) {
                ref_router_id = rtn.ref_router_id;
                break;
            }
        }
    }

    if (ref_router_id == -1) {
        std::cout << "can't forward the message, couldn't find the thing in table" << std::endl;
        return;
    }

    for (int i = 0; i < node->routing_table.size(); i++) {
        RoutingTableNode &rtn = node->routing_table.at(i);
        if (rtn.is_neighbor)
            continue;

        if (rtn.router_id == ref_router_id) {
            index = i;
            goto goto_forward;
        }
    }

    return;

goto_forward:
    std::cout << "forwarding the packet to " << node->routing_table.at(index).port << std::endl;

    std::string message = node->serialize_packet(&packet);
    node->send_udp(message, HOME_ADDR, std::to_string(node->routing_table.at(index).port));
}

/**
 * Message Struct:
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * first byte: type (update dv, msg)
 * second byte: dest_id
 * third byte: src_id
 * rest: data
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
void NodeRouter::handle_packet(Packet &packet, std::string message) {
    //std::cout << "Received: " << message << std::endl;

    if (message.length() < 2) {
        std::cout << "Invalid Packet Received. Message Length(): " << message.length() << std::endl;
        return;
    }
    //deserialize packet first
    packet.type = message[0];
    packet.dest_id = message[1];
    packet.src_id = message[2];
    packet.data = message.substr(3);
    //std::cout << "type: " << packet.type << std::endl;
    //std::cout << "dest_id: " << packet.dest_id << std::endl;
    //std::cout << "src_id: " << packet.src_id << std::endl;
    //std::cout << "data: " << packet.data << std::endl;

    if (packet.dest_id != node_id) {
        //message = packet_queue.back().dest_id;
        //message = message + " " + packet_queue.back().message;
        std::cout << "FORWARDING MESSSAGE" << std::endl;
        forward_message(this, packet);
    } else {

        switch (packet.type) {
            case HEADER_FIELD_TYPE_UPDATE_DV:
                update_dv_in_table(this, &packet);
                break;
            case HEADER_FIELD_TYPE_MSG:
                std::cout << "Final Destination: " << message << std::endl;

                std::cout << "HEADER_FIELD_TYPE_MSG" << std::endl;
                break;
            default:
                std::cout << "HEADER_FIELD_TYPE_INVALID" << std::endl;
        }
    }
}

void *adv_thread_func(void *args) {
    std::cout << "started adv thread" << std::endl;

    NodeRouter *node = (NodeRouter *)args;
    std::cout << "started adv thread: " << node->node_id << std::endl;

    //this should send whole routing table to its neighbors
    for (;;) {
        pthread_mutex_lock(&node->mutex_routing_table);

        for (int i = 0; i < node->routing_table.size(); i++) {
            RoutingTableNode rtn_neighbor = node->routing_table[i];
            if (!rtn_neighbor.is_neighbor)
                continue; //don't send to distant router, their neighbors will send their info

            for (int j = 0; j < node->routing_table.size(); j++) {
                RoutingTableNode rtn = node->routing_table[j];
                //if (rtn.is_neighbor)
                //    continue; 

                Packet packet;
                packet.type = HEADER_FIELD_TYPE_UPDATE_DV;

                //packet.data = std::to_string(rtn.router_id);// ...to reach the router
                packet.data = ((char)rtn.router_id);
                packet.data.append(std::to_string(rtn.cost)); //cost of link...
                //std::cout << "sending to " << packet.dest_id << " DATA: " << packet.data << " from " << packet.src_id << std::endl;

                packet.src_id = node->node_id; //this router
                packet.dest_id = rtn_neighbor.router_id; //neighbor is dest

                std::string message = node->serialize_packet(&packet);
                node->send_udp(message, HOME_ADDR, std::to_string(rtn_neighbor.port)); //send to neighbor (tc port)
            }
        }

        pthread_mutex_unlock(&node->mutex_routing_table);

        sleep(SLEEP_SEC);
    }

    pthread_exit(NULL);
}

void NodeRouter::run_advertisement_thread() {
    pthread_create(&adv_thread, NULL, adv_thread_func, (void *)this);
}

std::string NodeRouter::serialize_packet(Packet *packet) {
    std::string str = "";
    str += packet->type;
    str += packet->dest_id;
    str += packet->src_id;
    str += packet->data;

    return str;
}

/*bool NodeRouter::add_to_queue(std::string arg) {
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
}*/

/*unsigned short NodeRouter::find_port(char arg) {
    for (int i = 0; i < routing_table.size(); i++) {
        if (routing_table[i].router == arg) {
            return routing_table[i].next_router_port;
        }
    }

    return 0;
}*/


void NodeRouter::parse_file(char * filename) {
    char input[LINELENGTH];
    FILE *input_file;

    char router;
    char edge;
    unsigned short port;
    unsigned int weight;

    input_file = fopen(filename, "r");
    char token[SECTIONLENGTH];

    if (!input_file) {
        perror("File error");
    } else {
        while (!feof(input_file)) {
            fgets(input, LINELENGTH, (FILE *)input_file);
            for (int i = 0; i < 4; i++) {
                switch (i) {
                case 0:
                    strcpy(token, strtok(input, (char *)","));
                    router = token[0];
                    break;
                case 1:
                    strcpy(token, strtok(NULL, (char *)","));
                    edge = token[0];
                    break;
                case 2:
                    strcpy(token, strtok(NULL, (char *)","));
                    port = (unsigned short)atoi(token);
                    break;
                case 3:
                    strcpy(token, strtok(NULL, (char *)"\n"));
                    weight = (unsigned int)atoi(token);
                    break;
                }
            }
            if (router != node_id) {
                if (edge == node_id)
                    this->port = port;
                continue;
            }
            
            RoutingTableNode rtn;
            rtn.router_id = edge;
            rtn.cost = weight;
            rtn.port = port;
            rtn.is_neighbor = true;

            std::cout << "push " << rtn.router_id << std::endl;
            routing_table.push_back(rtn);
        }
    }

    fclose(input_file);
}

//build initial table with known neighbors
void NodeRouter::build_table() {

    std::cout << "build table";

    parse_file(TOPOLOGY_FILE);

    /*RoutingTableNode temp_routing_table;

    // Add this node as first entry to table
    temp_routing_table.cost = -1; //-1 means not set
    temp_routing_table.router_id = node_id;
//    temp_routing_table.next_router = node_id;
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
    }*/
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