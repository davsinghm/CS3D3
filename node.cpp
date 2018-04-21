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
        RoutingTableNode &router = routing_table.at(i);
        std::cout << router.router_id << " ";

        if (router.is_neighbor) {
            if (router.ref_router_id != -1) {
                std::string str_ref(1, router.ref_router_id);

                std::cout << router.cost << " @ " << router.port
                    << ", " << router.ref_cost << " through " << (router.ref_router_id < 'A' && router.ref_router_id > 'Z' ? std::to_string(router.ref_router_id) : str_ref) << " ";
            } else {
                std::cout << router.cost << " @ " << router.port << " ";
            }

            std::cout << "(Last Received: " << (std::time(0) - router.last_update) << " sec ago)";
        }
        else {
            std::cout << router.ref_cost << " through " << router.ref_router_id;
        }
        std::cout << std::endl;
    }
}

//TODO do this aync?
void NodeRouter::update_dv_in_table(Packet *packet) {
    unsigned int new_cost;
    char to_router_id;
    bool has_updated_table = false; //if added or updated the entry, used for printing

    to_router_id = packet->data[0];
    std::string cost_str = packet->data.substr(1);
    new_cost = atoi((char *)cost_str.c_str());

    //trying to update routing table.
    pthread_mutex_lock(&mutex_routing_table); //lock it

    RoutingTableNode *src_router = get_router_by_id(packet->src_id); //src, neighbor from where dv came
    if (DEBUG && src_router->is_dead) {
        std::cout << "Neighbor " << src_router->router_id << " is alive now. After " << (std::time(0) - src_router->last_update) << " sec." << std::endl;
        src_router->is_dead = false;
    }
    src_router->last_update = std::time(0); //update the last_update timestamp
    new_cost += src_router->cost;

    RoutingTableNode *router = get_router_by_id(to_router_id);
    if (router != NULL) {

        if (router->is_neighbor) { // if router is neighbor, the cost could be smaller through other neighbor. so treat this neighbor as neighbor.
            if (new_cost < router->ref_cost || (src_router->router_id == router->ref_router_id && new_cost != router->ref_cost)) {
                if (DEBUG)
                    std::cout << "Updating new ref_cost to " << new_cost << " of neighbor " << to_router_id << ". Received from " << src_router->router_id << std::endl;

                router->ref_cost = new_cost;
                router->ref_router_id = src_router->router_id;
                has_updated_table = true;
            }
        } else {
            if (new_cost < router->ref_cost || (src_router->router_id == router->ref_router_id && new_cost != router->ref_cost)) {
                if (DEBUG)
                    std::cout << "Updating new cost to " << new_cost << " of distant node " << to_router_id << ". Received from " << src_router->router_id << std::endl;
                router->ref_cost = new_cost;
                router->ref_router_id = src_router->router_id;
                has_updated_table = true;
            }
        }
    } else {
        //add if it's a distant router.

        RoutingTableNode rtn;
        rtn.is_neighbor = false;
        rtn.router_id = to_router_id;
        rtn.ref_cost = new_cost;
        rtn.ref_router_id = packet->src_id;
        rtn.port = 0;
        routing_table.push_back(rtn);

        has_updated_table = true;

        if (DEBUG)
            std::cout << "Inserting new router " << to_router_id << " in table with cost: " << new_cost << ". Received from " << src_router->router_id << std::endl;
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
                std::cout << "Final Destination: Source: " << packet.src_id << std::endl << "Message:" << std::endl << packet.data << std::endl;
                break;
            default:
                std::cout << "HEADER_FIELD_TYPE_INVALID" << std::endl;
        }
    }
}

RoutingTableNode *NodeRouter::get_router_by_id(char id) {
    for (int i = 0; i < routing_table.size(); i++) {
        RoutingTableNode &router = routing_table.at(i);

        if (router.router_id == id)
            return &router;
    }

    return NULL;
}

