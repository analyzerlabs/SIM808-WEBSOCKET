#ifndef DEBUGGING
#define DEBUGGING
#endif

//MS:

#include "global.h"
#include "WebSocketClient.h"

#include "sha1.h"
#include "Base64.h"

WebSocketClient::WebSocketClient(char * path, char * host) : path(path), host(host) {
}

bool WebSocketClient::handshake(DFRobot_SIM808 &client) {

	socket_client = &client;

	// If there is a connected client->
	if (socket_client->is_connected()) {
		// Check request and look for websocket handshake
#ifdef DEBUGGING
			Serial.println(F("Client connected"));
#endif
		if (analyzeRequest()) {
#ifdef DEBUGGING
				Serial.println(F("Websocket established"));
#endif

				return true;

		} else {
			// Might just need to break until out of socket_client loop.
#ifdef DEBUGGING
			Serial.println(F("Invalid handshake"));
#endif
			disconnectStream();

			return false;
		}
	} else {
		Serial.println(F("No conectao para enviar cabecera. Reconectará..."));
		socket_client->connect(TCP,"efectibit.com", 314);
		return false;
	}
}

bool WebSocketClient::analyzeRequest() {
	bool foundupgrade = false;
	unsigned long intkey[2];
	String serverKey;
	char keyStart[17];
	char b64Key[25];
	String key = "------------------------";

	randomSeed(analogRead(0));

	for (int i=0; i<16; ++i) {
		keyStart[i] = (char)random(1, 256);
	}

	base64_encode(b64Key, keyStart, 16);

	for (int i=0; i<24; ++i) {
		key[i] = b64Key[i];
	}

#ifdef DEBUGGING
	Serial.println(F("Sending websocket upgrade headers"));
#endif    

	String cabecera = "GET ";
	cabecera += path;
	cabecera += " HTTP/1.1\r\n";
	cabecera += "Host: ";
	cabecera += host;
	cabecera += CRLF; 
	cabecera += "Upgrade: websocket\r\n";
	cabecera += "Connection: Upgrade\r\n";
	
	cabecera += "Sec-WebSocket-Key: ";
	cabecera += key;
	cabecera += CRLF;
	//~ socket_client->print(F("Sec-WebSocket-Protocol: "));
	//~ socket_client->print(protocol);
	//~ socket_client->print(CRLF);
	cabecera += "Sec-WebSocket-Version: 13\r\n";
	cabecera += "Origin: http://efectibit.com\r\n";
	cabecera += CRLF;

	//Serial.print(cabecera);

	//while (!socket_client->is_connected() /*&& !socket_client->readable()*/) {
	//    delay(50);
	//    Serial.println("Waiting...");
	//}
	//Enviar datos de cabecera
	socket_client->send(cabecera.c_str(), cabecera.length());

#ifdef DEBUGGING
	Serial.println(F("Cabecera enviada y Analyzing response headers"));
#endif 
	
	// TODO: More robust string extraction
	//bite tiene el tamaño del nuevo buffer
	char buffer[512];
	int ret = socket_client->recv(buffer, sizeof(buffer)-1);
	if(ret <= 0){
		Serial.println("SIN nada");
	}
	else{
		Serial.println("HAY algo");
		String temp(buffer);
#ifdef DEBUGGING
			Serial.print("Got Header: " + temp);
#endif
			if (!foundupgrade && temp.startsWith("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade")) {
				foundupgrade = true;
				serverKey = temp.substring(97, 97 + 28); // Don't save last CR+LF

				Serial.print("Server key: ");
				Serial.println( serverKey );
			}
			temp = "";		
		//}

		//if(!socket_client->wait_redeable(10)) {
		  //delay(10);
		//}
	}

	key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	uint8_t *hash;
	char result[21];
	char b64Result[30];

	SHA1Context sha;
	int err;
	uint8_t Message_Digest[20];
	
	err = SHA1Reset(&sha);
	err = SHA1Input(&sha, reinterpret_cast<const uint8_t *>(key.c_str()), key.length());
	err = SHA1Result(&sha, Message_Digest);
	hash = Message_Digest;

	for (int i=0; i<20; ++i) {
		result[i] = (char)hash[i];
	}
	result[20] = '\0';

	base64_encode(b64Result, result, 20);

	// if the keys match, good to go
	return serverKey.equals(String(b64Result));
}


