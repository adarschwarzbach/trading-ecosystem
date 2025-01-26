#include "exchange/price_level_queue.hpp"
#include "exchange/order_node.hpp"
#include <gtest/gtest.h>
#include <ctime>
#include <string>
#include <stdexcept> // For std::runtime_error
#include <iostream>  // For logging

TEST(PriceLevelQueueTest, Initialization)
{
    PriceLevelQueue queue(1.0);

    EXPECT_FLOAT_EQ(queue.GetPrice(), 1.0);
    EXPECT_EQ(queue.HasOrders(), false);
}

TEST(PriceLevelQueueTest, AddOrder)
{
    PriceLevelQueue queue(1.0);

    time_t now = time(nullptr);
    OrderNode node(1, "user1", 100, 1.0, now, "AAPL", nullptr, nullptr);

    queue.AddOrder(node);

    EXPECT_EQ(queue.HasOrders(), true);
}

TEST(PriceLevelQueueTest, AddErrorWithWrongPrice)
{
    PriceLevelQueue queue(1.0);

    time_t now = time(nullptr);
    OrderNode error_node(1, "user1", 100, 50.0, now, "AAPL", nullptr, nullptr);

    EXPECT_THROW(queue.AddOrder(error_node), std::runtime_error);
}

TEST(PriceLevelQueueTest, RemoveOrderAndVerifyState)
{
    PriceLevelQueue queue(1.0);

    time_t now = time(nullptr);
    OrderNode node(1, "user1", 100, 1.0, now, "AAPL", nullptr, nullptr);

    // Add the order and store the state of the front and back nodes
    queue.AddOrder(node);

    const OrderNode *initial_front_next = queue.GetFrontNext(); // Store function result
    const OrderNode *initial_back_prev = queue.GetBackPrev();   // Store function result

    // Ensure the order was added correctly
    EXPECT_EQ(queue.HasOrders(), true);
    EXPECT_EQ(initial_front_next, &node); // Front's next should be the added order
    EXPECT_EQ(initial_back_prev, &node);  // Back's prev should be the added order

    // Remove the order
    queue.RemoveOrder(node);

    // Store the state of the front and back nodes after removal
    const OrderNode *back_prev = queue.GetBackPrev(); // Store function result
    const OrderNode *front_node = queue.GetFront();

    // Ensure the queue is empty and front/back nodes are updated correctly
    EXPECT_EQ(front_node, back_prev); // Back's prev should point to Front node
    EXPECT_EQ(queue.HasOrders(), false);
}

TEST(PriceLevelQueueTest, TwoOrdersOrdering)
{
    PriceLevelQueue queue(1.0);

    time_t now = time(nullptr);
    OrderNode order_one(1, "user1", 100, 1.0, now, "AAPL", nullptr, nullptr);
    OrderNode order_two(1, "uder2", 100, 1.0, now, "AAPL", nullptr, nullptr);

    queue.AddOrder(order_one);
    EXPECT_EQ(queue.HasOrders(), true);
    queue.AddOrder(order_two);
    EXPECT_EQ(queue.HasOrders(), true);

    // Order one comes before order two
    EXPECT_EQ(order_one.next, &order_two);
    EXPECT_EQ(order_two.prev, &order_one);
}

TEST(PriceLevelQueueTest, RemoveMiddleOrder)
{
    PriceLevelQueue queue(1.0);

    time_t now = time(nullptr);
    OrderNode order_one(1, "user1", 100, 1.0, now, "AAPL", nullptr, nullptr);
    OrderNode order_two(1, "user2", 100, 1.0, now, "AAPL", nullptr, nullptr);
    OrderNode order_three(1, "user3", 100, 1.0, now, "AAPL", nullptr, nullptr);

    queue.AddOrder(order_one);
    EXPECT_EQ(queue.HasOrders(), true);
    queue.AddOrder(order_two);
    queue.AddOrder(order_three);

    // Expect [o1] <-> [o2] <-> [o3]
    EXPECT_EQ(order_one.next, &order_two);
    EXPECT_EQ(order_two.next, &order_three);
    EXPECT_EQ(order_three.prev, &order_two);
    EXPECT_EQ(order_two.prev, &order_one);

    queue.RemoveOrder(order_two); // Remove order 2

    // Expect [o1] <-> [o3]
    EXPECT_EQ(order_one.next, &order_three);
    EXPECT_EQ(order_three.prev, &order_one);

    // Check for dangling reference
    EXPECT_EQ(order_two.next, nullptr);
    EXPECT_EQ(order_two.prev, nullptr);
}