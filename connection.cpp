#include "connection.hpp"

Connection::Connection() { initial_setup(); }

void Connection::initial_setup() {
    memset(&addr_hints, 0, sizeof(addr_hints));
    addr_hints.ai_family = AF_INET;
    addr_hints.ai_socktype = SOCK_DGRAM;
    addr_len = sizeof(struct sockaddr_in);
}

Connection::~Connection() {}

bool Connection::setup_connection(std::string address, std::string port) {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("Socket: ");
    }

    int info;
    info =
        getaddrinfo(address.c_str(), port.c_str(), &addr_hints, &this->setup);
    if (info != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(info));
        return false;
    }

    info = bind(this->sockfd, this->setup->ai_addr, this->setup->ai_addrlen);
    if (info == -1) {
        perror("Bind");
    }

    freeaddrinfo(setup);

    return true;
}

int Connection::send_udp(std::string request, std::string address_in,
                         std::string port_in) {
    int info;
    // Send to whoever last packet was received from
    if ((address_in == "") && (port_in == "")) {
        info = sendto(sockfd, request.c_str(), request.length(), 0,
                      (struct sockaddr *)&their_addr,
                      sizeof(struct sockaddr_storage));
        if (info == -1) {
            perror("Send error");
            exit(1);
        }

        return 0;
    }

    const char *address = address_in.c_str();
    const char *port = port_in.c_str();

    info = getaddrinfo(address, port, &addr_hints, &setup);
    if (info != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(info));
        return false;
    }

    info = sendto(sockfd, request.c_str(), strlen(request.c_str()), 0,
                  setup->ai_addr, setup->ai_addrlen);
    if (info == -1) {
        perror("Send error");
        exit(1);
    }

    freeaddrinfo(setup);
    return 0;
}

int Connection::recv_udp(std::string &request) {
    int info;
    char buffer[BUFFER_LEN];

    info = recvfrom(this->sockfd, buffer, (BUFFER_LEN - 1), 0,
                    (struct sockaddr *)&their_addr, (socklen_t *)&addr_len);
    if (info == -1) {
        perror("Receive error");
        exit(1);
    }
    buffer[info] = '\0';
    request = buffer;

    return 0;
}