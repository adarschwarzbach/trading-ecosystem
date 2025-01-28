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
    // Remove empty price levels
    // ToDo: write my own PQ to arbitrarily remove empty PLQ's
    CleanupPriorityQueue(ask_order_pq);
    CleanupPriorityQueue(bid_order_pq);

    const OrderType opposite_side = (order_type == OrderType::ASK)
                                        ? OrderType::BID
                                        : OrderType::ASK;

    auto &opposite_pq = (opposite_side == OrderType::ASK) ? ask_order_pq : bid_order_pq;

    std::unordered_map<int, int> &opposite_side_volume = (opposite_side == OrderType::BID)
                                                             ? bid_volume_at_price
                                                             : ask_volume_at_price;

    std::vector<Trade> trades; // Vector of trades exectued from order

    while (!opposite_pq.empty()) // If there are prices on the other side of the book
    {
        // Get the best PLQ for the opposite side
        if (opposite_pq.empty() || !opposite_pq.top())
        {
            throw std::runtime_error("Priority queue is empty or contains a null PriceLevelQueue");
        }
        const int best_opposite_price = opposite_pq.top()->GetPrice();
        PriceLevelQueue &opposite_best_price_queue = *((opposite_side == OrderType::ASK)
                                                           ? ask_order_queues[best_opposite_price]
                                                           : bid_order_queues[best_opposite_price]);

        while (
            opposite_best_price_queue.HasOrders() &&                                 // ensure price level is still valid
            volume > 0 &&                                                            // ensure that agressive order has not been fully satisfied
            PricesMatch(price, opposite_best_price_queue.GetPrice(), opposite_side)) // Ensure prices match
        {

            OrderNode &current_opposite_order = opposite_best_price_queue.Peek();
            int vol_filled = 0;

            if (current_opposite_order.volume > volume) // Simply edit opposite order
            {
                // Edit order
                current_opposite_order.volume = current_opposite_order.volume - volume;
                vol_filled = volume; //  Entirely filled the agressive volume
            }
            else // remove opposite order and fill entirely
            {
                OrderNode &popped_opposite_order = opposite_best_price_queue.Pop(); // Remove order - TODO free node memory
                volume -= popped_opposite_order.volume;                             // Decrease agressive order vol
                // Remove order
                order_node_map.erase(popped_opposite_order.order_id); // eventually gotta clean up memeory

                vol_filled = popped_opposite_order.volume; // Entirely filled opposite order volume
            }

            // Log trade
            Trade new_trade = GenerateTrade(
                opposite_side,
                user_id,
                current_opposite_order.user_id,
                price,
                vol_filled);

            trades.push_back(new_trade);
            filled_trades.push_back(new_trade);

            // Update volume
            opposite_side_volume[price] -= vol_filled;
        }

        if (!opposite_best_price_queue.HasOrders())
        {
            opposite_pq.pop();
        }
    }

    int new_order_id = -1;

    if (volume > 0) // Add order to book if volume remains
    {
        new_order_id = AddOrderToBook(user_id, order_type, volume, price, timestamp, ticker);

        std::unordered_map<int, int> &same_side_volume = (order_type == OrderType::BID)
                                                             ? bid_volume_at_price
                                                             : ask_volume_at_price;

        same_side_volume[price] += volume;
    }

    return OrderResult(
        trades.size() > 0,
        trades,
        new_order_id > 0,
        new_order_id);
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

    // Create a new order node
    OrderNode new_order(
        order_id,
        user_id,
        volume,
        price,
        order_type,
        timestamp,
        ticker,
        nullptr,
        nullptr);

    // Add the order node to the map
    order_node_map.emplace(order_id, new_order);

    // Select the appropriate side's price level queues
    auto &given_side_price_level_queues = (order_type == OrderType::ASK)
                                              ? ask_order_queues
                                              : bid_order_queues;

    auto &given_side_pq = (order_type == OrderType::ASK)
                              ? ask_order_pq
                              : bid_order_pq;

    // Check if a `PriceLevelQueue` already exists for this price
    auto it = given_side_price_level_queues.find(price);
    if (it == given_side_price_level_queues.end()) // No PLQ exists for this price
    {
        // Create a new `PriceLevelQueue` and store it as a shared pointer
        auto new_price_level_queue = std::make_shared<PriceLevelQueue>(price);

        // Add it to both the map and the priority queue
        given_side_price_level_queues[price] = new_price_level_queue;
        given_side_pq.push(new_price_level_queue);
    }

    // Add the new order to the appropriate `PriceLevelQueue`
    given_side_price_level_queues[price]->AddOrder(new_order);

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

int LimitOrderBook::GetVolume(int price, OrderType order_type)
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
    if (ask_order_pq.empty() || !ask_order_pq.top() ||
        bid_order_pq.empty() || !bid_order_pq.top() ||
        !ask_order_pq.top()->HasOrders() || !bid_order_pq.top()->HasOrders())
    {
        return TopOfBook(false, 0.0, 0, 0.0, 0);
    }

    int best_ask_price = ask_order_pq.top()->GetPrice();
    int best_bid_price = bid_order_pq.top()->GetPrice();

    return TopOfBook(
        true,
        best_ask_price,
        ask_volume_at_price[best_ask_price],
        best_bid_price,
        bid_volume_at_price[best_bid_price]);
}

#include "exchange/limit_order_book.hpp"

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
