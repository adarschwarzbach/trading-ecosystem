package(default_visibility = ["//visibility:public"])

cc_library(
    name = "order_node",
    srcs = ["order_node.cpp"],
    hdrs = ["//include/exchange:order_node.hpp"],
    copts = ["-Iinclude"],  # Allows for clean header file import
    deps = ["//include/utils:order_type"],
)

cc_library(
    name = "trade",
    srcs = ["trade.cpp"],
    hdrs = ["//include/exchange:trade.hpp"],
    copts = ["-Iinclude"],
)

cc_library(
    name = "order_result",
    srcs = ["order_result.cpp"],
    hdrs = ["//include/exchange:order_result.hpp"],
    copts = ["-Iinclude"],
    deps = [":trade"],
)

cc_library(
    name = "top_of_book",
    srcs = ["top_of_book.cpp"],
    hdrs = ["//include/exchange:top_of_book.hpp"],
    copts = ["-Iinclude"],
)

cc_library(
    name = "price_level_queue",
    srcs = ["price_level_queue.cpp"],
    hdrs = ["//include/exchange:price_level_queue.hpp"],
    copts = ["-Iinclude"],
    deps = [":order_node"],
)

cc_library(
    name = "limit_order_book",
    srcs = ["limit_order_book.cpp"],
    hdrs = ["//include/exchange:limit_order_book.hpp"],
    copts = ["-Iinclude"],
    deps = [
        ":order_node",
        ":order_result",
        ":price_level_queue",
        ":top_of_book",
        ":trade",
        "//include/utils:order_type",
    ],
)
