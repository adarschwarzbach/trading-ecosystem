#include "exchange/price_level_queue.hpp"
#include <ctime>
#include <stdexcept>
#include <iostream> // For logging

PriceLevelQueue::PriceLevelQueue(double price)
    : price(price),
      front(-1, "dummy_front", 0, 0.0, OrderType::ASK, time(nullptr), "dummy", nullptr, nullptr), // Initialize dummy front
      back(-1, "dummy_back", 0, 0.0, OrderType::ASK, time(nullptr), "dummy", nullptr, nullptr),   // Initialize dummy back
      has_orders(false)
{
    front.next = &back;
    back.prev = &front;
}

double PriceLevelQueue::GetPrice() const
{
    return price;
}

void PriceLevelQueue::AddOrder(OrderNode &order)
{
    if (order.price != price)
    {
        throw std::runtime_error("Order price does not match PriceLevelQueue price.");
    }

    has_orders = true;

    order.prev = back.prev;
    order.next = &back;

    if (back.prev)
    {
        back.prev->next = &order;
    }
    back.prev = &order;
}

bool PriceLevelQueue::HasOrders() const
{
    return has_orders;
}

void PriceLevelQueue::RemoveOrder(OrderNode &order)
{
    std::cout << "[PLQ] Trying to cancel order: " << order.order_id << std::endl;

    // Check if the order is actually part of this PriceLevelQueue
    if (order.prev == nullptr && order.next == nullptr)
    {
        std::cerr << "[ERROR] Order " << order.order_id << " is already removed or invalid." << std::endl;
        return;
    }

    OrderNode *earlier_node_ptr = order.prev;
    OrderNode *ltr_node_ptr = order.next;

    if (earlier_node_ptr)
    {
        earlier_node_ptr->next = ltr_node_ptr;
    }
    if (ltr_node_ptr)
    {
        ltr_node_ptr->prev = earlier_node_ptr;
    }

    // Clean up dangling references
    order.prev = nullptr;
    order.next = nullptr;

    std::cout << "[PLQ] Order " << order.order_id << " removed successfully." << std::endl;

    // Check if PLQ is empty
    if (front.next == &back)
    {
        has_orders = false;
        std::cout << "[PLQ] PriceLevelQueue is now empty." << std::endl;
    }
}

const OrderNode *PriceLevelQueue::GetFrontNext() const
{
    return front.next;
}

const OrderNode *PriceLevelQueue::GetBackPrev() const
{
    return back.prev;
}

const OrderNode *PriceLevelQueue::GetFront() const
{
    return &front;
}

OrderNode &PriceLevelQueue::Peek()
{
    if (front.next == &back)
    {
        throw std::runtime_error("Queue is empty. No order to peak.");
    }
    return *front.next;
}

OrderNode &PriceLevelQueue::Pop()
{
    if (front.next == &back)
    {
        throw std::runtime_error("Queue is empty. Cannot pop.");
    }

    // Get the actual front node
    OrderNode *node_to_remove = front.next;

    // Update links to remove the node
    front.next = node_to_remove->next;
    node_to_remove->next->prev = &front;

    // Clean up dangling references in the removed node
    node_to_remove->next = nullptr;
    node_to_remove->prev = nullptr;

    // Check if the queue is now empty
    if (front.next == &back)
    {
        has_orders = false;
    }

    return *node_to_remove;
}
