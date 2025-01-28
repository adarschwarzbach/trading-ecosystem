# trading-ecosystem
- Following [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- Using [GTest](https://google.github.io/googletest/) for testing
- [Bazel](https://bazel.build/start/cpp) for build
- [Benchmark](https://github.com/google/benchmark) for performance testing
- [Beej's](https://beej.us/guide/bgnet/pdf/bgnet_usl_c_1.pdf) for networking tipcs

## Project Structure 

```sh
trading-ecosystem/
├── include/                        # Public header files 
│   ├── exchange/
│   ├── utils/               
│   └── ...
├── src/
│   ├── exchange/                   # Exchange & LOB's
│   │   ├── BUILD                   # Build targets
│   │   ├── limit_order_book.cpp    # LOB implementation
│   │   └── ...
│   ├── utils/                      # Helpers
│   └── ...
├── tests/                          # Tests   
├── .gitignore  
├── BAZEL_COMMANDS.md               # Helpful Bazel commands
├── MODULE.bazel                    # Bazel root         
├── MODULE.bazel.lock            
├── README.md  
```



## Internal Resources

[Project specific Bazel commands & notes](/BAZEL_COMMANDS.md)