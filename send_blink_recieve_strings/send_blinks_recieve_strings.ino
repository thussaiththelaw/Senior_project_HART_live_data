#include <Wire.h>                                                        
String readString;                                                          
byte I2C_OnOff;     
int DO_Blink = 13;  
bool count = 0;
static int I2C_ID = 1;      

void setup()
{
  // initiate I2C 
  Wire.begin(I2C_ID);      
  // function to be called when an I2C event is received
  Wire.onReceive(recieveEvent);     

  // initialize serial terminal output
  Serial.begin(9600);
  Serial.println("starting send blinks recieve strings");   
}

void loop()
{
  if(count)
  {
    I2C_OnOff = 1;
  }
  if(!count)
  {
    I2C_OnOff = 0;
  }
  Wire.beginTransmission(15);                                            
  Wire.write(I2C_OnOff);   
  Wire.endTransmission();      
  delay(1000);
  count = ! count;                                             
}

char buffer[100];
void recieveEvent(int howMany)
{
  for(int i = 0; i < howMany; i++)
  {
    buffer[i] = Wire.read();
  }
  // this should not be in the interupt but... i want it to be for now
  Serial.println(buffer);
}