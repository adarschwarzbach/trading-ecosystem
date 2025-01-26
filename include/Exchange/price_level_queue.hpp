#ifndef PRIORITY_LEVEL_QUEUE_H
#define PRIORITY_LEVEL_QUEUE_H
#include "./order_node.hpp"

class PriceLevelQueue
{
private:
    int price;
    OrderNode front;
    OrderNode back;
    bool has_orders;

public:
    PriceLevelQueue(int price);
    int GetPrice() const;
    void AddOrder(OrderNode *order);
    bool HasOrders() const;
    void UpdateHasOrders(bool state);
};

#endif