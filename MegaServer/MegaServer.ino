#include <EEPROM.h>

#define XFER_HEADER_SIZE 5 //AA,55,version,id,len

#define XFER_CMD_BLE_SCAN   0
#define XFER_CMD_BLE_SCAN_STOP   1
#define XFER_CMD_BLE_ADD_CLIENT  2
#define XFER_CMD_BLE_REMOVE_CLIENT  3
#define XFER_EVENT_BLE_SCAN_DATA  4
#define XFER_EVENT_SENSOR_DATA  5

#define XFER_EVENT_ACK 128
#define XFER_EVENT_NACK 129

byte xfer_buf[512];
int xfer_size = 0;
byte xfer_buf2[512];
int xfer_size2 = 0;

byte BLEClient[16][7];

void BLEClientLoad(int idx);  //0~15
void BLEClientSave(int idx);

void BLEEventLoop();
void ESP8266EventLoop();
void UserEventLoop();

void parse_xfer_buf();
void parse_xfer_buf2();

void send_data(byte cmd, byte *data, byte len);

void send_data(byte cmd, byte *data, byte len)
{
    int i;

    Serial2.write(0xAA);
    Serial2.write(0x55);
    Serial2.write(0x01);
    Serial2.write(cmd);
    Serial2.write(len);
    for(i=0;i<len;i++)
      Serial2.write(data[i]);

    Serial.println("Send Data Done.");
}

void parse_xfer_buf()
{
  byte *buf = xfer_buf;
  byte id = 0, version = 0, len = 0;
  int size = xfer_size;
  int wait_data = 0;

  while(size >= XFER_HEADER_SIZE) {
    if(*buf++ != 0xAA) {
      size--;
      continue;
    }

    if(*buf++ != 0x55) {
      size--;
      continue;
    }

    version = *buf++;
    size--;

    id = *buf++;
    size--;

    len = *buf++;
    size--;

    if(size < len) { //wait more data
      int i;

      xfer_buf[0] = 0xAA;
      xfer_buf[1] = 0x55;
      xfer_buf[2] = version;
      xfer_buf[3] = id;
      xfer_buf[4] = len;

      for(i=0;i<size;i++) {
          xfer_buf[5+i] = buf[i];
      }

      xfer_size = size + 5;
      wait_data = 1;
      break;
    }

    Serial.print(id);
    Serial.println(" Received");

    switch (id) {
      case XFER_CMD_BLE_SCAN:
        //test
        send_data(XFER_EVENT_BLE_SCAN_DATA, NULL, 0);
      break;
      case XFER_CMD_BLE_SCAN_STOP:
      break;
      case XFER_CMD_BLE_ADD_CLIENT:
      break;
      case XFER_CMD_BLE_REMOVE_CLIENT:
      break;
      case XFER_EVENT_ACK:
        Serial.println("ACK Received");
      break;
      case XFER_EVENT_NACK:
        Serial.println("NACK Received");
      break;
    }

    size -= len;
    buf +=len;
  }

  if(size && wait_data == 0) {
    int i;
    for(i=0;i<size;i++)
    xfer_buf[i] = buf[i];
    xfer_size = size;
  }
}

void parse_xfer_buf2()
{
  byte *buf = xfer_buf2;
  byte id = 0, version = 0, len = 0;
  int size = xfer_size2;
  int wait_data = 0;

  while(size >= XFER_HEADER_SIZE) {
    if(*buf++ != 0xAA) {
      size--;
      continue;
    }

    if(*buf++ != 0x55) {
      size--;
      continue;
    }

    version = *buf++;
    size--;

    id = *buf++;
    size--;

    len = *buf++;
    size--;

    if(size < len) { //wait more data
      int i;

      xfer_buf2[0] = 0xAA;
      xfer_buf2[1] = 0x55;
      xfer_buf2[2] = version;
      xfer_buf2[3] = id;
      xfer_buf2[4] = len;

      for(i=0;i<size;i++) {
          xfer_buf2[5+i] = buf[i];
      }

      xfer_size2 = size + 5;
      wait_data = 1;
      break;
    }

    Serial.print(id);
    Serial.println("CMD Received");
    //parse data
    switch (id) {
      case XFER_EVENT_BLE_SCAN_DATA:
      break;
      case XFER_EVENT_ACK:
        Serial.println("ACK Received");
      break;
      case XFER_EVENT_NACK:
        Serial.println("NACK Received");
      break;
    }
    size -= len;
    buf +=len;
  }

  if(size && wait_data == 0) {
    int i;
    for(i=0;i<size;i++)
    xfer_buf2[i] = buf[i];
    xfer_size2 = size;
  }
}

void BLEEventLoop()
{
  while (Serial1.available() > 0) {
    if(xfer_size2 <= 512) {
      xfer_buf2[xfer_size2] = Serial1.read();
      xfer_size2++;
    } else {
      parse_xfer_buf2();
    }
  }
  parse_xfer_buf2();
}

//FIXME : Echo etc.
void UserEventLoop()
{
  while (Serial.available() > 0) {
    if(xfer_size <= 512) {
      xfer_buf[xfer_size] = Serial.read();
      xfer_size++;
    } else {
      //xfer_buf full : flush?
      parse_xfer_buf();
    }
  }
  //Serial.println("");
  //Serial.print(xfer_size, HEX);
  //Serial.println("");
}

void ESP8266EventLoop()
{
  while (Serial2.available() > 0) {
    if(xfer_size <= 512) {
      xfer_buf[xfer_size] = Serial2.read();
      xfer_size++;
    } else {
      parse_xfer_buf();
    }
  }
  parse_xfer_buf();
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

void setup()
{
  Serial.begin(9600);  //DEBUG
  Serial1.begin(9600); //BLE
  Serial2.begin(9600); //ESP8266
}

void loop()
{
  BLEEventLoop();
  ESP8266EventLoop();

//#define USER_EVENT
#ifdef USER_EVENT
  UserEventLoop();
#endif
}
