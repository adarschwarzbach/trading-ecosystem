import socket
import json
import os

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

def get_top_of_book():
    """Fetch the best bid and ask prices for a ticker."""
    ticker = input("Enter ticker symbol: ").strip().upper()
    response = send_request("get_top_of_book", {"ticker": ticker})
    
    if "error" in response:
        pretty_print(f"Error: {response['error']}")
    else:
        pretty_print(f"ORDER BOOK FOR {ticker}")
        print(f" Best Bid  | Price: {response.get('bid_price', 'N/A')} | Volume: {response.get('bid_volume', 'N/A')}")
        print(f" Best Ask  | Price: {response.get('ask_price', 'N/A')} | Volume: {response.get('ask_volume', 'N/A')}\n")

def place_order():
    """Place a buy or sell order."""
    try:
        user_id = input("Enter your user ID: ").strip()
        ticker = input("Enter ticker symbol: ").strip().upper()
        order_type = input("Enter order type (buy/sell): ").strip().lower()
        volume = int(input("Enter order volume: ").strip())
        price = float(input("Enter price per unit: ").strip())

        order_type_code = 0 if order_type == "buy" else 1
        response = send_request("handle_order", {
            "user_id": user_id,
            "order_type": order_type_code,
            "price": round(price, 2),
            "volume": volume,
            "ticker": ticker
        })

        print(response)
        if "order_id" in response:
            pretty_print(f"✅ ORDER PLACED SUCCESSFULLY!\nOrder ID: {response['order_id']}\nTicker: {ticker}\nType: {order_type.upper()}\nPrice: {price}\nVolume: {volume}")

        if "trades" in response and len(response["trades"]) > 0:
            print("TRADE(S) EXECUTED")
            for t in response["trades"]:
                print(t)

    except Exception as e:
        pretty_print(f"❌ ERROR PLACING ORDER: {e}")

def cancel_order():
    """Cancel an existing order."""
    user_id = input("Enter your user ID: ").strip()
    ticker = input("Enter ticker symbol: ").strip().upper()
    order_id = int(input("Enter order ID to cancel: ").strip())

    response = send_request("cancel_order", {"user_id": user_id, "ticker": ticker, "order_id": order_id})

    if response.get("success"):
        pretty_print(f"✅ Order {order_id} on {ticker} canceled successfully!")
    else:
        pretty_print(f"❌ Error canceling order: {response}")

def get_trades_by_user():
    """Fetch trade history for a user."""
    user_id = input("Enter your user ID: ").strip()
    response = send_request("get_trades_by_user", {"user_id": user_id})

    if "trades" in response:
        pretty_print(f"📜 TRADE HISTORY FOR {user_id}")
        for trade in response["trades"]:
            print(f"💰 {trade['bid_user_id']} bought from {trade['ask_user_id']} | Price: {trade['price']} | Volume: {trade['volume']} | Time: {trade['timestamp']}")
        print("\n")
    else:
        pretty_print(f"📭 No trades found for user {user_id}.")

def get_previous_trades():
    """Fetch previous trades for a ticker."""
    ticker = input("Enter ticker symbol: ").strip().upper()
    num_trades = int(input("Enter number of recent trades to fetch: ").strip())

    response = send_request("get_previous_trades", {"ticker": ticker, "num_previous_trades": num_trades})

    if "trades" in response:
        pretty_print(f"📊 PREVIOUS TRADES FOR {ticker}")
        for trade in response["trades"]:
            print(f"💰 {trade['bid_user_id']} bought from {trade['ask_user_id']} | Price: {trade['price']} | Volume: {trade['volume']} | Time: {trade['timestamp']}")
        print("\n")
    else:
        pretty_print(f"📭 No trades found for {ticker}.")

def clear_screen():
    """Clears the console screen."""
    os.system('cls' if os.name == 'nt' else 'clear')

def main_menu():
    """Displays the main menu and handles user input."""
    
    # Ask user if they want the menu to persist or refresh
    print("\n📢 Select Display Mode:")
    print(" 1️⃣  Persistent Query")
    print(" 2️⃣  Refresh Menu (Clears screen after each command)")
    display_choice = input("\nChoose an option (1 or 2): ").strip()
    persist_menu = display_choice == "1"

    def print_menu():
        print("\n🚀 WELCOME TO THE TRADING CLI 🚀")
        print("=" * 50)
        print(" 1️⃣  Register a New User")
        print(" 2️⃣  View Available Tickers")
        print(" 3️⃣  Check Top of Order Book")
        print(" 4️⃣  Place a New Order")
        print(" 5️⃣  Cancel an Order")
        print(" 6️⃣  View My Trade History")
        print(" 7️⃣  View Previous Trades for a Ticker")
        print(" 8️⃣  Exit")
        print(" 9️⃣  Show menu again")
        print("=" * 50)

    print_menu()

    while True:
        if not persist_menu:
            clear_screen()
        

        choice = input("Choose an option: ").strip()

        if choice == "1":
            register_user()
        elif choice == "2":
            get_tickers()
        elif choice == "3":
            get_top_of_book()
        elif choice == "4":
            place_order()
        elif choice == "5":
            cancel_order()
        elif choice == "6":
            get_trades_by_user()
        elif choice == "7":
            get_previous_trades()
        elif choice == "8":
            pretty_print("👋 Goodbye! Happy Trading! 🚀")
            break
        elif choice == "9":
            print_menu()
        else:
            pretty_print("❌ Invalid choice! Please enter a number between 1-8.")

if __name__ == "__main__":
    main_menu()
