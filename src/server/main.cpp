#include "server/server.hpp"

int main()
{
    std::vector<std::string> tickers = {"AAPL", "GOOG", "TSLA", "MSFT", "QQQ", "TQQQ"};
    Server server(tickers);
    server.start();
    return 0;
}
