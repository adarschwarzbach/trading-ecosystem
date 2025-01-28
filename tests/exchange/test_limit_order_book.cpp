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
    // Test initialization with a valid ticker
    LimitOrderBook lob("AAPL");

    // Verify that the ticker is initialized correctly
    ASSERT_EQ(lob.GetTicker(), "AAPL") << "Ticker should be initialized to AAPL";

    // Verify that the priority queues for orders are empty
    EXPECT_TRUE(lob.GetTopOfBook().book_has_top == false) << "Top of book should not be valid on initialization";
    EXPECT_NO_THROW({
        auto top_of_book = lob.GetTopOfBook();
        EXPECT_EQ(top_of_book.ask_price, 0.0) << "Best ask price should be initialized to 0.0";
        EXPECT_EQ(top_of_book.bid_price, 0.0) << "Best bid price should be initialized to 0.0";
        EXPECT_EQ(top_of_book.ask_volume, 0) << "Best ask volume should be initialized to 0";
        EXPECT_EQ(top_of_book.bid_volume, 0) << "Best bid volume should be initialized to 0";
    });

    // Verify that internal data structures are empty
    EXPECT_EQ(lob.GetVolume(100, OrderType::ASK), 0) << "ASK volume should be 0 at price 100";
    EXPECT_EQ(lob.GetVolume(100, OrderType::BID), 0) << "BID volume should be 0 at price 100";

    // Ensure cancellation of a non-existent order throws an exception
    EXPECT_THROW(lob.CancelOrder(1), std::out_of_range) << "Cancelling a non-existent order should throw an exception";

    // Ensure the previous trades vector is empty
    EXPECT_TRUE(lob.GetPreviousTrades(5).empty()) << "Previous trades should be empty on initialization";
}
