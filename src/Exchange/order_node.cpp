#include "../include/Exchange/order_node.hpp" // update to be cleaner path with cmake/makefile
#include <iostream>

OrderNode::OrderNode(int order_id, int volume, double price, time_t timestamp,
                     OrderNode *prev, OrderNode *next)
    : order_id(order_id), volume(volume), price(price), timestamp(timestamp), prev(prev), next(next) {}