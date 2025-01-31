import socket
import json
import threading
import time
import random

HOST = "127.0.0.1"
PORT = 8080

# Market-making parameters
SPREAD = 0.50
ORDER_SIZE = 10
BASE_PRICE = 150.0  # Default price if no market data is available

# Number of trading bots
NUM_BOTS = 10
PRINT_BOOK_INTERVAL = 3  # How often to print order book (in seconds)

def send_request(action, data={}):
    """Sends a request to the exchange server and returns JSON response."""
    try:
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect((HOST, PORT))
        data["action"] = action
        message = json.dumps(data)
        client.sendall(message.encode())

        response = client.recv(4096).decode()
        client.close()

        try:
            return json.loads(response)
        except json.JSONDecodeError:
            return {"error": "Invalid response"}
    except Exception as e:
        return {"error": str(e)}

def register_user(user_id):
    """Registers a user on the exchange."""
    return send_request("register_user", {"user_id": user_id})

def place_order(user_id, order_type, price, volume, ticker):
    """Places an order (buy/sell) on the given ticker."""
    if price <= 0: return None
    response = send_request("handle_order", {
        "user_id": user_id,
        "order_type": order_type,
        "price": price,
        "volume": volume,
        "ticker": ticker
    })
    print(f"[ORDER] {user_id} placed {'BID' if order_type == 1 else 'ASK'} {volume} @ {price} on {ticker} | Response: {response}")

    if response and "order_id" in response and response["order_id"] != -1:
        return response["order_id"]
    else:
        print(f"[ERROR] Order placement failed: {response}")
        return None


def cancel_order(user_id, order_id, ticker):
    """Cancels an existing order."""
    response = send_request("cancel_order", {
        "user_id": user_id,
        "ticker": ticker,
        "order_id": order_id
    })
    if response and response.get("success"):
        print(f"[CANCEL] {user_id} canceled order {order_id} on {ticker}")
    return response if response else {"error": "No response from server"}

def get_top_of_book(ticker):
    """Gets the current best bid/ask prices for a ticker."""
    response = send_request("get_top_of_book", {"ticker": ticker})
    if response and "bid_price" in response and "ask_price" in response:
        pretty_json = json.dumps(response, indent=4)
        print(f"[ORDER BOOK] {ticker} \n BID: {response['bid_price']} ({response['bid_volume']}) \nASK: {response['ask_price']} ({response['ask_volume']})")
    return response if response else {"error": "No response from server"}

def get_tickers():
    """Retrieves all tickers available for trading."""
    response = send_request("get_tickers")
    tickers = response.get("tickers", []) if response else []
    print(f"[*] Available tickers: {tickers}")
    return tickers

def market_maker(user_id, ticker):
    """Continuously places buy and sell orders around the mid-price."""
    while True:
        try:
            top_of_book = get_top_of_book(ticker)
            if top_of_book and "bid_price" in top_of_book and "ask_price" in top_of_book:
                mid_price = (top_of_book["bid_price"] + top_of_book["ask_price"]) / 2
            else:
                mid_price = BASE_PRICE

            if mid_price < 5:
                mid_price = 100 + random.randint(-20, 20)


            bid_price = round(mid_price - SPREAD, 2)
            ask_price = round(mid_price + SPREAD, 2)

            order1 = place_order(user_id, 1, bid_price, ORDER_SIZE, ticker)
            order2 = place_order(user_id, 0, ask_price, ORDER_SIZE, ticker)
            
            time.sleep(0.5)

            if order1 and "order_id" in order1 and order1["order_id"] != -1:
                cancel_order(user_id, order1["order_id"], ticker)
            if order2 and "order_id" in order2 and order2["order_id"] != -1:
                cancel_order(user_id, order2["order_id"], ticker)
        
        except Exception as e:
            print(f"[Market Maker {user_id} - {ticker}] Error: {e}")

def start_market_making():
    """Registers users and starts market-making bots for all tickers."""
    print("[*] Fetching available tickers...")
    tickers = get_tickers()
    if not tickers:
        print("[ERROR] No tickers found. Exiting.")
        return
    
    print("[*] Registering users...")
    for i in range(NUM_BOTS):
        register_user(f"bot_{i}")
    
    print("[*] Starting market-making bots...")
    threads = []
    for i in range(10):
        print("---Hit------\n\n\n")
        for i, ticker in enumerate(tickers):
            t = threading.Thread(target=market_maker, args=(f"bot_mm_{i}", ticker))
            t.start()
            threads.append(t)
        
        for t in threads:
            t.join()

# Run market making bots
if __name__ == "__main__":
    start_market_making()
