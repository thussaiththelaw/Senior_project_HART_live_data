#include <Wire.h>     
static int I2C_ID = 15;                         
int DO_Blink = 13;
byte I2C_OnOff;                                 
void setup() 
{
  pinMode(DO_Blink, OUTPUT);                    
  // initiate I2C 
  Wire.begin(I2C_ID);      
  // function to be called when an I2C event is received
  Wire.onReceive(recieveEvent);                      
}
void loop() 
{
  delay(1000);
  String num = String(random(10), DEC);
  String mesage = "sensor id: " + String(I2C_ID) + ":\t" + num;
  Wire.beginTransmission(1);
  Wire.write(mesage.c_str());
  // Wire.write("8");
  Wire.endTransmission();
}
void recieveEvent(int howMany)
{                   
  I2C_OnOff = Wire.read();
  if (I2C_OnOff == 1)
  {
   digitalWrite(DO_Blink, HIGH);                 
  }
  else if (I2C_OnOff == 0)
  {
   digitalWrite(DO_Blink, LOW);                 
  } 
}