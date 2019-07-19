#include "Auber.h"
#include <ArduinoRS485.h>
#include <ArduinoModbus.h>


bool Auber::setup() {
  if (!ModbusRTUClient.begin(BAUD_RATE)) {
    Serial.println("Failed to start modbus RTU client!");
    return false;
  }
  return true; // Success!
}

TempReading Auber::getProcessTemp() {
  float value = 0.0;
  bool ok = requestFloat(PROCESS_VALUE_ADDRESS, &value);
  return { value, ok };
}

TempReading Auber::getSetpointTemp() {
  float value = 0.0;
  bool ok = requestFloat(SET_VALUE_ADDRESS, &value);
  return { value, ok };
}

bool Auber::getStatus(AuberStatus *status_out) {
  uint32_t flags = 0;
  bool ok = requestBits(STATUS_ADDRESS, 8, &flags);
  status_out->flags = static_cast<uint8_t>(flags);
  return ok;
}


// If successful, pack the bits into `value_out` and return true. Otherwise
// return false. You can request up to 32 bits at once.
bool Auber::requestBits(uint16_t address, uint8_t num_bits, uint32_t *value_out){
  if (num_bits > 32) {
    // They won't all fit in `value_out`!
    return false;
  }
  if (!request(COILS, address, num_bits)) {
    Serial.print("Failed to request bits: ");
    Serial.println(ModbusRTUClient.lastError());
    return false;
  }

  uint32_t value = 0;
  for (uint8_t index = 0; index < num_bits; index++) {
    bool bit = false;
    if (!receiveBool(&bit)) {
      return false;
    }
    value |= bit << index;
  }
  // Succesfully read all the bits!
  *value_out = value;
  return true;
}

bool Auber::requestFloat(uint16_t address, float *value_out) {
  // Each word/value is 16bits, so we need 2 to read a full 32bit float.
  if (!request(HOLDING_REGISTERS, address, 2)) {
    Serial.print("Failed to request float: ");
    Serial.println(ModbusRTUClient.lastError());
    return false;
  }
  uint16_t word0 = 0;
  uint16_t word1 = 0;
  // TODO keep reading everything that's available regardless of if one word looks bad?
  if (!receiveWord(&word0)) {
    return false;
  }
  if (!receiveWord(&word1)) {
    return false;
  }
  // Success!
  *value_out = to_float(word0, word1);
  return true;
}

int Auber::request(int request_type, uint16_t address, uint8_t num_values) {
  // TODO think about what will happen when millis() overflows (50 days)
  uint32_t interval = millis() - m_LastRequestTime;
  if (interval < REQUEST_INTERVAL_MS) {
    // It's too soon to send another request! That would violate the modbus spec and confuse the device.
    // Wait until the required interval has passed before continuing.
    delay(REQUEST_INTERVAL_MS - interval);
  }
  int out = ModbusRTUClient.requestFrom(DEVICE_ID, request_type, address, num_values);
  m_LastRequestTime = millis();
  return out;
}

// Assumes you've already requested something.
bool Auber::receiveWord(uint16_t *word_out) {
  if(!ModbusRTUClient.available()) {
    return false; // No data available to read!
  }

  // If the read fails, it will give us -1 instead of a real 16bit value from
  // the device (which is always positive).
  int maybe_word = ModbusRTUClient.read();
  if (maybe_word < 0) {
    Serial.print("Failed to read word:  ");
    Serial.println(ModbusRTUClient.lastError());
    return false;
  }
  // Success!
  *word_out = static_cast<uint16_t>(maybe_word);
  return true;
}

// Assumes you've already requested something.
bool Auber::receiveBool(bool *bool_out) {
  uint16_t word = 0;
  if(!receiveWord(&word)){
    return false;
  }
  if (word == 0) {
    *bool_out = false;
  } else if (word == 1) {
    *bool_out = true;
  } else {
    // Let's treat this an error - maybe the value we read wasn't supposed to
    // represent a boolean.
    return false;
  }
  // Success!
  return true;
}


// Word0 is the first value received from the PID controller, and word1 is the second.
float Auber::to_float(uint16_t word0, uint16_t word1) {
  uint32_t raw = (word0<<16) | word1;
  float f;
  memcpy(&f, &raw, sizeof(f));
  return f;
}
