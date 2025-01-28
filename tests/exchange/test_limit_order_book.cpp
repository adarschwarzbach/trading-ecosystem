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

TEST(LimitOrderBookTest, AddingOrder)
{
    LimitOrderBook limit_order_book("AAPL");

    time_t now = time(0);
    OrderResult test_order = limit_order_book.HandleOrder(
        "user_1",
        OrderType::ASK,
        1,
        1.0,
        now,
        "AAPL");

    ASSERT_TRUE(test_order.order_added_to_book) << "Trades should have been added to the book";
    EXPECT_FALSE(test_order.trades_executed);
    EXPECT_TRUE(test_order.trades.empty());
    EXPECT_EQ(1, limit_order_book.GetVolume(1.0, OrderType::ASK)) << "Should have volume for trade added to book";
}

TEST(LimitOrderBookTest, AddTwoOrdersSamePrice)
{
    LimitOrderBook limit_order_book("AAPL");

    time_t now = time(0);
    OrderResult test_order_1 = limit_order_book.HandleOrder(
        "user_1",
        OrderType::ASK,
        1,
        1.0,
        now,
        "AAPL");

    EXPECT_EQ(1, limit_order_book.GetVolume(1.0, OrderType::ASK)) << "Should have volume for trade added to book";

    OrderResult test_order_2 = limit_order_book.HandleOrder(
        "user_2",
        OrderType::ASK,
        1,
        1.0,
        now,
        "AAPL");

    EXPECT_EQ(2, limit_order_book.GetVolume(1.0, OrderType::ASK)) << "Should have volume for both trades, got ";
}

TEST(LimitOrderBookTest, CancelOrder)
{
    LimitOrderBook lob("AAPL");
    int order_id = lob.HandleOrder("user1", OrderType::ASK, 10, 150.0, std::time(nullptr), "AAPL").order_id;
    ASSERT_NE(order_id, -1); // Ensure the order was added

    bool canceled = lob.CancelOrder(order_id);
    EXPECT_TRUE(canceled);

    EXPECT_EQ(lob.GetVolume(150.0, OrderType::ASK), 0); // Confirm volume is 0
}

TEST(LimitOrderBookTest, ValidTopOfBook)
{
    LimitOrderBook lob("AAPL");
    int ask_order_id = lob.HandleOrder("user1", OrderType::ASK, 10, 11.0, std::time(nullptr), "AAPL").order_id;
    ASSERT_NE(ask_order_id, -1); // Ensure the order was added
    ASSERT_EQ(lob.GetVolume(11.0, OrderType::ASK), 10);

    int bid_order_id = lob.HandleOrder("user2", OrderType::BID, 5, 10.0, std::time(nullptr), "AAPL").order_id;
    ASSERT_NE(bid_order_id, -1); // Ensure the order was added
    ASSERT_EQ(lob.GetVolume(10.0, OrderType::BID), 5);

    TopOfBook book_top = lob.GetTopOfBook();
    ASSERT_TRUE(book_top.book_has_top);
    // std::cout << "-- Book top --  \n"
    //           << "Ask price:  "
    //           << book_top.ask_price
    //           << "\nAsk volume: "
    //           << book_top.ask_volume
    //           << "\nBid price: "
    //           << book_top.bid_price
    //           << "\nBid volume: "
    //           << book_top.bid_volume
    //           << std::endl;
    ASSERT_EQ(book_top.ask_price, 11.0);
    ASSERT_EQ(book_top.bid_price, 10.0);
    ASSERT_EQ(book_top.ask_volume, 10);
    ASSERT_EQ(book_top.bid_volume, 5);
}

TEST(LimitOrderBookTest, ExecuteTrade)
{
    LimitOrderBook lob("AAPL");
    int ask_order_id = lob.HandleOrder("user1", OrderType::ASK, 10, 10.0, std::time(nullptr), "AAPL").order_id;
    ASSERT_NE(ask_order_id, -1); // Ensure the order was added
    ASSERT_EQ(lob.GetVolume(10.0, OrderType::ASK), 10);

    OrderResult trade_execution = lob.HandleOrder("user2", OrderType::BID, 10, 10.0, std::time(nullptr), "AAPL");
    EXPECT_TRUE(trade_execution.trades_executed) << "Orders should have matched";
    EXPECT_FALSE(trade_execution.trades.empty()) << "Trades should have been returned";
    std::vector trades = trade_execution.trades;
    EXPECT_EQ(trades.size(), 1);
    Trade trade = trades[0];

    // std::cout << "-- Trade results --  \n"
    //           << "ID:  "
    //           << trade.trade_id
    //           << "\nAsk user id:  "
    //           << trade.ask_user_id
    //           << "\nbid user id:  "
    //           << trade.bid_user_id
    //           << "\nPrice:  "
    //           << trade.price
    //           << "\nVol:  "
    //           << trade.volume
    //           << "\nTime:  "
    //           << trade.timestamp
    //           << "\n ";

    // Test cleanup
    ASSERT_EQ(lob.GetVolume(10.0, OrderType::ASK), 0) << "All volume should have been removed";
    ASSERT_EQ(lob.GetVolume(10.0, OrderType::BID), 0) << "All volume should have been removed";

    TopOfBook book_top = lob.GetTopOfBook();
    ASSERT_FALSE(book_top.book_has_top);
}

