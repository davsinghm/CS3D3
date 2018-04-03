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

#define BUFFER_LEN 1024

class Connection {
  private:
    struct addrinfo addr_hints;
    struct addrinfo *setup;
    int sockfd;
    std::size_t from_len;

    void initial_setup();

  protected:
    struct sockaddr_in their_addr;
    size_t addr_len;
    Connection();
    ~Connection();
    bool setup_connection(std::string address, std::string port);
    int send_udp(std::string request, std::string address_in,
                 std::string port_in);
    int recv_udp(std::string &request);
};

#endif