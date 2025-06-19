#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <SD.h>
#include <SPI.h>

// Set up SoftwareSerial on the working pins
SoftwareSerial mySerial(8, 7);  // GPS TX -> Pin 8, GPS RX -> Pin 7
Adafruit_GPS GPS(&mySerial);

#define GPSECHO true
#define chipSelect 10

uint32_t timer = millis();
File logfile;

// Convert GPS coordinates from DDMM.MMMM to decimal degrees
float convertToDecimalDegrees(float coordinate, char direction) {
  int degrees = int(coordinate / 100);
  float minutes = coordinate - (degrees * 100);
  float decimal = degrees + minutes / 60.0;

  if (direction == 'S' || direction == 'W') {
    decimal *= -1.0;
  }
  return decimal;
}

void setup() {
  Serial.begin(115200);
  // Remove the wait for Serial Monitor
  // while (!Serial);  // Wait for Serial Monitor to open

  Serial.println("Adafruit GPS parsing + SD logging test");

  mySerial.begin(9600);   // Confirmed working baud for GPS
  GPS.begin(9600);        // Initialize GPS parsing

  // Turn on RMC (Recommended Minimum) and GGA (Fix data)
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // Set update rate to 1 Hz (once per second)
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  // Request antenna status (optional)
  GPS.sendCommand(PGCMD_ANTENNA);

  delay(1000);
  mySerial.println(PMTK_Q_RELEASE); // Ask for firmware version

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
}

void loop() {
  char c = GPS.read();
  if (GPSECHO && c) Serial.write(c);

  // Only act when a new NMEA sentence is received and parsed
  if (GPS.newNMEAreceived()) {
    if (!GPS.parse(GPS.lastNMEA())) return;

    if (GPS.fix) {
      // Convert to decimal degrees
      float latitude = convertToDecimalDegrees(GPS.latitude, GPS.lat);
      float longitude = convertToDecimalDegrees(GPS.longitude, GPS.lon);
      float altitude = GPS.altitude;
      float speed_mps = GPS.speed * 0.514444; // knots to m/s

      // Print to Serial
      Serial.print(GPS.hour < 10 ? "0" : ""); Serial.print(GPS.hour); Serial.print(":");
      Serial.print(GPS.minute < 10 ? "0" : ""); Serial.print(GPS.minute); Serial.print(":");
      Serial.print(GPS.seconds < 10 ? "0" : ""); Serial.print(GPS.seconds); Serial.print(", ");
      Serial.print(latitude, 6); Serial.print(", ");
      Serial.print(longitude, 6); Serial.print(", ");
      Serial.print(altitude, 2); Serial.print(", ");
      Serial.println(speed_mps, 2);

      // Log to SD card
      logfile.print(GPS.hour < 10 ? "0" : ""); logfile.print(GPS.hour); logfile.print(":");
      logfile.print(GPS.minute < 10 ? "0" : ""); logfile.print(GPS.minute); logfile.print(":");
      logfile.print(GPS.seconds < 10 ? "0" : ""); logfile.print(GPS.seconds); logfile.print(",");
      logfile.print(latitude, 6); logfile.print(",");
      logfile.print(longitude, 6); logfile.print(",");
      logfile.print(altitude, 2); logfile.print(",");
      logfile.println(speed_mps, 2);

      logfile.flush();
    } else {
      Serial.println("No GPS fix yet.");
    }
  }
}
