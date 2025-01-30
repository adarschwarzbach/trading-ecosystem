#include <gtest/gtest.h>
#include "exchange/exchange.hpp"
#include "utils/order_type.hpp"
#include <stdexcept>
#include <iostream>

// -------------------------------------------------------------------
// 1) Test Exchange initialization with custom tickers
// -------------------------------------------------------------------
TEST(ExchangeTest, ConstructorWithAllowedTickers)
{
    std::vector<std::string> my_tickers = {"AAPL", "GOOG", "TSLA"};
    Exchange ex(my_tickers);

    auto ex_tickers = ex.GetTickers();
    EXPECT_EQ(ex_tickers.size(), 3u);
    EXPECT_TRUE(ex_tickers.find("AAPL") != ex_tickers.end());
    EXPECT_TRUE(ex_tickers.find("GOOG") != ex_tickers.end());
    EXPECT_TRUE(ex_tickers.find("TSLA") != ex_tickers.end());

    // Confirm unknown ticker throws
    EXPECT_THROW(ex.GetTopOfBook("MSFT"), std::runtime_error);
    EXPECT_THROW(ex.GetPreviousTrades("MSFT", 5), std::runtime_error);
}

// -------------------------------------------------------------------
// 2) Simple Add Order - Checking Volume and Top Of Book
// -------------------------------------------------------------------
TEST(ExchangeTest, AddSingleOrder_CheckVolume_TopOfBook)
{
    Exchange ex({"MSFT"});

    // Add a single BID
    ex.HandleOrder("bidUser", OrderType::BID, 100, 250.0, "MSFT");

    // Check that volume is reflected
    int vol = ex.GetVolume("MSFT", 250.0, OrderType::BID);
    EXPECT_EQ(vol, 100);

    // Check top of book
    auto top = ex.GetTopOfBook("MSFT");
    EXPECT_TRUE(top.book_has_top);
    EXPECT_EQ(top.bid_price, 250.0);
    EXPECT_EQ(top.bid_volume, 100);
    // No asks yet
    EXPECT_EQ(top.ask_price, 0.0);
    EXPECT_EQ(top.ask_volume, 0);
}

// -------------------------------------------------------------------
// 3) Crossing Orders - Should Execute a Trade
// -------------------------------------------------------------------
TEST(ExchangeTest, CrossingOrders_ImmediateTrade)
{
    Exchange ex({"AMZN"});

    // Place an ASK: user1, 10 shares @ 100.0
    auto ask_res = ex.HandleOrder("user1", OrderType::ASK, 10, 100.0, "AMZN");
    EXPECT_TRUE(ask_res.order_added_to_book);
    EXPECT_FALSE(ask_res.trades_executed);

    // Place a BID: user2, 10 shares @ 105 => crosses at 100
    auto bid_res = ex.HandleOrder("user2", OrderType::BID, 10, 105.0, "AMZN");
    EXPECT_TRUE(bid_res.trades_executed);
    EXPECT_FALSE(bid_res.order_added_to_book); // fully matched
    ASSERT_EQ(bid_res.trades.size(), 1u);

    // Check the trade
    auto &trade = bid_res.trades[0];
    EXPECT_EQ(trade.volume, 10);
    EXPECT_EQ(trade.price, 100.0); // matched at ASK price
    EXPECT_EQ(trade.ask_user_id, "user1");
    EXPECT_EQ(trade.bid_user_id, "user2");

    // The book should be empty now
    auto top = ex.GetTopOfBook("AMZN");
    EXPECT_FALSE(top.book_has_top);

    // Check Volume is zero
    EXPECT_EQ(ex.GetVolume("AMZN", 100.0, OrderType::ASK), 0);
}

