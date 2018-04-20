#include "node.hpp"
#include "connection.hpp"

bool DEBUG;

#define NODE_H_PORT (char *)"10006"

void run_injecter() {
    std::string buffer, temp;
    std::string server, port = "10000"; //inject to A.
    char node_dest = 'D'; //ultimate destination
    Connection connection;

    connection.setup_connection((char *)"127.0.0.1", NODE_H_PORT);

    //TODO use struct Packet and serialize methods from NodeRouter.
    buffer = "2";
    std::cout << "Enter node destination: " << std::endl;
    std::cin >> node_dest;
    buffer += node_dest;
    buffer += "HThe quick brown fox jumps over the lazy dog.";
    buffer += " on ";
    buffer += std::to_string(std::time(0));

    connection.send_udp(buffer.c_str(), (char *)"127.0.0.1", port.c_str());
    exit(0);
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        std::cout << "Invalid Args!" << std::endl;
        std::cout << "USAGE: " << argv[0] << " " << "<node> [<debug mode>]" << std::endl;
        std::cout << std::endl;
        std::cout << "       " << "<node> can be one from topology.dat " << std::endl;
        std::cout << "       " << "use <node> = H for injecting message to dest node" << std::endl;
        std::cout << "       " << "use <debug mode> = 1 for verbose printing everything in terminal instead of writing to file" << std::endl;
        return 1;
    }

    DEBUG = argc >= 3 && argv[2][0] == '1';

    if (argv[1][0] == 'H')
        run_injecter();
    else {
        NodeRouter node(*(argv[1]));
        node.run_router();
    }
    return 0;
}