bool NodeRouter::is_router_dead(RoutingTableNode &router) {
    if (!router.is_neighbor) {
        if (router.ref_router_id == -1) //if there's no neighbor ref to reach a distant router
            return true;
        else {
            RoutingTableNode *ref_router = get_router_by_id(router.ref_router_id);
            if (ref_router == NULL)
                return true;

            return std::time(0) - ref_router->last_update > SLEEP_SEC + 1; //if hasn't listened from ref'd neighbor for a while
        }
    }

    return std::time(0) - router.last_update > SLEEP_SEC + 1; //if hasn't listened from neighbor for a while
}

/**
 * returns last known link cost of @param{router} from this router
 * i.e. this function doesn't check if the router is dead or not.
 */
unsigned int NodeRouter::get_link_cost(RoutingTableNode &router) {
    if (router.is_neighbor) {
        if (router.ref_router_id != -1 && router.ref_cost < router.cost)
            return router.ref_cost;
        else
            return router.cost;
    } else
        return router.ref_cost;
}

void broadcast_table(NodeRouter *r, RoutingTableNode &dest_router) {

    if (!dest_router.is_neighbor) { //would never happen. only for debugging.
        if (DEBUG)
            std::cout << "Broadcast Table: Destination router is not neighbor, ignoring." << std::endl;
        return;
    }

    for (int j = 0; j < r->routing_table.size(); j++) {
        RoutingTableNode &router = r->routing_table.at(j);

        if (router.router_id == dest_router.router_id)
            continue; //don't send neighbor router its cost, as link cost is static.

        Packet packet;
        packet.type = HEADER_FIELD_TYPE_UPDATE_DV;
        packet.data = ((char)router.router_id); // to reach the router...

        unsigned int cost = r->get_link_cost(router); //last known cost
        if (r->is_router_dead(router)) //check if router is dead
            cost = INT_MAX;

        packet.data += std::to_string(cost); //...cost of link

        packet.src_id = r->node_id; //this router
        packet.dest_id = dest_router.router_id; //neighbor is dest

        std::string message = r->serialize_packet(&packet);
        r->connection.send_udp(message, HOME_ADDR, dest_router.port); //send to neighbor (tc port)
    }
}

void *adv_thread_func(void *args) {
    NodeRouter *r = (NodeRouter *)args;

    //this should send whole routing table to its neighbors
    for (;;) {
        pthread_mutex_lock(&r->mutex_routing_table);

        bool p_rt = false;

        //loop through all neighbors
        for (int i = 0; i < r->routing_table.size(); i++) {
            RoutingTableNode &neighbor_i = r->routing_table.at(i);
            if (!neighbor_i.is_neighbor)
                continue; //don't send to distant router, their neighbors will send their info

            if (r->is_router_dead(neighbor_i)) {
                if (DEBUG && !neighbor_i.is_dead) {
                    std::cout << "Neighbor " << neighbor_i.router_id << " is dead. Last Update: " << (std::time(0) - neighbor_i.last_update) << " sec ago" << std::endl;
                    neighbor_i.is_dead = true;
                }

                //update the table
                for (int j = 0; j < r->routing_table.size(); j++) {
                    RoutingTableNode &rtn = r->routing_table.at(j);

                    //updates all (neighbor or distant) routers' link cost which were reached through dead neighbor router.
                    if (rtn.ref_router_id == neighbor_i.router_id) {
                        p_rt = p_rt || rtn.ref_cost != INT_MAX; //print routing table if updated, and once
                        rtn.ref_cost = INT_MAX;
                        rtn.ref_router_id = -1;
                    }
                }
            }

            //this also include broadcasting to dead neighbor, it might be the first contact.
            broadcast_table(r, neighbor_i);
        }

        if (p_rt)
            r->print_routing_table();

        pthread_mutex_unlock(&r->mutex_routing_table);

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
            rtn.ref_cost = INT_MAX;
            rtn.ref_router_id = -1;
            rtn.port = port;
            rtn.is_neighbor = true;
            rtn.last_update = std::time(0); //0 means past, way > than min time delay

            routing_table.push_back(rtn);
        }
    }

    fclose(input_file);
}