// -------------------------------------------------------------------
// 4) Multi-Level Matching (Partial Fills Across Price Levels)
// -------------------------------------------------------------------
TEST(ExchangeTest, MultiLevel_PartialFill)
{
    Exchange ex({"TSLA"});

    // Place 2 ASKs at different prices
    // Price 500 => 3 shares
    // Price 505 => 5 shares
    ex.HandleOrder("askA", OrderType::ASK, 3, 500.0, "TSLA");
    ex.HandleOrder("askB", OrderType::ASK, 5, 505.0, "TSLA");
    EXPECT_EQ(ex.GetVolume("TSLA", 500.0, OrderType::ASK), 3);
    EXPECT_EQ(ex.GetVolume("TSLA", 505.0, OrderType::ASK), 5);

    // Now place a large BID at 510 => enough to sweep both price levels
    // e.g. volume=7 => first fill 3 shares @500, leftover 4 => fill 4 shares @505 (partial out of 5)
    auto res = ex.HandleOrder("bidUser", OrderType::BID, 7, 510.0, "TSLA");
    EXPECT_TRUE(res.trades_executed);
    // We expect 2 trades: one at 500, one at 505
    ASSERT_EQ(res.trades.size(), 2u);

    // The 3 shares at 500 are fully consumed
    EXPECT_EQ(ex.GetVolume("TSLA", 500.0, OrderType::ASK), 0);

    // At 505, we had 5 shares, matched 4 => leftover 1
    EXPECT_EQ(ex.GetVolume("TSLA", 505.0, OrderType::ASK), 1);

    // The incoming 7-lot is fully matched, so no leftover in the BID side
    auto top = ex.GetTopOfBook("TSLA");
    EXPECT_TRUE(top.book_has_top);
    EXPECT_EQ(top.ask_price, 505.0);
    EXPECT_EQ(top.ask_volume, 1);
    EXPECT_EQ(top.bid_price, 0.0);
    EXPECT_EQ(top.bid_volume, 0);
}

// -------------------------------------------------------------------
// 5) Cancel an existing order
// -------------------------------------------------------------------
TEST(ExchangeTest, CancelOrderTest)
{
    Exchange ex({"NFLX"});

    // Add an ASK of 10 shares @300
    auto res1 = ex.HandleOrder("askUser", OrderType::ASK, 10, 300.0, "NFLX");
    int ask_id = res1.order_id;

    EXPECT_TRUE(res1.order_added_to_book);
    EXPECT_EQ(ex.GetVolume("NFLX", 300.0, OrderType::ASK), 10);

    // Cancel the order
    bool cancelled = ex.CancelOrder("NFLX", ask_id);
    EXPECT_TRUE(cancelled);

    // Check that volume is now 0
    EXPECT_EQ(ex.GetVolume("NFLX", 300.0, OrderType::ASK), 0);

    // The top of book should be empty
    auto top = ex.GetTopOfBook("NFLX");
    EXPECT_FALSE(top.book_has_top);
}

// -------------------------------------------------------------------
// 6) GetTradesByUser - Ensures both participants see the trade
// -------------------------------------------------------------------
TEST(ExchangeTest, TradesByUser)
{
    Exchange ex({"IBM"});

    // userX places an ASK => 4 shares @ 100
    ex.HandleOrder("userX", OrderType::ASK, 4, 100.0, "IBM");

    // userY places a BID => 4 shares @ 120 => crosses
    ex.HandleOrder("userY", OrderType::BID, 4, 120.0, "IBM");

    auto tradesX = ex.GetTradesByUser("userX");
    auto tradesY = ex.GetTradesByUser("userY");
    EXPECT_EQ(tradesX.size(), 1u);
    EXPECT_EQ(tradesY.size(), 1u);

    // Both see the same trade
    EXPECT_EQ(tradesX[0].ask_user_id, "userX");
    EXPECT_EQ(tradesX[0].bid_user_id, "userY");
    EXPECT_EQ(tradesX[0].volume, 4);

    EXPECT_EQ(tradesY[0].ask_user_id, "userX");
    EXPECT_EQ(tradesY[0].bid_user_id, "userY");
    EXPECT_EQ(tradesY[0].volume, 4);
}

// -------------------------------------------------------------------
// 7) Zero or Negative Volume, Invalid Ticker
// -------------------------------------------------------------------
TEST(ExchangeTest, EdgeCase_InvalidInputs)
{
    Exchange ex({"BTC", "ETH"});

    // Invalid ticker
    EXPECT_THROW(ex.HandleOrder("userA", OrderType::BID, 10, 100.0, "DOGE"), std::runtime_error);

    // Negative volume
    // If your LOB throws an exception for negative volume, do this:
    EXPECT_THROW(ex.HandleOrder("userA", OrderType::ASK, -5, 200.0, "BTC"), std::runtime_error);

    // Zero volume
    // If you’ve decided 0 volume also throws:
    EXPECT_THROW(ex.HandleOrder("userB", OrderType::BID, 0, 210.0, "ETH"), std::runtime_error);
}

