# EngineParts
Want to build:
1. Matching engine
2. Basic networking
3. Simple bots (market maker, etc.)
4. Advanced networking (udp multicast, etc.)


**Exchange**
vars:
- <ticker: string, LimitOrderBook>

funcs:
- GetVolume(ticker, price, order_type) -> int volume
- GetTickers() -> [string symbol]
- GetTopOfBook(ticker) -> {top_of_book}

**LimitOrderBook**
vars: 
- ask: heap <PriceLevelQueue> 
    - comparator, min price
- bid: heap <PriceLevelQueue>

funcs:
- GetVolume(price, order_type)
- AddOrder(order_type, symbol, price, vol) -> (order_id || {trade})
- CancelOrder(order_id)
- GetTopOfBook() -> {top_of_book}

**PriceLevelQueue**
vars:
- price 
- volume
- front
- back

funcs:
- AddOrder()



**OrderNode**

funs:
- CancelOrder() 


**ToDo**
- Probably should build my own pq for arbitrary O(log(n)) removal
- Explore multithreading to update different LOB's in parallel
- Explore inlining and optimizations to improve speed at runtime for Exchange -> LOB
