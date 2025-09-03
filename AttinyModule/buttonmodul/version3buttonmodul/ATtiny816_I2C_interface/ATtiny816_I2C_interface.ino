/**
 * @file main.cpp
 * @brief Entry point for the I2C slave device using CommandProcessor.
 *
 * Initializes I2C, registers command handlers, and waits for commands
 * from the I2C master. After receiving a command, the processor will
 * handle it and prepare an optional response in the send buffer.
 */

#define MODULE_TYPE_ID 02  // kann an sich wegkommen aber passt schon
#define PIN_ENABLE_L 11
#define PIN_ENABLE_R 10

// Pins , am besten nenne ich sie hier mit dem Namen, dann kann ich immer darauf zugreifen 
// und kann dann abschauen
//#define ANALOG_PIN 2

#define DIGITAL_PIN_1 0
#define DIGITAL_PIN_2 1
#define DIGITAL_PIN_3 2
#define DIGITAL_PIN_4 3
#define DIGITAL_PIN_5 4
#define DIGITAL_PIN_6 5

#define PIN_SDA 8
#define PIN_SCL 9

//#define LED_PIN 16  // Deine LED an Pin 1


/// @brief Default I2C address of this device.
#define I2C_INIT_ADDRESS 0x25

// Position State
static const char* position = "Un";  // Default unknown


// Interner Zustand
uint8_t last_digital_1 = 0;
uint8_t last_digital_2 = 0;
uint8_t last_digital_3 = 0;
uint8_t last_digital_4 = 0;
uint8_t last_digital_5 = 0;
uint8_t last_digital_6 = 0;


extern volatile bool has_change;
extern char send_buffer[32];


//für die LED dauer
unsigned long led_on_until = 0;

//volatile bool has_change = false; //hier weiß ich nicht was von beiden 


#include <Wire.h>
#include "CommandProcessor.h"
#include "CommandHandlers.h"


/// @brief Global instance of the CommandProcessor.
CommandProcessor cmd; 







char send_buffer[32]; // kann bei Bedarf vergrößert werden


/**
 * @brief Arduino setup function.
 *

 * Initializes the CommandProcessor, registers commands, and sets up I2C
 * with receive and request callbacks.
 */
void setup() {

  waitForEnable();  // bleibt hier hängen bis enable aktiv wird
  initPosition();
  cmd.init();

  // Register command handlers
  cmd.addCommand("setI2CAddr",   cmd_set_I2CAddress);
  //cmd.addCommand("getSerialnum", cmd_read_serialnum);
  //cmd.addCommand("readDigital",  cmd_read_digital_GPIO);
  //cmd.addCommand("readAnalog",   cmd_read_analog_GPIO);
  // from now on from me
  //cmd.addCommand("writeDigital", cmd_write_digital_GPIO);
  cmd.addCommand("getBoardType", cmd_get_board_type);
  cmd.addCommand("getInfo", cmd_get_info);
  cmd.addCommand("getNew", cmd_get_new);
  cmd.addCommand("setEnable", cmd_set_other_enable);

  // Initialize I2C interface
  Wire.begin(I2C_INIT_ADDRESS);
  Wire.onReceive(CommandProcessor::I2C_dataIn);
  Wire.onRequest(CommandProcessor::I2C_dataOut);

  ////////////////
  //hier kommen die Pins rein die besonders sind für das Modul
  ////////////////
  pinMode(2, INPUT);
  pinMode(3, INPUT);
  pinMode(4, INPUT);
  pinMode(5, INPUT);
  pinMode(6, INPUT);
  pinMode(7, INPUT);

  //pinMode(LED_PIN, OUTPUT);
  //digitalWrite(LED_PIN, LOW);

  initSensorStates();
}

/**
 * @brief Arduino main loop.
 *
 * Waits for new data via I2C. If data has been received, processes the command
 * and clears the ready flag. Interrupts are temporarily disabled during processing
 * to avoid inconsistent access to shared buffers.
 */
void loop() {
  if (cmd.readDataReady()) {

    updateSensorStates();
    delay(5);  // Poll alle 5ms

    noInterrupts();  // Ensure safe access to shared resources
    cmd.processCommand();
    cmd.clearDataReady();
    interrupts();
  }
  //updateLED();
}
