#include "Exchange/price_level_queue.hpp"
#include <ctime>

PriceLevelQueue::PriceLevelQueue(int price)
    : price(price),
      has_orders(false),
      front(-1, "dummy_front", 0, 0.0, time(nullptr), "dummy", nullptr, nullptr), // Initialize dummy front
      back(-1, "dummy_back", 0, 0.0, time(nullptr), "dummy", nullptr, nullptr)    // Initialize dummy back
{
    front.next = &back;
    back.prev = &front;
}

int PriceLevelQueue::GetPrice() const
{
    return price;
}

void PriceLevelQueue::AddOrder(OrderNode *order)
{
    if (!order)
        return;

    has_orders = true;

    order->prev = back.prev;
    order->next = &back;

    if (back.prev)
    {
        back.prev->next = order;
    }
    back.prev = order;
}

bool PriceLevelQueue::HasOrders() const
{
    return has_orders;
}

void PriceLevelQueue::UpdateHasOrders(bool state)
{
    has_orders = state;
}