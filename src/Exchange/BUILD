package(default_visibility = ["//visibility:public"])

cc_library(
    name = "order_node",
    srcs = ["order_node.cpp"],  # Source file
    hdrs = ["../../include/Exchange/order_node.hpp"],  # Header file
)

cc_library(
    name = "trade",
    srcs = ["trade.cpp"],
    hdrs = ["../../include/Exchange/trade.hpp"],
)

cc_library(
    name = "top_of_book",
    srcs = ["top_of_book.cpp"],
    hdrs = ["../../include/Exchange/top_of_book.hpp"],
)

cc_library(
    name = "order_type",
    hdrs = ["../../include/Utils/order_type.hpp"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "price_level_queue",
    srcs = ["price_level_queue.cpp"],
    hdrs = ["../../include/Exchange/price_level_queue.hpp"],
    deps = [":order_node"],
)

cc_library(
    name = "limit_order_book",
    srcs = ["limit_order_book.cpp"],
    hdrs = ["../../include/Exchange/limit_order_book.hpp"],
    deps = [
        ":order_node",
        ":order_type",
        ":price_level_queue",
        ":top_of_book",
        ":trade",
    ],
)
