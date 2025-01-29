#include "portfolio/ticker_positions.hpp"

TickerPosition::TickerPosition(int net_shares, double avg_cost)
    : net_shares(net_shares),
      avg_cost(avg_cost)
{
}
