#include "node.hpp"

Node_Router::Node_Router(char this_router_in)
{
    // Nodes are parsed, edges are put into a queue
    parser.parse(*this);
    // When nodes are intialised, vertices are added
    flush_queue();
    // Set the home router based on the input to the constructor ^
    this_router = this_router_in;
    // Perform Bellman-Ford algorithm on graph
    bf_search(this_router);
    // Build routing table based on above function results on graph
    build_table();
    // Bind to router's port
    setup_connection(HOME_ADDR, std::to_string((int)node_list[find_location(this_router)].port));

    for(int i = 0; i < routing_table.size(); i++)
    {
        std::cout << routing_table[i].router << " " << routing_table[i].next_router << " " << routing_table[i].next_router_port << std::endl;
    }

    // Start routing
    router();
}

Node_Router::~Node_Router()
{
}

void Node_Router::router()
{
    std::string request;
    for(;;)
    {
        recv_udp(request);
        std::cout << "Received: " << request << std::endl;
        if(add_to_queue(request))
        {
            request = packet_queue.back().destination;
            request = request + " " + packet_queue.back().message;
            send_udp(request, HOME_ADDR, &packet_queue.back().port[0]);
        }
        else
        {
            std::cout << "Final Destination: " << packet_queue.back().message << std::endl;
        }
        packet_queue.pop_back();
    }
}

bool Node_Router::add_to_queue(std::string arg)
{
    size_t len = (arg.length() + 1) * sizeof(char);
    char* packet = (char*)malloc(len);
    char* message = (char*)malloc(len);
    char destination;
    std::string str_port;
    char port[6];

    strcpy(packet, arg.c_str());

    destination = *(strtok(packet, (char*)" "));
    
    strcpy(message, strtok(NULL, (char*)"\0"));
    str_port = std::to_string((int)find_port(destination));
    strcpy(port, str_port.c_str());

    temp_packet_queue.destination = destination;
    temp_packet_queue.message = message;
    strcpy(temp_packet_queue.port, port);
    packet_queue.push_back(temp_packet_queue);

    free((void*)packet);
    free((void*)message);
    if(destination == this_router)
    {
        return false;
    }
    else
    {
        return true;
    }
}

unsigned short Node_Router::find_port(char arg)
{
    for(int i = 0; i < routing_table.size(); i++)
    {
        if(routing_table[i].router == arg)
        {
            return routing_table[i].next_router_port;
        }
    }

    return 0;
}

void Node_Router::build_table()
{
    // Add this node as first entry to table
    temp_routing_table.cost = 0;
    temp_routing_table.router = this_router;
    temp_routing_table.next_router = this_router;
    routing_table.push_back(temp_routing_table);

    // Search through rest of the table
    for(int i = 0; i < node_list.size(); i++)
    {
        // Skip the the home router since this was already added
        if(node_list[i].router == this_router)
        {
            continue;
        }
        // If the router is a neighbour
        else if(node_list[node_list[i].prev].router == this_router)
        {
            temp_routing_table.cost = node_list[i].dist_start;
            temp_routing_table.router = node_list[i].router;
            temp_routing_table.next_router = node_list[i].router;
            temp_routing_table.next_router_port = node_list[i].port;
            routing_table.push_back(temp_routing_table);
        }
        // If the router is distant
        else
        {
            temp_routing_table.cost = node_list[i].dist_start;
            temp_routing_table.router = node_list[i].router;
            temp_routing_table.next_router = find_next_router(i);
            temp_routing_table.next_router_port = node_list[find_location(temp_routing_table.next_router)].port;
            routing_table.push_back(temp_routing_table);
        }
    }
}

char Node_Router::find_next_router(int l)
{
    int count = 0;
    while(count < SEARCH_LIM)
    {
        if(node_list[node_list[l].prev].router == this_router)
        {
            return node_list[l].router;
        }
        else
        {
            l = node_list[l].prev;
        }
        count++;
    }

    std::cout << "Error: find_next_router()\n";
    exit(1);

    return '\0';
}