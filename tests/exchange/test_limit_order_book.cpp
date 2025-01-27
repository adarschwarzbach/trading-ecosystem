#include "exchange/order_node.hpp"
#include "exchange/trade.hpp"
#include "utils/order_type.hpp"
#include "exchange/price_level_queue.hpp"
#include "exchange/top_of_book.hpp"
#include "exchange/order_result.hpp"
#include "exchange/limit_order_book.hpp"

#include <gtest/gtest.h>
#include <string>
#include <variant>
#include <ctime>
#include <unordered_map>
#include <queue>
#include <vector>
#include <stdexcept> // For std::runtime_error
#include <iostream>  // For logging

TEST(LimitOrderBookTest, Initialization)
{
    LimitOrderBook lob("AAPL");
}