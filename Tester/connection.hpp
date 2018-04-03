#ifndef CONNECTION
#define CONNECTION

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include <stdio.h>

#define BUFFER_LEN 30

class Connection {
  private:
    // Private Variables
    struct addrinfo addr_hints;
    struct addrinfo *setup;
    int sockfd;
    std::size_t from_len;

    // Private Functions
    void initial_setup();

  public:
    // Public Variables
    struct sockaddr_in their_addr;
    size_t addr_len;

    // Constructor
    Connection();
    // Connection(char* address, char* port);

    // Destructor
    ~Connection();

    // Public functions
    bool setup_connection(std::string address, std::string port);
    int send_udp(std::string request, std::string address_in,
                 std::string port_in);
    int recv_udp(std::string &request);
};

#endif