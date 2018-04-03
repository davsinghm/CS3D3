#ifndef THREAD_SERVER
#define THREAD_SERVER

#include <iostream>
#include <pthread.h>
#include <string.h>

#include "connection.hpp"

#define PORT_LEN 6

class Thread_Server {
  private:
    Connection client_connection;
    Connection server_connection;

    pthread_t server;
    pthread_t client;

  public:
    char port_client[PORT_LEN];
    char port_server[PORT_LEN];

    Thread_Server(char in_port_send[], char in_port_recv[]);
    ~Thread_Server();

    static void *server_thread(void *p);
    static void *client_thread(void *p);
};

#endif