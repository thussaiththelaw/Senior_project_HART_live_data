#include <Wire.h> //I2C library

// I2C variables
const int I2C_bufferSize = 32; //make sure it is the same size as the Wire.h buffer
bool I2C_recievedFlag;
char I2C_buffer[I2C_bufferSize];

// only thing that should need to be changed up here
const uint8_t I2C_myAddress = 0b0000001; //7bit number identifing this device on the I2C Bus

// string buffer variables
const int charBufferSize = 1024;
char charBuffer[charBufferSize];
int charBufferIndex;

// function headers
void I2C_recieve();
void I2C_interupt(int number_of_bytes);
void I2C_request();
void radioSend(char message[]);


void setup() {
  // put your setup code here, to run once:
  
  // set up buffers
  for (int i = 0; i < I2C_bufferSize; i++) I2C_buffer[i] = 0;
  for (int i = 0; i < charBufferSize; i++) charBuffer[i] = 0;
  charBufferIndex = 0;
  int i = 0;

  // initiate serial protocol
  Serial.begin(19200);

  // initeate I2C protocol 
  I2C_recievedFlag = false;
  Wire.begin(I2C_myAddress);
  Wire.onReceive(I2C_interupt);
  Wire.onRequest(I2C_request);
  radioSend("starting");
}

void loop() {
  if(I2C_recievedFlag) {
    I2C_recieve();
  }
}

/*  reads the I2C_buffer string
    sends the string to the radio if not empty
    */
void I2C_recieve() {
    int i = 0;
    // copy part of mesage to whole mesage
    do {
      charBuffer[charBufferIndex] = I2C_buffer[i];
      charBufferIndex++;
      i++;
    } while ((I2C_buffer[i-1] != 0) && (i < I2C_bufferSize) && (charBufferIndex < charBufferSize - 1));
    


    // if message is finished 
    if (I2C_buffer[i-1] == 0 || charBufferIndex >= charBufferSize - 1) {
      // send message
      if (charBuffer[0] != 0) radioSend(charBuffer);
      // clear message buffer
      i = 0;
      while (i <= charBufferIndex && i < charBufferSize) {
        charBuffer[i] = 0;
        i++;
      }
      charBufferIndex = 0;
    }
  // clear I2C_buffer
  for (int i = 0; i < I2C_bufferSize; i++) I2C_buffer[i] = 0;
  I2C_recievedFlag = false; //set flag to false
}

/*  handles an I2C interupt
    sets flag I2C_recieved_flag to true
    interprets incoming bytes as chars
    stores as string I2C_buffer 
    */
void I2C_interupt(int number_of_bytes) {
  for(int i = 0; i < number_of_bytes; i++)
  {
    I2C_buffer[i] = Wire.read();
  }
  I2C_recievedFlag = true; //set flag to true
}

void I2C_request() {
  Wire.write(I2C_recievedFlag);
}

/*  the code to send data over meshtastic
    for now it is a serial print
    */
void radioSend(char message[]){
  Serial.println(message);
}