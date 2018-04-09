#include "node.hpp"

NodeRouter::NodeRouter(char node) {
    node_id = node;
    // Build routing table based on above function results on graph
    build_table(TOPOLOGY_FILE);
    // Bind to router's port
    connection.setup_connection(HOME_ADDR, this->port);

    std::cout << "Initial ";
    print_routing_table();

    run_advertisement_thread();
}

NodeRouter::~NodeRouter() {
    pthread_join(adv_thread, NULL);
}

void NodeRouter::run_router() {
    std::string message;
    for (;;) {
        connection.recv_udp(message);

        Packet packet;
        handle_packet(packet, message);
    }
}

void NodeRouter::print_routing_table() {
    std::cout << "Routing Table:" << std::endl;
    for (int i = 0; i < routing_table.size(); i++) {
        std::cout << routing_table[i].router_id << " "
                  << routing_table[i].cost << " ";

        if (routing_table[i].is_neighbor) {
            std::cout << routing_table[i].port << " "
                      << "(Last Received: " << (std::time(0) - routing_table[i].last_update) << " sec ago)";
        }
        else {
            std::cout << "through "; 
            std::cout << routing_table[i].ref_router_id;
        }
        std::cout << std::endl;
    }
}

//TODO do this aync?
void NodeRouter::update_dv_in_table(Packet *packet) {
    int new_cost;
    char to_router_id;
    bool has_updated_table = false; //if added or updated the entry, used for printing

    to_router_id = packet->data[0];
    std::string cost_str = packet->data.substr(1);
    new_cost = atoi((char *)cost_str.c_str());

    //trying to update routing table.
    pthread_mutex_lock(&mutex_routing_table); //lock it

    //TODO can optimize if we sum the cost of this link in the node that's sending the DV. then we don't need to find the src (which is neighbor) in table
    RoutingTableNode *src_rtn = NULL; //src, neighbor from where dv came
    for (int i = 0; i < routing_table.size(); i++) {
        RoutingTableNode &rtn = routing_table.at(i);
        if (rtn.router_id == packet->src_id) {
            src_rtn = &rtn;

            //update the last_update timestamp
            rtn.last_update = std::time(0);
        }
    }

    bool has_node = false; //table has node info, it's updated instead of adding new one
    for (int i = 0; i < routing_table.size(); i++) {
        RoutingTableNode &rtn = routing_table.at(i);
        if (rtn.router_id == to_router_id) {
            has_node = true;

            if (/*(src_rtn->router_id == rtn.ref_router_id && src_rtn->cost + new_cost != rtn.cost) || */src_rtn->cost + new_cost < rtn.cost) {
                if (DEBUG)
                    std::cout << "Updating new cost to " << src_rtn->cost + new_cost << " of " << to_router_id << " in table of " << rtn.router_id << ". Received from " << src_rtn->router_id << std::endl;
                rtn.cost = src_rtn->cost + new_cost;
                rtn.ref_router_id = src_rtn->router_id;
                has_updated_table = true;
                break;
            }
        }
    }

    //add if it's a distant router.
    if (!has_node && to_router_id != node_id) {
        RoutingTableNode rtn;
        rtn.is_neighbor = false;
        rtn.router_id = to_router_id;
        rtn.cost = new_cost + src_rtn->cost;
        rtn.ref_router_id = packet->src_id;
        rtn.port = 0;
        routing_table.push_back(rtn);

        has_updated_table = true;

        if (DEBUG)
            std::cout << "Inserting new router " << to_router_id << " in table with cost: " << (src_rtn->cost + new_cost) << ". Received from " << src_rtn->router_id << std::endl;
    }

    pthread_mutex_unlock(&mutex_routing_table); //unlock

    if (has_updated_table)
        print_routing_table();
}

