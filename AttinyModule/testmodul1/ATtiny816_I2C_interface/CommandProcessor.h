#ifndef COMMANDPROCESSOR_H
#define COMMANDPROCESSOR_H

#include <Arduino.h>
#include <Wire.h>
#include <string.h>  // strtok, strcmp

#define MAX_COMMANDS 10
#define BUFFER_SIZE 32  ///< Size of the receive/send buffer

/// @brief Typedef for a command handler function pointer.
/// @param args_str Arguments as a null-terminated C string.
using CommandHandler = void (*)(const char* args_str);

/**
 * @brief Structure representing a single command.
 */
typedef struct {
  const char* name;         ///< The command name, e.g., "setI2CAddr"
  CommandHandler handler;   ///< Associated handler function
} Command;

/**
 * @brief Class for registering and executing custom commands.
 */
class CommandProcessor {
private:
  Command m_cmds[MAX_COMMANDS];     ///< Registered commands
  uint8_t m_cmd_cnt = 0;            ///< Number of active commands

  volatile char m_buffer[BUFFER_SIZE];  ///< Data buffer (e.g., RX)
  volatile uint8_t m_len = 0;           ///< Length of current data
  volatile bool m_ready = false;        ///< Flag indicating new data is available

  static CommandProcessor* m_instance;  ///< Singleton pointer for ISR access

public:
  /**
   * @brief Constructor – initializes internal buffer state.
   */
  CommandProcessor();

  /**
   * @brief Initializes internal state, clears buffers.
   */
  void init();

  /**
   * @brief Registers a new command.
   * 
   * @param name The name of the command.
   * @param handler Function pointer to the handler.
   * @return true on success, false if MAX_COMMANDS is exceeded.
   */
  bool addCommand(const char* name, CommandHandler handler);

  /**
   * @brief Copies data into the internal transmit buffer.
   * 
   * Data will be truncated if it exceeds the buffer size. Interrupts are temporarily disabled.
   * 
   * @param src Pointer to source data
   * @param len Number of bytes to copy
   */
  void copyToSendBuffer(char* src, uint8_t len);

  /**
   * @brief Parses the internal buffer and executes the matched command.
   */
  void processCommand();

  /**
   * @brief Reads and clears the "data ready" flag.
   * 
   * @return true if new data was available, false otherwise.
   */
  bool readDataReady();

  /**
   * @brief Clears the "data ready" flag without reading.
   */
  void clearDataReady();

  /**
   * @brief I2C receive callback (ISR).
   * 
   * Called by Wire.onReceive when data is received over I2C.
   * 
   * @param numBytes Number of received bytes
   */
  static void I2C_dataIn(int numBytes);

  /**
   * @brief I2C transmit callback (ISR).
   * 
   * Called by Wire.onRequest when data is requested via I2C.
   */
  static void I2C_dataOut();

private:
  /**
   * @brief Clears the internal receive buffer.
   */
  void clearBuffer();

  /**
   * @brief Appends a character to the internal buffer.
   * 
   * @param c Character to append
   */
  void appendToBuffer(char c);
};

#endif  // COMMANDPROCESSOR_H


