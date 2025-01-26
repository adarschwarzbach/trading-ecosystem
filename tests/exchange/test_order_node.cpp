#include "exchange/order_node.hpp"
#include <gtest/gtest.h>
#include <ctime>

// Test OrderNode initialization
TEST(OrderNodeTest, Initialization)
{
    time_t now = time(nullptr);
    OrderNode node(1, "user1", 100, 50.0, now, "AAPL", nullptr, nullptr);

    EXPECT_EQ(node.order_id, 1);
    EXPECT_EQ(node.user_id, "user1");
    EXPECT_EQ(node.volume, 100);
    EXPECT_DOUBLE_EQ(node.price, 50.0);
    EXPECT_EQ(node.ticker, "AAPL");
    EXPECT_EQ(node.prev, nullptr);
    EXPECT_EQ(node.next, nullptr);
}

TEST(OrderNodeTest, ForwardLinking)
{
    time_t now = time(nullptr);
    OrderNode node1(1, "user1", 100, 50.0, now, "AAPL", nullptr, nullptr);
    OrderNode node2(2, "user2", 200, 51.0, now, "AAPL", &node1, nullptr);

    node1.next = &node2;

    // Verify node1's next points to node2
    EXPECT_EQ(node1.next, &node2);
    EXPECT_EQ(node2.prev, &node1);

    // Verify node2's details
    EXPECT_EQ(node2.order_id, 2);
    EXPECT_EQ(node2.user_id, "user2");
}

TEST(OrderNodeTest, BackwardLinking)
{
    time_t now = time(nullptr);
    OrderNode node1(1, "user1", 100, 50.0, now, "AAPL", nullptr, nullptr);
    OrderNode node2(2, "user2", 200, 51.0, now, "AAPL", &node1, nullptr);

    node1.next = &node2;

    // Verify node2's prev points to node1
    EXPECT_EQ(node2.prev, &node1);
    EXPECT_EQ(node1.next, &node2);

    // Traverse backward
    EXPECT_EQ(node2.prev->order_id, 1);
}

TEST(OrderNodeTest, MultiNodeTraversal)
{
    time_t now = time(nullptr);
    OrderNode node1(1, "user1", 100, 50.0, now, "AAPL", nullptr, nullptr);
    OrderNode node2(2, "user2", 200, 51.0, now, "AAPL", &node1, nullptr);
    OrderNode node3(3, "user3", 300, 52.0, now, "AAPL", &node2, nullptr);

    node1.next = &node2;
    node2.next = &node3;

    // Verify forward traversal
    EXPECT_EQ(node1.next, &node2);
    EXPECT_EQ(node2.next, &node3);

    // Verify backward traversal
    EXPECT_EQ(node3.prev, &node2);
    EXPECT_EQ(node2.prev, &node1);

    // Forward traversal through values
    EXPECT_EQ(node1.next->next->order_id, 3);

    // Backward traversal through values
    EXPECT_EQ(node3.prev->prev->order_id, 1);
}

TEST(OrderNodeTest, InsertBetweenNodes)
{
    time_t now = time(nullptr);
    OrderNode node1(1, "user1", 100, 50.0, now, "AAPL", nullptr, nullptr);
    OrderNode node3(3, "user3", 300, 52.0, now, "AAPL", &node1, nullptr);

    node1.next = &node3;

    // Insert node2 between node1 and node3
    OrderNode node2(2, "user2", 200, 51.0, now, "AAPL", &node1, &node3);
    node1.next = &node2;
    node3.prev = &node2;

    // Verify linking
    EXPECT_EQ(node1.next, &node2);
    EXPECT_EQ(node2.next, &node3);
    EXPECT_EQ(node3.prev, &node2);
    EXPECT_EQ(node2.prev, &node1);
}
