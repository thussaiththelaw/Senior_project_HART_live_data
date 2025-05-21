#include <Wire.h> // I2C library

constexpr uint8_t I2C_MY_ADDRESS = 0b0000010; // This device's I2C address
constexpr uint8_t I2C_COLLECTOR_ADDRESS = 0b0000001; // Gateway's I2C address
constexpr uint8_t I2C_BUFFER_SIZE = 32; // Must match receiver's buffer size
constexpr int BUSY_TIMEOUT = 1000; // Number of attempts before giving up

void I2C_setUp();
void I2C_send(const char message[]);
void I2C_send(const String& message);

char animation[14][16] = {
  "(^*O*^)", "<(*O*<)", " <(*O*<)", "  <(*O*<)", "   <(*O*<)", "    <(*O*<)",
  "     <(*O*<)", "      (^*O*^)", "     (>*O*)>", "    (>*O*)>", "   (>*O*)>",
  "  (>*O*)>", " (>*O*)>", "(>*O*)>"
};
int animationCount = 0;

void setup() {
  I2C_setUp();
}

void loop() {
  I2C_send(animation[animationCount]);
  animationCount = (animationCount + 1) % 14;
  delay(30000);
}

// Initializes I2C communication
void I2C_setUp() {
  Wire.begin(I2C_MY_ADDRESS);
  Wire.setClock(100000);
}

/**
 * Sends a null-terminated char array to the I2C collector in 32-byte chunks.
 */
void I2C_send(const char message[]) {
  int index = 0;
  int bytes_sent = 0;

  // Gain control of the bus (dummy transmission)
  Wire.beginTransmission(I2C_COLLECTOR_ADDRESS);
  delay(10);
  if (Wire.endTransmission(false) > 1) return;

  do {
    // Wait until collector is not busy
    bool busy = true;
    int timeout = 0;
    while (busy) {
      Wire.requestFrom(I2C_COLLECTOR_ADDRESS, (size_t)1, false);
      delay(5);
      if (Wire.available()) {
        busy = Wire.read();
      } else {
        busy = true;
      }
      timeout++;
      if (timeout > BUSY_TIMEOUT) {
        // Optional: print a debug message
        // Serial.println("I2C busy timeout");
        return;
      }
    }

    // Send the message in 32-byte chunks
    Wire.beginTransmission(I2C_COLLECTOR_ADDRESS);
    bytes_sent = 0;
    do {
      Wire.write(message[index]);
      index++;
      bytes_sent++;
    } while (message[index - 1] != 0 && bytes_sent < I2C_BUFFER_SIZE);
    Wire.endTransmission(false); // Keep connection for next chunk if needed

  } while (message[index - 1] != 0);

  // Release control of the bus
  Wire.beginTransmission(I2C_COLLECTOR_ADDRESS);
  delay(10);
  Wire.endTransmission(true);
}

/**
 * Sends a String to the I2C collector.
 */
void I2C_send(const String& message) {
  I2C_send(message.c_str());
}
