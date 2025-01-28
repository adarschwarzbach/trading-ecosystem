// project headers
#include "exchange/limit_order_book.hpp"
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
#include <stdexcept>
#include <random>
#include <iostream> //For debugging

/**
 * Constructs a new LimitOrderBook for a given ticker symbol.
 *
 * @param ticker The ticker symbol for the order book (e.g., "AAPL").
 */
LimitOrderBook::LimitOrderBook(std::string ticker)
    : ticker(ticker),
      ask_order_pq([](const std::shared_ptr<PriceLevelQueue> &a, const std::shared_ptr<PriceLevelQueue> &b)
                   {
                       return a->GetPrice() > b->GetPrice(); // Min-heap for ask orders
                   }),
      bid_order_pq([](const std::shared_ptr<PriceLevelQueue> &a, const std::shared_ptr<PriceLevelQueue> &b)
                   {
                       return a->GetPrice() < b->GetPrice(); // Max-heap for bid orders
                   })
{
}

/**
 * Handles an incoming order, matching it against existing orders if possible
 * and adding the remaining volume to the order book if not fully matched.
 *
 * @param user_id The ID of the user submitting the order.
 * @param order_type The type of order (OrderType::ASK or OrderType::BID).
 * @param volume The number of shares in the order.
 * @param price The price at which the order is placed.
 * @param timestamp The timestamp of the order submission.
 * @param ticker The ticker symbol for the order.
 * @return An OrderResult containing the trade details and status of the order.
 */

OrderResult LimitOrderBook::HandleOrder(
    std::string user_id,
    OrderType order_type,
    int volume,
    double price,
    time_t timestamp,
    std::string ticker)
{
    if (ticker != GetTicker())
    {
        throw std::runtime_error("Order is not for the security that this book represents.");
    }
    if (price <= 0)
    {
        throw std::runtime_error("Price must be greater than zero");
    }
    if (volume <= 0)
    {
        throw std::runtime_error("Volume must be greater than zero");
    }

    // Cleanup empty price levels
    CleanupPriorityQueue(ask_order_pq);
    CleanupPriorityQueue(bid_order_pq);

    const OrderType opposite_side = (order_type == OrderType::ASK) ? OrderType::BID : OrderType::ASK;
    auto &opposite_pq = (opposite_side == OrderType::ASK) ? ask_order_pq : bid_order_pq;
    auto &opposite_volume_map = (opposite_side == OrderType::BID) ? bid_volume_at_price : ask_volume_at_price;

    std::vector<Trade> trades;

    // Process matching orders
    while (!opposite_pq.empty() && volume > 0)
    {
        auto best_opposite_queue = opposite_pq.top();
        if (!best_opposite_queue || !best_opposite_queue->HasOrders())
        {
            opposite_pq.pop();
            continue; // Skip invalid queues
        }

        PriceLevelQueue &opposite_best_price_queue = *best_opposite_queue;
        double best_opposite_price = opposite_best_price_queue.GetPrice();

        if (!PricesMatch(price, best_opposite_price, opposite_side))
        {
            break; // Stop if prices no longer match
        }

        OrderNode &current_opposite_order = opposite_best_price_queue.Peek();
        int vol_filled = std::min(volume, current_opposite_order.volume);

        // Adjust volumes
        current_opposite_order.volume -= vol_filled;
        volume -= vol_filled;

        // Log trade
        Trade trade = GenerateTrade(opposite_side, user_id, current_opposite_order.user_id, best_opposite_price, vol_filled);
        trades.push_back(trade);
        filled_trades.push_back(trade);

        // Remove fully matched orders
        if (current_opposite_order.volume == 0)
        {
            opposite_best_price_queue.Pop();
            order_node_map.erase(current_opposite_order.order_id);
        }

        opposite_volume_map[best_opposite_price] -= vol_filled;

        if (!opposite_best_price_queue.HasOrders())
        {
            opposite_pq.pop();
        }
    }

    int new_order_id = -1;
    if (volume > 0)
    {
        // Add remaining order to the book
        new_order_id = AddOrderToBook(user_id, order_type, volume, price, timestamp, ticker);
        auto &same_side_volume = (order_type == OrderType::BID) ? bid_volume_at_price : ask_volume_at_price;
        same_side_volume[price] += volume;
    }

    return OrderResult(!trades.empty(), trades, new_order_id > 0, new_order_id);
}

