#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ESP8266mDNS.h>

MDNSResponder mdns;

WiFiManager wifiManager;

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void send_cmd(byte id, byte *data, byte len);
void parse_xfer_buf();

#define XFER_CMD_BLE_SCAN   0
#define XFER_CMD_BLE_SCAN_STOP   1
#define XFER_CMD_BLE_ADD_CLIENT  2
#define XFER_CMD_BLE_REMOVE_CLIENT  3
#define XFER_EVENT_BLE_SCAN_DATA  4
#define XFER_EVENT_SENSOR_DATA  5

#define XFER_EVENT_ACK 128
#define XFER_EVENT_NACK 129

#define XFER_HEADER_SIZE 5 //AA,55,version,id,len

byte xfer_buf[512];
int xfer_size;

static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name = "viewport" content = "width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0">
<title>ESP8266 WebSocket Demo</title>
<style>
"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }"
</style>
<script>
var websock;
function start() {
  websock = new WebSocket('ws://' + window.location.hostname + ':81/');
  websock.onopen = function(evt) { console.log('websock open'); };
  websock.onclose = function(evt) { console.log('websock close'); };
  websock.onerror = function(evt) { console.log(evt); };
  websock.onmessage = function(evt) {
    console.log(evt);
    var e = document.getElementById('ledstatus');
    if (evt.data === 'ledon') {
      e.style.color = 'red';
    }
    else if (evt.data === 'ledoff') {
      e.style.color = 'black';
    }
    else {
      console.log('unknown event');
    }
    //SCAN LIST
    //TODO BLE ADD Button
    //DOM Add
  };
}
function buttonclick(e) {
  websock.send(e.id);
}
</script>
</head>
<body onload="javascript:start();">
<h1>Cargo Status Demo</h1>

<ul>
<li>Door Sensor: <div id="DOOR_SEN"><b>Status</b></div></li>
<li>Cargo Sensor 1: <div id="CARGO_SEN1"><b>Status</b></div></li>
<li>Cargo Sensor 2: <div id="CARGO_SEN2"><b>Status</b></div></li>
<li>Cargo Sensor 3: <div id="CARGO_SEN3"><b>Status</b></div></li>
<li><button id="BLEREMOVE:0" type="button" onclick="buttonclick(this);">Remove</button></li>
</ul>

<ul>
<li>TPMS1 Battery: <div id="TPMS1_BATT"><b>Status</b></div></li>
<li>TPMS1 Pressure: <div id="TPMS1_PRES"><b>Status</b></div></li>
<li>TPMS1 Temperature: <div id="TPMS1_TEMP"><b>Status</b></div></li>
<li><button id="BLEREMOVE:1" type="button" onclick="buttonclick(this);">Remove</button></li>
</ul>

<ul>
<li>TPMS2 Battery: <div id="TPMS2_BATT"><b>Status</b></div></li>
<li>TPMS2 Pressure: <div id="TPMS2_PRES"><b>Status</b></div></li>
<li>TPMS2 Temperature: <div id="TPMS2_TEMP"><b>Status</b></div></li>
<li><button id="BLEREMOVE:2" type="button" onclick="buttonclick(this);">Remove</button></li>
</ul>

<ul>
<li>TPMS3 Battery: <div id="TPMS3_BATT"><b>Status</b></div></li>
<li>TPMS3 Pressure: <div id="TPMS3_PRES"><b>Status</b></div></li>
<li>TPMS3 Temperature: <div id="TPMS3_TEMP"><b>Status</b></div></li>
<li><button id="BLEREMOVE:3" type="button" onclick="buttonclick(this);">Remove</button></li>
</ul>

<ul>
<li>TPMS4 Battery: <div id="TPMS4_BATT"><b>Status</b></div></li>
<li>TPMS4 Pressure: <div id="TPMS4_PRES"><b>Status</b></div></li>
<li>TPMS4 Temperature: <div id="TPMS4_TEMP"><b>Status</b></div></li>
<li><button id="BLEREMOVE:4" type="button" onclick="buttonclick(this);">Remove</button></li>
</ul>

<button id="BLESCAN" type="button" onclick="buttonclick(this);">Scan</button>
<button id="BLESTOP" type="button" onclick="buttonclick(this);">Stop</button>

</body>
</html>
)rawliteral";

