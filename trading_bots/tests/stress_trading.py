import socket
import json
import threading
import time
import random

HOST = "127.0.0.1"
PORT = 8080
TICKER = "AAPL"

# Market-making parameters
SPREAD = 0.50
ORDER_SIZE = 10
BASE_PRICE = 150.0

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

def place_order(user_id, order_type, price, volume):
    """Places an order (buy/sell) and logs failures."""
    if price <= 0: return
    side = 'BID' if order_type == 1 else 'ASK'
    response = send_request("handle_order", {
        "user_id": user_id,
        "order_type": order_type,
        "price": price,
        "volume": volume,
        "ticker": TICKER
    })

    if response['order_id'] == "-1" or response['order_id'] == -1:
        print(f"[TRADES EXECUTED] for {TICKER} ----")
        for trade in response["trades"]:
            print(f"Price: {trade["price"]} | Volume: {trade["volume"]}")
            print(f"Selling user: {trade["ask_user_id"]}")
            print(f"Buying user: {trade["bid_user_id"]}")
            print('--------')

    elif  "order_id" not in response:
        print(f"[ERROR] Order failed to place: {response}")
    else:
        print(f"[ORDER] {user_id} {'BID' if order_type == 1 else 'ASK'} {volume} @ {price} | Order ID: {response['order_id']}")

    return response

def cancel_order(user_id, order_id):
    """Cancels an existing order and logs success/failure."""
    response = send_request("cancel_order", {
        "user_id": user_id,
        "ticker": TICKER,
        "order_id": order_id
    })

    if response["success"]:
        print(f"[CANCEL] {user_id} canceled order {order_id}")
    else:
        print(f"[ERROR] Failed to cancel order {order_id}: {response}")

def get_top_of_book():
    """Gets the current best bid/ask prices."""
    return send_request("get_top_of_book", {"ticker": TICKER})

def print_order_book():
    """Prints a clean order book snapshot every few seconds."""
    while True:
        time.sleep(PRINT_BOOK_INTERVAL)
        top_of_book = get_top_of_book()
        print("\n", top_of_book, "\n")

        if "bid_price" in top_of_book and "ask_price" in top_of_book:
            print("\n--- Order Book Snapshot ---")
            print(f"📈 BID: {top_of_book['bid_volume']} @ {top_of_book['bid_price']}")
            print(f"📉 ASK: {top_of_book['ask_volume']} @ {top_of_book['ask_price']}")
            print("---------------------------\n")
        else:
            print("[ERROR] Could not fetch order book:", top_of_book)

def market_maker(user_id):
    """Continuously places buy and sell orders around the mid-price."""
    while True:
        try:
            top_of_book = get_top_of_book()
            if "bid_price" in top_of_book and "ask_price" in top_of_book:
                mid_price = (top_of_book["bid_price"] + top_of_book["ask_price"]) / 2
            else:
                mid_price = BASE_PRICE  # Default starting price

            bid_price = round(mid_price - SPREAD, 2)
            ask_price = round(mid_price + SPREAD, 2)

            # Place bid and ask orders
            order1 = place_order(user_id, 1, bid_price, ORDER_SIZE)  # Buy order
            order2 = place_order(user_id, 0, ask_price, ORDER_SIZE)  # Sell order

            # Short delay to simulate real-world trading frequency
            time.sleep(0.5)

            # Cancel old orders if placed properly
            if "order_id" in order1 and order1["order_id"] != -1:
                cancel_order(user_id, order1["order_id"])
            if "order_id" in order2 and order2["order_id"] != -1:
                cancel_order(user_id, order2["order_id"])

        except Exception as e:
            print(f"[Market Maker] Error: {e}")

def adversarial_trader(user_id):
    """Places and cancels orders rapidly to disrupt the order book."""
    while True:
        try:
            price = round(BASE_PRICE + random.uniform(-1, 1), 2)
            volume = random.randint(1, 20)

            # Rapidly place and cancel an order
            response = place_order(user_id, random.choice([0, 1]), price, volume)
            if "order_id" in response and response["order_id"] != -1:
                cancel_order(user_id, response["order_id"])

            time.sleep(random.uniform(0.1, 0.2))  # Fast execution time to flood the book

        except Exception as e:
            print(f"[Adversarial Trader] Error: {e}")

def high_frequency_trader(user_id):
    """Places very quick orders based on market movements."""
    while True:
        try:
            top_of_book = get_top_of_book()
            if "bid_price" in top_of_book and "ask_price" in top_of_book:
                bid_price = top_of_book["bid_price"]
                ask_price = top_of_book["ask_price"]

                # If spread is large, place an aggressive market order
                if ask_price - bid_price > 1.0:
                    volume = random.randint(5, 15)
                    place_order(user_id, 1, ask_price, volume)  # Market buy
                    place_order(user_id, 0, bid_price, volume)  # Market sell

            time.sleep(0.2)  # Short delay to simulate rapid trading

        except Exception as e:
            print(f"[High-Frequency Trader] Error: {e}")

def start_stress_test():
    """Registers users and starts market-making & adversarial bots."""
    print("[*] Registering users...")
    for i in range(NUM_BOTS):
        register_user(f"bot_{i}")

    print("[*] Starting market-making and stress bots...")
    threads = []

    # Start order book printer
    t = threading.Thread(target=print_order_book)
    t.daemon = True
    t.start()
    threads.append(t)

    # Start market makers
    for i in range(3):
        t = threading.Thread(target=market_maker, args=(f"bot_mm_{i}",))
        t.start()
        threads.append(t)

    # Start adversarial traders
    for i in range(3, 6):
        t = threading.Thread(target=adversarial_trader, args=(f"bot_adv_{i}",))
        t.start()
        threads.append(t)

    # Start high-frequency traders
    for i in range(6, NUM_BOTS):
        t = threading.Thread(target=high_frequency_trader, args=(f"bot_hft_{i}",))
        t.start()
        threads.append(t)

    # Keep running
    for t in threads:
        t.join()

# Run stress test
if __name__ == "__main__":
    start_stress_test()
