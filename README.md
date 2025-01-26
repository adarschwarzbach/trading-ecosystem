# Little Exchange 
Following [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
Using [GTest](https://google.github.io/googletest/)

## Project Structure 

```sh
litte-exchange/
├── include/                        # Public header files 
│   ├── Exchange/
│   ├── Utils/               
│   └── ...
├── src/
│   ├── Exchange/                   # Exchange & LOB's
│   │   ├── BUILD                   # Build targets
│   │   ├── limit_order_book.cpp    # LOB implementation
│   │   └── ...
│   ├── Utils/                      # Helpers
│   └── ...
├── tests/                          # Tests   
├── .gitignore  
├── BAZEL_COMMANDS.md               # Helpful Bazel commands
├── MODULE.bazel                    # Bazel root         
├── MODULE.bazel.lock            
├── README.md  
```

## Getting started
***ToDo***

## 
[Project specific Bazel commands & notes](/BAZEL_COMMANDS.md)

## Online Resources 
- [C++ Project Structure](https://medium.com/heuristics/c-application-development-part-1-project-structure-454b00f9eddc)
- [Bazel](https://bazel.build/start/cpp)
    - [Bazel Intro](https://medium.com/@d.s.m/understanding-bazel-an-introductory-overview-0c9ddb1b1ce9)