/**
 * Helper to determine if a trade will be executed
 *
 * @param aggressive_price the price of the order just submitted.
 * @param opposite_side_price the best price on the other side of the book.
 * @param opposite_side whether the other side of the book is BID or ASK.
 * @return True if the prices match within a tolerance, otherwise false.
 *
 * Inline
 */
inline bool LimitOrderBook::PricesMatch(double aggressive_price, double opposite_side_price, OrderType opposite_side)
{
    const double epsilon = 1e-6; // Tolerance for floating-point comparison

    double bid_price = (opposite_side == OrderType::BID)
                           ? opposite_side_price
                           : aggressive_price;
    double ask_price = (opposite_side == OrderType::BID)
                           ? aggressive_price
                           : opposite_side_price;

    return bid_price >= ask_price - epsilon;
}

/**
 * Generates a Trade object representing a successful match between two orders.
 *
 * @param opposite_side The type of the opposite side (OrderType::ASK or OrderType::BID).
 * @param user_id The ID of the user submitting the aggressive order.
 * @param opposite_user_id The ID of the user with the resting order.
 * @param price The price at which the trade was executed.
 * @param volume The number of shares traded.
 * @return A Trade object containing details of the executed trade.
 */

Trade LimitOrderBook::GenerateTrade(OrderType opposite_side, std::string user_id, std::string opposite_user_id, double price, int volume)
{
    time_t now = time(0);
    std::string bid_user_id;
    std::string ask_user_id;
    if (opposite_side == OrderType::ASK)
    {
        ask_user_id = opposite_user_id;
        bid_user_id = user_id;
    }
    else
    {
        bid_user_id = opposite_user_id;
        ask_user_id = user_id;
    }
    return Trade(
        GenerateId(),
        price,
        volume,
        now,
        bid_user_id,
        ask_user_id);
}

/**
 * Generates a unique ID for orders or trades.
 *
 * @return A unique integer ID.
 */

int LimitOrderBook::GenerateId()
{
    static std::atomic<int> id_counter(0);
    return ++id_counter;
}

/**
 * Adds a new order to the order book and creates a new PriceLevelQueue if
 * one does not already exist for the specified price.
 *
 * @param user_id The ID of the user submitting the order.
 * @param order_type The type of order (OrderType::ASK or OrderType::BID).
 * @param volume The number of shares in the order.
 * @param price The price at which the order is placed.
 * @param timestamp The timestamp of the order submission.
 * @param ticker The ticker symbol for the order.
 * @return The unique ID of the newly added order.
 */

int LimitOrderBook::AddOrderToBook(std::string user_id,
                                   OrderType order_type,
                                   int volume,
                                   double price,
                                   time_t timestamp,
                                   std::string ticker)
{
    int order_id = GenerateId();

    // Create a local OrderNode, but only use it to initialize the map.
    // The map will contain the *official* (long-lived) copy.
    OrderNode tmp_order(
        order_id,
        user_id,
        volume,
        price,
        order_type,
        timestamp,
        ticker,
        nullptr,
        nullptr);

    // Emplace it into the map (by value), then get a reference to
    // the newly stored node inside the map
    auto [map_it, inserted] = order_node_map.emplace(order_id, tmp_order);
    OrderNode &stored_node = map_it->second;

    // NOW pass that reference to PriceLevelQueue
    auto &side_price_level_queues =
        (order_type == OrderType::ASK) ? ask_order_queues : bid_order_queues;
    auto &side_pq =
        (order_type == OrderType::ASK) ? ask_order_pq : bid_order_pq;

    // If no PriceLevelQueue exists, create a new one
    auto it = side_price_level_queues.find(price);
    if (it == side_price_level_queues.end())
    {
        auto new_price_level_queue = std::make_shared<PriceLevelQueue>(price);
        side_price_level_queues[price] = new_price_level_queue;
        side_pq.push(new_price_level_queue);
    }

    // Add the *map's* node, not the temporary
    side_price_level_queues[price]->AddOrder(stored_node);

    return order_id;
}

const std::string &LimitOrderBook::GetTicker() const
{
    return ticker;
}

/**
 * Retrieves the total volume available at a specific price and order type.
 *
 * @param price The price level to query.
 * @param order_type The type of order (OrderType::ASK or OrderType::BID).
 * @return The total volume available at the specified price level.
 */

int LimitOrderBook::GetVolume(double price, OrderType order_type)
{
    const auto &volume_map = (order_type == OrderType::ASK) ? ask_volume_at_price : bid_volume_at_price;
    auto it = volume_map.find(price);
    if (it != volume_map.end())
    {
        return it->second;
    }
    return 0; // Default volume if price not found
}