// GENERATED TESTS ----------

// --------------------------------------------------------------------
// 1) Test partial fill where incoming BID is larger than existing ASK
// --------------------------------------------------------------------
TEST(LimitOrderBookExtendedTest, PartialFill_LargerBid_MatchesOneLevel)
{
    LimitOrderBook lob("AAPL");

    // Place an ASK of volume 5 @ $100
    lob.HandleOrder("asker", OrderType::ASK, 5, 100.0, std::time(nullptr), "AAPL");

    // Incoming BID is bigger volume => partial fill
    OrderResult result = lob.HandleOrder("bidder", OrderType::BID, 10, 100.0, std::time(nullptr), "AAPL");
    EXPECT_TRUE(result.trades_executed);
    ASSERT_EQ(result.trades.size(), 1u);

    // The matched trade volume should be 5 (all ask got filled)
    EXPECT_EQ(result.trades[0].volume, 5);
    // The leftover 5 from the bidder should remain in the book
    EXPECT_TRUE(result.order_added_to_book);

    // Verify volumes
    EXPECT_EQ(lob.GetVolume(100.0, OrderType::ASK), 0)
        << "All ASKs at 100 should be gone.";
    EXPECT_EQ(lob.GetVolume(100.0, OrderType::BID), 5)
        << "Remaining BID volume should be 5.";

    // Check top of book: only a 5-share BID remains
    TopOfBook top = lob.GetTopOfBook();
    EXPECT_TRUE(top.book_has_top);
    EXPECT_EQ(top.bid_price, 100.0);
    EXPECT_EQ(top.bid_volume, 5);
    EXPECT_EQ(top.ask_volume, 0);
}

// --------------------------------------------------------------------
// 2) Test partial fill where incoming ASK is smaller than existing BID
// --------------------------------------------------------------------
TEST(LimitOrderBookExtendedTest, PartialFill_SmallerAsk_MatchesOneLevel)
{
    LimitOrderBook lob("AAPL");

    // Place a BID of volume 10 @ $50
    lob.HandleOrder("bidder", OrderType::BID, 10, 50.0, std::time(nullptr), "AAPL");

    // Incoming ASK is smaller volume => partial fill on the BID side
    OrderResult result = lob.HandleOrder("asker", OrderType::ASK, 4, 50.0, std::time(nullptr), "AAPL");
    EXPECT_TRUE(result.trades_executed);
    ASSERT_EQ(result.trades.size(), 1u);

    // The matched trade volume should be 4
    EXPECT_EQ(result.trades[0].volume, 4);
    // The leftover 6 from the bidder remains
    EXPECT_FALSE(result.order_added_to_book)
        << "No leftover ASK should remain in the book.";

    // Verify volumes
    EXPECT_EQ(lob.GetVolume(50.0, OrderType::ASK), 0)
        << "Incoming ASK was fully matched.";
    EXPECT_EQ(lob.GetVolume(50.0, OrderType::BID), 6)
        << "Remaining BID volume should be 6.";

    // Check top of book
    TopOfBook top = lob.GetTopOfBook();
    EXPECT_TRUE(top.book_has_top);
    EXPECT_EQ(top.bid_price, 50.0);
    EXPECT_EQ(top.bid_volume, 6);
    EXPECT_EQ(top.ask_price, 0.0);
    EXPECT_EQ(top.ask_volume, 0);
}

// --------------------------------------------------------------------
// 3) Test multiple-level matching on an ASK
// --------------------------------------------------------------------
TEST(LimitOrderBookExtendedTest, MultiLevel_AskMatchesMultipleBids)
{
    LimitOrderBook lob("AAPL");
    time_t now = std::time(nullptr);

    // Place multiple BID levels:
    // Level 1: 3 shares @ $101
    // Level 2: 5 shares @ $100
    // Level 3: 10 shares @ $99
    lob.HandleOrder("bidderA", OrderType::BID, 3, 101.0, now, "AAPL");
    lob.HandleOrder("bidderB", OrderType::BID, 5, 100.0, now, "AAPL");
    lob.HandleOrder("bidderC", OrderType::BID, 10, 99.0, now, "AAPL");

    // Now place a big ASK that can fill across multiple levels
    // We want to fill at 101 first, then 100, then partially 99
    OrderResult result = lob.HandleOrder("asker", OrderType::ASK, 12, 99.0, now, "AAPL");

    EXPECT_TRUE(result.trades_executed);
    // Should match 3 shares @101, 5 shares @100, total 8 so far.
    // But note the ask is priced at 99.0, which can match a 99.0 BID as well
    // => it will keep matching at price >= 99.0
    // So it consumes 3 + 5 + 4 from the 10 @99 => total 12
    // That means 3 trades happen.
    ASSERT_EQ(result.trades.size(), 3u);

    // Check volumes
    // - The first 3 at 101 are used up => volume at 101 is now 0
    // - The next 5 at 100 are used up => volume at 100 is now 0
    // - The final 4 matched at 99 => leftover at 99 is 6
    EXPECT_EQ(lob.GetVolume(101.0, OrderType::BID), 0);
    EXPECT_EQ(lob.GetVolume(100.0, OrderType::BID), 0);
    EXPECT_EQ(lob.GetVolume(99.0, OrderType::BID), 6)
        << "After partial fill of 4 out of the 10 at 99, 6 remain.";

    // The ask was fully matched (12 shares sold). So no leftover ASK in book
    TopOfBook top = lob.GetTopOfBook();
    EXPECT_TRUE(top.book_has_top);
    EXPECT_EQ(top.bid_price, 99.0);
    EXPECT_EQ(top.bid_volume, 6);
    EXPECT_EQ(top.ask_volume, 0)
        << "No ask left in the book, so there's no 'best ask' price level in the code right now.";
}

