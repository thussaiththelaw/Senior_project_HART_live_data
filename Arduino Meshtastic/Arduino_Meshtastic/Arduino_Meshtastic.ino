//This is an example code to implement communication from an Arduino Microcontroller
//to a Meshtastic radio.
//Things you can change:
// -The RX and TX pins
// -When you send data to send Meshtastic
// -What messages will execute commands or responses
//Things you should change:
// -DEVICE_NAME: for sending commands to this specific device
// -
//Things you should not change:
// -The character limit per message
// -The serial speed


#include <SoftwareSerial.h>

#define RX_PIN 10
#define TX_PIN 11
#define MAX_MSG_LEN 230

SoftwareSerial meshtasticSerial(RX_PIN, TX_PIN);  // RX, TX

char receivedMessage[MAX_MSG_LEN + 1];  // Buffer for incoming messages

// Change this if you want a different command prefix
const char* DEVICE_NAME = "@arduino";

void setup() {
  meshtasticSerial.begin(115200);  // Match your Meshtastic node’s serial baud rate
  delay(1000);
  
  sendSerialMessage("Arduino connected and ready for commands!");
}

// Sends a message (up to 230 characters) to Meshtastic
void sendSerialMessage(const char* message) {
  for (int i = 0; i < MAX_MSG_LEN && message[i] != '\0'; i++) {
    meshtasticSerial.write(message[i]);
  }
  meshtasticSerial.write('\n');  // Meshtastic expects newline
}

// Receives a full line (message) from Meshtastic
bool receiveSerialMessage() {
  static byte index = 0;

  while (meshtasticSerial.available() > 0) {
    char c = meshtasticSerial.read();

    if (c == '\n' || c == '\r') {
      if (index > 0) {
        receivedMessage[index] = '\0';
        index = 0;
        return true;  // Message ready
      }
    } 
    else if (index < MAX_MSG_LEN) {
      receivedMessage[index++] = c;
    }
  }

  return false;
}

// Handles messages directed to this Arduino (starting with @arduino)
void handleIncomingMessage() {
  // Check if message starts with @arduino
  if (strncmp(receivedMessage, DEVICE_NAME, strlen(DEVICE_NAME)) != 0)
    return;  // Ignore if not for this device

  // Skip past "@arduino" and any space that follows
  const char* command = receivedMessage + strlen(DEVICE_NAME);
  while (*command == ' ') command++;

  if (strncasecmp(command, "ping", 4) == 0) {
    sendSerialMessage("pong");
  }
  else if (strncasecmp(command, "status", 6) == 0) {
    sendSerialMessage("Arduino online and listening!");
  }
  else if (strncasecmp(command, "hello", 5) == 0) {
    sendSerialMessage("Hi there from Arduino!");
  }
  else {
    char response[MAX_MSG_LEN + 30];
    snprintf(response, sizeof(response), "Unknown command: %s", command);
    sendSerialMessage(response);
  }
}

void loop() {
  if (receiveSerialMessage()) {
    handleIncomingMessage();
  }

  // Optional periodic heartbeat
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 60000) {  // Every 60 seconds
    sendSerialMessage("Arduino heartbeat: online.");
    lastSend = millis();
  }
}
