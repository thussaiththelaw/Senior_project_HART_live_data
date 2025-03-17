

// I2C settup from senior project team
#include <Wire.h> //I2C library
const uint8_t I2C_myAddress = 0b0000010; //7bit number identifing this device on the I2C Bus
const uint8_t I2C_dataCollectorAddress = 0b0000001; //7bit number identifing where data should be sent to
void I2C_setUp();
void I2C_send(char message[]);
void I2C_send(String message);

void setup() {
  // put your setup code here, to run once:
  I2C_setUp();
}

void loop() {
  delay(5000);
  I2C_send("2 resonable message");

}

// run at startup initilizes I2C comunication
void I2C_setUp() {
  Wire.begin(I2C_myAddress);
  Wire.setClock(10000);
  Wire.setWireTimeout(25000);//0 is no timeout
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
  delay(10);
  Wire.endTransmission(true);
} 
/*  sends data to I2C data controller
    takes in a string
    functions like println() but to controller
    */
void I2C_send(String message){
  I2C_send(message.c_str());
}