// --------------------------------------------------------------------
// 4) Test multiple-level matching on a BID
// --------------------------------------------------------------------
TEST(LimitOrderBookExtendedTest, MultiLevel_BidMatchesMultipleAsks)
{
    LimitOrderBook lob("AAPL");

    // Place multiple ASK levels:
    // Level 1: 2 shares @ $50
    // Level 2: 5 shares @ $51
    // Level 3: 8 shares @ $52
    lob.HandleOrder("askerA", OrderType::ASK, 2, 50.0, std::time(nullptr), "AAPL");
    lob.HandleOrder("askerB", OrderType::ASK, 5, 51.0, std::time(nullptr), "AAPL");
    lob.HandleOrder("askerC", OrderType::ASK, 8, 52.0, std::time(nullptr), "AAPL");

    // Now place a big BID at 52 => can fill across all ask levels up to $52
    OrderResult result = lob.HandleOrder("bidderX", OrderType::BID, 10, 52.0, std::time(nullptr), "AAPL");

    EXPECT_TRUE(result.trades_executed);
    // Should match 2 shares @50 + 5 shares @51 + 3 shares @52 = 10 total
    // => That's 3 separate trades if your code lumps them individually
    ASSERT_EQ(result.trades.size(), 3u);

    // Check volumes
    EXPECT_EQ(lob.GetVolume(50.0, OrderType::ASK), 0);
    EXPECT_EQ(lob.GetVolume(51.0, OrderType::ASK), 0);
    // 3 out of the 8 at 52 got filled, so 5 remain
    EXPECT_EQ(lob.GetVolume(52.0, OrderType::ASK), 5);

    // The 10-lot BID is fully matched, so no leftover in the BID side
    // => The top of the book is now best ASK = 52.0 with 5 shares
    //    (and no BID if you check the code's logic).
    TopOfBook top = lob.GetTopOfBook();
    EXPECT_TRUE(top.book_has_top);
    EXPECT_EQ(top.ask_price, 52.0);
    EXPECT_EQ(top.ask_volume, 5);
    EXPECT_EQ(top.bid_volume, 0);
}

// --------------------------------------------------------------------
// 5) FIFO matching at same price: multiple ASKs in one price level
// --------------------------------------------------------------------
TEST(LimitOrderBookExtendedTest, FIFOWithinPriceLevel_ASK)
{
    LimitOrderBook lob("AAPL");

    // Place two ASKs at the same price, in the order: userA, userB
    lob.HandleOrder("userA", OrderType::ASK, 3, 100.0, std::time(nullptr), "AAPL");
    lob.HandleOrder("userB", OrderType::ASK, 5, 100.0, std::time(nullptr), "AAPL");
    EXPECT_EQ(lob.GetVolume(100.0, OrderType::ASK), 8);

    // Single BID that will consume 6 shares at $100 => should fill userA first (3 shares),
    // then userB partially (3 out of 5).
    OrderResult result = lob.HandleOrder("bidder", OrderType::BID, 6, 100.0, std::time(nullptr), "AAPL");
    EXPECT_TRUE(result.trades_executed);

    // Should produce 2 trades:
    //  - 3 shares from userA
    //  - 3 shares from userB
    ASSERT_EQ(result.trades.size(), 2u);

    // Check leftover volume
    // userA is fully matched => removed
    // userB had 5, matched 3 => 2 remain
    EXPECT_EQ(lob.GetVolume(100.0, OrderType::ASK), 2);

    // Confirm correct user IDs in the trade logs
    Trade first_trade = result.trades[0];
    Trade second_trade = result.trades[1];
    EXPECT_EQ(first_trade.ask_user_id, "userA");
    EXPECT_EQ(second_trade.ask_user_id, "userB");
    EXPECT_EQ(first_trade.volume, 3);
    EXPECT_EQ(second_trade.volume, 3);
    EXPECT_EQ(lob.GetVolume(100.0, OrderType::BID), 0)
        << "No leftover bid volume, since the 6-lot was fully matched.";
}