bool WebSocketClient::handleStream(String& data, uint8_t *opcode) {
	uint8_t msgtype;
	uint8_t bite;
	unsigned int length;
	uint8_t mask[4];
	uint8_t index;
	unsigned int i;
	bool hasMask = false;

	if (!socket_client->is_connected() /*|| !socket_client->readable()*/ )
	{
		return false;
	}      

	msgtype = timedRead();
	if (!socket_client->is_connected()) {
		return false;
	}

	length = timedRead();

	if (length & WS_MASK) {
		hasMask = true;
		length = length & ~WS_MASK;
	}


	if (!socket_client->is_connected()) {
		return false;
	}

	index = 6;

	if (length == WS_SIZE16) {
		length = timedRead() << 8;
		if (!socket_client->is_connected()) {
			return false;
		}
			
		length |= timedRead();
		if (!socket_client->is_connected()) {
			return false;
		}   

	} else if (length == WS_SIZE64) {
#ifdef DEBUGGING
		Serial.println(F("No support for over 16 bit sized messages"));
#endif
		return false;
	}

	if (hasMask) {
		// get the mask
		mask[0] = timedRead();
		if (!socket_client->is_connected()) {
			return false;
		}

		mask[1] = timedRead();
		if (!socket_client->is_connected()) {

			return false;
		}

		mask[2] = timedRead();
		if (!socket_client->is_connected()) {
			return false;
		}

		mask[3] = timedRead();
		if (!socket_client->is_connected()) {
			return false;
		}
	}
		
	data = "";
		
	if (opcode != NULL)
	{
	  *opcode = msgtype & ~WS_FIN;
	}
				
	if (hasMask) {
		for (i=0; i<length; ++i) {
			data += (char) (timedRead() ^ mask[i % 4]);
			if (!socket_client->is_connected()) {
				return false;
			}
		}
	}
  else {
		for (i=0; i<length; ++i) {
			data += (char) timedRead();
			if (!socket_client->is_connected()) {
				return false;
			}
		}            
	}
	
	return true;
}

void WebSocketClient::disconnectStream() {
#ifdef DEBUGGING
	Serial.println(F("Terminating socket"));
#endif
	// Should send 0x8700 to server to tell it I'm quitting here.
	char mensaje[2] = { (uint8_t) 0x87, (uint8_t) 0x00 };
	socket_client->send( mensaje, 2 );
	
	//socket_client->flush();
	delay(10);
	//~ socket_client->stop();
	socket_client->close();
}

bool WebSocketClient::getData(String& data, uint8_t *opcode) {
	
	return handleStream(data, opcode);
}    

bool WebSocketClient::sendData(const char *str, uint8_t opcode) {
#ifdef DEBUGGING
	Serial.print(F("Sending data: "));
	Serial.println(str);
#endif
	if (socket_client->is_connected()) {
		sendEncodedData(str, opcode);
		return true;
	}
	return false;
}

bool WebSocketClient::sendData(String str, uint8_t opcode) {
#ifdef DEBUGGING
	Serial.print(F("Sending data: "));
	Serial.println(str);
#endif
	if (socket_client->is_connected()) {
		sendEncodedData(str, opcode);
		return true;
	}
	return false;
}

int WebSocketClient::timedRead() {
  while (!socket_client->readable()) {
	delay(20);  
  }

  char buffer[512];
  return socket_client->recv(buffer, sizeof(buffer)-1);
}

void WebSocketClient::sendEncodedData(char *str, uint8_t opcode) {
	uint8_t mask[4];
	int size = strlen(str);

	// Opcode; final fragment
	//socket_client->write(opcode | WS_FIN);
	String respuesta( (char) (opcode | WS_FIN) );

	// NOTE: no support for > 16-bit sized messages
	if (size > 125) {
		//socket_client->write(WS_SIZE16 | WS_MASK);
		//socket_client->write((uint8_t) (size >> 8));
		//socket_client->write((uint8_t) (size & 0xFF));
		respuesta += (char) WS_SIZE16 | WS_MASK;
		respuesta += (char) (size >> 8);
		respuesta += (char) (size & 0xFF);
	} else {
		//socket_client->write((uint8_t) size | WS_MASK);
		respuesta += (char) (size | WS_MASK);
	}

	mask[0] = random(0, 256);
	mask[1] = random(0, 256);
	mask[2] = random(0, 256);
	mask[3] = random(0, 256);
	
	//socket_client->write(mask[0]);
	//socket_client->write(mask[1]);
	//socket_client->write(mask[2]);
	//socket_client->write(mask[3]);
	respuesta += (char) mask[0];
	respuesta += (char) mask[1];
	respuesta += (char) mask[2];
	respuesta += (char) mask[3];
	 
	for (int i=0; i<size; ++i) {
		respuesta += (char) (str[i] ^ mask[i % 4]);
	}

	socket_client->send(respuesta.c_str(), respuesta.length());
	Serial.print(F("Datos enviados.\n"));
}

void WebSocketClient::sendEncodedData(String str, uint8_t opcode) {
	int size = str.length() + 1;
	char cstr[size];

	str.toCharArray(cstr, size);

	sendEncodedData(cstr, opcode);
}
