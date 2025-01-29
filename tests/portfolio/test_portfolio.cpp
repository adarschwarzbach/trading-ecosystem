#include <gtest/gtest.h>
#include "portfolio/portfolio.hpp" // Adjust the include path as needed

// Helper function to build a simple price map for unrealized PnL checks
static std::unordered_map<std::string, double> MakePriceMap(const std::string &ticker, double price)
{
    std::unordered_map<std::string, double> mp;
    mp[ticker] = price;
    return mp;
}

// -------------------------------------------------------------------
// 1) Test basic buy
// -------------------------------------------------------------------
TEST(PortfolioTest, BasicBuy)
{
    Portfolio pf(10000.0); // start with 10,000 cash

    // Buy 10 shares @ 100 => volume=+10
    pf.Trade("AAPL", +10, 100.0);

    // Check new state
    // cash_balance => 10,000 - (10*100) = 9,000
    EXPECT_EQ(pf.cash_balance, 9000.0);
    // net_shares => +10
    auto &pos = pf.positions["AAPL"];
    EXPECT_EQ(pos.net_shares, 10);
    // avg_cost => 100
    EXPECT_EQ(pos.avg_cost, 100.0);
    // realized_pnl => 0 so far
    EXPECT_EQ(pf.realized_pnl, 0.0);

    // Check unrealized if current price=105
    auto pxMap = MakePriceMap("AAPL", 105.0);
    double unreal = pf.ComputeUnrealizedPnL("AAPL", 105.0);
    // (105 - 100)* 10 = 50
    EXPECT_EQ(unreal, 50.0);

    // total value => 9000 + 0 + 50= 9050
    double totalVal = pf.ComputeTotalValue(pxMap);
    EXPECT_EQ(totalVal, 9050.0);
}

// -------------------------------------------------------------------
// 2) Add to existing position (same direction, average cost update)
// -------------------------------------------------------------------
TEST(PortfolioTest, AddToExistingPosition)
{
    Portfolio pf(10000.0);

    // Buy 10 shares @ 100 => leftover cash=9000, pos=(+10, avg=100)
    pf.Trade("TSLA", +10, 100.0);

    // Buy 5 more @ 110 => leftover cash=9000 - (5*110)= 9000 - 550= 8450
    // new net_shares= 15
    // new avg_cost= (10*100 + 5*110)/ 15= (1000 + 550)/ 15= 1550/15= 103.33...
    pf.Trade("TSLA", +5, 110.0);

    auto &pos = pf.positions["TSLA"];
    EXPECT_EQ(pf.cash_balance, 8450.0);
    EXPECT_EQ(pos.net_shares, 15);
    // check approximate average cost
    EXPECT_NEAR(pos.avg_cost, 103.3333, 1e-4);
    EXPECT_EQ(pf.realized_pnl, 0.0);

    // If current price=120 => unrealized= (120 - 103.3333)*15= ~ 16.6667*15= 250
    auto pxMap = MakePriceMap("TSLA", 120.0);
    EXPECT_NEAR(pf.ComputeUnrealizedPnL("TSLA", 120.0), 16.6667 * 15, 1e-3);

    // total => 8450 + 0 + 250= 8700 approx
    EXPECT_NEAR(pf.ComputeTotalValue(pxMap), 8700.0, 1.0);
}

// -------------------------------------------------------------------
// 3) Partial close (opposite direction, partial real)
///   E.g. we hold +10, we sell 4 => realized PnL on 4 shares
// -------------------------------------------------------------------
TEST(PortfolioTest, PartialClose)
{
    Portfolio pf(10000.0);

    // 1) Buy 10 @ 50 => cost= 500 => leftover=9500, net_shares=+10, avg=50
    pf.Trade("IBM", +10, 50.0);

    // 2) Sell 4 @ 60 => volume= -4 => partial close
    // realized => (60 - 50)* 4= +40
    pf.Trade("IBM", -4, 60.0);

    // check portfolio
    // cash => was 9500, minus cost( -4*60= -240) => 9500 - (-240)= 9500 +240= 9740
    // realized_pnl=> +40
    // net_shares => 6
    // avg_cost => still 50 for leftover 6
    EXPECT_EQ(pf.cash_balance, 9740.0);
    EXPECT_EQ(pf.realized_pnl, 40.0);
    auto &pos = pf.positions["IBM"];
    EXPECT_EQ(pos.net_shares, 6);
    EXPECT_EQ(pos.avg_cost, 50.0);

    // if current px= 55 => unreal= (55-50)*6= 30 => total= 9740+40+30= 9810
    auto pxMap = MakePriceMap("IBM", 55.0);
    EXPECT_EQ(pf.ComputeUnrealizedPnL("IBM", 55.0), 30.0);
    EXPECT_EQ(pf.ComputeTotalValue(pxMap), 9810.0);
}

