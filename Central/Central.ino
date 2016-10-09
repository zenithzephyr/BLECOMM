#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <EEPROM.h>

WiFiManager wifiManager;

ESP8266WebServer server(80);

typedef struct {
  int temperature;
  int pressure;
  int battery;
} tpms_t;

tpms_t tpms[8];
int door;
int cargo[3];

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

void handleRoot() {
  server.send(200, "text/plain", "hello from esp8266!");
}

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);

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
    server.on ( "/", handleRoot );
     server.begin();
    Serial.println("Server started");
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
        //incomingByte = Serial.read();//readbyte
    }

    #if 0
    //TODO : connect TCP Server and send msg for PC
    char *host = "192.168.0.14";
    Serial.print("Connecting to ");
    Serial.println(host);
    WiFiClient client;
    const int tcpPort = 2222;
    if(!client.connect(host, tcpPort)) {
      Serial.println("Connection Failed");
    } else {
      client.println("AAA"); //json or etc
      client.stop();
    }
    #endif

    //TODO : add web server setup page for add client and server ip
    server.handleClient();

    delay(1000);
}

void server_routine()
{
  //cmd to BL620 ble scan (check adv packet)
  //get ble addr

  //add ble addr
  // EEPROM.begin(256);
  // EEPROM.read(1);
  // EEPROM.write(0,1);
  // EEPROM.end();
}
