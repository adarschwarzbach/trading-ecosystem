#ifndef TOP_OF_BOOK
#define TOP_OF_BOOK

struct TopOfBook
{
    const bool book_has_top;
    const int ask_price;
    const int ask_volume;
    const int bid_price;
    const int bid_volume;

    TopOfBook(
        bool book_has_top,
        int ask_price,
        int ask_volume,
        int bid_price,
        int bid_volume);
};

#endif