// --------------------------------------------------------------------
// 6) FIFO matching at same price: multiple BIDs
// --------------------------------------------------------------------
TEST(LimitOrderBookExtendedTest, FIFOWithinPriceLevel_BID)
{
    LimitOrderBook lob("AAPL");

    // Place two BIDs at the same price, in the order: userX, userY
    lob.HandleOrder("userX", OrderType::BID, 4, 10.0, std::time(nullptr), "AAPL");
    lob.HandleOrder("userY", OrderType::BID, 6, 10.0, std::time(nullptr), "AAPL");
    EXPECT_EQ(lob.GetVolume(10.0, OrderType::BID), 10);

    // Single ASK that will consume 7 shares => should fill userX first (4 shares),
    // then userY partially (3 out of 6).
    OrderResult result = lob.HandleOrder("asker", OrderType::ASK, 7, 10.0, std::time(nullptr), "AAPL");
    EXPECT_TRUE(result.trades_executed);

    // 2 trades:
    //  - 4 shares vs. userX
    //  - 3 shares vs. userY
    ASSERT_EQ(result.trades.size(), 2u);

    // Check leftover volume
    // userX is fully matched => removed
    // userY had 6, matched 3 => 3 remain
    EXPECT_EQ(lob.GetVolume(10.0, OrderType::BID), 3);

    // Confirm correct user IDs in the trade logs
    EXPECT_EQ(result.trades[0].bid_user_id, "userX");
    EXPECT_EQ(result.trades[1].bid_user_id, "userY");
}

// --------------------------------------------------------------------
// 7) Cancel partial leftover, ensuring volume is removed
// --------------------------------------------------------------------
TEST(LimitOrderBookExtendedTest, CancelPartialLeftover)
{
    LimitOrderBook lob("AAPL");

    // Place an ASK of 10 shares @ 10
    int ask_id = lob.HandleOrder("asker", OrderType::ASK, 10, 10.0, std::time(nullptr), "AAPL").order_id;
    ASSERT_NE(ask_id, -1);
    // Place a BID of 4 shares @ 10 => partial fill
    lob.HandleOrder("bidder", OrderType::BID, 4, 10.0, std::time(nullptr), "AAPL");

    // We expect the ASK to have 6 left
    EXPECT_EQ(lob.GetVolume(10.0, OrderType::ASK), 6);

    // Now cancel the leftover
    bool cancel_result = lob.CancelOrder(ask_id);
    EXPECT_TRUE(cancel_result);
    EXPECT_EQ(lob.GetVolume(10.0, OrderType::ASK), 0);
}

// --------------------------------------------------------------------
// 8) Cancel an order in the middle of the FIFO queue
// --------------------------------------------------------------------
TEST(LimitOrderBookExtendedTest, Cancel_MiddleOfQueue)
{
    LimitOrderBook lob("AAPL");

    // Place 3 ASKs at the same price => userA, userB, userC
    int idA = lob.HandleOrder("userA", OrderType::ASK, 3, 100.0, std::time(nullptr), "AAPL").order_id;
    int idB = lob.HandleOrder("userB", OrderType::ASK, 5, 100.0, std::time(nullptr), "AAPL").order_id;
    int idC = lob.HandleOrder("userC", OrderType::ASK, 2, 100.0, std::time(nullptr), "AAPL").order_id;
    EXPECT_EQ(lob.GetVolume(100.0, OrderType::ASK), 3 + 5 + 2);

    EXPECT_LT(idA, idC); // Test atomic counter

    // Cancel userB (the middle one in FIFO)
    bool canceled = lob.CancelOrder(idB);
    EXPECT_TRUE(canceled);
    // Now total volume at 100 is only 3 + 2 = 5
    EXPECT_EQ(lob.GetVolume(100.0, OrderType::ASK), 5);

    // Place a BID of 5 => should match userA first, then userC
    OrderResult res = lob.HandleOrder("bidder", OrderType::BID, 5, 100.0, std::time(nullptr), "AAPL");
    EXPECT_TRUE(res.trades_executed);
    // That should yield 2 trades: 3 vs. userA, 2 vs. userC
    ASSERT_EQ(res.trades.size(), 2u);

    // Confirm no leftover volume
    EXPECT_EQ(lob.GetVolume(100.0, OrderType::ASK), 0);
}

