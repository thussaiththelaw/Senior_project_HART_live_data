// Code for Adafruit GPS modules using MTK3329/MTK3339 driver
// March 8, 2025 modified by Daniel Fuller for BYU-Idaho HART team
//
// This code just echos whatever is coming from the GPS unit to the
// serial monitor and to the SD. By doing this, any errors in parsing
// can be bypassed, as all useful data is directly echoed to the SD.
// For simplicity and reduced memory use, this appends to a single file,
// which may already exist-- This program just adds more data to it.
// Empty this file after each flight.
//
// The L13 light will do a double flash each second if the SD is working.
// If the light doesn't do this, the SD initialization has failed.
// 
// The first few lines in the SD, after resetting the GPS, are just garbage. Ignore them.
// 
// The "$ GNGGA" line first gives time in seconds (according to UTC), 
// then it gives latitude and longitude:
// "Despite appearances, the geolocation data is NOT in decimal degrees. 
// It is in degrees and minutes in the following format:
// Latitude: DDMM.MMMM (The first two characters are the degrees.) 
// Longitude: DDDMM.MMMM (The first three characters are the degrees.)"
// - from https://learn.adafruit.com/adafruit-ultimate-gps/f-a-q
// In between the remaining information is the altitude, in meters.
// It is comma index 9 (starting from 0), known as "Orthometric height"
// For more information on splitting this data for plotting, look up "GPS GGA"
//
// The $GNRMC line occassionally gets cut off- we don't need it anyway...
// 
// Debugging Help:
// LIBUSB_ERROR_TIMEOUT:
// This happens if the arduino is trying to write too much data to serial at once, such that new sketch files can't be uploaded to it, because it is too busy.
// Fix by double pressing the reset button, then upload a sketch to it with a higher baud rate for Serial
// https://forum.arduino.cc/t/arduino-uno-r4-minima/1247231/5?_gl=1*eehdvm*_up*MQ..*_ga*MTUzMDg1NDMwMy4xNzQxNDU1NDA5*_ga_NEXN8H46L5*MTc0MTQ1NTQwOC4xLjAuMTc0MTQ1NTQwOC4wLjAuODg2ODI3MDQ3

#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <SD.h>

// Connect the GPS Power pin to 5V
// Connect the GPS Ground pin to ground
// Connect the GPS TX (transmit) pin to Digital 8
// Connect the GPS RX (receive) pin to Digital 7

// You can change the pin numbers to match your wiring:
SoftwareSerial mySerial(8, 7);

#define PMTK_SET_NMEA_UPDATE_1HZ  "$PMTK220,1000*1F"
//#define PMTK_SET_NMEA_UPDATE_5HZ  "$PMTK220,200*2C"
//#define PMTK_SET_NMEA_UPDATE_10HZ "$PMTK220,100*2F"

// turn on only the second sentence (GPRMC)
//#define PMTK_SET_NMEA_OUTPUT_RMCONLY "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29"
// turn on GPRMC and GGA
#define PMTK_SET_NMEA_OUTPUT_RMCGGA "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"
// turn on ALL THE DATA
//#define PMTK_SET_NMEA_OUTPUT_ALLDATA "$PMTK314,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28"
// turn off output
//#define PMTK_SET_NMEA_OUTPUT_OFF "$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"

#define PMTK_Q_RELEASE "$PMTK605*31"

// SD variables:
#define SD_PIN 10 // this should correspond to the CS "Chip Select" pin on the GPS + SD shield
File logfile;
String line = "";

// I2C settup from senior project team
#include <Wire.h> //I2C library
const uint8_t I2C_myAddress = 0b0000010; //7bit number identifing this device on the I2C Bus
const uint8_t I2C_dataCollectorAddress = 0b0000001; //7bit number identifing where data should be sent to
void I2C_setUp();
void I2C_send(char message[]);
void I2C_send(String message);

void setup() {
  Serial.begin(115200); // The serial port for the Arduino IDE port output 
  // (The above needs to be kept high, or else the arduino gets too busy to be able to accept new sketches uploaded to it)
  mySerial.begin(9600);
  I2C_setUp();
  delay(2000);

  //Serial.println("Software Serial GPS Echoing Straight to SD");
  // you can send various commands to get it started
  //mySerial.println(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  mySerial.println(PMTK_SET_NMEA_OUTPUT_RMCGGA); // this gives 2 lines of data
  mySerial.println(PMTK_SET_NMEA_UPDATE_1HZ);

  // start SD commands
  //SD.begin(SD_PIN);
  //delay(5000);
  init_SD(); // this does the above, but does an infinite loop here the program if the SD fails.
  // Create folder for logs if it doesn't already exist
  if (!SD.exists("/LOGS/"))
    SD.mkdir("/LOGS/");
  
  // let people know the GPS has been reset
  logfile=SD.open("/LOGS/GPS_Echo.TXT", FILE_WRITE);
  logfile.print("\n~~~RESET~~~\n");
  logfile.close();
}

void loop() {
  if (mySerial.available()) { // if the GPS has sent back a single character
    char c = mySerial.read();

    // print raw output message direct to SD to avoid losing data from bugs
    if((c < 127 && c > 31) || c==10) // skip weird characters
    {
      line+= c; // append new character to line
    }
    if(c=='\n') // print to SD when a new line was just appended
    {
      // open and print the line, then close the file.
      logfile=SD.open("/LOGS/GPS_Echo.TXT", FILE_WRITE);
      logfile.print(line); // print the line on the SD file
      //logfile.flush(); // "You don't need to call flush unless there is a chance of your program crashing before you close the file." - arduino forums
      logfile.close(); // close the file
      Serial.print(line);
      I2C_send(line);
      line=""; // reset the line
    }
  }
}

// ======================================================
// attempts to initialize the SD card for reading/writing
// ======================================================
void init_SD() {
  Serial.print("Init SD-");

  if (!SD.begin(SD_PIN)) {
    Serial.println(" fail!");
    while (1)
    {
      // flash an error code to the terminal
      Serial.println("SD: Fail!"); // this stays on until the SD initializes sucessfully for debug use.
      delay(3000); 
    }
  }

  Serial.println(" pass");
}

// run at startup initilizes I2C comunication
void I2C_setUp() {
  Wire.begin(I2C_myAddress);
  Wire.setClock(10000);
}

/*  sends data to I2C data controller
    takes in a char array
    functions like println() but to controller
    */
void I2C_send(char message[]) {
  int index = 0;
  int bytes_sent = 0;

  // gain controll of the buss
  Wire.beginTransmission(I2C_dataCollectorAddress);
  delay(10);
  if (Wire.endTransmission(false) > 1) return;
  do {
    // makes sure data collector isnt busy
    bool busy = true;
    int timeout = 0;
    while (busy) {
      Wire.requestFrom(I2C_dataCollectorAddress, 1, false);
      delay(5);
      busy = Wire.read();
      timeout++;
      if (timeout > 1000) return;
    }

    // send the mesage in 32 byte chunks
    Wire.beginTransmission(I2C_dataCollectorAddress);
    do {
      Wire.write(message[index]);
      
      index++;
      bytes_sent++;
    } while(message[index-1] != 0 && bytes_sent < 32);

    Wire.endTransmission(false);
    bytes_sent = 0;
  } while(message[index-1] != 0);

  // release control of the buss
  Wire.beginTransmission(I2C_dataCollectorAddress);
  delay(5);
  Wire.endTransmission(true);
} 
/*  sends data to I2C data controller
    takes in a string
    functions like println() but to controller
    */
void I2C_send(String message){
  I2C_send(message.c_str());
}
