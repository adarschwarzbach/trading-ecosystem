// project headers
#include "exchange/trade.hpp"
#include "utils/order_type.hpp"
#include "exchange/top_of_book.hpp"
#include "exchange/order_result.hpp"
#include "exchange/limit_order_book.hpp"
#include "exchange/exchange.hpp"

// std headers
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <ctime>

Exchange::Exchange(const std::vector<std::string> &allowed_tickers)
{
    for (const auto &tk : allowed_tickers)
    {
        tickers.insert(tk);
        limit_order_books.emplace(tk, LimitOrderBook(tk));
    }
}

inline void Exchange::ThrowIfTickerNotFound(std::string ticker)
{
    if (tickers.find(ticker) == tickers.end())
    {
        throw std::runtime_error("Ticker not found");
    }
}

std::unordered_set<std::string> Exchange::GetTickers()
{
    return tickers;
}

int Exchange::GetVolume(std::string ticker, double price, OrderType order_type)
{
    ThrowIfTickerNotFound(ticker);
    return limit_order_books.at(ticker).GetVolume(price, order_type);
}

TopOfBook Exchange::GetTopOfBook(std::string ticker)
{
    ThrowIfTickerNotFound(ticker);
    return limit_order_books.at(ticker).GetTopOfBook();
}

std::vector<Trade> Exchange::GetPreviousTrades(std::string ticker, int num_previous_trades)
{
    ThrowIfTickerNotFound(ticker);
    return limit_order_books.at(ticker).GetPreviousTrades(num_previous_trades);
}

bool Exchange::CancelOrder(std::string ticker, int order_id)
{
    ThrowIfTickerNotFound(ticker);
    return limit_order_books.at(ticker).CancelOrder(order_id);
}

OrderResult Exchange::HandleOrder(
    std::string user_id,
    OrderType order_type,
    int volume,
    double price,
    std::string ticker)
{
    // Make sure ticker is valid
    ThrowIfTickerNotFound(ticker);

    // If you want the order to use the passed-in timestamp, do so directly.
    // Or you can override with 'time(0)' if that is your desired design.
    // For demonstration, let's just use 'timestamp' as provided:
    OrderResult new_order = limit_order_books.at(ticker).HandleOrder(
        user_id,
        order_type,
        volume,
        price,
        time(0),
        ticker);

    // If trades occurred, record them under this user
    if (new_order.trades_executed)
    {
        for (const auto &tr : new_order.trades)
        {
            trades_by_user[tr.bid_user_id].push_back(tr);
            trades_by_user[tr.ask_user_id].push_back(tr);
        }
    }

    return new_order;
}

std::vector<Trade> Exchange::GetTradesByUser(std::string user_id)
{
    auto it = trades_by_user.find(user_id);
    if (it != trades_by_user.end())
    {
        return it->second;
    }
    return {};
}