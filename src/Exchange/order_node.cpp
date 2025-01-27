#include "exchange/order_node.hpp"
#include "utils/order_type.hpp"
#include <iostream>
#include <ctime>
#include <string>

OrderNode::OrderNode(int order_id, const std::string user_id, int volume, double price, OrderType order_type, time_t timestamp,
                     const std::string ticker, OrderNode *prev, OrderNode *next)
    : order_id(order_id),
      user_id(user_id),
      volume(volume),
      price(price),
      order_type(order_type),
      timestamp(timestamp),
      ticker(ticker),
      prev(prev),
      next(next) {}
