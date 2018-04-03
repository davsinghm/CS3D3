#include "parser.hpp"

void Parser::parse(Graph_Vector &graph) {
    char input[LINELENGTH];
    FILE *input_file;

    char router;
    char edge;
    unsigned short port;
    unsigned int weight;

    input_file = fopen(TOPOLOGY_FILE, "r");
    char token[SECTIONLENGTH];

    if (!input_file) {
        perror("File error");
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
            graph.add_to_graph(router, edge, port, weight);
        }
    }

    fclose(input_file);
}