#ifndef LIMIT_ORDER_BOOK
#define LIMIT_ORDER_BOOK
// project headers
#include "./order_node.hpp"
#include "./trade.hpp"
#include "../Utils/order_type.hpp"
#include "./price_level_queue.hpp"
// std headers
#include <string>
#include <variant>
#include <vector>
#include <ctime>
#include <unordered_map>
#include <queue>
#include <vector>
#include <functional>

// When true is returned, it means the order is NOT correct and swapping of elements takes place.
class AskComparator
{
public:
    bool operator()(PriceLevelQueue a, PriceLevelQueue b)
    {
        return a.GetPrice() > b.GetPrice();
    }
};

class BidComparator
{
public:
    bool operator()(PriceLevelQueue a, PriceLevelQueue b)
    {
        return a.GetPrice() < b.GetPrice();
    }
};

class LimitOrderBook
{
private:
    std::unordered_map<int, int> ask_volume_at_price;
    std::unordered_map<int, int> bid_volume_at_price;

    // PQ's of PLQ's
    std::priority_queue<PriceLevelQueue, std::vector<PriceLevelQueue>, AskComparator> ask_orders;
    std::priority_queue<PriceLevelQueue, std::vector<PriceLevelQueue>, BidComparator> bid_orders;

public:
    const std::string ticker;
    // Returns confirmation or vector of trades
    std::variant<bool, std::vector<Trade>> HandleOrder(
        std::string user_id,
        OrderType order_type,
        int volume,
        double price,
        time_t timestamp,
        std::string ticker);
};

#endif