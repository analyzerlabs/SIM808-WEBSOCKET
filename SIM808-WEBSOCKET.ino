#include "WebSocketClient.h"

#include <DFRobot_sim808.h>
#include <SoftwareSerial.h>

/**
 * Este proyecto maneja la conexión física al servidor y luego lo saluda
 * con el protocolo de WebSocket.
 * Si se duerme, ninguna instrucción por Tx es aceptada por SIM808, pero
 * es despertada con cualquiera de esas instrucciones.
 * Por lo tanto esa instrucción es perdida. Para evitar esa "falla", le
 * mandamos al pin DTR pull LOW por más de 50 ms.
 * http://www.python-exemplary.com/download/sim800_series_tcpip_application_note_v1.00.pdf
 */

char path[] = "ws://efectibit.com:314/";
char host[] = "efectibit.com:314";

WebSocketClient webBrowser(path, host);
//~ webBrowser.path = path;
//~ webBrowser.host = host;

#define PIN_TX 18
#define PIN_RX 19
#define PIN_DTR 13
#define DEBUGGING

SoftwareSerial puertoSerieLocal(PIN_TX, PIN_RX);
DFRobot_SIM808 sim808( &puertoSerieLocal );

/**
 * Hacer una conexión TCP al socket del servidor en la nube.
 */
void conectarServidorReal() {
	//Conexión real a la tarjeta de red del servidor
	while (!sim808.connect(TCP, "efectibit.com", 314)) {
		#ifdef DEBUGGING
			Serial.println("E:TCP no connected");
		#endif
	}
	Serial.println("Connect efectibit: OK");
}

/**
 * Hacer un saludo protocolar al WebSocket.
 */
void conectarServidorVirtual() {
	// Handshake with the server
	while( !webBrowser.handshake(sim808) ) {
		Serial.println("Reshaking");
	}
	Serial.println("Handshake OK.");
}

void setup() {
	Serial.begin(19200);
	delay(100);
	
	#ifdef DEBUGGING
		Serial.print("I:Sim808 debugging\r\n");
	#endif

	puertoSerieLocal.begin(19200);
	//******** Initialize sim808 module *********
	while (!sim808.init()) {
		delay(2000);
		#ifdef DEBUGGING
			Serial.print("E:Sim808 init\r\n");
		#endif
	}
	delay(3500);
	//*********** Attempt DHCP ***************
	while (!sim808.join(F("movistar.pe"))) {
		#ifdef DEBUGGING
			Serial.println("E:Sim808 join network");
		#endif
		delay(5000);
	}
	//************ Successful DHCP ************
	Serial.println("I:IP Address: ");
	Serial.println(sim808.getIPAddress());
	//*********** Establish a TCP connection ********

	delay(1000);

	conectarServidorReal();

	delay(1000);
	
	//~ conectarServidorVirtual();
	//esp_task_wdt_reset();
	Serial.println("I:Todo listo. Virtualiza.");
}

//String glonass_inf; // = "";
void loop() {
	while (Serial.available() > 0) {
		switch(Serial.read()) {
			case 'c':
				Serial.println("Reconectará ws...");
				conectarServidorVirtual();
				break;
			case 'e':
				//glonass_inf="";
				if(webBrowser.sendData("Rehola.") ) {
					Serial.println("I:rehola sent");
				}
				while(true) {
					static bool gpsRunStatus;
					static char glonassInfo[200];
					if (sim808.attachGPS()) {
						Serial.println("GPS listo");
						if(sim808.getInf(glonassInfo, &gpsRunStatus)) {
							Serial.println("Llegó glonnasInfo a SIM808-WebSockete");
							Serial.print("GPS RUN STATUS : ");
							Serial.println(gpsRunStatus);
							if(!gpsRunStatus){
								delay(1000);
								continue;
							}
							if( webBrowser.sendData(glonassInfo) ) {
								Serial.println("I:glonass info sent");
							}
							else {
								Serial.println("We:Virtual desconectao");
							}
							break;
						}
						else{
							Serial.println("W: No hay Info GPS");
						}
						Serial.print("Glonass Info: ");
						Serial.println(glonassInfo);
						while( !sim808.detachGPS() ) {
							delay(100);
						}
					}
				}
			#ifdef DEBUGGING
				//~ sim808.sendSMS("993118714", "MAYDAY please");
				Serial.println("I:\"e\" executed");
			#endif
				break;
			case 'E':
			#ifdef DEBUGGING
				if( webBrowser.sendData("Letra E...") ) {
					Serial.println("I:Hola sent");
				}
				else {
					Serial.println("WE:Virtual desconectao");
				}
				Serial.println("I:'E' executed");
			#endif
		}
	}

	/*String data;
	webBrowser.getData(data);
	  if (data.length() > 0) {
		Serial.print("Received data: ");
		Serial.println(data);
	  }
	  else {
		Serial.println("noDato");
	  }*/

	/*String data;*/

	/*if (sim808.is_connected()) {	
		webBrowser.getData(data);
		if (data.length() > 0) {
			Serial.print("Received data: ");
			Serial.println(data);
		}
	else {
	  Serial.println("noDato");
	}
		
		//~ // capture the value of analog 1, send it along
		//~ pinMode(1, INPUT);
		//~ data = String(analogRead(1));
		
		//~ webBrowser.sendData(data);
		
	} else {
		Serial.println("sim808 disconnected.");
		//~ while (1) {
			//~ // Hang on disconnect.
		//~ }
	}
	
	//~ // wait to fully let the client disconnect
	delay(3000);
	*/
}
