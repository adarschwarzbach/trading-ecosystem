# **Bazel Build and Test Commands**

## **Build Commands**

**Build a Single Target**
To build a specific target (e.g., `order_node`):
```bash
bazel build //src/exchange:order_node
```

## Test all

**Build All Tests**
```bash
bazel build //tests:all
```

**Run All Tests**
```bash
bazel test //tests:all
```

**Run All Tests with Verbose Output**
```bash
bazel test //tests:all --test_output=all
```

**Display Errors Only**
```bash
bazel test //tests:all --test_output=errors
```

## Specific tests

**Run a Specific Test**
```bash
bazel test //tests:test_order_node
```

**Run a Specific Test Case in a Target**
```bash
bazel test //tests:test_order_node --test_filter=OrderNodeTest.Initialization
```