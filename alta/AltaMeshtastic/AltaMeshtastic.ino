#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <SD.h>
#include <SPI.h>

// --- GPS Setup ---
SoftwareSerial gpsSerial(8, 7);  // GPS TX -> Pin 8, RX -> Pin 7
Adafruit_GPS GPS(&gpsSerial);
#define GPSECHO true

// --- SD Card Setup ---
#define chipSelect 10
File logfile;

// --- Meshtastic Setup ---
SoftwareSerial meshtasticSerial(6, 5); // TX -> Pin 6, RX -> Pin 5
const char* DEVICE_NAME = "@arduino";

// --- Timing ---
uint32_t lastGPSTime = 0;       // Last time GPS was logged
uint32_t lastMeshTime = 0;      // Last time GPS sent to Meshtastic
const uint32_t GPS_INTERVAL = 1000;     // 1 second
const uint32_t MESH_INTERVAL = 20000;   // 20 seconds

// --- Helper Functions ---
float convertToDecimalDegrees(float coordinate, char direction) {
  int degrees = int(coordinate / 100);
  float minutes = coordinate - (degrees * 100);
  float decimal = degrees + minutes / 60.0;
  if (direction == 'S' || direction == 'W') decimal *= -1.0;
  return decimal;
}

// --- Meshtastic Functions ---
void sendMeshtasticMessage(const char* message) {
  for (int i = 0; i < 230 && message[i] != '\0'; i++) {
    meshtasticSerial.write(message[i]);
  }
  meshtasticSerial.write('\n'); // Meshtastic expects newline
}

bool receiveMeshtasticMessage(char* buffer, size_t max_len) {
  static byte index = 0;
  while (meshtasticSerial.available() > 0) {
    char c = meshtasticSerial.read();
    if (c == '\n' || c == '\r') {
      if (index > 0) {
        buffer[index] = '\0';
        index = 0;
        return true;
      }
    } else if (index < max_len - 1) {
      buffer[index++] = c;
    }
  }
  return false;
}

void handleMeshtasticCommand(const char* message) {
  // Only respond to messages starting with @arduino
  if (strncmp(message, DEVICE_NAME, strlen(DEVICE_NAME)) != 0) return;
  const char* command = message + strlen(DEVICE_NAME);
  while (*command == ' ') command++;

  // Command responses
  if (strncasecmp(command, "ping", 4) == 0) sendMeshtasticMessage("pong");
  else if (strncasecmp(command, "status", 6) == 0) sendMeshtasticMessage("Arduino online and logging GPS!");
  else if (strncasecmp(command, "hello", 5) == 0) sendMeshtasticMessage("Hello from Arduino GPS logger!");
  else sendMeshtasticMessage("Unknown command");
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600);
  GPS.begin(9600);

  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  GPS.sendCommand(PGCMD_ANTENNA);

  delay(1000);
  gpsSerial.println(PMTK_Q_RELEASE);

  // SD card setup
  pinMode(chipSelect, OUTPUT);
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card init failed!");
    while (1);
  }
  Serial.println("SD card initialized.");

  // Create a unique log file
  char filename[] = "GPSLOG00.TXT";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = '0' + i / 10;
    filename[7] = '0' + i % 10;
    if (!SD.exists(filename)) break;
  }
  logfile = SD.open(filename, FILE_WRITE);
  if (!logfile) {
    Serial.print("Couldn't create ");
    Serial.println(filename);
    while (1);
  }
  Serial.print("Logging to ");
  Serial.println(filename);

  // Meshtastic serial start
  meshtasticSerial.begin(19200);  // Match Meshtastic baud rate
}

// --- Main Loop ---
void loop() {
  char c = GPS.read();
  if (GPSECHO && c) Serial.write(c);

  uint32_t now = millis();

  // Check for Meshtastic messages
  char meshtasticBuffer[231];
  if (receiveMeshtasticMessage(meshtasticBuffer, sizeof(meshtasticBuffer))) {
    handleMeshtasticCommand(meshtasticBuffer);
  }

  // Only act when a new NMEA sentence is received and parsed
  if (GPS.newNMEAreceived()) {
    if (!GPS.parse(GPS.lastNMEA())) return;

    if (GPS.fix) {
      float latitude = convertToDecimalDegrees(GPS.latitude, GPS.lat);
      float longitude = convertToDecimalDegrees(GPS.longitude, GPS.lon);
      float altitude = GPS.altitude;
      float speed_mps = GPS.speed * 0.514444;

      // Log GPS to SD every second
      if (now - lastGPSTime >= 1000) {
        lastGPSTime = now;

        Serial.print(GPS.hour < 10 ? "0" : ""); Serial.print(GPS.hour); Serial.print(":");
        Serial.print(GPS.minute < 10 ? "0" : ""); Serial.print(GPS.minute); Serial.print(":");
        Serial.print(GPS.seconds < 10 ? "0" : ""); Serial.print(GPS.seconds); Serial.print(", ");
        Serial.print(latitude, 6); Serial.print(", ");
        Serial.print(longitude, 6); Serial.print(", ");
        Serial.print(altitude, 2); Serial.print(", ");
        Serial.println(speed_mps, 2);

        logfile.print(GPS.hour < 10 ? "0" : ""); logfile.print(GPS.hour); logfile.print(":");
        logfile.print(GPS.minute < 10 ? "0" : ""); logfile.print(GPS.minute); logfile.print(":");
        logfile.print(GPS.seconds < 10 ? "0" : ""); logfile.print(GPS.seconds); logfile.print(",");
        logfile.print(latitude, 6); logfile.print(",");
        logfile.print(longitude, 6); logfile.print(",");
        logfile.print(altitude, 2); logfile.print(",");
        logfile.println(speed_mps, 2);
        logfile.flush();
      }

      // Send GPS to Meshtastic every 20 seconds
      if (now - lastMeshTime >= 20000) {
        lastMeshTime = now;

        char gpsMsg[230];
        snprintf(gpsMsg, sizeof(gpsMsg), "@arduino gps %.6f %.6f %.2f %.2f",
                 latitude, longitude, altitude, speed_mps);
        sendMeshtasticMessage(gpsMsg);
      }

    } else {
      Serial.println("No GPS fix yet.");
    }
  }
}
