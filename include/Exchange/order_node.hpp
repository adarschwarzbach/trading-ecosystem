#ifndef ORDER_NODE_H
#define ORDER_NODE_H
#include <ctime>

struct OrderNode
{
    int order_id;
    int volume;
    double price;
    time_t timestamp;
    OrderNode *next;
    OrderNode *prev;

    OrderNode(int order_id, int volume, double price, time_t timestamp,
              OrderNode *prev = nullptr, OrderNode *next = nullptr);
};

#endif
