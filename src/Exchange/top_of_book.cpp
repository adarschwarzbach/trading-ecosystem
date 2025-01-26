#include "../include/Exchange/top_of_book.hpp" // update to be cleaner path with cmake/makefile

TopOfBook::TopOfBook(
    int ask_price,
    int ask_volume,
    int bid_price,
    int bid_volume)
    : ask_price(ask_price),
      ask_volume(ask_volume),
      bid_price(bid_price),
      bid_volume(bid_volume) {}
