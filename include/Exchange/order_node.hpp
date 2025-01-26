#ifndef ORDER_NODE_H
#define ORDER_NODE_H
#include <ctime>
#include <string>

struct OrderNode
{
    const int order_id;
    const std::string user_id;
    const int volume;
    const double price;
    const time_t timestamp;
    const std::string ticker;
    OrderNode *next;
    OrderNode *prev;

    OrderNode(int order_id, const std::string user_id, int volume, double price, time_t timestamp, const std::string ticker,
              OrderNode *prev = nullptr, OrderNode *next = nullptr);
};

#endif
