#include <Wire.h>
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <SD.h>
#include <SPI.h>

// ========== GPS Setup ==========
SoftwareSerial mySerial(8, 7);
Adafruit_GPS GPS(&mySerial);

#define GPSECHO  false

// ========== I2C Setup ==========
constexpr uint8_t I2C_MY_ADDRESS = 0b0000100; // This device's I2C address
constexpr uint8_t I2C_COLLECTOR_ADDRESS = 0b0000001; // Gateway's I2C address
constexpr uint8_t I2C_BUFFER_SIZE = 32; // Must match receiver's buffer size
constexpr int BUSY_TIMEOUT = 1000; // Number of attempts before giving up

void I2C_setUp();
void I2C_send(const char message[]);

typedef struct {
  bool initialized;
  File logfile;
} SDCardState;
SDCardState sdCard = {false, File()};

// ========== Timing ==========
unsigned long lastSend = 0;
constexpr unsigned long SEND_INTERVAL_MS = 30000; // 30 seconds

// ========== SD Card Setup ==========
#define chipSelect 10

void setup() {
  Serial.begin(9600);
  I2C_setUp();

  Serial.println("GPS I2C Sender Starting...");

  // GPS setup
  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA); // Get RMC and GGA sentences (includes fix, time, lat, lon, alt)
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);    // 1Hz update rate
  GPS.sendCommand(PGCMD_NOANTENNA);

  // SD card setup
  pinMode(10, OUTPUT);
  if (!SD.begin(chipSelect)) {
    Serial.println("Card init. failed!");
    sdCard.initialized = false;
  } else {
    Serial.println("Card initialized");
    char filename[15];
    strcpy(filename, "GPSLOG00.txt");
    for (uint8_t i = 0; i < 100; i++) {
      filename[6] = '0' + i/10;
      filename[7] = '0' + i%10;
      if (!SD.exists(filename)) {
        break;
      }
    }
    sdCard.logfile = SD.open(filename, FILE_WRITE);
    if (!sdCard.logfile) {
      Serial.print("Couldn't create ");
      Serial.println(filename);
      sdCard.initialized = false;
    } else {
      Serial.print("Writing to ");
      Serial.println(filename);
      sdCard.initialized = true;
    }
  }

  // Wait for GPS to be ready
  delay(1000);
}

void loop() {
  // Read GPS data
  while (GPS.available()) {
    char c = GPS.read();
    if (GPSECHO && c) Serial.write(c);
  }

  // Parse new NMEA sentence
  if (GPS.newNMEAreceived()) {
    if (!GPS.parse(GPS.lastNMEA())) return;
  }

  // Send data every 30 seconds if GPS has a fix
  if (millis() - lastSend >= SEND_INTERVAL_MS) {
    lastSend = millis();

    if (GPS.fix) {
      // Format: HH:MM:SS,lat,lon,alt\n
      char message[64];
      snprintf(
        message, sizeof(message),
        "%02d:%02d:%02d,%.6f,%.6f,%.2f",
        GPS.hour, GPS.minute, GPS.seconds,
        convertDegMinToDecDeg(GPS.latitude) * (GPS.lat == 'S' ? -1 : 1),
        convertDegMinToDecDeg(GPS.longitude) * (GPS.lon == 'W' ? -1 : 1),
        GPS.altitude
      );
      Serial.print("Sending over I2C: ");
      Serial.println(message);
      I2C_send(message);

      // Log to SD card
      if (sdCard.initialized) {
        sdCard.logfile.println(message);
        sdCard.logfile.flush();
        Serial.println("Logged to SD card.");
      } else {
        Serial.println("SD card not initialized, not logging.");
      }
    } else {
      Serial.println("No GPS fix, not sending or logging.");
    }
  }
}

// ========== Helper Functions ==========

double convertDegMinToDecDeg(float degMin) {
  double min = fmod((double)degMin, 100.0);
  int deg = (int)(degMin / 100);
  return deg + (min / 60.0);
}

void I2C_setUp() {
  Wire.begin(I2C_MY_ADDRESS);
  Wire.setClock(100000);
}

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
        Serial.println("I2C busy timeout");
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