// -------------------------------------------------------------------
// 8) Confirming leftover partial fill is in the correct side
// -------------------------------------------------------------------
TEST(ExchangeTest, PartialLeftoverInBook)
{
    Exchange ex({"TSLA"});

    // Place a smaller ASK => 2 shares @ 700
    ex.HandleOrder("userA", OrderType::ASK, 2, 700.0, "TSLA");

    // Place a bigger BID => 5 shares @ 700 => partial fill
    auto res = ex.HandleOrder("userB", OrderType::BID, 5, 700.0, "TSLA");
    // We match 2, leftover 3 => leftover is a 3-lot BID at 700
    // So the new_order was partially filled => leftover 3 should remain
    EXPECT_TRUE(res.order_added_to_book); // leftover remains
    EXPECT_TRUE(res.trades_executed);
    EXPECT_EQ(res.trades[0].volume, 2);

    // The ask side is now empty
    EXPECT_EQ(ex.GetVolume("TSLA", 700.0, OrderType::ASK), 0);

    // The leftover BID is 3
    EXPECT_EQ(ex.GetVolume("TSLA", 700.0, OrderType::BID), 3);

    // Confirm top-of-book => best bid=700, volume=3
    auto top = ex.GetTopOfBook("TSLA");
    EXPECT_TRUE(top.book_has_top);
    EXPECT_EQ(top.bid_price, 700.0);
    EXPECT_EQ(top.bid_volume, 3);
    EXPECT_EQ(top.ask_price, 0.0);
    EXPECT_EQ(top.ask_volume, 0);
}

// -------------------------------------------------------------------
// 9) Check that we can retrieve previous trades by Ticker
// -------------------------------------------------------------------
TEST(ExchangeTest, PreviousTradesByTicker)
{
    Exchange ex({"XRP"});

    // Place ask => 5 @ 1.00
    ex.HandleOrder("askUser", OrderType::ASK, 5, 1.0, "XRP");

    // Place bid => 5 @ 2.00 => match => trade
    ex.HandleOrder("bidUser", OrderType::BID, 5, 2.0, "XRP");

    // Another ask => 3 @ 1.50
    ex.HandleOrder("askUser2", OrderType::ASK, 3, 1.50, "XRP");

    // Another bid => 3 @ 2.00 => match => trade
    ex.HandleOrder("bidUser2", OrderType::BID, 3, 2.0, "XRP");

    // We should have 2 trades total for this ticker
    auto recent_trades = ex.GetPreviousTrades("XRP", 10);
    EXPECT_EQ(recent_trades.size(), 2u);

    // If we request fewer, we get fewer
    auto last_trade_only = ex.GetPreviousTrades("XRP", 1);
    EXPECT_EQ(last_trade_only.size(), 1u);

    // If we request a huge number, we still only get 2
    auto too_many = ex.GetPreviousTrades("XRP", 999);
    EXPECT_EQ(too_many.size(), 2u);
}

// -------------------------------------------------------------------
// 10) Stress Test: multiple BIDs & ASKs at varying prices
// -------------------------------------------------------------------
TEST(ExchangeTest, StressTest_MultipleOrders)
{
    Exchange ex({"AAPL"});

    // Bunch of ASKs
    ex.HandleOrder("askA", OrderType::ASK, 2, 150.0, "AAPL");
    ex.HandleOrder("askB", OrderType::ASK, 3, 151.0, "AAPL");
    ex.HandleOrder("askC", OrderType::ASK, 5, 152.0, "AAPL");

    // Bunch of BIDs
    ex.HandleOrder("bidA", OrderType::BID, 4, 149.0, "AAPL");
    ex.HandleOrder("bidB", OrderType::BID, 6, 148.0, "AAPL");
    ex.HandleOrder("bidC", OrderType::BID, 10, 145.0, "AAPL");

    // So far no immediate crosses because best bid=149 < best ask=150
    auto top = ex.GetTopOfBook("AAPL");
    EXPECT_TRUE(top.book_has_top);
    EXPECT_EQ(top.bid_price, 149.0);
    EXPECT_EQ(top.bid_volume, 4);
    EXPECT_EQ(top.ask_price, 150.0);
    EXPECT_EQ(top.ask_volume, 2);

    // Now place a big BID at 152 => crosses multiple levels
    auto res = ex.HandleOrder("bigBid", OrderType::BID, 10, 152.0, "AAPL");
    // This should match:
    //   2 shares @150.0 (fully consumes askA)
    //   leftover 8 => match 3 shares @151.0 (fully consumes askB)
    //   leftover 5 => match 5 shares @152.0 (fully consumes askC)
    // total matched: 2+3+5=10 => leftover 0 => no new order in the book
    EXPECT_TRUE(res.trades_executed);
    EXPECT_EQ(res.trades.size(), 3u);
    EXPECT_FALSE(res.order_added_to_book);

    // Now top-of-book => best ask is none => ask side is empty
    // best bid is still 149 (4 shares)
    TopOfBook new_top = ex.GetTopOfBook("AAPL");
    EXPECT_TRUE(new_top.book_has_top);
    EXPECT_EQ(new_top.bid_price, 149.0);
    EXPECT_EQ(new_top.bid_volume, 4);
    EXPECT_EQ(new_top.ask_price, 0.0);
    EXPECT_EQ(new_top.ask_volume, 0);
}

