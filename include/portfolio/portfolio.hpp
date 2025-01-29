#ifndef PORTFOLIO_HPP
#define PORTFOLIO_HPP

#include <string>
#include <unordered_map>
#include <unordered_map>
#include "ticker_positions.hpp"

/**
 * The Portfolio class tracks:
 *  - cash_balance
 *  - realized_pnl
 *  - positions[ticker] => TickerPosition
 *
 * The 'Trade(...)' method handles a fill event of 'volume' shares at 'price':
 *   volume > 0 => buy
 *   volume < 0 => sell
 *
 * Trade(...) is only called after an actual fill (Trade) occurs in the matching engine.
 */
class Portfolio
{
public:
    // ----- Data -----
    double cash_balance; // available cash
    double realized_pnl; // accumulated realized PnL from closed trades

    // Ticker -> TickerPosition
    std::unordered_map<std::string, TickerPosition> positions;

    // ----- Constructor -----
    explicit Portfolio(double initial_cash = 0.0);

    /**
     * Execute a fill for 'volume' shares at 'price'.
     * volume > 0 => buy
     * volume < 0 => sell
     *
     * Updates:
     *   - cash_balance
     *   - positions[ticker].net_shares & avg_cost
     *   - realized_pnl
     */
    void Trade(const std::string &ticker, int volume, double price);

    /**
     * Compute the unrealized PnL for 'ticker' given 'current_price'.
     * If net_shares == 0, returns 0.
     */
    double ComputeUnrealizedPnL(const std::string &ticker, double current_price) const;

    /**
     * Sum of unrealized PnL over all tickers given a map of current prices.
     */
    double ComputeTotalUnrealizedPnL(const std::unordered_map<std::string, double> &currentPrices) const;

    /**
     * Full "mark-to-market" portfolio value =
     *    cash_balance + realized_pnl + sum of unrealized PnL
     */
    double ComputeTotalValue(const std::unordered_map<std::string, double> &currentPrices) const;
};

#endif // PORTFOLIO_HPP
