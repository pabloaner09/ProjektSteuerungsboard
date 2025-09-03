#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <Arduino.h>



/**
 * @brief Initializes position detection based on enable pins.
 * Should be called once in setup after checking enableActive().
 */
void initPosition() {
  pinMode(PIN_ENABLE_L, INPUT_PULLUP);
  pinMode(PIN_ENABLE_R, INPUT_PULLUP);

  if (digitalRead(PIN_ENABLE_L) == LOW) {
    position = "Re";  // Rechts, weil links Low heißt der Attiny sitzt Rechts
  } else if (digitalRead(PIN_ENABLE_R) == LOW) {
    position = "Li";  // Links, weil rechts Low heißt der Attiny sitzt Links
  } else {
    position = "Un";  // Falls keiner LOW ist (theoretisch sollte das nicht vorkommen)
  }
}

void waitForEnable() {
  pinMode(PIN_ENABLE_L, INPUT_PULLUP);
  pinMode(PIN_ENABLE_R, INPUT_PULLUP);

  while (digitalRead(PIN_ENABLE_L) == HIGH && digitalRead(PIN_ENABLE_R) == HIGH) {
    // Optional: kleines Power Saving, z.B. sleep oder delay
    delay(50);  // Entprellen, geringe CPU-Last
  }
}

//////////////////////////////////
// Hier muss auch alles immer neu angepasst werden 
/////////////////////////////////

void initSensorStates() {
  last_digital_1 = digitalRead(Button_1);
  last_digital_2 = digitalRead(X_1);
  last_digital_3 = digitalRead(Y_1);
  last_digital_4 = digitalRead(Button_2);
  last_digital_5 = digitalRead(X_2);
  last_digital_6 = digitalRead(Y_2);
  last_digital_7 = digitalRead(Button_3);
  last_digital_8 = digitalRead(X_3);
  last_digital_9 = digitalRead(Y_3);
  
  has_change = false;
  send_buffer[0] = '\0';
}

void updateSensorStates() {
  bool change = false;
  char buffer[32] = {0};
  int pos = 0;

  // Button 1
  uint8_t d1 = digitalRead(Button_1);
  if (d1 != last_digital_1) {
    last_digital_1 = d1;
    pos += snprintf(buffer + pos, sizeof(buffer) - pos, "D,B1,%d;", d1);
    change = true;
  }

  // X1
  uint8_t d2 = digitalRead(X_1);
  if (d2 != last_digital_2) {
    last_digital_2 = d2;
    pos += snprintf(buffer + pos, sizeof(buffer) - pos, "D,X1,%d;", d2);
    change = true;
  }

  // Y1
  uint8_t d3 = digitalRead(Y_1);
  if (d3 != last_digital_3) {
    last_digital_3 = d3;
    pos += snprintf(buffer + pos, sizeof(buffer) - pos, "D,Y1,%d;", d3);
    change = true;
  }

  // Button 2
  uint8_t d4 = digitalRead(Button_2);
  if (d4 != last_digital_4) {
    last_digital_4 = d4;
    pos += snprintf(buffer + pos, sizeof(buffer) - pos, "D,B2,%d;", d4);
    change = true;
  }

  // X2
  uint8_t d5 = digitalRead(X_2);
  if (d5 != last_digital_5) {
    last_digital_5 = d5;
    pos += snprintf(buffer + pos, sizeof(buffer) - pos, "D,X2,%d;", d5);
    change = true;
  }

  // Y2
  uint8_t d6 = digitalRead(Y_2);
  if (d6 != last_digital_6) {
    last_digital_6 = d6;
    pos += snprintf(buffer + pos, sizeof(buffer) - pos, "D,Y2,%d;", d6);
    change = true;
  }

  // Button 3
  uint8_t d7 = digitalRead(Button_3);
  if (d7 != last_digital_7) {
    last_digital_7 = d7;
    pos += snprintf(buffer + pos, sizeof(buffer) - pos, "D,B3,%d;", d7);
    change = true;
  }

  // X3
  uint8_t d8 = digitalRead(X_3);
  if (d8 != last_digital_8) {
    last_digital_8 = d8;
    pos += snprintf(buffer + pos, sizeof(buffer) - pos, "D,X3,%d;", d8);
    change = true;
  }

  // Y3
  uint8_t d9 = digitalRead(Y_3);
  if (d9 != last_digital_9) {
    last_digital_9 = d9;
    pos += snprintf(buffer + pos, sizeof(buffer) - pos, "D,Y3,%d;", d9);
    change = true;
  }

  

  if (change) {
    noInterrupts();
    strncpy(send_buffer, buffer, sizeof(send_buffer));
    send_buffer[sizeof(send_buffer) - 1] = '\0';
    has_change = true;
    interrupts();
  }
}

// //Für die LED
// void startBlink() {
//   digitalWrite(LED_PIN, HIGH);
//   led_on_until = millis() + 100;
// }

// void updateLED() {
//   if (led_on_until != 0 && millis() >= led_on_until) {
//     digitalWrite(LED_PIN, LOW);
//     led_on_until = 0;
//   }
// }



#endif // FUNCTIONS_H