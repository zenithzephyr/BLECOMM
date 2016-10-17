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

static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name = "viewport" content = "width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0">
<title>Central Server Demo</title>
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

		var recvData = evt.data;
		if (recvData.indexOf("&&") == 0) {
			var type = recvData.charAt(2);
			var addrData = recvData.substr(3,14); 
			var i;

			if(type == '0')
				type = "Cargo ";
			else
				type = "TPMS ";

			for(i=0;i<10;i++) {
				if(document.getElementById("ADDR"+i).innerHTML == "None") {
					document.getElementById("TYPE"+i).innerHTML = type;
					document.getElementById("ADDR"+i).innerHTML = addrData;
					break;
				}
				else if(document.getElementById("ADDR"+i).innerHTML == addrData) {
					break;
				}
			}
		}
		else if (recvData.indexOf("^^") == 0) {
			var dat_idx = recvData.charAt(3); 
			var tpms_str = recvData.substr(2,1);

      if(tpms_str == "0") {       
        var eventData = recvData.substr(4,2);
        switch(dat_idx) {
         case '0':
            document.getElementById("DOOR_SEN").innerHTML = eventData;
            break;
          case '1':
           document.getElementById("CARGO_SEN1").innerHTML = eventData;
            break;
        }
      } else {      
        var eventData = recvData.substr(4,4);
		  	switch(dat_idx) {
		  		case '0':
		  			document.getElementById("TPMS"+tpms_str+"_BATT").innerHTML = eventData;
		  			break;
		  		case '1':
		  			document.getElementById("TPMS"+tpms_str+"_TEMP").innerHTML = eventData;
		  			break;
		  		case '2':
		  			document.getElementById("TPMS"+tpms_str+"_PRES").innerHTML = eventData;
		  			break;
			  }
      }

		}
		else {
			console.log('unknown event');
		}
	};
}

function buttonclick(e) {
	websock.send(e.id);
}

function addButtonclick(e) {

	var idx = e.id.charAt(4);
	var type = "TYPE"+idx;
	var addr = "ADDR"+idx;
	var sel = "SEL"+idx;
	var send_msg;

	if(e.id.substr(0,4) == "ADDB") {
		if(document.getElementById(type).innerHTML == "Cargo ") { 
			send_msg = "BLEADD0" + document.getElementById(addr).innerHTML;	
			websock.send(send_msg);
		}
		else if(document.getElementById(type).innerHTML == "TPMS ") { 
			send_msg = "BLEADD"+ document.getElementById(sel).selectedIndex + document.getElementById(addr).innerHTML;	
			websock.send(send_msg);
		} 
	}
}
</script>
</head>
<body onload="javascript:start();">
<h1>Cargo Status Demo</h1>

<ul>
	<li>Door Sensor: <span id="DOOR_SEN">Status</span></li>
	<li>Cargo Sensor 1: <span id="CARGO_SEN1">Status</span></li>
	<li>Cargo Sensor 2: <span id="CARGO_SEN2">Status</span></li>
	<li>Cargo Sensor 3: <span id="CARGO_SEN3">Status</span></li>
	<button id="BLEDEL0" type="button" onclick="buttonclick(this);">Remove</button>
</ul>

<ul>
	<li>TPMS1 Battery: <span id="TPMS1_BATT">Status</span></li>
	<li>TPMS1 Pressure: <span id="TPMS1_PRES">Status</span></li>
	<li>TPMS1 Temperature: <span id="TPMS1_TEMP">Status</span></li>
	<button id="BLEDEL1" type="button" onclick="buttonclick(this);">Remove</button>
</ul>

<ul>
	<li>TPMS2 Battery: <span id="TPMS2_BATT">Status</span></li>
	<li>TPMS2 Pressure: <span id="TPMS2_PRES">Status</span></li>
	<li>TPMS2 Temperature: <span id="TPMS2_TEMP">Status</span></li>
	<button id="BLEDEL2" type="button" onclick="buttonclick(this);">Remove</button>
</ul>

<ul>
	<li>TPMS3 Battery: <span id="TPMS3_BATT">Status</span></li>
	<li>TPMS3 Pressure: <span id="TPMS3_PRES">Status</span></li>
	<li>TPMS3 Temperature: <span id="TPMS3_TEMP">Status</span></li>
	<button id="BLEDEL3" type="button" onclick="buttonclick(this);">Remove</button>
</ul>

