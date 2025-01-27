#ifndef ORDER_RESULT
#define ORDER_RESULT

#include <string>
#include <vector>
#include <variant>
#include "exchange/trade.hpp"

/**
 * @brief Struct to represent the results of an order
 */
struct OrderResult
{
    const bool trades_executed;
    const std::vector<Trade> trades;
    const bool order_added_to_book;
    const int order_id;

    OrderResult(
        bool trades_executed,
        std::vector<Trade> trades,
        bool order_added_to_book,
        int order_id);
};

#endif // ORDER_RESULT
