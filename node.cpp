#include "node.hpp"

Router::Router(char id) {
    router_id = id;
    // Build routing table based on above function results on graph
    build_table(TOPOLOGY_FILE);
    // Bind to router's port
    connection.setup_connection(HOME_ADDR, this->port);

    std::cout << "Initial ";
    print_routing_table();

    run_advertisement_thread();
}

Router::~Router() { pthread_join(adv_thread, NULL); }

void Router::run_router() {
    std::string message;
    for (;;) {
        connection.recv_udp(message);

        Packet packet;
        handle_packet(packet, message);
    }
}

void Router::print_routing_table() {
    std::cout << "Routing Table:" << std::endl;
    for (int i = 0; i < routing_table.size(); i++) {
        RouterEntry &router = routing_table.at(i);
        std::cout << router.router_id << " ";

        if (router.is_neighbor) {
            if (router.ref_router_id != -1) {
                std::string str_ref(1, router.ref_router_id);

                std::cout << router.cost << " @ " << router.port << ", "
                          << router.ref_cost << " through "
                          << (router.ref_router_id < 'A' &&
                                      router.ref_router_id > 'Z'
                                  ? std::to_string(router.ref_router_id)
                                  : str_ref)
                          << " ";
            } else {
                std::cout << router.cost << " @ " << router.port << " ";
            }

            std::cout << "(Last Received: "
                      << (std::time(0) - router.last_update) << " sec ago)";
        } else {
            std::cout << router.ref_cost << " through " << router.ref_router_id;
        }
        std::cout << std::endl;
    }
}

// TODO do this aync?
void Router::update_dv_in_table(Packet *packet) {
    unsigned int new_cost;
    char to_router_id;
    bool has_updated_table =
        false; // if added or updated the entry, used for printing

    to_router_id = packet->data[0];
    std::string cost_str = packet->data.substr(1);
    new_cost = atoi((char *)cost_str.c_str());

    // trying to update routing table.
    pthread_mutex_lock(&mutex_routing_table); // lock it

    RouterEntry *src_router =
        get_router_by_id(packet->src_id); // src, neighbor from where dv came
    if (DEBUG && src_router->is_dead) {
        std::cout << "Neighbor " << src_router->router_id
                  << " is alive now. After "
                  << (std::time(0) - src_router->last_update) << " sec."
                  << std::endl;
        src_router->is_dead = false;
    }
    src_router->last_update = std::time(0); // update the last_update timestamp
    new_cost += src_router->cost;

    RouterEntry *router = get_router_by_id(to_router_id);
    if (router != NULL) {

        if (router->is_neighbor) { // if router is neighbor, the cost could be
                                   // smaller through other neighbor. so treat
                                   // this neighbor as neighbor.
            if (new_cost < router->ref_cost ||
                (src_router->router_id == router->ref_router_id &&
                 new_cost != router->ref_cost)) {
                if (DEBUG)
                    std::cout << "Updating new ref_cost to " << new_cost
                              << " of neighbor " << to_router_id
                              << ". Received from " << src_router->router_id
                              << std::endl;

                router->ref_cost = new_cost;
                router->ref_router_id = src_router->router_id;
                has_updated_table = true;
            }
        } else {
            if (new_cost < router->ref_cost ||
                (src_router->router_id == router->ref_router_id &&
                 new_cost != router->ref_cost)) {
                if (DEBUG)
                    std::cout << "Updating new cost to " << new_cost
                              << " of distant router " << to_router_id
                              << ". Received from " << src_router->router_id
                              << std::endl;
                router->ref_cost = new_cost;
                router->ref_router_id = src_router->router_id;
                has_updated_table = true;
            }
        }
    } else {
        // add if it's a distant router.

        RouterEntry router;
        router.is_neighbor = false;
        router.router_id = to_router_id;
        router.ref_cost = new_cost;
        router.ref_router_id = packet->src_id;
        router.port = 0;
        routing_table.push_back(router);

        has_updated_table = true;

        if (DEBUG)
            std::cout << "Inserting new router " << to_router_id
                      << " in table with cost: " << new_cost
                      << ". Received from " << src_router->router_id
                      << std::endl;
    }

    pthread_mutex_unlock(&mutex_routing_table); // unlock

    if (has_updated_table)
        print_routing_table();
}

