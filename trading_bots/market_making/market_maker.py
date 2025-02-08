import socket
import json
import threading
import time
import random

# Server connection details
HOST = "127.0.0.1"
PORT = 8080

volume_by_ticker = {}



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

        return json.loads(response) if response else {"error": "Empty response"}
    except Exception as e:
        return {"error": str(e)}

def get_top_of_book(ticker):
    """Retrieves best bid and ask prices."""
    response = send_request("get_top_of_book", {"ticker": ticker})
    if response.get("error"):
        print(f"[ERROR] Failed to fetch top of book for {ticker}: {response['error']}")
        return None
    
    bid_price = response.get("bid_price", 0)
    ask_price = response.get("ask_price", 0)
    
    print(f"[ORDER BOOK] {ticker} | BID: {bid_price} | ASK: {ask_price}")
    return bid_price, ask_price


def place_order(user_id, order_type, price, volume, ticker):
    """Places a sell (1) or buy (0) order ensuring it does not immediately execute."""
    response = send_request("handle_order", {
        "user_id": user_id,
        "order_type": order_type,
        "price": round(price, 2),
        "volume": volume,
        "ticker": ticker
    })

    if response and response.get("order_id") != -1:
        order_id = response["order_id"]
        print(f"[ORDER] {user_id} {'BID' if order_type == 1 else 'ASK'} {volume} @ {price} on {ticker} (Order ID: {order_id})")
        return order_id
    else:
        print(f"[ERROR] Order placement failed for {user_id} on {ticker}: {response}")
        return None

def cancel_order(user_id, order_id, ticker):
    """Cancels an existing order."""
    response = send_request("cancel_order", {"user_id": user_id, "ticker": ticker, "order_id": order_id})
    if response.get("success", False):
        print(f"[CANCEL] {user_id} canceled order {order_id} on {ticker}")
    return response.get("success", False)

def market_maker(user_id, ticker):
    """Market-making bot that places and maintains orders in the book."""
    print(f"[START] Market Maker {user_id} started for {ticker}")

    while True:
        try:
            top_of_book = get_top_of_book(ticker)
            place_order(user_id, 1, 105, 10, ticker)
            place_order(user_id, 0, 100, 10, ticker)
            time.sleep(5)

        except Exception as e:
            print(f"[ERROR] Market Maker {user_id} - {ticker}: {e}")

def start_market_making():
    """Initialize bots and start market making."""
    print("[*] Fetching available tickers...")
    tickers = send_request("get_tickers").get("tickers", [])

    if not tickers:
        print("[ERROR] No tickers found. Exiting.")
        return

    print("[*] Registering bots...")
    for i in range(len(tickers)):
        send_request("register_user", {"user_id": f"bot_{i}"})

    print("[*] Starting market-making bots...")
    threads = []
    for i in range(len(tickers)):
        bot_id = f"bot_mm_{i}"
        ticker = tickers[i] # Assign bot to ticker index
        t = threading.Thread(target=market_maker, args=(bot_id, ticker))
        t.start()
        threads.append(t)

    for t in threads:
        t.join()

if __name__ == "__main__":
    start_market_making()
