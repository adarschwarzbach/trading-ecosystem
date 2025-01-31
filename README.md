# Trading Ecosystem
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) for style
- [GTest](https://google.github.io/googletest/) for testing (unit & mocks)
- [Bazel](https://bazel.build/start/cpp) for build
- [Benchmark](https://github.com/google/benchmark) for performance testing
- [Beej's](https://beej.us/guide/bgnet/pdf/bgnet_usl_c_1.pdf) for networking/socket tips

## Project Structure 

```sh
trading-ecosystem/
├── include/                        # Public header files      
│   └── ...
├── src/
│   ├── exchange/                   # Exchange & LOB
│   │   ├── BUILD                   # Build targets
│   │   ├── limit_order_book.cpp    # LOB implementation
│   │   └── ...
│   ├── portfolio/                  # User portfolio (positions, realized/unrealized PnL)
│   │   └── ...
│   ├── risk/                       # Risk management (margin, lending pool, etc.)
│   │   └── ...
│   ├── server/                     # Exchange server & networking (sockets)
│   │   └── ...
│   ├── utils/                      # Helpers
│   └── ...
├── trading_bots/                   # Trading bots (predominantly python)
│   ├── market_making/                       
│   │   └── ...
│   ├── tests/                       
│   │   └── ...
│   ├── utils/                       
│   │   └── ...
├── tests/                          # Exchange tests (GTest)   
├── .gitignore  
├── BAZEL_COMMANDS.md               # Helpful Bazel commands
├── MODULE.bazel                    # Bazel root         
├── MODULE.bazel.lock            
├── README.md  
```



## Internal Resources

[Project specific Bazel commands & notes](/BAZEL_COMMANDS.md)