TEST(ExchangeTest, InvalidTickerTest)
{
    Exchange ex({"AAPL", "GOOG"});

    // Invalid ticker
    EXPECT_THROW(ex.HandleOrder("user1", OrderType::BID, 10, 150.0, "MSFT"), std::runtime_error);

    // Empty ticker
    EXPECT_THROW(ex.HandleOrder("user2", OrderType::ASK, 5, 200.0, ""), std::runtime_error);
}

TEST(ExchangeTest, EdgeCase_InvalidPrice)
{
    Exchange ex({"BTC"});

    // Negative price
    EXPECT_THROW(ex.HandleOrder("userA", OrderType::BID, 10, -100.0, "BTC"), std::runtime_error);

    // Zero price
    EXPECT_THROW(ex.HandleOrder("userB", OrderType::ASK, 5, 0.0, "BTC"), std::runtime_error);
}

TEST(ExchangeTest, CrossingOrders_PartialFillLeftover)
{
    Exchange ex({"TSLA"});

    // Place an ASK for 5 shares at $500
    ex.HandleOrder("askUser", OrderType::ASK, 5, 500.0, "TSLA");

    // Place a BID for 10 shares at $500 (5 will match, 5 will be leftover)
    auto bidRes = ex.HandleOrder("bidUser", OrderType::BID, 10, 500.0, "TSLA");

    EXPECT_TRUE(bidRes.trades_executed);
    EXPECT_EQ(bidRes.trades.size(), 1u); // Single trade for 5 shares
    EXPECT_EQ(bidRes.trades[0].volume, 5);
    EXPECT_EQ(ex.GetVolume("TSLA", 500.0, OrderType::BID), 5); // Leftover BID for 5
    EXPECT_EQ(ex.GetVolume("TSLA", 500.0, OrderType::ASK), 0); // ASK fully consumed
}

TEST(ExchangeTest, MultipleOrdersBySameUser)
{
    Exchange ex({"GOOG"});

    // Same user places multiple ASKs at different prices
    ex.HandleOrder("userX", OrderType::ASK, 5, 2000.0, "GOOG");
    ex.HandleOrder("userX", OrderType::ASK, 10, 2100.0, "GOOG");

    EXPECT_EQ(ex.GetVolume("GOOG", 2000.0, OrderType::ASK), 5);
    EXPECT_EQ(ex.GetVolume("GOOG", 2100.0, OrderType::ASK), 10);

    // Place a BID that partially fills the first ASK
    auto res = ex.HandleOrder("userY", OrderType::BID, 3, 2000.0, "GOOG");
    EXPECT_TRUE(res.trades_executed);
    EXPECT_EQ(res.trades[0].volume, 3);

    // Remaining ASK volume at $2000
    EXPECT_EQ(ex.GetVolume("GOOG", 2000.0, OrderType::ASK), 2);
}