/**
 * Cancels an order in the order book by its unique ID.
 *
 * ToDo: Clean up allocated memory for ordernode
 *
 * @param order_id The unique ID of the order to cancel.
 * @return True if the order was successfully canceled, otherwise false.
 * @throws std::out_of_range if the order ID is not found.
 * @throws std::runtime_error if the associated PriceLevelQueue is invalid.
 */

bool LimitOrderBook::CancelOrder(int order_id)
{
    // Check if the order exists
    auto order_it = order_node_map.find(order_id); // Rename to `order_it` to avoid redeclaration conflicts
    if (order_it == order_node_map.end())
    {
        throw std::out_of_range("Order: " + std::to_string(order_id) + " not found");
    }

    // Reference the order to cancel
    OrderNode &order_to_cancel = order_it->second;

    // Reference the appropriate price level queue
    auto &given_side_price_level_queues = (order_to_cancel.order_type == OrderType::ASK)
                                              ? ask_order_queues
                                              : bid_order_queues;

    auto queue_it = given_side_price_level_queues.find(order_to_cancel.price); // Rename to `queue_it`
    if (queue_it == given_side_price_level_queues.end() || !queue_it->second)
    {
        throw std::runtime_error("PriceLevelQueue not found or null for price: " + std::to_string(order_to_cancel.price));
    }

    PriceLevelQueue &price_level = *(queue_it->second);

    // Decrease volume
    std::unordered_map<int, int> &same_side_volume = (order_to_cancel.order_type == OrderType::BID)
                                                         ? bid_volume_at_price
                                                         : ask_volume_at_price;
    same_side_volume[order_to_cancel.price] -= order_to_cancel.volume;

    // Remove the order from the price level queue
    price_level.RemoveOrder(order_to_cancel);

    // Erase the order from the map
    order_node_map.erase(order_it); // Use the correctly scoped `order_it`

    return true;
}

/**
 * Retrieves top of LOB
 *
 * @return A TopOfBook object containing the best bid, best ask, and their volumes.
 */

TopOfBook LimitOrderBook::GetTopOfBook()
{

    double best_ask_price = 0.0;
    int best_ask_volume = 0;
    bool has_ask = false;

    while (!ask_order_pq.empty())
    {
        auto ask_top = ask_order_pq.top();
        // If top pointer is invalid or empty, pop and continue
        if (!ask_top || !ask_top->HasOrders())
        {
            ask_order_pq.pop();
            continue;
        }
        double price = ask_top->GetPrice();
        int vol = ask_volume_at_price[price];
        if (vol > 0)
        {
            best_ask_price = price;
            best_ask_volume = vol;
            has_ask = true;
        }
        // If volume is zero, also pop. Otherwise we found our best ask
        if (vol == 0)
        {
            ask_order_pq.pop();
            continue;
        }
        break;
    }

    double best_bid_price = 0.0;
    int best_bid_volume = 0;
    bool has_bid = false;

    while (!bid_order_pq.empty())
    {
        auto bid_top = bid_order_pq.top();
        if (!bid_top || !bid_top->HasOrders())
        {
            bid_order_pq.pop();
            continue;
        }
        double price = bid_top->GetPrice();
        int vol = bid_volume_at_price[price];
        if (vol > 0)
        {
            best_bid_price = price;
            best_bid_volume = vol;
            has_bid = true;
        }
        if (vol == 0)
        {
            bid_order_pq.pop();
            continue;
        }
        break;
    }

    // If neither ask nor bid exists, no top
    if (!has_ask && !has_bid)
    {
        return TopOfBook(false, 0.0, 0, 0.0, 0);
    }

    // Otherwise, we have at least one side
    //    If no ask, we keep ask_price=0, ask_volume=0
    //    If no bid, we keep bid_price=0, bid_volume=0
    return TopOfBook(
        true,
        best_ask_price,
        best_ask_volume,
        best_bid_price,
        best_bid_volume);
}

std::vector<Trade> LimitOrderBook::GetPreviousTrades(int num_previous_trades)
{
    if (num_previous_trades <= 0)
    {
        return {};
    }

    // Return up to `num_previous_trades` trades, starting from the most recent
    int start_idx = std::max(0, static_cast<int>(filled_trades.size()) - num_previous_trades);
    return std::vector<Trade>(filled_trades.begin() + start_idx, filled_trades.end());
}
