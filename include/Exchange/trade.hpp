#ifndef TRADE
#define TRADE
#include <ctime>
#include <string>

/**
 * @brief Struct to represent a trade
 */
struct Trade
{
    const int trade_id;
    const int price;
    const int volume;
    const time_t timestamp;
    const std::string bid_user_id;
    const std::string ask_user_id;

    /**
     * Trade constructor
     *
     * @param trade_id id of the trade
     * @param price execution price
     * @param volume vol
     * @param timestamp execution time
     * @param bid_user_id bid user
     * @param ask_user_id ask user
     *
     */
    Trade(int trade_id, int price, int volume, time_t timestamp,
          const std::string &bid_user_id, const std::string &ask_user_id);
};

#endif