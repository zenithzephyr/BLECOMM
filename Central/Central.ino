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

typedef struct {
  int temperature;
  int pressure;
  int battery;
} tpms_t;

tpms_t tpms[8];
int door;
int cargo[3];
int LOFF;
//test data 고정길이 and TEXT BASE
// READ FORMAT
// &IDX|DATATYPE|DATA|CRC|
// &10|1|CRC|
// &11|0|CRC|
// TYPE 0 : CARGO + DOOR, TYPE 1 : TPMS
// DATA TYPE : 0: CARGO 1:DOOR, 2: TEMPERATURE, 3:PRESSURE, 4:BATTERY

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
  };
}
function buttonclick(e) {
  websock.send(e.id);
}
</script>
</head>
<body onload="javascript:start();">
<h1>ESP8266 WebSocket Demo</h1>
<div id="ledstatus"><b>LED</b></div>
<button id="ledon"  type="button" onclick="buttonclick(this);">On</button>
<button id="ledoff" type="button" onclick="buttonclick(this);">Off</button>
</body>
</html>
)rawliteral";

// GPIO#0 is for Adafruit ESP8266 HUZZAH board. Your board LED might be on 13.
const int LEDPIN = 0;
// Current LED status
bool LEDStatus;

// Commands sent through Web Socket
const char LEDON[] = "ledon";
const char LEDOFF[] = "ledoff";

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
        // Send the current LED status
        if (LEDStatus) {
          webSocket.sendTXT(num, LEDON, strlen(LEDON));
        }
        else {
          webSocket.sendTXT(num, LEDOFF, strlen(LEDOFF));
        }
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\r\n", num, payload);

      if (strcmp(LEDON, (const char *)payload) == 0) {
        writeLED(true);
      }
      else if (strcmp(LEDOFF, (const char *)payload) == 0) {
        writeLED(false);
      }
      else {
        Serial.println("Unknown command");
      }
      // send data to all connected clients
      webSocket.broadcastTXT(payload, length);
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

void handleRoot() {
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

static void writeLED(bool LEDon)
{
  LEDStatus = LEDon;
  // Note inverted logic for Adafruit HUZZAH board
  if (LEDon) {
    digitalWrite(LEDPIN, 0);
  }
  else {
    digitalWrite(LEDPIN, 1);
  }
}

void setup() {
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

void loop() {
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

    //TODO : read data to uart (BLE Data)
    if(Serial.available() > 0) {
        //check protocol and parse data
        //incomingByte = Serial.readStringUntil('\n');//readbyte
    }

    //TODO : add web server setup page for add client and server ip
    webSocket.loop();
    server.handleClient();

    if(LOFF ==1) {
    webSocket.broadcastTXT(LEDOFF, strlen(LEDOFF));
    LOFF = 0;
    } else {
      webSocket.broadcastTXT(LEDON, strlen(LEDON));
    LOFF = 1;
    }
    delay(1000);
}

