import socket
import json

HOST = "127.0.0.1"
PORT = 8080

def send_request(action, data={}):
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client.connect((HOST, PORT))
    data["action"] = action
    message = json.dumps(data)
    client.sendall(message.encode())
    response = client.recv(2048)
    print("Server response:", response.decode())
    client.close()

# Get tickers
send_request("get_tickers")

# Get top of book
send_request("get_top_of_book", {"ticker": "AAPL"})

# Get volume
send_request("get_volume", {"ticker": "AAPL", "price": 150.0, "order_type": 1})

# Get previous trades
send_request("get_previous_trades", {"ticker": "AAPL", "num_previous_trades": 5})

# Place an order
send_request("handle_order", {
    "user_id": "trader123",
    "order_type": 1,  # Assume 1 = Buy
    "volume": 10,
    "price": 150.0,
    "ticker": "AAPL"
})

# Get trades by user
send_request("get_trades_by_user", {"user_id": "trader123"})

# Register a user
send_request("register_user", {"user_id": "trader456"})
