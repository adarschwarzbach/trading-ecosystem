#ifndef PRIORITY_LEVEL_QUEUE_H
#define PRIORITY_LEVEL_QUEUE_H
#include "exchange/order_node.hpp"
#include "utils/order_type.hpp"

class PriceLevelQueue
{
private:
    double price;
    OrderNode front;
    OrderNode back;
    bool has_orders;

public:
    PriceLevelQueue(double price);
    double GetPrice() const;
    void AddOrder(OrderNode &order);
    bool HasOrders() const;
    void RemoveOrder(OrderNode &order);
    // Additional methods for testing
    const OrderNode *GetFrontNext() const;
    const OrderNode *GetBackPrev() const;
    const OrderNode *GetFront() const;
    // Peak
    OrderNode &Peek();
    // Pop
    OrderNode &Pop();
};

#endif