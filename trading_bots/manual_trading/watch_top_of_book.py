import socket
import json
import os
import time
# Server details
HOST = "127.0.0.1"
PORT = 8080

def send_request(action, data={}):
    """Send a request to the exchange server and return response."""
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

def pretty_print(msg):
    """Formats text with a clear visual separator."""
    print("\n" + "=" * 60)
    print(msg)
    print("=" * 60 + "\n")

def register_user():
    """Registers a new user for trading."""
    user_id = input("Enter your user ID: ").strip()
    response = send_request("register_user", {"user_id": user_id})
    pretty_print(f"User Registration Response: {response}")

def get_tickers():
    """Fetch available trading tickers."""
    response = send_request("get_tickers")
    tickers = response.get("tickers", [])
    pretty_print(f"Available Trading Pairs: {', '.join(tickers) if tickers else 'No tickers found'}")

def get_top_of_book(ticker):
    """Fetch the best bid and ask prices for a ticker with a visual volume bar."""
    response = send_request("get_top_of_book", {"ticker": ticker})
    
    if "error" in response:
        pretty_print(f"Error: {response['error']}")
        return

    # Extract price and volume values (default to 0 if not provided)
    bid_price = response.get("bid_price", "N/A")
    bid_volume = response.get("bid_volume", 0)
    ask_price = response.get("ask_price", "N/A")
    ask_volume = response.get("ask_volume", 0)

    # Convert volumes to numeric, handling "N/A" just in case
    if isinstance(bid_volume, str) and not bid_volume.isdigit():
        bid_volume_num = 0
    else:
        bid_volume_num = float(bid_volume)

    if isinstance(ask_volume, str) and not ask_volume.isdigit():
        ask_volume_num = 0
    else:
        ask_volume_num = float(ask_volume)

    # Determine the maximum volume for normalizing the bar length
    max_volume = max(bid_volume_num, ask_volume_num)

    def volume_bar(volume, max_vol, bar_length=20):
        """Generate a simple horizontal bar proportional to the volume."""
        if max_vol == 0:  # To avoid ZeroDivisionError
            return ""
        
        # Scale the bar based on the ratio of volume to maximum volume
        fill_length = int((volume / max_vol) * bar_length)
        return "█" * fill_length + " " * (bar_length - fill_length)

    pretty_print(f"ORDER BOOK FOR {ticker}")

    # Print best bid row
    print(
        f" Best Bid  | Price: {bid_price:<5} | "
        f"Volume: {bid_volume:<5} "
        f"{volume_bar(bid_volume_num, max_volume)}"
    )

    # Print best ask row
    print(
        f" Best Ask  | Price: {ask_price:<5} | "
        f"Volume: {ask_volume:<5} "
        f"{volume_bar(ask_volume_num, max_volume)}\n"
    )



def clear_screen():
    """Clears the console screen."""
    os.system('cls' if os.name == 'nt' else 'clear')

def main_menu():
    """Displays the main menu and handles user input."""
    
    # Ask user if they want the menu to persist or refresh
    tickers = get_tickers()
    ticker = input("\n📢 Select Ticker:").strip()


    while True:
        clear_screen()
        get_top_of_book(ticker)
        time.sleep(.5)

if __name__ == "__main__":
    main_menu()
