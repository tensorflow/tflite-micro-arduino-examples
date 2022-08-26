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

#ifndef PERIPHERALS_I2C_H_
#define PERIPHERALS_I2C_H_

#include <cstdint>

namespace peripherals {

class I2C {
 public:
  virtual bool Initialize() = 0;
  virtual void Write(uint8_t address, uint16_t value) = 0;
  virtual uint16_t Read(uint8_t address) = 0;
};

}  // namespace peripherals

#endif  // PERIPHERALS_I2C_H_
