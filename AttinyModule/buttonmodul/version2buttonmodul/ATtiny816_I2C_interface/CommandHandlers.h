/**
 * @file functions.cpp
 * @brief Contains command handler implementations for the CommandProcessor.
 *
 * These handlers are registered with the CommandProcessor and are executed
 * when a matching command is received over I2C.
 *
 * @details
 * After calling a handler, the master processor is expected to read the response
 * data from the I2C slave via an I2C request. The content and length of the response
 * depend on the specific handler. Read each handler's documentation to understand
 * what kind of data (if any) will be prepared in the send buffer for the master
 * to retrieve.
 *
 * Data is always sent in ASCII format in both directions.
 */

#include "functions.h"

extern CommandProcessor cmd;

/**
 * @brief Set the I2C address of the device.
 *
 * Parses a numeric I2C address from the input string and reinitializes
 * the I2C peripheral with the new address.
 *
 * @param args_str Argument string containing the new address (e.g. "0x1A" or "26").
 */
void cmd_set_I2CAddress(const char* args_str) {
  char* endptr = nullptr;
  unsigned long val = strtoul(args_str, &endptr, 0);  // Auto-detect base

  if (endptr == args_str) {
    // Invalid format
    return;
  }

  if (val > 0x7F) {
    // Address out of 7-bit range
    return;
  }

  Wire.end();
  Wire.begin((uint8_t)val);
}





/**
 * @brief Set the opposite enable pin to LOW or HIGH based on position and argument.
 * 
 * @param args_str Argument string: "0" (LOW) or "1" (HIGH)
 */
void cmd_set_other_enable(const char* args_str) {
  uint8_t value = (uint8_t)strtoul(args_str, nullptr, 10);
  
  if (value != 0 && value != 1) {
    cmd.copyToSendBuffer("ERR", 4);
    return;
  }

  // Ziel-Pin bestimmen
  int targetPin = -1;
  if (strcmp(position, "Le") == 0) {
    targetPin = PIN_ENABLE_L;
  } else if (strcmp(position, "Re") == 0) {
    targetPin = PIN_ENABLE_R;
  } else {
    cmd.copyToSendBuffer("ERR", 4);
    return;
  }

  pinMode(targetPin, OUTPUT);
  digitalWrite(targetPin, value ? HIGH : LOW);

  cmd.copyToSendBuffer("OK", 3);
}





void cmd_get_board_type(const char* args_str) {
  // Konvertiere die Modul-Typnummer in einen ASCII-String
  char typeStr[4];  // Max. "255\0"
  snprintf(typeStr, sizeof(typeStr), "%d", MODULE_TYPE_ID);
  cmd.copyToSendBuffer(typeStr, strlen(typeStr) + 1);  // +1 für Nullterminierung
}


//////////////////////////////
// Das hier ist auch verschieden je nach konfiguration
/////////////////////////////

void cmd_get_info(const char* args_str) {
  char buffer[32] = {0};
  snprintf(buffer, sizeof(buffer),
           "P,%s;D,1;D,2;D,3;D,4;D,5;D,6;",
           position);

  cmd.copyToSendBuffer(buffer, strlen(buffer) + 1);
}

// Handler für getNew
void cmd_get_new(const char* args_str) {
  noInterrupts();
  if (has_change) {
    cmd.copyToSendBuffer(send_buffer, strlen(send_buffer) + 1);
    has_change = false;  // Nach Abruf löschen
    //startBlink();
  } else {
    char empty[1] = { '\0' };
    cmd.copyToSendBuffer(empty, 1);
  }
  interrupts();
}
