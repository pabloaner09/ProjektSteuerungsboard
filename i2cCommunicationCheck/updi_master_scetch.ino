#include <Wire.h>

#define SERIAL_BUFFER_SIZE 64
#define I2C_READ_LENGTH 32  // Anzahl der maximal zu lesenden Bytes

char serialBuffer[SERIAL_BUFFER_SIZE];
uint8_t serialIndex = 0;



void setup() {
  
  Serial.begin(115200);
  Wire.begin();  // Master starten
  Serial.println("Ready. Enter commands like: 0x2A hello or 0x2A request");
}

void loop() {

 




  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      if (serialIndex > 0) {
        serialBuffer[serialIndex] = 0;  // Null-terminieren
        processCommand(serialBuffer);
        serialIndex = 0;
      }
    } else if (serialIndex < SERIAL_BUFFER_SIZE - 1) {
      serialBuffer[serialIndex++] = c;
    }
  }
}

void processCommand(const char* cmd) {
  char addrStr[8];
  const char* command = nullptr;

  // Adresse extrahieren
  int i = 0;
  while (cmd[i] != ' ' && cmd[i] != 0 && i < sizeof(addrStr) - 1) {
    addrStr[i] = cmd[i];
    i++;
  }
  addrStr[i] = 0;

  // Zeiger auf den Rest (Befehl oder Nachricht)
  command = (cmd[i] == ' ') ? &cmd[i + 1] : "";

  // Adresse konvertieren (hex oder dez)
  uint8_t address = (uint8_t)strtoul(addrStr, NULL, 0);

  // Befehl senden
  Wire.beginTransmission(address);
  Wire.write((const uint8_t*)command, strlen(command));
  uint8_t error = Wire.endTransmission();

  if (error != 0) {
    Serial.print("I2C Error: ");
    Serial.println(error);
    return;
  }

  Serial.print("Sent to 0x");
  Serial.print(address, HEX);
  Serial.print(": ");
  Serial.println(command);

  // Slave braucht Zeit zum Verarbeiten → kurz warten
  delay(5);  // optional feinjustieren (2–10 ms je nach Slave-Verhalten)

  // Antwort anfordern
  Wire.requestFrom((int)address, I2C_READ_LENGTH);
  String data = "";

  while (Wire.available()) {
    char c = Wire.read();
    if (c == '\0') break;
    data += c;
  }

  Serial.print("Response from 0x");
  Serial.print(address, HEX);
  Serial.print(": ");
  Serial.println(data);
}