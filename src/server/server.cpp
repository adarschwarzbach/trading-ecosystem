#include "server/server.hpp"
#include "exchange/exchange.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

Server::Server(const std::vector<std::string> &allowed_tickers)
    : exchange(allowed_tickers) {}

void Server::start()
{
    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_PENDING_CONNECTIONS) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    // Start worker threads using (available cores - 2)
    int num_threads = std::thread::hardware_concurrency() - 2;
    for (int i = 0; i < num_threads; i++)
    {
        workers.emplace_back(&Server::worker_thread, this);
    }

    while (true)
    {
        int new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket < 0)
        {
            perror("Accept failed");
            continue;
        }

        // std::cout << "New client connected.\n";

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            client_queue.push(new_socket);
        }
    }

    close(server_fd);
}

void Server::worker_thread()
{
    while (true)
    {
        int client_socket;

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (client_queue.empty())
                continue;
            client_socket = client_queue.front();
            client_queue.pop();
        }

        handle_client(client_socket);
    }
}

void Server::handle_client(int client_socket)
{
    char buffer[2048] = {0}; // Increased buffer size for large responses

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0)
        {
            close(client_socket);
            return;
        }

        nlohmann::json response; // Declare outside try-catch

        try
        {
            std::string received_data(buffer);
            nlohmann::json request = nlohmann::json::parse(received_data);

            std::string action = request["action"];

            std::cout << "Handling request: " << action << '\n';

            if (action == "get_tickers")
            {
                response["tickers"] = exchange.GetTickers();
            }
            else if (action == "get_top_of_book")
            {
                std::string ticker = request["ticker"];
                TopOfBook top = exchange.GetTopOfBook(ticker);

                response["has_top"] = top.book_has_top;
                response["bid_price"] = top.bid_price;
                response["ask_price"] = top.ask_price;
                response["bid_volume"] = top.bid_volume;
                response["ask_volume"] = top.ask_volume;
                std::cout << "Ticker: " << ticker << "\n";
                std::cout << "BID: " << top.bid_price << " vol: " << top.bid_volume << "\n";
                std::cout << "ASK: " << top.ask_price << " vol: " << top.ask_volume << "\n";
            }
            else if (action == "get_volume")
            {
                std::string ticker = request["ticker"];
                double price = request["price"];
                int side = static_cast<int>(request["order_type"]);
                OrderType order_type;
                if (side == 1)
                {
                    order_type = OrderType::ASK;
                }
                else
                {
                    order_type = OrderType::BID;
                }
                int volume = exchange.GetVolume(ticker, price, order_type);
                response["volume"] = volume;
            }
            else if (action == "get_previous_trades")
            {
                std::string ticker = request["ticker"];
                int num_trades = request["num_previous_trades"];
                std::vector<Trade> trades = exchange.GetPreviousTrades(ticker, num_trades);

                response["trades"] = nlohmann::json::array();
                for (const auto &trade : trades)
                {
                    response["trades"].push_back({{"bid_user_id", trade.bid_user_id},
                                                  {"ask_user_id", trade.ask_user_id},
                                                  {"price", trade.price},
                                                  {"volume", trade.volume},
                                                  {"timestamp", trade.timestamp}});
                }
            }
            else if (action == "cancel_order")
            {
                std::string ticker = request["ticker"];
                int order_id = request["order_id"];
                bool success = exchange.CancelOrder(ticker, order_id);
                response["success"] = success;
            }
            else if (action == "handle_order")
            {
                std::string user_id = request["user_id"];
                OrderType order_type = static_cast<OrderType>(request["order_type"]);

                int volume = request["volume"];
                double price = request["price"];
                std::string ticker = request["ticker"];

                OrderResult result = exchange.HandleOrder(user_id, order_type, volume, price, ticker);

                response["order_added_to_book"] = result.order_added_to_book;
                response["order_id"] = result.order_id;
                response["trades_executed"] = result.trades_executed;
                response["trades"] = nlohmann::json::array();

                for (const auto &trade : result.trades)
                {
                    response["trades"].push_back({{"bid_user_id", trade.bid_user_id},
                                                  {"ask_user_id", trade.ask_user_id},
                                                  {"price", trade.price},
                                                  {"volume", trade.volume},
                                                  {"timestamp", trade.timestamp}});
                }

                std::cout << "ORDER ADDED TO BOOK" << result.order_added_to_book << "\n";
                std::cout << "From: " << user_id << "\n";
                if (order_type == OrderType::ASK)
                {
                    std::cout << "Side: " << "ASK" << "\n";
                }
                else
                {
                    std::cout << "Side: " << "BID" << "\n";
                }
            }
            else if (action == "get_trades_by_user")
            {
                std::string user_id = request["user_id"];
                std::vector<Trade> trades = exchange.GetTradesByUser(user_id);

                response["trades"] = nlohmann::json::array();
                for (const auto &trade : trades)
                {
                    response["trades"].push_back({{"bid_user_id", trade.bid_user_id},
                                                  {"ask_user_id", trade.ask_user_id},
                                                  {"price", trade.price},
                                                  {"volume", trade.volume},
                                                  {"timestamp", trade.timestamp}});
                }
            }
            else if (action == "register_user")
            {
                std::string user_id = request["user_id"];
                bool success = exchange.RegisterUser(user_id);
                response["success"] = success;
            }
            else
            {
                response["error"] = "Unknown action";
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error processing request: " << e.what() << std::endl;
            response["error"] = "Exception caught during processing";
        }

        // Send the response outside try/catch
        std::string response_str = response.dump();
        send(client_socket, response_str.c_str(), response_str.size(), 0);
    }
}
