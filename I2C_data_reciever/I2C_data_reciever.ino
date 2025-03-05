#include <Wire.h>  // I2C library
#include <Meshtastic.h>  // Meshtastic library

// I2C variables
const int I2C_bufferSize = 32; // Make sure it is the same size as the Wire.h buffer
bool I2C_recievedFlag;
char I2C_buffer[I2C_bufferSize];

// Device I2C address (7-bit)
const uint8_t I2C_myAddress = 0b0000001;

// String buffer variables
const int charBufferSize = 1024;
char charBuffer[charBufferSize];
int charBufferIndex;

// Meshtastic settings
#define SERIAL_RX_PIN 2
#define SERIAL_TX_PIN 3
#define BAUD_RATE 19200
bool not_yet_connected = true;

// Function headers
void I2C_recieve();
void I2C_interupt(int number_of_bytes);
void I2C_request();
void radioSend(const char message[]);
void connected_callback(mt_node_t *node, mt_nr_progress_t progress);
void text_message_callback(uint32_t from, uint32_t to, uint8_t channel, const char* text);

void setup() {
  // Initialize buffers
  memset(I2C_buffer, 0, I2C_bufferSize);
  memset(charBuffer, 0, charBufferSize);
  charBufferIndex = 0;

  // Start serial communication
  Serial.begin(9600);
  while (!Serial && millis() < 5000);  // Wait for Serial to be available

  Serial.println("Booting Meshtastic send/receive client...");

  // Initialize I2C
  I2C_recievedFlag = false;
  Wire.begin(I2C_myAddress);
  Wire.onReceive(I2C_interupt);
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
  bool can_send = mt_loop(now);

  if (I2C_recievedFlag) {
    I2C_recieve();
  }
}

// Reads the I2C_buffer string and sends it via Meshtastic if not empty
void I2C_recieve() {
  int i = 0;
  do {
    charBuffer[charBufferIndex] = I2C_buffer[i];
    charBufferIndex++;
    i++;
  } while ((I2C_buffer[i - 1] != 0) && (i < I2C_bufferSize) && (charBufferIndex < charBufferSize - 1));

  // If the message is complete
  if (I2C_buffer[i - 1] == 0 || charBufferIndex >= charBufferSize - 1) {
    if (charBuffer[0] != 0) {
      radioSend(charBuffer);
    }
    memset(charBuffer, 0, charBufferSize);
    charBufferIndex = 0;
  }

  memset(I2C_buffer, 0, I2C_bufferSize);
  I2C_recievedFlag = false;
}

// Handles an I2C interrupt
void I2C_interupt(int number_of_bytes) {
  for (int i = 0; i < number_of_bytes; i++) {
    I2C_buffer[i] = Wire.read();
  }
  I2C_recievedFlag = true;
}

// Handles an I2C request
void I2C_request() {
  Wire.write(I2C_recievedFlag);
}

// Sends data over Meshtastic
void radioSend(const char message[]) {
  uint32_t dest = BROADCAST_ADDR;  // Broadcast by default
  uint8_t channel_index = 1;
  mt_send_text(message, dest, channel_index);
  Serial.print("Sent: ");
  Serial.println(message);
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
