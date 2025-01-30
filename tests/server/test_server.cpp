#include "server/server.hpp"
#include "exchange/exchange.hpp"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using ::testing::_;
using ::testing::Return;

// Mock class for Exchange
class MockExchange : public Exchange
{
public:
    MockExchange() : Exchange({}) {}

    MOCK_METHOD(std::unordered_set<std::string>, GetTickers, (), (override));
    MOCK_METHOD(TopOfBook, GetTopOfBook, (std::string ticker), (override));
    MOCK_METHOD(int, GetVolume, (std::string ticker, double price, OrderType order_type), (override));
    MOCK_METHOD(std::vector<Trade>, GetPreviousTrades, (std::string ticker, int num_previous_trades), (override));
    MOCK_METHOD(bool, CancelOrder, (std::string ticker, int order_id), (override));
    MOCK_METHOD(OrderResult, HandleOrder, (std::string user_id, OrderType order_type, int volume, double price, std::string ticker), (override));
    MOCK_METHOD(std::vector<Trade>, GetTradesByUser, (std::string user_id), (override));
    MOCK_METHOD(bool, RegisterUser, (std::string user_id), (override));
};

// **Test Case 1: Test GetTickers**
TEST(ServerTest, GetTickers)
{
    MockExchange mock_exchange;
    Server server({"AAPL", "GOOG", "TSLA"});

    // Simulate exchange returning tickers
    EXPECT_CALL(mock_exchange, GetTickers())
        .WillOnce(Return(std::unordered_set<std::string>{"AAPL", "GOOG"}));

    // Start the server in a background thread
    std::thread server_thread([&]()
                              { server.start(); });

    sleep(1); // Ensure the server has time to start

    // Connect to the server
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_NE(client_socket, -1);

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);

    ASSERT_EQ(connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)), 0);

    // Send request
    std::string request = R"({"action": "get_tickers"})";
    send(client_socket, request.c_str(), request.length(), 0);

    // Receive response
    char buffer[1024] = {0};
    recv(client_socket, buffer, sizeof(buffer), 0);

    // Parse response
    nlohmann::json response = nlohmann::json::parse(buffer);
    EXPECT_TRUE(response.contains("tickers"));
    std::unordered_set<std::string> expected_tickers = {"AAPL", "GOOG"};
    std::unordered_set<std::string> received_tickers = response["tickers"].get<std::unordered_set<std::string>>();
    EXPECT_EQ(received_tickers, expected_tickers);

    close(client_socket);
    server_thread.detach(); // Allow the server to keep running
}

// **Test Case 2: Test Order Handling**
TEST(ServerTest, HandleOrder)
{
    MockExchange mock_exchange;
    Server server({"AAPL"});

    // Construct a valid OrderResult
    OrderResult mock_result(
        false, // trades_executed
        {},    // trades
        true,  // order_added_to_book
        12345  // order_id
    );

    EXPECT_CALL(mock_exchange, HandleOrder("trader123", OrderType::BID, 10, 150.0, "AAPL"))
        .WillOnce(Return(mock_result));

    std::thread server_thread([&]()
                              { server.start(); });
    sleep(1);

    // Connect to the server
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_NE(client_socket, -1);

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);

    ASSERT_EQ(connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)), 0);

    // Send order request
    std::string request = R"({
        "action": "handle_order",
        "user_id": "trader123",
        "order_type": 0,  // 0 = BID (formerly BUY)
        "volume": 10,
        "price": 150.0,
        "ticker": "AAPL"
    })";

    send(client_socket, request.c_str(), request.length(), 0);

    // Receive response
    char buffer[1024] = {0};
    recv(client_socket, buffer, sizeof(buffer), 0);
    nlohmann::json response = nlohmann::json::parse(buffer);

    EXPECT_TRUE(response.contains("order_status"));
    EXPECT_TRUE(response["order_status"]);

    close(client_socket);
    server_thread.detach();
}
