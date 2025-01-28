#ifndef LIMIT_ORDER_BOOK
#define LIMIT_ORDER_BOOK
// project headers
#include "exchange/order_node.hpp"
#include "exchange/trade.hpp"
#include "utils/order_type.hpp"
#include "exchange/price_level_queue.hpp"
#include "exchange/top_of_book.hpp"
#include "exchange/order_result.hpp"

// std headers
#include <string>
#include <variant>
#include <ctime>
#include <unordered_map>
#include <queue>
#include <vector>

class LimitOrderBook
{
private:
    const std::string ticker;
    // volume
    std::unordered_map<int, int> ask_volume_at_price;
    std::unordered_map<int, int> bid_volume_at_price;

    // price level
    std::unordered_map<double, std::shared_ptr<PriceLevelQueue>> ask_order_queues;
    std::unordered_map<double, std::shared_ptr<PriceLevelQueue>> bid_order_queues;

    // PQ's of PLQ's
    std::priority_queue<
        std::shared_ptr<PriceLevelQueue>,
        std::vector<std::shared_ptr<PriceLevelQueue>>,
        std::function<bool(const std::shared_ptr<PriceLevelQueue> &, const std::shared_ptr<PriceLevelQueue> &)>>
        ask_order_pq;

    std::priority_queue<
        std::shared_ptr<PriceLevelQueue>,
        std::vector<std::shared_ptr<PriceLevelQueue>>,
        std::function<bool(const std::shared_ptr<PriceLevelQueue> &, const std::shared_ptr<PriceLevelQueue> &)>>
        bid_order_pq;

    // Orders
    std::unordered_map<int, OrderNode> order_node_map;

    // Previous filled trades
    std::vector<Trade> filled_trades;

    // Helper to add order to book
    int AddOrderToBook(std::string user_id,
                       OrderType order_type,
                       int volume,
                       double price,
                       time_t timestamp,
                       std::string ticker);

    // std::variant<void, Trade> HandleOrderMatching();

    inline bool PricesMatch(double aggressive_price,
                            double opposite_side_price,
                            OrderType opposite_side);

    Trade GenerateTrade(OrderType opposite_side,
                        std::string user_id,
                        std::string opposite_user_id,
                        double price,
                        int volume);

public:
    LimitOrderBook(std::string ticker);
    int GenerateId();

    // Returns confirmation or vector of trades
    OrderResult HandleOrder(
        std::string user_id,
        OrderType order_type,
        int volume,
        double price,
        time_t timestamp,
        std::string ticker);

    const std::string &GetTicker() const;
    int GetVolume(double price, OrderType order_type);
    bool CancelOrder(int order_id);
    TopOfBook GetTopOfBook();
    std::vector<Trade> GetPreviousTrades(int num_previous_trades);
};

template <typename Comparator>
void CleanupPriorityQueue(std::priority_queue<std::shared_ptr<PriceLevelQueue>,
                                              std::vector<std::shared_ptr<PriceLevelQueue>>,
                                              Comparator> &pq)
{
    while (!pq.empty() && pq.top() && !pq.top()->HasOrders())
    {
        pq.pop();
    }
}

#endif