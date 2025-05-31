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
  while (!Serial);  // Wait for Serial Monitor to open

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
  // Read incoming data from GPS
  char c = GPS.read();
  if (GPSECHO && c) {
    Serial.write(c);  // Echo raw NMEA sentences to Serial Monitor
  }

  // If a full sentence is received, parse it
  if (GPS.newNMEAreceived()) {
    if (!GPS.parse(GPS.lastNMEA())) {
      return; // Failed to parse, wait for the next one
    }
  }

  // Every 2 seconds, print and log GPS summary info
  if (millis() - timer > 2000) {
    timer = millis();

    Serial.println();
    Serial.print("Time: ");
    if (GPS.hour < 10) Serial.print('0');
    Serial.print(GPS.hour); Serial.print(':');
    if (GPS.minute < 10) Serial.print('0');
    Serial.print(GPS.minute); Serial.print(':');
    if (GPS.seconds < 10) Serial.print('0');
    Serial.println(GPS.seconds);

    Serial.print("Date: ");
    Serial.print(GPS.day); Serial.print('/');
    Serial.print(GPS.month); Serial.print("/20");
    Serial.println(GPS.year);

    Serial.print("Fix: "); Serial.print((int)GPS.fix);
    Serial.print(" | Quality: "); Serial.println((int)GPS.fixquality);

    if (GPS.fix) {
      // Convert to decimal degrees
      float latitude = convertToDecimalDegrees(GPS.latitude, GPS.lat);
      float longitude = convertToDecimalDegrees(GPS.longitude, GPS.lon);

      Serial.print("Location (Google Maps format): ");
      Serial.print(latitude, 6);
      Serial.print(", ");
      Serial.println(longitude, 6);

      Serial.print("Speed (knots): "); Serial.println(GPS.speed);
      Serial.print("Angle: "); Serial.println(GPS.angle);
      Serial.print("Altitude: "); Serial.println(GPS.altitude);
      Serial.print("Satellites: "); Serial.println((int)GPS.satellites);

      // Log to SD card
      logfile.print("Time: ");
      if (GPS.hour < 10) logfile.print('0');
      logfile.print(GPS.hour); logfile.print(':');
      if (GPS.minute < 10) logfile.print('0');
      logfile.print(GPS.minute); logfile.print(':');
      if (GPS.seconds < 10) logfile.print('0');
      logfile.print(GPS.seconds); logfile.print(", ");

      logfile.print("Date: ");
      logfile.print(GPS.day); logfile.print('/');
      logfile.print(GPS.month); logfile.print("/20");
      logfile.print(GPS.year); logfile.print(", ");

      logfile.print("Lat: "); logfile.print(latitude, 6); logfile.print(", ");
      logfile.print("Lon: "); logfile.print(longitude, 6); logfile.print(", ");

      logfile.print("Speed: "); logfile.print(GPS.speed); logfile.print(" knots, ");
      logfile.print("Angle: "); logfile.print(GPS.angle); logfile.print(", ");
      logfile.print("Altitude: "); logfile.print(GPS.altitude); logfile.print(" m, ");
      logfile.print("Satellites: "); logfile.println((int)GPS.satellites);

      logfile.flush();
    } else {
      Serial.println("No GPS fix yet.");
    }
  }
}