// -------------------------------------------------------------------
// 4) Full close crossing zero
//    E.g. we hold +10, then we sell 15 => crossing zero => we open new short of 5
//    We realize PnL on the 10 shares
// -------------------------------------------------------------------
TEST(PortfolioTest, CrossingZero)
{
    Portfolio pf(5000.0);

    // 1) Buy 10 @ 100 => leftover=5000-1000=4000, net_shares=+10, avg=100
    pf.Trade("MSFT", +10, 100.0);

    // 2) Sell 15 @ 90 => we close all +10, plus an extra 5 => new position= -5
    // realized => (90 - 100)* +1 * 10= -10*10= -100 => lose 100
    // new net_shares= -5 => open short 5 with avg_cost= 90
    // cash => was 4000 => cost= volume*price= (-15)*90= -1350 => 4000-(-1350)= 5350
    // realized_pnl => -100
    // new pos => net_shares= -5, avg= 90
    pf.Trade("MSFT", -15, 90.0);

    EXPECT_EQ(pf.cash_balance, 5350.0);
    EXPECT_EQ(pf.realized_pnl, -100.0);

    auto &pos = pf.positions["MSFT"];
    EXPECT_EQ(pos.net_shares, -5);
    EXPECT_EQ(pos.avg_cost, 90.0);

    // If current px= 80 => we shorted at 90 => unreal= (avg_cost - px)*abs(shares)= (90-80)*5= 50
    auto pxMap = MakePriceMap("MSFT", 80.0);
    EXPECT_EQ(pf.ComputeUnrealizedPnL("MSFT", 80.0), 50.0);

    // total => 5350 + (-100) + 50= 5300
    EXPECT_EQ(pf.ComputeTotalValue(pxMap), 5300.0);
}

// -------------------------------------------------------------------
// 5) Basic short from zero
// -------------------------------------------------------------------
TEST(PortfolioTest, BasicShort)
{
    Portfolio pf(2000.0);
    // Sell 5 shares @ 100 => from 0 => open short of -5 => cost= volume * price= (-5)*100= -500
    // cash=> 2000-(-500)= 2500 => realized=0 => net_shares= -5 => avg=100
    pf.Trade("AMZN", -5, 100.0);

    EXPECT_EQ(pf.cash_balance, 2500.0);
    EXPECT_EQ(pf.realized_pnl, 0.0);
    auto &pos = pf.positions["AMZN"];
    EXPECT_EQ(pos.net_shares, -5);
    EXPECT_EQ(pos.avg_cost, 100.0);

    // If px=110 => short is losing => unreal= (100-110)*5= -50
    auto pxMap = MakePriceMap("AMZN", 110.0);
    double unreal = pf.ComputeUnrealizedPnL("AMZN", 110.0);
    EXPECT_EQ(unreal, -50.0);
    // total= 2500+0-50=2450
    EXPECT_EQ(pf.ComputeTotalValue(pxMap), 2450.0);
}

// -------------------------------------------------------------------
// 6) Cover partial short
// -------------------------------------------------------------------
TEST(PortfolioTest, CoverPartialShort)
{
    Portfolio pf(5000.0);

    // Open short 10 @50 => leftover=5000-(-10*50)= 5500 => net_shares=-10, avg=50
    pf.Trade("NFLX", -10, 50.0);

    // now buy back 4 @ 40 => volume=+4 => partial close
    // realized => (avg_cost - price)* shares_closed= (50-40)*4= 40 => gain 40
    // leftover short= -6 => same avg=50
    // cost= 4*40= 160 => leftover cash= 5500-160= 5340
    pf.Trade("NFLX", +4, 40.0);

    EXPECT_EQ(pf.cash_balance, 5340.0);
    EXPECT_EQ(pf.realized_pnl, 40.0);

    auto &pos = pf.positions["NFLX"];
    EXPECT_EQ(pos.net_shares, -6);
    EXPECT_EQ(pos.avg_cost, 50.0);

    // if current px= 35 => unreal= (50 - 35)* 6= 15*6=90 => total=5340+40+90=5470
    auto pxMap = MakePriceMap("NFLX", 35.0);
    double unreal = pf.ComputeUnrealizedPnL("NFLX", 35.0);
    EXPECT_EQ(unreal, 90.0);
    EXPECT_EQ(pf.ComputeTotalValue(pxMap), 5470.0);
}

// -------------------------------------------------------------------
// 7) Multiple tickers, check sums
// -------------------------------------------------------------------
TEST(PortfolioTest, MultipleTickers)
{
    Portfolio pf(10000.0);

    // Trade #1: Buy 5 "AAPL" @ 200 => cost=1000 => leftover=9000 => pos(AAPL)=+5, avg=200
    pf.Trade("AAPL", +5, 200.0);

    // Trade #2: Short 10 "TSLA" @100 => leftover=9000-(-10*100)= 9000+1000= 10000 => interesting
    // net_shares(TSLA)= -10 => avg=100 => realized=0 => cash=10000
    pf.Trade("TSLA", -10, 100.0);
    EXPECT_EQ(pf.cash_balance, 10000.0);

    // If pxMap => {AAPL= 210, TSLA=90}
    std::unordered_map<std::string, double> pxMap;
    pxMap["AAPL"] = 210.0;
    pxMap["TSLA"] = 90.0;

    // AAPL => unreal= (210-200)*5= 50
    // TSLA => short => (100-90)*10= 100 => total unreal=150
    // realized=0, cash=10000 => total= 10000+0+150= 10150
    double unreal = pf.ComputeTotalUnrealizedPnL(pxMap);
    EXPECT_EQ(unreal, 150.0);
    EXPECT_EQ(pf.ComputeTotalValue(pxMap), 10150.0);
}

// End of test suite
