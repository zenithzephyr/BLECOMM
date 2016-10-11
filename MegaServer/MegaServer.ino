/*
  SerialPassthrough sketch

  Some boards, like the Arduino 101, the MKR1000, Zero, or the Micro,
  have one hardware serial port attached to Digital pins 0-1, and a
  separate USB serial port attached to the IDE Serial Monitor.
  This means that the "serial passthrough" which is possible with
  the Arduino UNO (commonly used to interact with devices/shields that
  require configuration via serial AT commands) will not work by default.

  This sketch allows you to  emulate the serial passthrough behaviour.
  Any text you type in the IDE Serial monitor will be written
  out to the serial port on Digital pins 0 and 1, and vice-versa.

  On the 101, MKR1000, Zero, and Micro, "Serial" refers to the USB Serial port
  attached to the Serial Monitor, and "Serial1" refers to the hardware
  serial port attached to pins 0 and 1. This sketch will emulate Serial passthrough
  using those two Serial ports on the boards mentioned above,
  but you can change these names to connect any two serial ports on a board
  that has multiple ports.

   Created 23 May 2016
   by Erik Nyquist
*/
#include <EEPROM.h>

void BLEClientLoad(int idx);  //0~15
void BLEClientSave(int idx);

void BL620EventLoop();
void ESP8266EventLoop();

byte BLEClient[16][7];
void setup() {
  int i;
  
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial2.begin(9600);

#if 0
  BLEClient[0][0] = 0x00;
  BLEClient[0][1] = 0x00;
  BLEClient[0][2] = 0x00;
  BLEClient[0][3] = 0xAA;
  BLEClient[0][4] = 0xAB;
  BLEClient[0][5] = 0x05;
  BLEClient[0][6] = 0xAD;
  BLEClientSave(0);
  BLEClient[0][0] = 0x00;
  BLEClient[0][1] = 0x00;
  BLEClient[0][2] = 0x00;
  BLEClient[0][3] = 0x00;
  BLEClient[0][4] = 0x00;
  BLEClient[0][5] = 0x00;
  BLEClient[0][6] = 0x00;
  BLEClientLoad(0);
  for(i=0;i<7;i++) {
  if(BLEClient[0][i] < 0x10)
    Serial.print('0');
  Serial.print(BLEClient[0][i], HEX);
  }
  Serial.println("");
  #endif
}

void loop() {
  //BL620EventLoop();

  ESP8266EventLoop();

  #if 0     //FOR TEST ONLY
  if (Serial.available()) {      // If anything comes in Serial (USB),
    Serial1.write(Serial.read());   // read it and send it out Serial1 (pins 0 & 1)
  }
  #endif

  //TEST
    Serial2.println("BLESCANDATA:00AABBCCDDEEFF|AAAAAAAAAAAA|AAAAAAAAAAAAAAA|AAAAAAAAAAAAA|AAAAAAAAAAAA");
    delay(1000);
}

void BL620EventLoop()
{
  if (Serial1.available()) {    
    Serial.println("");
    Serial.print("BL620: ");
    Serial.print(Serial1.readStringUntil('\n'));   // read it and send it out Serial (USB)
    Serial.println("");
  }  
}

void ESP8266EventLoop()
{
  if (Serial2.available()) {    
    Serial.println("");
    Serial.print("ESP8266: ");
    Serial.print(Serial2.readStringUntil('\n'));   // read it and send it out Serial (USB)
    Serial.println("");
  }
}

void BLEClientLoad(int idx)  //0~15
{
  int i;
  
  for(i=0;i<7;i++)
    BLEClient[idx][i] = EEPROM.read(idx*7+i);

  Serial.println("BLEClientLoad");
}

void BLEClientSave(int idx)
{
  int i;
  
  for(i=0;i<7;i++)
    EEPROM.write(idx*7+i,BLEClient[idx][i]);

  Serial.println("BLEClientSave");
}

