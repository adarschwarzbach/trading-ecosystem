package(default_visibility = ["//visibility:public"])

cc_library(
    name = "server",
    srcs = ["server.cpp"],
    hdrs = ["//include/server:server.hpp"],
    copts = [
        "-I$(GENDIR)/external/nlohmann_json/include",
        "-Iexternal/nlohmann_json/include",
        "-Iinclude",
    ],
    deps = [
        "//src/exchange",
        "@nlohmann_json//:json",
    ],
)

cc_binary(
    name = "server_main",
    srcs = ["main.cpp"],
    copts = [
        "-Iinclude",
    ],
    deps = [":server"],
)
