#include "portfolio/portfolio.hpp"
#include <cmath>     // for std::abs
#include <algorithm> // for std::min

Portfolio::Portfolio(double initial_cash)
    : cash_balance(initial_cash),
      realized_pnl(0.0)
{
}

// ---------------------------------------------------------------------
// Trade(...) - main method to handle a fill of 'volume' shares at 'price'
// volume > 0 => buy (cash_balance -= volume * price)
// volume < 0 => sell (cash_balance -= volume * price => effectively plus if volume<0)
//
// Then we update net_shares, avg_cost, and realized_pnl accordingly.
// ---------------------------------------------------------------------
void Portfolio::Trade(const std::string &ticker, int volume, double price)
{
    // volume>0 => buy, volume<0 => sell
    // cost from perspective of portfolio:
    double cost = price * volume;
    cash_balance -= cost;

    // Access existing position or create if none
    TickerPosition &pos = positions[ticker];
    int old_shares = pos.net_shares;
    double old_avg = pos.avg_cost;

    // If no existing position => open brand new
    if (old_shares == 0)
    {
        pos.net_shares = volume;
        pos.avg_cost = price;
        return;
    }

    // Check if we are adding in the same direction or opposite
    bool same_direction = ((old_shares > 0 && volume > 0) ||
                           (old_shares < 0 && volume < 0));

    if (same_direction)
    {
        // 1) Adding to existing position in the SAME direction
        int new_shares = old_shares + volume;

        double old_abs = std::abs((double)old_shares);
        double new_abs = std::abs((double)new_shares);
        double trade_abs = std::abs((double)volume);

        // Weighted average cost
        double old_position_cost = old_avg * old_abs;
        double new_trade_cost = price * trade_abs;
        double updated_avg = (old_position_cost + new_trade_cost) / new_abs;

        pos.net_shares = new_shares;
        pos.avg_cost = updated_avg;
    }
    else
    {
        // 2) Trading in OPPOSITE direction => partial or full close
        int shares_to_close = std::min(std::abs(old_shares), std::abs(volume));

        // sign: if old_shares>0 => we were long => realized = (price - old_avg) * shares_closed
        //       if old_shares<0 => we were short => realized = (old_avg - price) * shares_closed
        double side = (old_shares > 0) ? 1.0 : -1.0;
        double pnl_for_closed = (price - old_avg) * side * (double)shares_to_close;
        realized_pnl += pnl_for_closed;

        // Now update net_shares
        int new_shares = old_shares + volume; // volume is negative if we are selling from a long, or positive if we are buying from a short

        // Check if we remain in the same sign or cross zero
        if ((old_shares > 0 && new_shares >= 0) || (old_shares < 0 && new_shares <= 0))
        {
            // partial close: we stay on the same side or go to zero
            pos.net_shares = new_shares;
            // if new_shares != 0 => leftover old position with same avg_cost
            // if new_shares == 0 => fully closed => remove or set avg_cost=0
            if (new_shares == 0)
            {
                pos.avg_cost = 0.0;
            }
        }
        else
        {
            // We fully closed the old position (realizing that PnL)
            // AND we cross to a new position in the opposite direction
            pos.net_shares = new_shares;
            if (new_shares == 0)
            {
                // exactly closed, no new position
                pos.avg_cost = 0.0;
            }
            else
            {
                // we flip to new direction => reset avg_cost at the new price
                pos.avg_cost = price;
            }
        }
    }
}

// ---------------------------------------------------------------------
// ComputeUnrealizedPnL(ticker, current_price)
//   If net_shares>0 => unrealized = (current_price - avg_cost) * net_shares
//   If net_shares<0 => short => unrealized = (avg_cost - current_price)* abs(net_shares)
// ---------------------------------------------------------------------
double Portfolio::ComputeUnrealizedPnL(const std::string &ticker, double current_price) const
{
    auto it = positions.find(ticker);
    if (it == positions.end())
    {
        return 0.0;
    }
    const TickerPosition &pos = it->second;
    if (pos.net_shares == 0)
    {
        return 0.0;
    }

    int shares = pos.net_shares;
    double side = (shares > 0) ? 1.0 : -1.0;
    double diff = (current_price - pos.avg_cost) * side;
    return diff * std::abs((double)shares);
}

// ---------------------------------------------------------------------
// Sum the unrealized PnL for all tickers
// ---------------------------------------------------------------------
double Portfolio::ComputeTotalUnrealizedPnL(const std::unordered_map<std::string, double> &currentPrices) const
{
    double total = 0.0;
    for (auto &kv : positions)
    {
        const std::string &ticker = kv.first;
        auto itPrice = currentPrices.find(ticker);
        if (itPrice == currentPrices.end())
        {
            // no current price => skip
            continue;
        }
        double px = itPrice->second;
        total += ComputeUnrealizedPnL(ticker, px);
    }
    return total;
}

// ---------------------------------------------------------------------
// Full mark-to-market: cash + realized_pnl + sum of unrealized
// ---------------------------------------------------------------------
double Portfolio::ComputeTotalValue(const std::unordered_map<std::string, double> &currentPrices) const
{
    double unreal = ComputeTotalUnrealizedPnL(currentPrices);
    return cash_balance + realized_pnl + unreal;
}
