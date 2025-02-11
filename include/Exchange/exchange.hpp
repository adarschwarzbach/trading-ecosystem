#ifndef EXCHANGE
#define EXCHANGE
// project headers
#include "exchange/trade.hpp"
#include "utils/order_type.hpp"
#include "exchange/top_of_book.hpp"
#include "exchange/order_result.hpp"
#include "exchange/limit_order_book.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <ctime>

class Exchange
{
private:
    std::unordered_map<std::string, LimitOrderBook> limit_order_books;
    std::unordered_set<std::string> tickers;
    std::unordered_map<std::string, std::vector<Trade>> trades_by_user;
    std::unordered_set<std::string> users;
    inline void ThrowIfTickerNotFound(std::string ticker);

public:
    Exchange(const std::vector<std::string> &allowed_tickers);
    std::unordered_set<std::string> GetTickers();
    int GetVolume(std::string ticker, double price, OrderType order_type);
    TopOfBook GetTopOfBook(std::string ticker);
    std::vector<Trade> GetPreviousTrades(std::string ticker, int num_previous_trades);
    bool CancelOrder(std::string ticker, int order_id);
    OrderResult HandleOrder(
        std::string user_id,
        OrderType order_type,
        int volume,
        double price,
        std::string ticker);
    std::vector<Trade> GetTradesByUser(std::string user_id);
    bool RegisterUser(std::string user_id);
};

#endif