TEST(ExchangeTest, LargeOrdersTest)
{
    Exchange ex({"ETH"});

    // Add a huge ASK
    ex.HandleOrder("askUser", OrderType::ASK, 1'000'000, 2000.0, "ETH");

    // Place a large BID to partially fill
    auto res = ex.HandleOrder("bidUser", OrderType::BID, 750'000, 2000.0, "ETH");
    EXPECT_TRUE(res.trades_executed);
    EXPECT_EQ(res.trades[0].volume, 750'000);

    // Remaining ASK volume
    EXPECT_EQ(ex.GetVolume("ETH", 2000.0, OrderType::ASK), 250'000);
}

TEST(ExchangeTest, StressTest_ManyPriceLevels)
{
    Exchange ex({"AAPL"});

    // Add ASKs at multiple price levels
    for (int i = 150; i <= 200; i += 5)
    {
        ex.HandleOrder("askUser" + std::to_string(i), OrderType::ASK, i, i * 1.0, "AAPL");
    }

    // Place a large BID to sweep multiple price levels
    auto res = ex.HandleOrder("bidUser", OrderType::BID, 2000 /* or 2500, etc. */, 200.0, "AAPL");
    ASSERT_EQ(res.trades.size(), 11); // now it will fill all 11 levels
}

TEST(ExchangeTest, CancelPartialOrder)
{
    Exchange ex({"NFLX"});

    // Add an ASK for 10 shares
    auto res = ex.HandleOrder("askUser", OrderType::ASK, 10, 300.0, "NFLX");
    int orderId = res.order_id;

    // Place a BID for 5 shares (partial fill)
    ex.HandleOrder("bidUser", OrderType::BID, 5, 300.0, "NFLX");

    // Cancel the remaining 5 shares of the ASK
    bool cancelled = ex.CancelOrder("NFLX", orderId);
    EXPECT_TRUE(cancelled);

    // Volume at $300 should now be zero
    EXPECT_EQ(ex.GetVolume("NFLX", 300.0, OrderType::ASK), 0);
}

// -------------------------------------------------------------------
// 1) Test User Registration
// -------------------------------------------------------------------
TEST(ExchangeTest, UserRegistration)
{
    Exchange ex({"AAPL"});

    // Register a new user
    EXPECT_TRUE(ex.RegisterUser("trader1"));
    EXPECT_FALSE(ex.RegisterUser("trader1")); // Duplicate should fail
}

// -------------------------------------------------------------------
// 2) Trade History Retrieval for Registered Users
// -------------------------------------------------------------------
TEST(ExchangeTest, UserTradeHistory)
{
    Exchange ex({"TSLA"});
    ex.RegisterUser("userA");
    ex.RegisterUser("userB");

    // Place an order that results in a trade
    ex.HandleOrder("userA", OrderType::ASK, 10, 300.0, "TSLA");
    ex.HandleOrder("userB", OrderType::BID, 10, 305.0, "TSLA"); // Match

    // Check trade history
    auto tradesA = ex.GetTradesByUser("userA");
    auto tradesB = ex.GetTradesByUser("userB");

    EXPECT_EQ(tradesA.size(), 1u);
    EXPECT_EQ(tradesB.size(), 1u);

    EXPECT_EQ(tradesA[0].ask_user_id, "userA");
    EXPECT_EQ(tradesA[0].bid_user_id, "userB");
    EXPECT_EQ(tradesA[0].volume, 10);

    EXPECT_EQ(tradesB[0].ask_user_id, "userA");
    EXPECT_EQ(tradesB[0].bid_user_id, "userB");
    EXPECT_EQ(tradesB[0].volume, 10);
}

// -------------------------------------------------------------------
// 3) Empty Trade History for New Users
// -------------------------------------------------------------------
TEST(ExchangeTest, EmptyTradeHistory)
{
    Exchange ex({"GOOG"});
    ex.RegisterUser("userC");

    // No trades yet
    auto trades = ex.GetTradesByUser("userC");
    EXPECT_EQ(trades.size(), 0u);
}

// -------------------------------------------------------------------
// 4) Trade History with Multiple Orders
// -------------------------------------------------------------------
TEST(ExchangeTest, MultipleTradesByUser)
{
    Exchange ex({"MSFT"});
    ex.RegisterUser("userX");
    ex.RegisterUser("userY");
    ex.RegisterUser("userZ");

    // Multiple trades for userX
    ex.HandleOrder("userX", OrderType::ASK, 5, 250.0, "MSFT");
    ex.HandleOrder("userY", OrderType::BID, 5, 255.0, "MSFT"); // Match

    ex.HandleOrder("userX", OrderType::ASK, 3, 260.0, "MSFT");
    ex.HandleOrder("userZ", OrderType::BID, 3, 265.0, "MSFT"); // Match

    auto tradesX = ex.GetTradesByUser("userX");
    EXPECT_EQ(tradesX.size(), 2u);
}

// // -------------------------------------------------------------------
// // 5) Attempting to Place Order with Unregistered User
// // -------------------------------------------------------------------
// TEST(ExchangeTest, OrderFromUnregisteredUser)
// {
//     Exchange ex({"BTC"});

//     // Unregistered user attempts to place an order
//     EXPECT_THROW(ex.HandleOrder("ghostTrader", OrderType::BID, 10, 50000.0, "BTC"), std::runtime_error);
// }
