#This is an example code to implement communication from an Raspberry Pi Pico running CircuitPython to a Meshtastic radio.

#Things you can change:
# -The RX and TX pins
# -When you send data to send Meshtastic

#Things you should change:
# -DEVICE_NAME: for sending commands to this specific device
# -What messages will execute commands or responses

#Things you should not change:
# -The character limit per message
# -The serial speed

import board
import busio
import digitalio
import time

# ------------------ Configuration ------------------

DEVICE_NAME = "@pico"          # Change this if you want a different prefix
MAX_MSG_LEN = 230
BAUD_RATE = 115200             # Match your Meshtastic node’s serial speed
TX_PIN = board.GP4             # Pico TX -> Meshtastic RX
RX_PIN = board.GP5             # Pico RX <- Meshtastic TX
HEARTBEAT_INTERVAL = 60        # seconds

# ------------------ Hardware Setup ------------------

# Initialize UART
uart = busio.UART(TX_PIN, RX_PIN, baudrate=BAUD_RATE, timeout=0.1)

# Onboard LED
led = digitalio.DigitalInOut(board.LED)
led.direction = digitalio.Direction.OUTPUT

# ------------------ Helper Functions ------------------

def send_serial_message(message: str):
    """Send a message to Meshtastic with newline terminator."""
    data = (message[:MAX_MSG_LEN] + "\n").encode("utf-8")
    uart.write(data)
    print("Sent:", message)

def receive_serial_message() -> str | None:
    """Read a full line from UART. Returns None if no complete line."""
    line = uart.readline()
    if line:
        try:
            msg = line.decode("utf-8").strip()
            if msg:
                return msg
        except UnicodeError:
            pass  # Ignore any corrupted bytes
    return None

def handle_incoming_message(message: str):
    """Respond to messages that begin with @pico."""
    if not message.lower().startswith(DEVICE_NAME.lower()):
        return  # Ignore messages not addressed to this device

    command = message[len(DEVICE_NAME):].strip().lower()

    if command.startswith("ping"):
        send_serial_message("pong")

    elif command.startswith("status"):
        send_serial_message("Pico online and listening!")

    elif command.startswith("hello"):
        send_serial_message("Hi there from Pico!")

    elif command.startswith("led on"):
        led.value = True
        send_serial_message("LED turned ON.")

    elif command.startswith("led off"):
        led.value = False
        send_serial_message("LED turned OFF.")

    else:
        send_serial_message(f"Unknown command: {command}")

# ------------------ Main Loop ------------------

print("Pico Meshtastic bridge started.")
send_serial_message("Pico connected and ready for commands!")

last_heartbeat = time.monotonic()

while True:
    msg = receive_serial_message()
    if msg:
        print("Received:", msg)
        handle_incoming_message(msg)

    # Periodic heartbeat
    now = time.monotonic()
    if now - last_heartbeat >= HEARTBEAT_INTERVAL:
        send_serial_message("Pico heartbeat: online.")
        last_heartbeat = now

    time.sleep(0.05)