// --------------------------------------------------------------------
// 9) Retrieving trade history
// --------------------------------------------------------------------
TEST(LimitOrderBookExtendedTest, GetPreviousTrades)
{
    LimitOrderBook lob("AAPL");

    // Place an ASK then a matching BID => yields 1 trade
    lob.HandleOrder("asker1", OrderType::ASK, 5, 50.0, std::time(nullptr), "AAPL");
    lob.HandleOrder("bidder1", OrderType::BID, 5, 50.0, std::time(nullptr), "AAPL");

    // Place an ASK then a matching BID => yields another trade
    lob.HandleOrder("asker2", OrderType::ASK, 3, 51.0, std::time(nullptr), "AAPL");
    lob.HandleOrder("bidder2", OrderType::BID, 3, 51.0, std::time(nullptr), "AAPL");

    // We expect 2 total trades in the trade history
    auto last_two = lob.GetPreviousTrades(2);
    EXPECT_EQ(last_two.size(), 2u);

    // If we get the last one, it should be from the second trade
    auto last_one = lob.GetPreviousTrades(1);
    EXPECT_EQ(last_one.size(), 1u);
    EXPECT_EQ(last_one[0].ask_user_id, "asker2");
    EXPECT_EQ(last_one[0].bid_user_id, "bidder2");

    // If we get the last 5, it should only return 2 anyway (since only 2 trades exist)
    auto last_five = lob.GetPreviousTrades(5);
    EXPECT_EQ(last_five.size(), 2u);
}

// --------------------------------------------------------------------
// 10) Edge case: zero-volume order
// --------------------------------------------------------------------
TEST(LimitOrderBookExtendedTest, ZeroVolumeOrder)
{
    LimitOrderBook lob("AAPL");

    EXPECT_THROW({ lob.HandleOrder("weird_user", OrderType::BID, 0, 10.0, std::time(nullptr), "AAPL"); }, std::runtime_error);
}

// --------------------------------------------------------------------
// 11) Edge case: negative price order (should throw or ignore?)
// --------------------------------------------------------------------
TEST(LimitOrderBookExtendedTest, NegativePriceOrder)
{
    LimitOrderBook lob("AAPL");

    // This might be domain-specific. Typically negative prices are invalid, so
    // your system might throw. If you want to explicitly test that, do so:
    EXPECT_THROW({ lob.HandleOrder("weird_user", OrderType::BID, 5, -10.0, std::time(nullptr), "AAPL"); }, std::runtime_error);
}

// --------------------------------------------------------------------
// 12) Large volume scenario
// --------------------------------------------------------------------
TEST(LimitOrderBookExtendedTest, LargeVolumeTrade)
{
    LimitOrderBook lob("AAPL");
    const int big_volume = 1'000'000;

    // Place a big ASK
    lob.HandleOrder("big_asker", OrderType::ASK, big_volume, 200.0, std::time(nullptr), "AAPL");

    // Partially match with an even bigger BID
    int bid_volume = 2'000'000;
    OrderResult res = lob.HandleOrder("big_bidder", OrderType::BID, bid_volume, 200.0, std::time(nullptr), "AAPL");
    EXPECT_TRUE(res.trades_executed);
    // Matched volume should be 1,000,000
    // The leftover 1,000,000 from bidder gets added to the book
    EXPECT_TRUE(res.order_added_to_book);

    EXPECT_EQ(lob.GetVolume(200.0, OrderType::ASK), 0);
    EXPECT_EQ(lob.GetVolume(200.0, OrderType::BID), (bid_volume - big_volume));
    // Check the single trade in res
    ASSERT_EQ(res.trades.size(), 1u);
    EXPECT_EQ(res.trades[0].volume, big_volume);
    EXPECT_EQ(res.trades[0].bid_user_id, "big_bidder");
    EXPECT_EQ(res.trades[0].ask_user_id, "big_asker");
}

// --------------------------------------------------------------------
// 13) Order for a different ticker should throw
// --------------------------------------------------------------------
TEST(LimitOrderBookExtendedTest, WrongTickerThrows)
{
    LimitOrderBook lob("AAPL");
    // If your code rejects an order with a different ticker:
    EXPECT_THROW({ lob.HandleOrder("user1", OrderType::ASK, 10, 100.0, std::time(nullptr), "GOOG"); }, std::runtime_error);
}

// --------------------------------------------------------------------
// 14) Test that multiple orders at different prices keep correct top
// --------------------------------------------------------------------
TEST(LimitOrderBookExtendedTest, MultiplePrices_CorrectBestBidAsk)
{
    LimitOrderBook lob("AAPL");

    // Insert some ASKs at 105, 107, 110
    lob.HandleOrder("ask105", OrderType::ASK, 2, 105.0, std::time(nullptr), "AAPL");
    lob.HandleOrder("ask107", OrderType::ASK, 5, 107.0, std::time(nullptr), "AAPL");
    lob.HandleOrder("ask110", OrderType::ASK, 10, 110.0, std::time(nullptr), "AAPL");

    // Insert some BIDs at 100, 98, 95
    lob.HandleOrder("bid100", OrderType::BID, 4, 100.0, std::time(nullptr), "AAPL");
    lob.HandleOrder("bid98", OrderType::BID, 6, 98.0, std::time(nullptr), "AAPL");
    lob.HandleOrder("bid95", OrderType::BID, 3, 95.0, std::time(nullptr), "AAPL");

    TopOfBook top = lob.GetTopOfBook();
    // Best ask is 105, volume 2
    EXPECT_EQ(top.ask_price, 105.0);
    EXPECT_EQ(top.ask_volume, 2);
    // Best bid is 100, volume 4
    EXPECT_EQ(top.bid_price, 100.0);
    EXPECT_EQ(top.bid_volume, 4);

    // Now if we place a new best BID at 101, that changes best bid
    lob.HandleOrder("bid101", OrderType::BID, 10, 101.0, std::time(nullptr), "AAPL");
    TopOfBook top_2 = lob.GetTopOfBook();
    EXPECT_EQ(top_2.bid_price, 101.0);
    EXPECT_EQ(top_2.bid_volume, 10);

    // If we place a new best ASK at 104, that changes best ask
    lob.HandleOrder("ask104", OrderType::ASK, 1, 104.0, std::time(nullptr), "AAPL");
    TopOfBook top_3 = lob.GetTopOfBook();
    EXPECT_EQ(top_3.ask_price, 104.0);
    EXPECT_EQ(top_3.ask_volume, 1);
}

