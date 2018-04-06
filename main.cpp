#include "node.hpp"
#include "connection.hpp"

#define NODE_H_PORT (char *)"10006"

void run_injecter() {
    std::cout << "*** Starting Server Thread ***" << std::endl;
    std::string buffer, temp;
    std::string server, port = "10000";
    Connection connection;

    connection.setup_connection((char *)"127.0.0.1", NODE_H_PORT);

    for (;;) {
        std::cout << "Enter message followed by $: ";
        buffer = "2DThe quick brown fox jumps over the lazy dog.";
        temp = "";
        while (temp != "$") {
            std::cin >> temp;
            buffer = buffer + temp + " ";
        }
//        std::cout << "Enter server: ";
//        std::cin >> server;
//        server = server + buffer + " ";

       // std::cout << "Enter port: ";
       // std::cin >> port;

        connection.send_udp(buffer.c_str(), (char *)"127.0.0.1",
                            port.c_str());
    }

    std::cout << "*** Exiting Server Thread ***" << std::endl;
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        std::cout << "Invalid Args!" << std::endl;
        std::cout << "USAGE: " << argv[0] << " " << "<node>" << std::endl;
        std::cout << "For injecter: " << argv[0] << " " << "H" << std::endl;
        return 1;
    }

    if (argv[1][0] == 'H')
        run_injecter();
    else {
        NodeRouter node(*(argv[1]));
        node.run_router();
    }
    return 0;
}