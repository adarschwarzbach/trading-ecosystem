package(default_visibility = ["//visibility:public"])

cc_library(
    name = "ticker_positions",
    srcs = ["ticker_positions.cpp"],
    hdrs = ["//include/portfolio:ticker_positions.hpp"],
    copts = ["-Iinclude"],
)

cc_library(
    name = "portfolio",
    srcs = ["portfolio.cpp"],
    hdrs = ["//include/portfolio:portfolio.hpp"],
    copts = ["-Iinclude"],  # Allows for clean header file import
    deps = [":ticker_positions"],
)
