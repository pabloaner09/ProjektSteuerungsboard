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
  last_digital_1 = digitalRead(2);
  last_digital_2 = digitalRead(3);
  last_digital_3 = digitalRead(4);
  last_digital_4 = digitalRead(5);
  last_digital_5 = digitalRead(6);
  last_digital_6 = digitalRead(7);
  
  has_change = false;
  send_buffer[0] = '\0';
}

void updateSensorStates() {
  bool change = false;
  char buffer[32] = {0};
  int pos = 0;

  // Digital Pin 1
  uint8_t d1 = digitalRead(2);
  if (d1 != last_digital_1) {
    last_digital_1 = d1;
    pos += snprintf(buffer + pos, sizeof(buffer) - pos, "D,1,%d;", d1);
    change = true;
  }

  // Digital Pin 2
  uint8_t d2 = digitalRead(3);
  if (d2 != last_digital_2) {
    last_digital_2 = d2;
    pos += snprintf(buffer + pos, sizeof(buffer) - pos, "D,2,%d;", d2);
    change = true;
  }

  // Digital Pin 3
  uint8_t d3 = digitalRead(4);
  if (d3 != last_digital_3) {
    last_digital_3 = d3;
    pos += snprintf(buffer + pos, sizeof(buffer) - pos, "D,3,%d;", d3);
    change = true;
  }

  // Digital Pin 4
  uint8_t d4 = digitalRead(5);
  if (d4 != last_digital_4) {
    last_digital_4 = d4;
    pos += snprintf(buffer + pos, sizeof(buffer) - pos, "D,4,%d;", d4);
    change = true;
  }

  // Digital Pin 5
  uint8_t d5 = digitalRead(6);
  if (d5 != last_digital_5) {
    last_digital_5 = d5;
    pos += snprintf(buffer + pos, sizeof(buffer) - pos, "D,5,%d;", d5);
    change = true;
  }

  // Digital Pin 6
  uint8_t d6 = digitalRead(7);
  if (d5 != last_digital_6) {
    last_digital_6 = d6;
    pos += snprintf(buffer + pos, sizeof(buffer) - pos, "D,6,%d;", d6);
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