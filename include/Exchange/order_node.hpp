#ifndef ORDER_NODE_H
#define ORDER_NODE_H
#include <ctime>
#include <string>
#include "utils/order_type.hpp"

struct OrderNode
{
    int order_id;
    std::string user_id;
    int volume;
    double price;
    OrderType order_type;
    time_t timestamp;
    std::string ticker;
    OrderNode *prev;
    OrderNode *next;

    OrderNode(int order_id,
              std::string user_id,
              int volume,
              double price,
              OrderType order_type,
              time_t timestamp,
              std::string ticker,
              OrderNode *prev = nullptr,
              OrderNode *next = nullptr);
};

#endif
