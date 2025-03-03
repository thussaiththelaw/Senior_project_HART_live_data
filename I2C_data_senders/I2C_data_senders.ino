#include <Wire.h> //I2C library

// I2C settup
const uint8_t I2C_myAddress = 0b0000010; //7bit number identifing this device on the I2C Bus
const uint8_t I2C_controllerAddress = 0b0000001; //7bit number identifing where data should be sent to
void I2C_setUp();
void I2C_send(char message[]);

void setup() {
  // put your setup code here, to run once:
  
}

void loop() {
  delay(5000);
  I2C_send("2 resonable message");

}

/*  sends data to I2C data controller
    takes in a string
    functions like println() but to controller
    */
void I2C_send(char message[]) {
  Serial.println("in function");
  int index = 0;
  int bytes_sent = 0;

  // gain controll of the buss
  Wire.beginTransmission(I2C_controllerAddress);
  delay(10);
  Wire.endTransmission(false);
  Serial.println("first tansmision");

  do {
    // makes sure data collector isnt busy
    bool busy = true;
    while (busy) {
      Wire.requestFrom(I2C_controllerAddress, 1, false);
      delay(5);
      busy = Wire.read();
    }

    // send the mesage in 32 byte chunks
    Wire.beginTransmission(I2C_controllerAddress);
    do {
      Wire.write(message[index]);
      
      index++;
      bytes_sent++;
    } while(message[index-1] != 0 && bytes_sent < 32);

    Wire.endTransmission(false);
    bytes_sent = 0;
  } while(message[index-1] != 0);

  // release control of the buss
  Wire.beginTransmission(I2C_controllerAddress);
  delay(10);
  Wire.endTransmission(true);
} 
