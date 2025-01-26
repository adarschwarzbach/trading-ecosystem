#ifndef TRADE
#define TRADE
#include <ctime>
#include <string>

struct Trade
{
    const int trade_id;
    const int price;
    const int volume;
    const time_t timestamp;
    const std::string bid_user_id;
    const std::string ask_user_id;

    Trade(int trade_id, int price, int volume, time_t timestamp,
          const std::string &bid_user_id, const std::string &ask_user_id);
};

#endif