<ul>
	<li>TPMS4 Battery: <span id="TPMS4_BATT">Status</span></li>
	<li>TPMS4 Pressure: <span id="TPMS4_PRES">Status</span></li>
	<li>TPMS4 Temperature: <span id="TPMS4_TEMP">Status</span></li>
	<button id="BLEDEL4" type="button" onclick="buttonclick(this);">Remove</button>
</ul>

<ul>
	<button id="BLESCN" type="button" onclick="buttonclick(this);">Scan</button>
	<button id="BLESTP" type="button" onclick="buttonclick(this);">Stop</button>
</ul>

<ul>
	<li><span id="TYPE0">None</span><span id="ADDR0">None</span><button id="ADDB0" type="button" onclick="addButtonclick(this);">Add</button><select id="SEL0"><option value="0">0</option><option value="1">1</option><option value="2">2</option><option value="3">3</option><option value="4">4</option></select></li>
	<li><span id="TYPE1">None</span><span id="ADDR1">None</span><button id="ADDB1" type="button" onclick="addButtonclick(this);">Add</button><select id="SEL1"><option value="0">0</option><option value="1">1</option><option value="2">2</option><option value="3">3</option><option value="4">4</option></select></li>
	<li><span id="TYPE2">None</span><span id="ADDR2">None</span><button id="ADDB2" type="button" onclick="addButtonclick(this);">Add</button><select id="SEL2"><option value="0">0</option><option value="1">1</option><option value="2">2</option><option value="3">3</option><option value="4">4</option></select></li>
	<li><span id="TYPE3">None</span><span id="ADDR3">None</span><button id="ADDB3" type="button" onclick="addButtonclick(this);">Add</button><select id="SEL3"><option value="0">0</option><option value="1">1</option><option value="2">2</option><option value="3">3</option><option value="4">4</option></select></li>
	<li><span id="TYPE4">None</span><span id="ADDR4">None</span><button id="ADDB4" type="button" onclick="addButtonclick(this);">Add</button><select id="SEL4"><option value="0">0</option><option value="1">1</option><option value="2">2</option><option value="3">3</option><option value="4">4</option></select></li>
	<li><span id="TYPE5">None</span><span id="ADDR5">None</span><button id="ADDB5" type="button" onclick="addButtonclick(this);">Add</button><select id="SEL5"><option value="0">0</option><option value="1">1</option><option value="2">2</option><option value="3">3</option><option value="4">4</option></select></li>
	<li><span id="TYPE6">None</span><span id="ADDR6">None</span><button id="ADDB6" type="button" onclick="addButtonclick(this);">Add</button><select id="SEL6"><option value="0">0</option><option value="1">1</option><option value="2">2</option><option value="3">3</option><option value="4">4</option></select></li>
	<li><span id="TYPE7">None</span><span id="ADDR7">None</span><button id="ADDB7" type="button" onclick="addButtonclick(this);">Add</button><select id="SEL7"><option value="0">0</option><option value="1">1</option><option value="2">2</option><option value="3">3</option><option value="4">4</option></select></li>
	<li><span id="TYPE8">None</span><span id="ADDR8">None</span><button id="ADDB8" type="button" onclick="addButtonclick(this);">Add</button><select id="SEL8"><option value="0">0</option><option value="1">1</option><option value="2">2</option><option value="3">3</option><option value="4">4</option></select></li>
	<li><span id="TYPE9">None</span><span id="ADDR9">None</span><button id="ADDB9" type="button" onclick="addButtonclick(this);">Add</button><select id="SEL9"><option value="0">0</option><option value="1">1</option><option value="2">2</option><option value="3">3</option><option value="4">4</option></select></li>
</ul>

</body>
</html>
)rawliteral";

// Commands sent through Web Socket
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  //Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
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
      //Serial.printf("[%u] get Text: %s\r\n", num, payload);
      Serial.printf("%s\r\n",payload);
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
  String ChipId = "AutoConAP_";
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

void loop()
{
    // put your main code here, to run repeatedly:
    //check gpio and reset settings
#if 1
    if(digitalRead(2) == 0) {
      Serial.println("AP Reset Triggered");
      delay(1000);
      wifiManager.resetSettings();
      Serial.println("AP Configuration Reseted");
      delay(1000);
      //esp restart
      ESP.restart();
    }
#endif

    if(Serial.available() > 0) {
      String buf;
      buf = Serial.readStringUntil('\n');//readbyte
      webSocket.broadcastTXT(buf);
    }

    //TODO : add web server setup page for add client and server ip
    webSocket.loop();
    server.handleClient();

}
