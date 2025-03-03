// ===================================================
// RobotDyn SD/RTC Arduino shield example
// Nolan Chandler
// Last revised: 2020-11-18

//
// Meant to serve as a starting point for data logging
// using both a real-time clock (RTC) and a microSD
// card.
//
// Collects data at a given rate, and saves them with
// a timestamp on the SD card to files called
// "LOG000.TXT", "LOG001.TXT", etc. A new file is
// opened every N samples to collect another N samples.
// ===================================================

 
#include <SPI.h>
#include <SD.h>

//magnetometer
#include <Adafruit_MMC56x3.h>

/* Assign a unique ID to this sensor at the same time */
Adafruit_MMC5603 mmc = Adafruit_MMC5603(12345);

//magnetic components
float mx;
float my;
float mz;
// float M[3];
//end of magnetometer

#include "RTClib.h"
const int SD_PIN = 9; //might need to change to pin 10

File logfile;
RTC_DS1307 rtc;

float data;
int i = 0;
int N = 60; // Number of samples per file
int waittime_ms = 1000*30; // milliseconds between samples

// ====================================================

void setup()
{

 // Open serial communications for debugging

// Serial.begin(9600);
init_RTC();
 // (note 24-hour time: 3pm -> 15)
  // This line sets the RTC with an
  // explicit date & time, for example:
  // to set February 23, 2024 at 5:30pm
  // you would use the following line:
  //rtc.adjust(DateTime(2024, 3, 15, 18, 26, 0));
init_SD();
 logfile = open_next_logfile();

//magnetometer
  Serial.begin(9600);
  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit_MMC5603 Continuous Mode Magnetometer Test");
  Serial.println("");

  /* Initialise the sensor */
  if (!mmc.begin(MMC56X3_DEFAULT_ADDRESS, &Wire)) {  // I2C mode
    /* There was a problem detecting the MMC5603 ... check your connections */
    Serial.println("Ooops, no MMC5603 detected ... Check your wiring!");
    while (1) delay(10);
  }

  /* Display some basic information on this sensor */
  mmc.printSensorDetails();

  mmc.setDataRate(100); // in Hz, from 1-255 or 1000
  mmc.setContinuousMode(true);
//end of magnetometer
}

// ==================================================

void loop () {
 if (i < N) {
    // generate a random number (for test purposes), This is where the data from our magnetometer would go.
    //magnetometer
    // Get a new sensor event 
  sensors_event_t event;
  mmc.getEvent(&event);

  //Give values to components
  mx=event.magnetic.x;
  my=event.magnetic.y;
  mz=event.magnetic.z;
  // M[0]=mx;
  // M[1]=my;
  // M[2]=mz;

  // Display the results (magnetic vector values are in micro-Tesla (uT))
  // Serial.print(M[0]);
  // Serial.print(",");
  // Serial.print(M[1]);
  // Serial.print(",");
  // Serial.print(M[2]);
  // Serial.print("\n");
  
  // Delay before the next sample
  delay(waittime_ms);
    //end of magnetometer

// data = M;      // get X Y and Z data at once;

DateTime now = rtc.now();

// Write the date, time and data to log file
// Same as printing to Serial!
logfile.print(now.year()); 
logfile.print("-"); 
logfile.print(now.month()); 
logfile.print("-"); 
logfile.print(now.day()); 
logfile.print(",");
logfile.print(now.hour());
logfile.print(":"); 
logfile.print(now.minute()); 
logfile.print(":"); 
logfile.print(now.second()); 
logfile.print(","); 
logfile.print(mx);
logfile.print(","); 
logfile.print(my);
logfile.print(","); 
logfile.print(mz);
logfile.print(","); 
logfile.print(sqrt(mx*mx+my*my+mz*mz));
logfile.println();
 
// write same data to serial (again, only for debugging purposes)
// Serial.print(now.year()); 
// Serial.print("-"); 
// Serial.print(now.month()); 
// Serial.print("-"); 
// Serial.print(now.day()); 
// Serial.print(",");
// Serial.print(now.hour());
// Serial.print(":"); 
// Serial.print(now.minute()); 
// Serial.print(":"); 
// Serial.print(now.second()); 
// Serial.print(","); 
// Serial.print(M[0]);
// Serial.print(","); 
// Serial.print(M[1]);
// Serial.print(","); 
// Serial.print(M[2]);
// Serial.print(","); 
// Serial.print(sqrt(M[0]*M[0]+M[1]*M[1]+M[2]*M[2]));
// Serial.println();
delay(waittime_ms); //Delay between samples (ms)

i++; }
// Reached N samples, open the next log
// file to record N more
else {

 logfile.close();

 // comment out the next two lines to stop
  // recording after the first file

i = 0;
   logfile = open_next_logfile();
  }

}
//This next part is for setting up everything, DO NOT CHANGE
// =========================================
// initializes the RTC,
// and checks to see if it has been set
// =========================================

void init_RTC()
{

Serial.print("Initializing RTC...");
if (!rtc.begin()) { Serial.println(" failed!"); while (1);
}
Serial.println(" done!");
if (!rtc.isrunning()) Serial.println(
    "WARNING: RTC has not been previously set");

}
// ======================================================
// attempts to initialize the SD card for reading/writing
// ======================================================

void init_SD()
{

Serial.print("Initializing SD card...");
if (!SD.begin(SD_PIN)) { Serial.println(" failed!"); while (1);
}
Serial.println(" done!"); }
// =======================================================
// Opens the next available log file in SD:/LOGS/
// Write to the file using logfile.print() or println(),
// just like Serial
// =======================================================

File open_next_logfile()
{

 char filename[24];

 // Create folder for logs if it doesn’t already exist

 if (!SD.exists("/LOGS/"))
    SD.mkdir("/LOGS/");

 // find the first file LOGxxx.TXT that doesn’t exist,
  // then create, open and use that file
  for (int logn = 0; logn < 1000; logn++) {

   sprintf(filename, "/LOGS/LOG%03d.TXT", logn);

if (!SD.exists(filename)) { Serial.print("Opened \’SD:"); Serial.print(filename); Serial.println("\’ for logging."); break;
} }
 return SD.open(filename, FILE_WRITE);
}


