# EngineParts
Want to build:
1. Matching engine
2. Basic networking
3. Simple bots (market maker, etc.)
4. Advanced networking (udp multicast, etc.)


**Exchange**
- <symbol: string, LimitOrderBook>


- funcs
    - GetVolume(symbol, price, type: buy/sell)
    - AddOrder(type: bid/ask, symbol, price, vol) -> (order_id || {trade})
    - CancelOrder(order_id)
    - 

**PriceLevelQueue**

**OrderNode**

Probably should build my own pq for arbitrary O(log(n)) removal