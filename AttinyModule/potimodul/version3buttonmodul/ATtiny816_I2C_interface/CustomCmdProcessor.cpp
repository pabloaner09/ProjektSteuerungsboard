#include "CommandProcessor.h"

CommandProcessor* CommandProcessor::m_instance = nullptr;

//hier freestyle ich einfach, chat meinte mach so
volatile bool has_change = false;

CommandProcessor::CommandProcessor() {
  m_instance = this;
}

void CommandProcessor::init() {
  clearBuffer();
}

bool CommandProcessor::addCommand(const char* name, CommandHandler handler) {
  if (m_cmd_cnt >= MAX_COMMANDS) return false;
  m_cmds[m_cmd_cnt++] = { name, handler };
  return true;
}

void CommandProcessor::appendToBuffer(char c) {
  if (m_len < BUFFER_SIZE) {
    m_buffer[m_len++] = c;
  }
}

void CommandProcessor::clearBuffer() {
  memset((void*)m_buffer, 0, BUFFER_SIZE);
  m_len = 0;
}

void CommandProcessor::copyToSendBuffer(const char* src, uint8_t len) {
  if (len > BUFFER_SIZE) {
    len = BUFFER_SIZE;
  }

  for (uint8_t i = 0; i < len; i++) {
    m_buffer[i] = src[i];
  }
  m_len = len;
}

void CommandProcessor::processCommand() {
  m_buffer[BUFFER_SIZE - 1] = '\0';

  char temp_buffer[BUFFER_SIZE];
  memcpy(temp_buffer, (const void*)m_buffer, BUFFER_SIZE);

  // Tokens extrahieren
  char* cmd_str = strtok(temp_buffer, " \t\r\n");
  char* args_str = strtok(nullptr, "\r\n");

  if (!cmd_str) {
    return;
  }

  for (size_t i = 0; i < m_cmd_cnt; ++i) {
    if (strcmp(cmd_str, m_cmds[i].name) == 0) {
      m_cmds[i].handler(args_str);
      return;
    }
  }
  clearBuffer();
}

bool CommandProcessor::readDataReady() {
  return m_ready;
}

void CommandProcessor::clearDataReady() {
  m_ready = false;
}

void CommandProcessor::I2C_dataIn(int numBytes) {
  CommandProcessor::m_instance->clearBuffer();
  uint8_t recv_buffer_len = (numBytes > BUFFER_SIZE) ? BUFFER_SIZE : numBytes;

  for (uint8_t i = 0; i < recv_buffer_len && Wire.available(); i++) {
    CommandProcessor::m_instance->appendToBuffer(Wire.read());
  }
  
  CommandProcessor::m_instance->m_ready = true;
}

void CommandProcessor::I2C_dataOut() {
  Wire.write((const uint8_t*)CommandProcessor::m_instance->m_buffer, CommandProcessor::m_instance->m_len);
  CommandProcessor::m_instance->clearBuffer();
}
