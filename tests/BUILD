cc_test(
    name = "test_order_node",
    srcs = ["exchange/test_order_node.cpp"],
    copts = ["-Iinclude"],
    deps = [
        "//src/exchange:order_node",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_price_level_queue",
    srcs = ["exchange/test_price_level_queue.cpp"],
    copts = ["-Iinclude"],
    deps = [
        "//src/exchange:order_node",
        "//src/exchange:price_level_queue",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
