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
  last_digital_4 = digitalRead(4);
  last_digital_5 = digitalRead(5);
  int analog = analogRead(2) / 4;
  last_analog_2 = analog;
  has_change = false;
  send_buffer[0] = '\0';
}

void updateSensorStates() {
  bool change = false;
  char buffer[32] = {0};
  int pos = 0;

  // Analog Pin 2 (8 Bit, mit Toleranz)
  uint8_t a2 = analogRead(2) / 4;
  if (abs(a2 - last_analog_2) > 2) {  // Hysterese von 2/255
    last_analog_2 = a2;
    pos += snprintf(buffer + pos, sizeof(buffer) - pos, "A,2,%d;", a2);
    change = true;
  }

  // Digital Pin 4
  uint8_t d4 = digitalRead(4);
  if (d4 != last_digital_4) {
    last_digital_4 = d4;
    pos += snprintf(buffer + pos, sizeof(buffer) - pos, "D,4,%d;", d4);
    change = true;
  }

  // Digital Pin 5
  uint8_t d5 = digitalRead(5);
  if (d5 != last_digital_5) {
    last_digital_5 = d5;
    pos += snprintf(buffer + pos, sizeof(buffer) - pos, "D,5,%d;", d5);
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

//Für die LED
void startBlink() {
  digitalWrite(LED_PIN, HIGH);
  led_on_until = millis() + 100;
}

void updateLED() {
  if (led_on_until != 0 && millis() >= led_on_until) {
    digitalWrite(LED_PIN, LOW);
    led_on_until = 0;
  }
}



#endif // FUNCTIONS_H