void Router::forward_message(Packet &packet) {

    int index = -1;
    int ref_router_id = -1;

    if (DEBUG)
        std::cout << "Packet destination: " << packet.dest_id
                  << ". Source: " << packet.src_id << std::endl;

    RouterEntry *fwd_router = get_router_by_id(packet.dest_id);
    if (fwd_router == NULL) {
        if (DEBUG)
            std::cout << "Couldn't find router " << packet.dest_id
                      << " in table. Packet dropped." << std::endl;
        return;
    }

    if (fwd_router != NULL) {
        if (!fwd_router->is_neighbor) {
            fwd_router = get_router_by_id(fwd_router->ref_router_id);
        }
    }

    if (fwd_router == NULL) {
        if (DEBUG)
            std::cout << "Couldn't find ref router for " << packet.dest_id
                      << " in table. Packet dropped." << std::endl;
        return;
    }

    std::cout << "Forwarding the packet to neighbor " << fwd_router->router_id
              << " on Port: " << fwd_router->port << std::endl;

    std::string message = serialize_packet(&packet);
    connection.send_udp(message, HOME_ADDR, fwd_router->port);
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
void Router::handle_packet(Packet &packet, std::string message) {
    // std::cout << "Received: " << message << std::endl;

    if (message.length() < 2) {
        std::cerr << "Invalid Packet Received. Message Length(): "
                  << message.length() << std::endl;
        return;
    }
    // deserialize packet
    packet.type = message[0];
    packet.dest_id = message[1];
    packet.src_id = message[2];
    packet.data = message.substr(3);

    // TODO put the forwarding packet in queue and send async from
    // listening/recv thread
    if (packet.dest_id != router_id) {
        if (DEBUG)
            std::cout << "Forward Message" << std::endl;
        pthread_mutex_lock(
            &mutex_routing_table); // TODO move to more approp place.
        forward_message(packet);
        pthread_mutex_unlock(&mutex_routing_table);
    } else {

        switch (packet.type) {
        case HEADER_FIELD_TYPE_UPDATE_DV:
            update_dv_in_table(&packet);
            break;
        case HEADER_FIELD_TYPE_MSG:
            std::cout << "Final Destination: Source: " << packet.src_id
                      << std::endl
                      << "Message:" << std::endl
                      << packet.data << std::endl;
            break;
        default:
            std::cout << "HEADER_FIELD_TYPE_INVALID" << std::endl;
        }
    }
}

RouterEntry *Router::get_router_by_id(char id) {
    for (int i = 0; i < routing_table.size(); i++) {
        RouterEntry &router = routing_table.at(i);

        if (router.router_id == id)
            return &router;
    }

    return NULL;
}

bool Router::is_router_dead(RouterEntry &router) {
    if (!router.is_neighbor) {
        if (router.ref_router_id ==
            -1) // if there's no neighbor ref to reach a distant router
            return true;
        else {
            RouterEntry *ref_router = get_router_by_id(router.ref_router_id);
            if (ref_router == NULL)
                return true;

            return std::time(0) - ref_router->last_update >
                   SLEEP_SEC +
                       1; // if hasn't listened from ref'd neighbor for a while
        }
    }

    return std::time(0) - router.last_update >
           SLEEP_SEC + 1; // if hasn't listened from neighbor for a while
}

/**
 * returns last known link cost of @param{router} from this router
 * i.e. this function doesn't check if the router is dead or not.
 */
unsigned int Router::get_link_cost(RouterEntry &router) {
    if (router.is_neighbor) {
        if (router.ref_router_id != -1 && router.ref_cost < router.cost)
            return router.ref_cost;
        else
            return router.cost;
    } else
        return router.ref_cost;
}

void broadcast_table(Router *r, RouterEntry &dest_router) {

    if (!dest_router.is_neighbor) { // would never happen. only for debugging.
        if (DEBUG)
            std::cout << "Broadcast Table: Destination router is not neighbor, "
                         "ignoring."
                      << std::endl;
        return;
    }

    for (int j = 0; j < r->routing_table.size(); j++) {
        RouterEntry &router = r->routing_table.at(j);

        if (router.router_id == dest_router.router_id)
            continue; // don't send neighbor router its cost, as link cost is
                      // static.

        Packet packet;
        packet.type = HEADER_FIELD_TYPE_UPDATE_DV;
        packet.data = ((char)router.router_id); // to reach the router...

        unsigned int cost = r->get_link_cost(router); // last known cost
        if (r->is_router_dead(router))                // check if router is dead
            cost = INT_MAX;

        packet.data += std::to_string(cost); //...cost of link

        packet.src_id = r->router_id;           // this router
        packet.dest_id = dest_router.router_id; // neighbor is dest

        std::string message = r->serialize_packet(&packet);
        r->connection.send_udp(message, HOME_ADDR,
                               dest_router.port); // send to neighbor (tc port)
    }
}

void *adv_thread_func(void *args) {
    Router *r = (Router *)args;

    // this should send whole routing table to its neighbors
    for (;;) {
        pthread_mutex_lock(&r->mutex_routing_table);

        bool p_rt = false;

        // loop through all neighbors
        for (int i = 0; i < r->routing_table.size(); i++) {
            RouterEntry &neighbor_i = r->routing_table.at(i);
            if (!neighbor_i.is_neighbor)
                continue; // don't send to distant router, their neighbors will
                          // send their info

            if (r->is_router_dead(neighbor_i)) {
                if (DEBUG && !neighbor_i.is_dead) {
                    std::cout << "Neighbor " << neighbor_i.router_id
                              << " is dead. Last Update: "
                              << (std::time(0) - neighbor_i.last_update)
                              << " sec ago" << std::endl;
                    neighbor_i.is_dead = true;
                }

                // update the table
                for (int j = 0; j < r->routing_table.size(); j++) {
                    RouterEntry &router = r->routing_table.at(j);

                    // updates all (neighbor or distant) routers' link cost
                    // which were reached through dead neighbor router.
                    if (router.ref_router_id == neighbor_i.router_id) {
                        p_rt = p_rt || router.ref_cost !=
                                           INT_MAX; // print routing table if
                                                    // updated, and once
                        router.ref_cost = INT_MAX;
                        router.ref_router_id = -1;
                    }
                }
            }

            // this also include broadcasting to dead neighbor, it might be the
            // first contact.
            broadcast_table(r, neighbor_i);
        }

        if (p_rt)
            r->print_routing_table();

        pthread_mutex_unlock(&r->mutex_routing_table);

        sleep(SLEEP_SEC);
    }

    pthread_exit(NULL);
}

void Router::run_advertisement_thread() {
    pthread_create(&adv_thread, NULL, adv_thread_func, (void *)this);
}

std::string Router::serialize_packet(Packet *packet) {
    std::string str = "";
    str += packet->type;
    str += packet->dest_id;
    str += packet->src_id;
    str += packet->data;

    return str;
}

// build initial table with known neighbors
void Router::build_table(char *filename) {
    char input[LINELENGTH];
    FILE *input_file;

    char router;
    char edge;
    unsigned int port;
    unsigned int weight;

    input_file = fopen(filename, "r");

    if (!input_file) {
        std::cerr << "Unable to open file: " << filename << std::endl;
    } else {
        while (!feof(input_file)) {
            fgets(input, LINELENGTH, (FILE *)input_file);
            router = strtok(input, (char *)",")[0];
            edge = strtok(NULL, (char *)",")[0];
            port = (unsigned int)atoi(strtok(NULL, (char *)","));
            weight = (unsigned int)atoi(strtok(NULL, (char *)"\n"));

            if (router != router_id) {
                if (edge == router_id)
                    this->port = port;
                continue;
            }

            RouterEntry router;
            router.router_id = edge;
            router.cost = weight;
            router.ref_cost = INT_MAX;
            router.ref_router_id = -1;
            router.port = port;
            router.is_neighbor = true;
            router.last_update =
                std::time(0); // 0 means past, way > than min time delay

            routing_table.push_back(router);
        }
    }

    fclose(input_file);
}
