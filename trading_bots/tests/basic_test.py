import socket
import json
import time

HOST = "127.0.0.1"
PORT = 8080

def send_request(action, data={}):
    """Helper function to send JSON request and receive response from server."""
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
        return {"error": "Invalid response from server", "response": response}

# ---- TEST FUNCTIONS ----
def test_register_users():
    print("\n[TEST] Registering Users...")
    assert send_request("register_user", {"user_id": "trader1"})["success"]
    assert send_request("register_user", {"user_id": "trader2"})["success"]
    assert not send_request("register_user", {"user_id": "trader1"})["success"], "Duplicate user registration should fail"

def test_get_tickers():
    print("\n[TEST] Retrieving allowed tickers...")
    response = send_request("get_tickers")
    assert "tickers" in response, f"Unexpected response: {response}"
    print("Available tickers:", response["tickers"])

def test_place_order():
    print("\n[TEST] Placing orders...")
    order1 = send_request("handle_order", {
        "user_id": "trader1",
        "order_type": 1,  # Buy
        "volume": 10,
        "price": 150.0,
        "ticker": "AAPL"
    })
    assert "order_id" in order1, f"Failed to place order: {order1}"
    return order1["order_id"]

def test_get_top_of_book():
    print("\n[TEST] Retrieving top of book...")
    response = send_request("get_top_of_book", {"ticker": "AAPL"})
    assert "bid_price" in response and "ask_price" in response, f"Unexpected response: {response}"
    print("Top of book:", response)

def test_cancel_order(order_id):
    print(f"\n[TEST] Cancelling order {order_id}...")
    response = send_request("cancel_order", {"ticker": "AAPL", "order_id": order_id})
    assert response["success"], f"Failed to cancel order: {response}"

def test_get_trades():
    print("\n[TEST] Retrieving previous trades...")
    response = send_request("get_previous_trades", {"ticker": "AAPL", "num_previous_trades": 5})
    assert "trades" in response, f"Unexpected response: {response}"
    print("Previous trades:", response["trades"])

def test_get_trades_by_user():
    print("\n[TEST] Retrieving trades by user trader1...")
    response = send_request("get_trades_by_user", {"user_id": "trader1"})
    assert "trades" in response, f"Unexpected response: {response}"
    print("Trades for trader1:", response["trades"])

# ---- RUN TEST SUITE ----
if __name__ == "__main__":
    test_register_users()
    test_get_tickers()
    order_id = test_place_order()
    time.sleep(1)  # Allow time for order book update
    test_get_top_of_book()
    test_cancel_order(order_id)
    time.sleep(1)  # Allow time for order book update
    test_get_trades()
    test_get_trades_by_user()

    print("\n[ALL TESTS COMPLETED SUCCESSFULLY]")
