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

#ifndef PERIPHERALS_I2C_ARDUINO_H_
#define PERIPHERALS_I2C_ARDUINO_H_

#include <Arduino.h>
#include <Wire.h>

#include "i2c.h"

namespace peripherals {

class I2C_Arduino : public I2C {
 public:
  static I2C& Instance0();
  static I2C& Instance1();

 private:
  arduino::MbedI2C& channel_;
  bool channel_initialized_;

  I2C_Arduino() = delete;
  explicit I2C_Arduino(arduino::MbedI2C& channel);

  bool Initialize() override;
  void Write(uint8_t address, uint16_t value) override;
  uint16_t Read(uint8_t address) override;
};

}  // namespace peripherals

#endif  // PERIPHERALS_I2C_ARDUINO_H_