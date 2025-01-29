#ifndef TICKER_POSITION
#define TICKER_POSITION

struct TickerPosition
{
    int net_shares = 0;    // + for long, - for short
    double avg_cost = 0.0; // Weighted average cost of the current net position

    TickerPosition() = default;

    TickerPosition(int net_shares, double avg_cost);
};

#endif
