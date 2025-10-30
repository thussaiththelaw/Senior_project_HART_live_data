#include <Wire.h>  // I2C library
#include <Meshtastic.h>  // Meshtastic library

// Constants
const int I2C_BUFFER_SIZE = 32; // Make sure it is the same size as the Wire.h buffer
const int CHAR_BUFFER_SIZE = 1024;
const uint8_t I2C_MY_ADDRESS = 0b0000001;
const unsigned long SERIAL_WAIT_TIMEOUT_MS = 5000;
const uint8_t MESHTASTIC_CHANNEL_INDEX = 1;

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 20000; // 20 seconds
int sendCounter = 1; // Start at 1
const int maxCount = 20;


// I2C and message buffers encapsulated in structs
struct I2CData {
  bool receivedFlag;
  char buffer[I2C_BUFFER_SIZE];
};
I2CData i2cData;

struct CharBuffer {
  char buffer[CHAR_BUFFER_SIZE];
  int index;
};
CharBuffer charBufferData;

// Meshtastic settings
#define SERIAL_RX_PIN 2
#define SERIAL_TX_PIN 3
#define BAUD_RATE 19200
bool not_yet_connected = true;

// Function headers
void I2C_receive();
void I2C_interrupt(int number_of_bytes);
void I2C_request();
void radioSend(const char message[]);
void connected_callback(mt_node_t *node, mt_nr_progress_t progress);
void text_message_callback(uint32_t from, uint32_t to, uint8_t channel, const char* text);

void setup() {
  // Initialize buffers
  memset(i2cData.buffer, 0, I2C_BUFFER_SIZE);
  i2cData.receivedFlag = false;
  memset(charBufferData.buffer, 0, CHAR_BUFFER_SIZE);
  charBufferData.index = 0;

  // Start serial communication
  Serial.begin(9600);
  unsigned long startMillis = millis();
  while (!Serial && (millis() - startMillis < SERIAL_WAIT_TIMEOUT_MS));  // Wait for Serial to be available

  Serial.println("Booting Meshtastic send/receive client...");

  // Initialize I2C
  Wire.begin(I2C_MY_ADDRESS);
  Wire.onReceive(I2C_interrupt);
  Wire.onRequest(I2C_request);

  // Initialize Meshtastic (Serial connection)
  mt_serial_init(SERIAL_RX_PIN, SERIAL_TX_PIN, BAUD_RATE);
  mt_set_debug(false);
  mt_request_node_report(connected_callback);
  set_text_message_callback(text_message_callback);

  radioSend("Meshtastic Telemetry Started");
}

void loop() {
  uint32_t now = millis();

  // Send numbers 1 to 20 every 20 seconds, loop indefinitely
  if (now - lastSendTime >= sendInterval) {
    char message[16];
    snprintf(message, sizeof(message), "Number: %d", sendCounter);
    radioSend(message);
    sendCounter++;
    if (sendCounter > maxCount) {
      sendCounter = 1; // Reset after reaching 20
    }
    lastSendTime = now;
  }

  bool can_send = mt_loop(now);

  if (i2cData.receivedFlag) {
    I2C_receive();
  }
}



// Reads the I2C buffer string and sends it via Meshtastic if not empty
void I2C_receive() {
  int i = 0;
  // Copy from I2C buffer to charBuffer, ensure no overflow and null-termination
  while ((i < I2C_BUFFER_SIZE) && (charBufferData.index < CHAR_BUFFER_SIZE - 1)) {
    char c = i2cData.buffer[i];
    charBufferData.buffer[charBufferData.index++] = c;
    if (c == '\0') break;
    i++;
  }
  // Ensure null-termination
  charBufferData.buffer[charBufferData.index] = '\0';

  // If the message is complete
  if (i2cData.buffer[i] == '\0' || charBufferData.index >= CHAR_BUFFER_SIZE - 1) {
    if (charBufferData.buffer[0] != '\0') {
      radioSend(charBufferData.buffer);
    }
    memset(charBufferData.buffer, 0, CHAR_BUFFER_SIZE);
    charBufferData.index = 0;
  }

  memset(i2cData.buffer, 0, I2C_BUFFER_SIZE);
  i2cData.receivedFlag = false;
}

// Handles an I2C interrupt
void I2C_interrupt(int number_of_bytes) {
  int bytesToRead = (number_of_bytes < I2C_BUFFER_SIZE) ? number_of_bytes : I2C_BUFFER_SIZE;
  for (int i = 0; i < bytesToRead; i++) {
    i2cData.buffer[i] = Wire.read();
  }
  // Null-terminate if possible
  if (bytesToRead < I2C_BUFFER_SIZE) {
    i2cData.buffer[bytesToRead] = '\0';
  } else {
    i2cData.buffer[I2C_BUFFER_SIZE - 1] = '\0';
  }
  i2cData.receivedFlag = true;
}

// Handles an I2C request
void I2C_request() {
  Wire.write(i2cData.receivedFlag);
}

// Sends data over Meshtastic
void radioSend(const char message[]) {
  uint32_t dest = BROADCAST_ADDR;  // Broadcast by default
  uint8_t channel_index = MESHTASTIC_CHANNEL_INDEX;
  int result = mt_send_text(message, dest, channel_index);
  Serial.print("Sent: ");
  Serial.println(message);
  if (result != 0) {
    Serial.print("Error sending message, code: ");
    Serial.println(result);
  }
}

// Called when connected to a Meshtastic node
void connected_callback(mt_node_t *node, mt_nr_progress_t progress) {
  if (not_yet_connected) {
    Serial.println("Connected to Meshtastic device!");
  }
  not_yet_connected = false;
}

// Called when a message is received via Meshtastic
void text_message_callback(uint32_t from, uint32_t to, uint8_t channel, const char* text) {
  Serial.print("Received a message on channel ");
  Serial.print(channel);
  Serial.print(" from ");
  Serial.print(from);
  Serial.print(" to ");
  Serial.print(to);
  Serial.print(": ");
  Serial.println(text);
} 
