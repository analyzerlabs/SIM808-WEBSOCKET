//~ #include <ESP8266WiFi.h>
#include "WebSocketClient.h"
#include <DFRobot_sim808.h>
#include <SoftwareSerial.h>

//~ const char* ssid     = "SSID HERE";
//~ const char* password = "PASSWORD HERE";
char path[] = "/";
char host[] = "efectibit.com:314";

WebSocketClient webSocketClient;

#define PIN_TX    18
#define PIN_RX    19

// Use WiFiClient class to create TCP connections
//~ WiFiClient client;
SoftwareSerial mySerial(PIN_TX,PIN_RX);
DFRobot_SIM808 client(&mySerial);//Connect RX,TX,PWR,

void setup() {
	Serial.begin(115200);
	delay(10);

	//~ Serial.println();
	//~ Serial.println();
	//~ Serial.print("Connecting to ");
	//~ Serial.println(ssid);
	
	//~ WiFi.begin(ssid, password);
	
	//~ while (WiFi.status() != WL_CONNECTED) {
		//~ delay(500);
		//~ Serial.print(".");
	//~ }

	//~ Serial.println("");
	//~ Serial.println("WiFi connected");  
	//~ Serial.println("IP address: ");
	//~ Serial.println(WiFi.localIP());
	
	mySerial.begin(19200);
	Serial.begin(19200);
	//******** Initialize sim808 module *********
	while(!sim808.init()) {
			delay(2000);
			Serial.print("Sim808 init error\r\n");
	}
	delay(4000);
	//*********** Attempt DHCP ***************
	while(!sim808.join(F("movistar.pe"))) {
		Serial.println("Sim808 join network error");
		delay(3000);      
	}
	//************ Successful DHCP ************
	Serial.println("IP Address is ");
	Serial.println(sim808.getIPAddress());
	//*********** Establish a TCP connection ********

	delay(1000);
	
	

	// Connect to the websocket server
	//~ if (client.connect("echo.websocket.org", 80)) {
		//~ Serial.println("Connected");
	//~ } else {
		//~ Serial.println("Connection failed.");
		//~ while(1) {
			//~ // Hang on failure
		//~ }
	//~ }
	while(!sim808.connect(TCP,"efectibit.com", 314)) {
		Serial.println("Connect error");
	}
	Serial.println("Connect efectibit success");

	// Handshake with the server
	webSocketClient.path = path;
	webSocketClient.host = host;
	if (webSocketClient.handshake(client)) {
		Serial.println("Handshake successful");
	} else {
		Serial.println("Handshake failed.");
		while(1) {
			// Hang on failure
		}
	}

}


//~ void loop() {
	//~ String data;

	//~ if (client.is_connected()) {
		
		//~ webSocketClient.getData(data);
		//~ if (data.length() > 0) {
			//~ Serial.print("Received data: ");
			//~ Serial.println(data);
		//~ }
		
		//~ // capture the value of analog 1, send it along
		//~ pinMode(1, INPUT);
		//~ data = String(analogRead(1));
		
		//~ webSocketClient.sendData(data);
		
	//~ } else {
		//~ Serial.println("Client disconnected.");
		//~ while (1) {
			//~ // Hang on disconnect.
		//~ }
	//~ }
	
	//~ // wait to fully let the client disconnect
	//~ delay(3000);
	
//~ }