// Commands sent through Web Socket
const char BLESCAN[] = "BLESCAN";
const char BLESTOP[] = "BLESTOP";
const char BLEADD[] = "BLEADD";
const char BLEREMOVE[] = "BLEREMOVE";

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\r\n", num, payload);

      if (strcmp(BLESCAN, (const char *)payload) == 0) {
        //Send BLE SCAN CMD to Meag
        Serial.printf("%s\n", payload);
        send_cmd(XFER_CMD_BLE_SCAN, NULL, 0);
      }
      else if (strcmp(BLESTOP, (const char *)payload) == 0) {
        //Send BLE SCAN STOP CMD to Mega
        Serial.printf("%s\n", payload);
        send_cmd(XFER_CMD_BLE_SCAN_STOP, NULL, 0);
      }
      else if (strncmp(BLEADD, (const char *)payload, 6) == 0) {  //FIXME:strncmp
        //Send BLE ADD CMD to Mega
        Serial.printf("%s\n",payload);
        send_cmd(XFER_CMD_BLE_ADD_CLIENT, NULL, 0); //FIXME
      }
      else if (strncmp(BLEREMOVE, (const char *)payload, 9) == 0) { //FIXME:strncmp
        //Send BLE ADD CMD to Mega
        Serial.printf("%s\n",payload);
        send_cmd(XFER_CMD_BLE_REMOVE_CLIENT, NULL, 0);
      }
      else {
        Serial.println("Unknown command");
      }
      // send data to all connected clients
      //webSocket.broadcastTXT(payload, length);
      break;
    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\r\n", num, length);
      hexdump(payload, length);

      // echo data back to browser
      webSocket.sendBIN(num, payload, length);
      break;
    default:
      Serial.printf("Invalid WStype [%d]\r\n", type);
      break;
  }
}

void handleRoot()
{
  server.send(200, "text/html", INDEX_HTML);
}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  //WiFiManager wifiManager;
  //reset saved settings
  //wifiManager.resetSettings();

  //set custom ip for portal
  wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  String ChipId = "ConAP_";
  ChipId += String(ESP.getChipId(), HEX);
  ChipId.toUpperCase();
  Serial.println(ChipId);
  char buf[32] = "";
  ChipId.toCharArray(buf, 32);
  wifiManager.autoConnect(buf, "12345678");
  //or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();


  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");


  if (mdns.begin("apcon", WiFi.localIP())) {
    Serial.println("MDNS responder started");
    mdns.addService("http", "tcp", 80);
    mdns.addService("ws", "tcp", 81);

    server.on ( "/", handleRoot );
    server.onNotFound(handleNotFound);

    server.begin();

    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    Serial.println("Server started");

  }
  else {
    Serial.println("MDNS.begin failed");
  }
  Serial.print("Connect to http://apcon.local or http://");
  Serial.println(WiFi.localIP());

}

void send_cmd(byte id, byte *data, byte len)
{
    int i;

    Serial.write(0xAA);
    Serial.write(0x55);
    Serial.write(0x01);
    Serial.write(id);
    Serial.write(len);
    for(i=0;i<len;i++)
      Serial.write(data[i]);

    Serial.println("Send CMD Done.");
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

    //Serial.print(id);
    //Serial.println("data Received");
    //parse data
    switch (id) {
      case XFER_EVENT_BLE_SCAN_DATA:
      //webSocket.broadcastTXT(LEDON, strlen(LEDON));
      send_cmd(XFER_EVENT_ACK, NULL, 0);
      break;
      case XFER_EVENT_SENSOR_DATA:
      send_cmd(XFER_EVENT_NACK, NULL, 0);
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

void loop()
{
    // put your main code here, to run repeatedly:
    //check gpio and reset settings
    if(digitalRead(2) == 0) {
      Serial.println("AP Reset Triggered");
      delay(1000);
      wifiManager.resetSettings();
      Serial.println("AP Configuration Reseted");
      delay(1000);
      //esp restart
      ESP.restart();
    }

#if 0
    //TODO : read data to uart (BLE Data)
    if(Serial.available() > 0) {
        String eventString;
        char eventChar[128] = {0, };
        eventString = Serial.readStringUntil('\n');//readbyte
        eventString.toCharArray(eventChar,128);
        Serial.printf("[%s]\n", eventChar);
    }
#endif
    while(Serial.available() > 0) {
      if(xfer_size < 512) {
        xfer_buf[xfer_size] = Serial.read();
        xfer_size++;
      } else {
          //xfer_buf full : flush?
          parse_xfer_buf();
      }
    }
    parse_xfer_buf();

    //TODO : add web server setup page for add client and server ip
    webSocket.loop();
    server.handleClient();

}
