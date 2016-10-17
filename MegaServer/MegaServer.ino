#include <EEPROM.h>

byte BLEClient[16][7];

void BLEClientLoad(int idx);  //0~15
void BLEClientSave(int idx);

void BLEEventLoop();
void ESP8266EventLoop();
void UserEventLoop();

void BLEEventLoop()
{
  String buf;
  if(Serial1.available() > 0) {
    buf = Serial1.readStringUntil('\n');
    if(buf.substring(0,2) == "&&" || buf.substring(0,2) == "^^") {
      Serial2.print(buf);
      Serial2.print("\n");
    }
    Serial.print("BLE:");
    Serial.println(buf);
  }
}

//FIXME : Echo etc.
void UserEventLoop()
{
  String buf;

  if(Serial.available() > 0) {
    buf = Serial.readStringUntil('\n');
    if(buf.substring(0,3) == "BLE") {
      Serial.print(buf);
      Serial.print("\r");
      Serial1.print(buf);
      Serial1.print("\r");
    } else if(buf.substring(0,2) == "^^" || buf.substring(0,2) == "&&") {
      Serial.print(buf);
      Serial.print("\n");
      Serial2.print(buf);
      Serial2.print("\n");
    }
  }
    //Serial.print("BLE:");
    //Serial.println(buf);
}

void ESP8266EventLoop()
{
  String buf;
  if(Serial2.available() > 0) {
    buf = Serial2.readStringUntil('\n');
    if(buf.substring(0,3) == "BLE") {
      Serial1.print(buf);
      Serial1.print("\r");
    } else if(buf.substring(0,2) == "AT") {
      Serial1.print(buf);
      Serial1.print("\r");
    }
    Serial.print("ESP:");
    Serial.println(buf);
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

void setup()
{
  Serial.begin(9600);  //DEBUG
  Serial1.begin(9600); //BLE
  Serial2.begin(9600); //ESP8266

  Serial.println("Mega Start");

  //delay(2000); //Wait 2Sec
  
  //Serial1.print("tpms\r"); //Start BLE

  Serial.println("BLE Start Done");
}

  int first_run=1;
  
void loop()
{
  if(first_run) {
    #if 1
    Serial1.print("BLEON\r");
    Serial.print("BLEON\r");
  Serial1.print("BLEADD001E77AC39DB297\r");
  Serial.print("BLEADD001E77AC39DB297\n");
  //delay(1000);
  Serial1.print("BLEADD1000000AAAB05AD\r");
  Serial.print("BLEADD1000000AAAB05AD\n");
  //delay(1000);
  Serial1.print("BLEADD2000000AAABAC08\r");
  Serial.print("BLEADD2000000AAABAC08\n");
  //delay(1000);
  Serial1.print("BLECON0\r");
  Serial.print("BLECON0\r");
  //TODO: Load client and Start Connect
  //delay(1000);
 // Serial1.print("BLESCN\r");
  //Serial.print("BLESCN\r");
  #endif
    first_run = 0;
  }
  BLEEventLoop();
  ESP8266EventLoop();

#define USER_EVENT
#ifdef USER_EVENT
  UserEventLoop();
#endif
}
