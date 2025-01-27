#include "exchange/order_result.hpp"
#include "exchange/trade.hpp"
#include <variant>

OrderResult::OrderResult(
    bool trades_executed,
    std::vector<Trade> trades,
    bool order_added_to_book,
    int order_id)
    : trades_executed(trades_executed),
      trades(trades),
      order_added_to_book(order_added_to_book),
      order_id(order_id) {}