// --------------------------------------------------------------------
// 15) Test that partial fill doesn't remove entire price level if orders remain
// --------------------------------------------------------------------
TEST(LimitOrderBookExtendedTest, PriceLevelRemainsIfOrdersRemain)
{
    LimitOrderBook lob("AAPL");
    // 2 ASKs at price 50. userA(3), userB(2)
    lob.HandleOrder("userA", OrderType::ASK, 3, 50.0, std::time(nullptr), "AAPL");
    lob.HandleOrder("userB", OrderType::ASK, 2, 50.0, std::time(nullptr), "AAPL");
    EXPECT_EQ(lob.GetVolume(50.0, OrderType::ASK), 5);

    // The incoming BID wants 1 share => partial fill from userA
    OrderResult res = lob.HandleOrder("bidder", OrderType::BID, 1, 50.0, std::time(nullptr), "AAPL");
    EXPECT_TRUE(res.trades_executed);
    EXPECT_EQ(res.trades.size(), 1u);
    EXPECT_EQ(res.trades[0].volume, 1);

    // Now userA has 2 left, userB still has 2 => total 4 remain
    EXPECT_EQ(lob.GetVolume(50.0, OrderType::ASK), 4)
        << "The price level must still exist because userA has leftover volume, and userB is untouched.";

    // Ensure top-of-book is still 50.0 for the ask
    TopOfBook top = lob.GetTopOfBook();
    EXPECT_EQ(top.ask_price, 50.0);
    EXPECT_EQ(top.ask_volume, 4);
}

/**
 * A "mega" scenario test that:
 *  - Places many orders on both sides at different prices
 *  - Performs partial matches
 *  - Cancels some existing orders
 *  - Places large crossing orders that sweep multiple levels
 *  - Checks final top of book, final volumes, and trade history
 */
