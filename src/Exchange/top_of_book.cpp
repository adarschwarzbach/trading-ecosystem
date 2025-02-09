#include "exchange/top_of_book.hpp"

TopOfBook::TopOfBook(
    bool book_has_top,
    int ask_price,
    int ask_volume,
    int bid_price,
    int bid_volume)
    : book_has_top(book_has_top),
      ask_price(ask_price),
      ask_volume(ask_volume),
      bid_price(bid_price),
      bid_volume(bid_volume) {}
