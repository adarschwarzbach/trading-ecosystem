#ifndef SERVER_HPP
#define SERVER_HPP

// Project headers
#include "exchange/exchange.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <queue>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <nlohmann/json.hpp>

#define PORT 8080
#define MAX_PENDING_CONNECTIONS 100

class Server
{
private:
    Exchange exchange;
    std::queue<int> client_queue;
    std::mutex queue_mutex;
    std::vector<std::thread> workers;

    void worker_thread();                  // Handles client connections
    void handle_client(int client_socket); // Processes each client request

public:
    Server(const std::vector<std::string> &allowed_tickers);
    void start(); // Starts the server
};

#endif
