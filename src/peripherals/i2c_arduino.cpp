/* Copyright 2022 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "i2c_arduino.h"

#include "peripherals.h"

namespace peripherals {

I2C& I2C_Arduino::Instance0() {
  static I2C_Arduino i2c_arduino_static_0(Wire);

  return i2c_arduino_static_0;
}

I2C& I2C_Arduino::Instance1() {
  static I2C_Arduino i2c_arduino_static_1(Wire1);

  return i2c_arduino_static_1;
}

I2C_Arduino::I2C_Arduino(arduino::MbedI2C& channel)
    : I2C(), channel_(channel), channel_initialized_(false) {}

bool I2C_Arduino::Initialize() {
  if (!channel_initialized_) {
    channel_.begin();
    channel_.setClock(kI2C_CLOCK);
    channel_initialized_ = true;
  }
  return true;
}

void I2C_Arduino::Write(uint8_t address, uint16_t value) {
  if (!channel_initialized_) {
    return;
  }

  uint8_t result = 0;
  do {
    channel_.beginTransmission(address);
    channel_.write(static_cast<uint8_t>(value >> 8));
    channel_.write(static_cast<uint8_t>(value & 0xFF));
    result = channel_.endTransmission();
  } while (result != 0);
}

uint16_t I2C_Arduino::Read(uint8_t address) {
  if (!channel_initialized_) {
    return 0;
  }

  uint16_t value = 0;
  channel_.requestFrom(address, sizeof(value));
  value = (channel_.read() << 8) & 0xFF00;
  value |= channel_.read() & 0xFF;
  return value;
}

}  // namespace peripherals