void NodeRouter::forward_message(Packet &packet) {

    int index = -1;
    int ref_router_id = -1;
    int neighbor_min_cost = INT_MAX;
    int neighbor_min_cost_i = -1; // neighbor's index with min cost to greedy select in case distant router in not in the table.
    if (DEBUG)
        std::cout << "Packet destination: " << packet.dest_id << ". Source: " << packet.src_id << std::endl;
    for (int i = 0; i < routing_table.size(); i++) {
        RoutingTableNode &rtn = routing_table.at(i);
        if (rtn.is_neighbor) {
            if (rtn.router_id == packet.dest_id) {
                index = i;
                goto goto_forward;
            }

            if (neighbor_min_cost > rtn.cost && (std::time(0) - rtn.last_update) <= SLEEP_SEC) {
                neighbor_min_cost_i = i;
                neighbor_min_cost = rtn.cost;
            }
        } else {
            if (rtn.router_id == packet.dest_id) {
                ref_router_id = rtn.ref_router_id;
                break;
            }
        }
    }

    if (ref_router_id == -1) {
        if (neighbor_min_cost_i != -1) {
            RoutingTableNode &fwd_rtn = routing_table.at(neighbor_min_cost_i);

            if (DEBUG)
                std::cout << "Couldn't find distant router " <<  packet.dest_id << " in table. Greedily selecting alive neighbor: " << fwd_rtn.router_id << std::endl;

            std::string message = serialize_packet(&packet);
            connection.send_udp(message, HOME_ADDR, fwd_rtn.port);
            return;
        }
    }

    //TODO convert to function
    for (int i = 0; i < routing_table.size(); i++) {
        RoutingTableNode &rtn = routing_table.at(i);
        if (!rtn.is_neighbor)
            continue;

        if (rtn.router_id == ref_router_id) {
            index = i;
            goto goto_forward;
        }
    }

    if (DEBUG)
        std::cout << "DEBUG: Can't forward the message. No alive neighbor found. Packet dropped." << std::endl;
    return;

goto_forward:
    RoutingTableNode &fwd_rtn = routing_table.at(index);
    if (DEBUG)
        std::cout << "Forwarding the packet to neighbor " << fwd_rtn.router_id << " on Port: " << fwd_rtn.port << std::endl;

    std::string message = serialize_packet(&packet);
    connection.send_udp(message, HOME_ADDR, fwd_rtn.port);
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
        std::cerr << "Invalid Packet Received. Message Length(): " << message.length() << std::endl;
        return;
    }
    //deserialize packet
    packet.type = message[0];
    packet.dest_id = message[1];
    packet.src_id = message[2];
    packet.data = message.substr(3);

    //TODO put the forwarding packet in queue and send async from listening/recv thread
    if (packet.dest_id != node_id) {
        if (DEBUG)
            std::cout << "Forward Message" << std::endl;
        pthread_mutex_lock(&mutex_routing_table); //TODO move to more approp place.
        forward_message(packet);
        pthread_mutex_unlock(&mutex_routing_table);
    } else {

        switch (packet.type) {
            case HEADER_FIELD_TYPE_UPDATE_DV:
                update_dv_in_table(&packet);
                break;
            case HEADER_FIELD_TYPE_MSG:
                if (DEBUG)
                    std::cout << "Final Destination: Source: " << packet.src_id << " Message:" << std::endl << packet.data << std::endl;
                break;
            default:
                std::cout << "HEADER_FIELD_TYPE_INVALID" << std::endl;
        }
    }
}

void *adv_thread_func(void *args) {
    NodeRouter *node = (NodeRouter *)args;

    //this should send whole routing table to its neighbors
    for (;;) {
        pthread_mutex_lock(&node->mutex_routing_table);

        bool p_rt = false;

        for (int i = 0; i < node->routing_table.size(); i++) {
            RoutingTableNode rtn_neighbor = node->routing_table[i];
            if (!rtn_neighbor.is_neighbor)
                continue; //don't send to distant router, their neighbors will send their info
            bool is_rtn_neighbor_dead = std::time(0) - rtn_neighbor.last_update > SLEEP_SEC;

            if (is_rtn_neighbor_dead) {
                if (DEBUG)
                    std::cout << "Neighbor " << rtn_neighbor.router_id << " is dead. Last Update: " << (std::time(0) - rtn_neighbor.last_update) << " sec ago" << std::endl;
            }

            for (int j = 0; j < node->routing_table.size(); j++) {
                RoutingTableNode &rtn = node->routing_table.at(j);

                //updates all distant router's cost which were sent through dead neighbor router.
                if (!rtn.is_neighbor && rtn.ref_router_id == rtn_neighbor.router_id && is_rtn_neighbor_dead) {
                    p_rt = p_rt || rtn.cost != INT_MAX; //print routing table if updated, and once
                    rtn.cost = INT_MAX;
                    rtn.ref_router_id = 'Z';
                }

                Packet packet;
                packet.type = HEADER_FIELD_TYPE_UPDATE_DV;
                packet.data = ((char)rtn.router_id); // to reach the router...

                //this tells the rtn_neighbor that other neighbor rtn is dead
                if (rtn.is_neighbor && /*is_dead: */std::time(0) - rtn.last_update > SLEEP_SEC)
                    packet.data += std::to_string(INT_MAX);
                else
                    packet.data += std::to_string(rtn.cost); //...cost of link

                packet.src_id = node->node_id; //this router
                packet.dest_id = rtn_neighbor.router_id; //neighbor is dest

                std::string message = node->serialize_packet(&packet);
                node->connection.send_udp(message, HOME_ADDR, rtn_neighbor.port); //send to neighbor (tc port)
            }
        }

        if (p_rt)
            node->print_routing_table();

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

//build initial table with known neighbors
void NodeRouter::build_table(char *filename) {
    char input[LINELENGTH];
    FILE *input_file;

    char router;
    char edge;
    unsigned short port;
    unsigned int weight;
    char token[128];

    input_file = fopen(filename, "r");

    if (!input_file) {
        std::cerr << "Unable to open file: " << filename << std::endl;
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
            rtn.last_update = std::time(0) - 1 - SLEEP_SEC; //0 means past, way > than min time delay

            routing_table.push_back(rtn);
        }
    }

    fclose(input_file);
}
