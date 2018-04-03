#include<pthread.h>
#include "thread_server.hpp"
#include "connection.hpp"

Thread_Server::Thread_Server(char* in_port_send, char* in_port_recv)
{
    strcpy(this->port_client, in_port_send);
    strcpy(this->port_server, in_port_recv);

    pthread_create(&server, NULL, this->server_thread, this);
    pthread_create(&client, NULL, this->client_thread, this);
}

Thread_Server::~Thread_Server()
{
    // std::cout << "*** Destructor Initialise ***\n";
    pthread_join(client, NULL);
    pthread_join(server, NULL);
    // std::cout << "*** Destructor Finished ***\n";
}

void* Thread_Server::server_thread(void* p)
{
    std::cout << "*** Starting Server Thread ***\n";
    Thread_Server* this_p = (Thread_Server*)p;
    std::string buffer, temp;
    std::string server, port;
    this_p->server_connection.setup_connection((char*)"127.0.0.1", (char*)"10006");

    for(;;)
    {
        std::cout << "Enter message followed by $: ";
        buffer = "";
        temp = "";
        while(temp != "$")
        {
            buffer = buffer + " " + temp;
            std::cin >> temp;
        }
        std::cout << "Enter server: ";
        std::cin >> server;
        server = server + buffer + " ";
        
        std::cout << "Enter port: ";
        std::cin >> port;
        
        this_p->server_connection.send_udp(server.c_str(), (char*)"127.0.0.1", port.c_str());
    }
    
    std::cout << "*** Exiting Server Thread ***\n";

    pthread_exit(NULL);
}

void* Thread_Server::client_thread(void* p)
{
    // std::cout << "*** Starting Client Thread ***\n";
    // Thread_Server* this_p = (Thread_Server*)p;
    // std::string buffer;
    // this_p->client_connection.setup_connection((char*)"127.0.0.1", this_p->port_client);

    // std::cout << "Press enter to send packet...\n";
    // getchar();
    // std::cout << "Sending..." << std::endl;

    // this_p->client_connection.send_udp((char*)"henlo der", (char*)"127.0.0.1", this_p->port_server);
    // this_p->client_connection.recv_udp(buffer);
    // std::cout << "Received from server: " << buffer << std::endl;
    // this_p->client_connection.send_udp(buffer, (char*)"127.0.0.1", this_p->port_server);

    // std::cout << "*** Exiting Client Thread ***\n";
    pthread_exit(NULL);
}