#pragma once

#include <stdint.h> // for type aliases like `uint32_t`


/// A simple struct that represents a single temperature reading.
struct TempReading {
  TempReading(float _value, bool _ok) : value(_value), ok(_ok) {
  }

  // TODO should we check if the controller is set to farenheit or celsius?
  float value = 0.0;

  // If 'ok' is false, it means something went wrong and we can't trust this reading.
  bool ok = false;
};

// TODO namespace inside Auber?
class AuberStatus {
public:
  AuberStatus(uint8_t _flags): flags(flags) {
  }

  AuberStatus(): flags(0) {
  }

  bool isAutoTuning() {
    // If not, it's 'normal'
    return get(0);
  }

  bool isInManualMode() {
    // If not, it's 'normal'
    return get(1);
  }

  bool isInCoolingMode() {
    // If not, it's in heating mode.
    return get(2);
  }

  bool isInStaticParamSettingMode() {
    // If not, it's 'normal'
    return get(3);
  }

  bool hasAnomaly() {
    return get(4);
  }

  bool isAlarmActivated() {
    return get(5);
  }

  uint8_t flags;

private:
  bool get(uint8_t bit_index) {
    return flags & (0x1 << bit_index);
  }
};


class Auber {
public:
  ////// These are nice, simple functions meant for users:
  bool setup();
  TempReading getProcessTemp();
  TempReading getSetpointTemp();
  bool getStatus(AuberStatus *status_out);


private:
  ///// These functions handle the messy details of communicating with the controller:
  bool requestBits(uint16_t address, uint8_t num_bits, uint32_t *value_out);
  bool requestFloat(uint16_t address, float *value_out);
  int request(int request_type, uint16_t address, uint8_t num_values);
  bool receiveWord(uint16_t *word_out);
  bool receiveBool(bool *bool_out);
  static float to_float(uint16_t word0, uint16_t word1);


  uint32_t m_LastRequestTime = 0;

  // TODO calculate actual required interval for 9600 baud
  static const uint32_t REQUEST_INTERVAL_MS = 20;

  ////// These are magic numbers from the Auber manual:

  static const uint8_t DEVICE_ID = 0x5;
  static const uint16_t BAUD_RATE = 9600;

  // TODO why does the manual say 'Status' and 'Set Value' have the same address?
  // Do they both work?
  // Is it ok because one uses the COIL command and the other uses the HOLDING_REGISTERS command?
  static const uint16_t STATUS_ADDRESS = 0x0;
  static const uint16_t SET_VALUE_ADDRESS = 0x0;
  static const uint16_t PROCESS_VALUE_ADDRESS = 0x0164;
};
