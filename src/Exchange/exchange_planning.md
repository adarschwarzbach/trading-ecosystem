# EngineParts
Want to build:
1. Matching engine
2. Basic networking
3. Simple bots (market maker, etc.)
4. Advanced networking (udp multicast, etc.)

**portfolio**
vars: 
- Realized PNL 
- Trades
- LongPositions: {}
- ShortPositions
- LongOrders
- ShortOrders

**Exchange**
- [ ] status
vars:
- lobs: <ticker: string, LimitOrderBook>
- trades: <ticker: [trade]>


funcs:
- GetVolume(ticker, price, order_type) -> int volume
- GetTickers() -> [string symbol]
- GetTopOfBook(ticker) -> {top_of_book}

**LimitOrderBook**
- [ ] status
vars: 
- ask: heap <PriceLevelQueue> 
    - comparator, min price
- bid: heap <PriceLevelQueue>
- ticker

funcs:
- GetVolume(price, order_type)
- AddOrder(order_type, symbol, price, vol) -> (order_id || {trade})
- CancelOrder(order_id)
    - Needs to slice node out of PLQ, delete node & check if PLQ still has members 
- GetTopOfBook() -> {top_of_book}

**PriceLevelQueue**
- [ ] status
<br/>
vars:
- price 
- volume
- front
- back

funcs:
- AddOrder()


**OrderNode**
- [x] status


**Trade**
- [x] status



**ToDo**
- [ ] Probably should build my own pq for arbitrary O(log(n)) removal
- [ ] Explore multithreading to update different LOB's in parallel
- [ ] Explore inlining, compiler flags & optimizations to improve speed at runtime for Exchange -> LOB
