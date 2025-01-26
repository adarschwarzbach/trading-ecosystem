#include <iostream>
#include "../include/Exchange/order_node.hpp"
#include "../include/Exchange/price_level_queue.hpp"

int main()
{
    // Create a price level queue
    PriceLevelQueue queue(100);

    // Create some orders
    time_t now = time(nullptr);
    OrderNode *order1 = new OrderNode(1, "user123", 10, 100.0, now);
    OrderNode *order2 = new OrderNode(2, "user456", 20, 100.0, now);

    // Add orders to the queue
    queue.AddOrder(order1);
    queue.AddOrder(order2);

    // Print the price and volume of the queue
    std::cout << "Price Level: " << queue.GetPrice() << "\n";

    // Clean up dynamically allocated memory
    delete order1;
    delete order2;

    return 0;
}
