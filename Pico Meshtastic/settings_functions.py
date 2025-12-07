from machine import UART, Pin

# ------------------ Configuration ------------------

DEVICE_NAME = "@pico"
MAX_MSG_LEN = 230
BAUD_RATE = 115200
TX_PIN = 4
RX_PIN = 5
HEARTBEAT_INTERVAL = 60

heartbeat_enabled = False

# ------------------ Hardware Setup ------------------

uart = UART(1, BAUD_RATE, tx=Pin(TX_PIN), rx=Pin(RX_PIN))
led = Pin("LED", Pin.OUT)

# ------------------ Command Help Text ------------------

COMMAND_HELP = {
    "ping": "Usage: @pico ping\nResponds with 'pong'.",
    "status": "Usage: @pico status\nReports device health.",
    "hello": "Usage: @pico hello\nGreets you.",
    "led on": "Usage: @pico led on\nTurns the Pico LED ON.",
    "led off": "Usage: @pico led off\nTurns the Pico LED OFF.",
    "heartbeat on": "Usage: @pico heartbeat on\nEnables periodic heartbeat messages.",
    "heartbeat off": "Usage: @pico heartbeat off\nDisables periodic heartbeat messages.",
    "heartbeat set": "Usage: @pico heartbeat set <seconds>\nSets heartbeat interval.",
    "help": (
        "Usage: @pico help OR @pico <command> help\n"
        "Shows list of commands or details about a specific command."
    )
}

# ------------------ Serial Helpers ------------------

def send_serial_message(message: str):
    data = (message[:MAX_MSG_LEN] + "\n").encode("utf-8")
    uart.write(data)
    print("Sent:", message)

def receive_serial_message():
    if uart.any():
        line = uart.readline()
        if line:
            try:
                return line.decode("utf-8").strip()
            except UnicodeError:
                return None
    return None

# ------------------ Command Help ------------------

def send_help(command=None):
    if command:
        if command in COMMAND_HELP:
            send_serial_message(COMMAND_HELP[command])
        else:
            send_serial_message(f"No help available for '{command}'.")
        return

    send_serial_message("Available commands:")
    for cmd in COMMAND_HELP:
        send_serial_message(f" - {cmd}")

# ------------------ Message / Command Parsing ------------------

def handle_incoming_message(message: str):
    global HEARTBEAT_INTERVAL, heartbeat_enabled

    # Extract starting at "@pico"
    idx = message.lower().find(DEVICE_NAME.lower())
    if idx == -1:
        return  # No command for us
    message = message[idx:]  # Keep "@pico ..."


    command = message[len(DEVICE_NAME.lstrip("@")):].strip().lower()

    # --- HELP COMMANDS ---
    if command == "help":
        send_help()
        return

    if command.endswith(" help"):
        base_cmd = command[:-5].strip()
        send_help(base_cmd)
        return

    # --- COMMANDS ---
    if command.startswith("ping"):
        send_serial_message("pong")
    elif command.startswith("status"):
        send_serial_message("Pico online and listening!")
    elif command.startswith("hello"):
        send_serial_message("Hi there from Pico!")
    elif command == "led on":
        led.value(1)
        send_serial_message("LED turned ON.")
    elif command == "led off":
        led.value(0)
        send_serial_message("LED turned OFF.")

    elif command.startswith("tower") or command.startswith("antenna") or command.startswith("radio"):
        send_serial_message("📡")

    # --- HEARTBEAT ---
    elif command == "heartbeat on":
        heartbeat_enabled = True
        send_serial_message("Heartbeat enabled.")

    elif command == "heartbeat off":
        heartbeat_enabled = False
        send_serial_message("Heartbeat disabled.")

    elif command.startswith("heartbeat set"):
        parts = command.split()
        if len(parts) == 3 and parts[2].isdigit():
            HEARTBEAT_INTERVAL = int(parts[2])
            send_serial_message(f"Heartbeat interval set to {HEARTBEAT_INTERVAL} seconds.")
        else:
            send_serial_message("Invalid format. Use: @pico heartbeat set <seconds>")

    else:
        send_serial_message(f"Unknown command: {command}")
