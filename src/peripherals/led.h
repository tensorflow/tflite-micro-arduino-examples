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

#ifndef PERIPHERALS_LED_H_
#define PERIPHERALS_LED_H_

#include <cstdint>

namespace peripherals {

class LED {
 public:
  static LED& Instance();

  virtual void Show(const bool on) = 0;
  virtual void Blink() = 0;
  virtual void SetBlinkParams(const float duty_on,
                              const uint16_t cycle_time_ms) {
    duty_on_ = duty_on;
    if (duty_on_ > 1.0f) {
      duty_on_ = 1.0f;
    } else if (duty_on < 0.0f) {
      duty_on_ = 0.0f;
    }
    cycle_time_ms_ = cycle_time_ms;
  }

 protected:
  float duty_on_;
  uint16_t cycle_time_ms_;

  LED() : duty_on_{0.5f}, cycle_time_ms_{1000} {}
};

}  // namespace peripherals

#endif  // PERIPHERALS_LED_H_