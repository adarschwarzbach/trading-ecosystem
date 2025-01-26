#include "../include/Exchange/trade.hpp" // update to be cleaner path with cmake/makefile
#include <string>
#include <ctime>

Trade::Trade(int trade_id,
             int price,
             int volume,
             time_t timestamp,
             const std::string &bid_user_id,
             const std::string &ask_user_id)
    : trade_id(trade_id),
      price(price),
      volume(volume),
      timestamp(timestamp),
      bid_user_id(bid_user_id),
      ask_user_id(ask_user_id) {}
