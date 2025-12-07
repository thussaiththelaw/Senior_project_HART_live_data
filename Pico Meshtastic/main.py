#This is a working example code to implement communication from an Raspberry Pi Pico running MicroPython to a Meshtastic radio.
#This is the main function and calls settings_functions.py for settings and message handling

from machine import UART, Pin
import time
from settings_functions import (
    DEVICE_NAME,
    MAX_MSG_LEN,
    HEARTBEAT_INTERVAL,
    heartbeat_enabled,
    uart,
    led,
    send_serial_message,
    receive_serial_message,
    handle_incoming_message,
)


print("Pico Meshtastic bridge started.")
send_serial_message("Pico connected and ready for commands!")

last_heartbeat = time.time()

while True:
    msg = receive_serial_message()
    if msg:
        print("Received:", msg)
        handle_incoming_message(msg)

    # Periodic heartbeat
    now = time.time()
    if heartbeat_enabled and now - last_heartbeat >= HEARTBEAT_INTERVAL:
        send_serial_message("Pico heartbeat: online.")
        last_heartbeat = now

    time.sleep(0.05)