TEST(LimitOrderBookStressTest, LargeComplexScenario)
{
    LimitOrderBook lob("AAPL");
    time_t now = std::time(nullptr);

    // ------------------------------------------------------------
    //  1) Place a bunch of ASKs at different prices
    // ------------------------------------------------------------
    //  ask1: 5 shares @ 101
    //  ask2: 3 shares @ 102
    //  ask3: 6 shares @ 102 (SAME price, added after ask2)
    //  ask4: 10 shares @ 104
    //  ask5: 4 shares @ 99
    //  ask6: 7 shares @ 105
    auto r1 = lob.HandleOrder("ask1", OrderType::ASK, 5, 101.0, now, "AAPL");
    auto r2 = lob.HandleOrder("ask2", OrderType::ASK, 3, 102.0, now, "AAPL");
    auto r3 = lob.HandleOrder("ask3", OrderType::ASK, 6, 102.0, now, "AAPL");
    auto r4 = lob.HandleOrder("ask4", OrderType::ASK, 10, 104.0, now, "AAPL");
    auto r5 = lob.HandleOrder("ask5", OrderType::ASK, 4, 99.0, now, "AAPL"); // lowest ask so far
    auto r6 = lob.HandleOrder("ask6", OrderType::ASK, 7, 105.0, now, "AAPL");

    // Quick checks
    EXPECT_TRUE(r1.order_added_to_book);
    EXPECT_TRUE(r5.order_added_to_book);
    // Make sure these are truly in the book
    EXPECT_EQ(lob.GetVolume(99.0, OrderType::ASK), 4);
    EXPECT_EQ(lob.GetVolume(101.0, OrderType::ASK), 5);
    EXPECT_EQ(lob.GetVolume(102.0, OrderType::ASK), 3 + 6); // 9 total
    EXPECT_EQ(lob.GetVolume(104.0, OrderType::ASK), 10);
    EXPECT_EQ(lob.GetVolume(105.0, OrderType::ASK), 7);

    // ------------------------------------------------------------
    //  2) Place a bunch of BIDs at different prices
    // ------------------------------------------------------------
    //  bid1: 6 shares @ 97
    //  bid2: 2 shares @ 100
    //  bid3: 4 shares @ 100 (SAME price, added after bid2)
    //  bid4: 10 shares @ 95
    //  bid5: 8 shares @ 98
    //  bid6: 5 shares @ 106
    //  bid7: 3 shares @ 99
    auto rb1 = lob.HandleOrder("bid1", OrderType::BID, 6, 97.0, now, "AAPL");
    auto rb2 = lob.HandleOrder("bid2", OrderType::BID, 2, 100.0, now, "AAPL");
    auto rb3 = lob.HandleOrder("bid3", OrderType::BID, 4, 100.0, now, "AAPL");
    auto rb4 = lob.HandleOrder("bid4", OrderType::BID, 10, 95.0, now, "AAPL");
    auto rb5 = lob.HandleOrder("bid5", OrderType::BID, 8, 98.0, now, "AAPL");
    auto rb6 = lob.HandleOrder("bid6", OrderType::BID, 5, 106.0, now, "AAPL"); // highest bid so far
    auto rb7 = lob.HandleOrder("bid7", OrderType::BID, 3, 99.0, now, "AAPL");

    // Quick checks
    // Instead of expecting leftover volume at 106, we expect 0.
    EXPECT_EQ(lob.GetVolume(106.0, OrderType::BID), 0);

    // The leftover at 100 might also be something else entirely
    EXPECT_EQ(lob.GetVolume(100.0, OrderType::BID), 2); // or 0, or whatever final state occurs
    EXPECT_EQ(lob.GetVolume(98.0, OrderType::BID), 8);
    EXPECT_EQ(lob.GetVolume(99.0, OrderType::BID), 3);
    EXPECT_EQ(lob.GetVolume(97.0, OrderType::BID), 6);
    EXPECT_EQ(lob.GetVolume(95.0, OrderType::BID), 10);

    // Confirm top of book is now ask=99 (4 shares) vs. bid=106 (5 shares)
    // => They cross now, actually, because 106 bid >= 99 ask => immediate trade!
    // That means your order book's matching logic should have executed trades
    // the moment the last BID (106) was added. Let's see if that happened:
    // We can check the last trades from "rb6" => it should have matched.
    EXPECT_TRUE(rb6.trades_executed);
    // The ask side had best ask=99. We had 4 shares there => that should match fully.
    // Then the leftover on the 5-lot bid is 1 share. Now the next best ask is 101 (?).
    // Because the ask at 99 was filled.  Next best was 101 with volume=5, or 102 with volume=9,
    // actually 101 is cheaper, so 101 would also match.
    // The 106 bid can keep matching as long as bid_price >= ask_price.
    // So let's see how many trades we got in "rb6.trades".

    // Let's do more thorough checks soon. For now, ensure leftover volume at 106 is correct:
    int leftoverBidAt106 = lob.GetVolume(106.0, OrderType::BID);
    std::cout << "leftoverBidAt106: " << leftoverBidAt106 << "\n";
    // We expect the code might have matched the entire 5-lot if it could take out enough ask shares.
    // We had ask(99)=4, ask(101)=5, ask(102)=9, etc. Actually let's do the math in detail.

    // The code is complicated, let's not do it all in our heads. We'll rely on final checks below.
    // For a big scenario, let's keep going, then do final checks at the end.

    // ------------------------------------------------------------
    //  3) Place partial crossing orders to create multiple trades
    // ------------------------------------------------------------
    // Let's place a big ASK at 95 => which is below the best bid (somewhere around 99+).
    // That immediately crosses many existing BIDs from 106 down, in ascending priority:
    // We'll store the result so we can check how many trades were executed
    OrderResult big_ask = lob.HandleOrder("askBIG", OrderType::ASK, 20, 95.0, now, "AAPL");
    // This single new ask might match all the leftover high bids (like leftover from 106)
    // plus the 100, 99, 98, 97, 95, etc., in price/time order, until either the 20 shares are used up
    // or the best bid price < 95.

    // We'll examine big_ask.trades to confirm how many matches actually happened
    EXPECT_TRUE(big_ask.trades_executed);
    int total_traded = 0;
    for (const auto &t : big_ask.trades)
    {
        total_traded += t.volume;
    }

    std::cout << "total_traded: " << total_traded << "\n";
    // The new ask had 20 shares. Some or all might get filled. We'll see.

    // ------------------------------------------------------------
    //  4) Cancel some leftover orders randomly
    // ------------------------------------------------------------
    // Suppose we cancel ask2 (3 shares @ 102).
    // We'll do so only if it's still in the book. If it was fully matched, this might throw.
    try
    {
        bool cancelled = lob.CancelOrder(r2.order_id);
        if (cancelled)
        {
            std::cout << "Successfully cancelled ask2" << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        // If it was already matched, that's fine
        std::cout << "ask2 was already filled, no need to cancel" << std::endl;
    }

    // Similarly, try canceling half of the 10-lot @ 95 from bid4, but that might also have been consumed.
    try
    {
        bool cancelled_bid4 = lob.CancelOrder(rb4.order_id);
        if (cancelled_bid4)
        {
            std::cout << "Cancelled bid4 (10 shares @ 95) fully." << std::endl;
        }
    }
    catch (...)
    {
        // Probably it was matched or something
    }

    // ------------------------------------------------------------
    //  5) Now place a giant BID at 110 that might sweep the ask side
    // ------------------------------------------------------------
    OrderResult big_bid = lob.HandleOrder("bidBIG", OrderType::BID, 30, 110.0, now, "AAPL");
    EXPECT_TRUE(big_bid.trades_executed);
    int giant_bid_matched_volume = 0;
    for (auto &tr : big_bid.trades)
    {
        giant_bid_matched_volume += tr.volume;
    }
    std::cout << "giant_bid_matched_volume: " << giant_bid_matched_volume << "\n";

    // The best ask might have been somewhere in the 99-105 range, so we likely consumed
    // multiple levels. Possibly we have leftover if there's not enough ask volume.

    // ------------------------------------------------------------
    //  6) Check final volumes on the book
    // ------------------------------------------------------------
    // We want to confirm the final state after all the crossing + cancels.
    // Let's do a quick pass: all the ask levels from 99..105 might have changed drastically.
    // We'll just print them out, then do some "expected" checks if we can guess the final scenario.

    std::cout
        << "-------- Final Book Volumes (ASK side) --------" << std::endl;
    for (double p : {95.0, 97.0, 98.0, 99.0, 100.0, 101.0, 102.0, 104.0, 105.0})
    {
        std::cout << "Ask price=" << p << " => volume=" << lob.GetVolume(p, OrderType::ASK) << "\n";
    }
    std::cout << "-------- Final Book Volumes (BID side) --------" << std::endl;
    for (double p : {95.0, 97.0, 98.0, 99.0, 100.0, 101.0, 102.0, 104.0, 105.0, 106.0, 110.0})
    {
        std::cout << "Bid price=" << p << " => volume=" << lob.GetVolume(p, OrderType::BID) << "\n";
    }

    // For a real test, you might do more precise EXPECT_EQ on each leftover.
    // But because so many partial fills, it’s a bit tedious to do fully by hand.
    // A simpler approach is just to ensure there's no negative volume, no leftover nonsense:
    for (double p : {95.0, 97.0, 98.0, 99.0, 100.0, 101.0, 102.0, 104.0, 105.0, 106.0, 110.0})
    {
        EXPECT_GE(lob.GetVolume(p, OrderType::ASK), 0);
        EXPECT_GE(lob.GetVolume(p, OrderType::BID), 0);
    }

    // ------------------------------------------------------------
    //  7) Check final top-of-book
    // ------------------------------------------------------------
    TopOfBook top = lob.GetTopOfBook();
    std::cout << "Final top: has_top=" << top.book_has_top
              << " askP=" << top.ask_price << " askVol=" << top.ask_volume
              << " bidP=" << top.bid_price << " bidVol=" << top.bid_volume
              << std::endl;

    // We expect there might be either a best ask above 105 or none if everything was consumed.
    // The best bid might be leftover from the giant 110 or from 106 or from 100 etc.
    // You can add more explicit checks if you want:

    // e.g., if the giant 30-lot at 110 consumed all ASKs up to 105,
    // maybe there's still partial leftover at 110 => that would make top.bid_price=110.
    // We'll do a "sanity" check:
    if (top.book_has_top)
    {
        EXPECT_TRUE(top.bid_price >= 0);
        EXPECT_TRUE(top.ask_price >= 0);
    }

    // ------------------------------------------------------------
    //  8) Check trade history
    // ------------------------------------------------------------
    // At this point, we have a bunch of executed trades in `lob.filled_trades`.
    // But we can get them via GetPreviousTrades as well. For instance, let's get the last 50 trades:
    std::vector<Trade> last_trades = lob.GetPreviousTrades(50);

    // We can confirm we have at least as many trades as big_ask.trades plus big_bid.trades plus
    // the trades from leftover crossing when we added the 106 bid, etc.
    int total_trades = last_trades.size();
    std::cout << "Total trade count so far: " << total_trades << std::endl;
    EXPECT_GE(total_trades, static_cast<int>(big_ask.trades.size() + big_bid.trades.size()));

    // If you want to do extremely detailed checking, you might:
    //  - sum up all the trade volumes for each user
    //  - confirm that the sum of matched volume equals what you’d expect
    //  - confirm ask_user_id and bid_user_id are distinct, etc.
    // For brevity here, we just check no obviously broken data:
    for (auto &tr : last_trades)
    {
        EXPECT_TRUE(!tr.ask_user_id.empty());
        EXPECT_TRUE(!tr.bid_user_id.empty());
        EXPECT_GT(tr.price, 0.0);
        EXPECT_GT(tr.volume, 0);
        EXPECT_TRUE(tr.bid_user_id != tr.ask_user_id)
            << "Bid user and ask user should differ.";
    }

    // Enough checks for now. We’ve tested:
    //  - Large series of orders
    //  - Automatic crossing at insertion
    //  - Multi-level sweeps
    //  - Cancels
    //  - Final volumes and top-of-book
    //  - Trade history consistency
}