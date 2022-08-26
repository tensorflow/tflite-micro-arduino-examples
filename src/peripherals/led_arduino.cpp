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

#include <Arduino.h>

#include <cstdint>

#include "led.h"
#include "peripherals.h"
#include "utility.h"

namespace peripherals {

class LED_Arduino : LED {
 public:
  static LED& Instance() {
    static LED_Arduino led_static;

    if (!led_static.is_initialized_) {
      pinMode(kLED_DEFAULT_GPIO, OUTPUT);
      digitalWrite(kLED_DEFAULT_GPIO, LOW);
      led_static.is_initialized_ = true;
    }

    return led_static;
  }

  void Show(const bool on) override {
    if (on) {
      led_state_ = HIGH;
    } else {
      led_state_ = LOW;
    }
    digitalWrite(kLED_DEFAULT_GPIO, led_state_);
    led_time_ = 0;
  }

  void Blink() override {
    if (led_time_ == 0) {
      led_time_ = peripherals::MillisecondsCounter();
      if (on_time_ > off_time_) {
        led_state_ = HIGH;
      } else {
        led_state_ = LOW;
      }
      digitalWrite(kLED_DEFAULT_GPIO, led_state_);
      return;
    }

    auto current_time = peripherals::MillisecondsCounter();
    auto elapsed_time = current_time - led_time_;

    if (led_state_ == HIGH && elapsed_time > on_time_) {
      led_state_ = LOW;
      digitalWrite(kLED_DEFAULT_GPIO, led_state_);
      led_time_ = current_time;

    } else if (led_state_ == LOW && elapsed_time > off_time_) {
      led_state_ = HIGH;
      digitalWrite(kLED_DEFAULT_GPIO, led_state_);
      led_time_ = current_time;
    }
  }

  void SetBlinkParams(const float duty_on,
                      const uint16_t cycle_time_ms) override {
    LED::SetBlinkParams(duty_on, cycle_time_ms);
    on_time_ = duty_on_ * cycle_time_ms_;
    off_time_ = (1.0f - duty_on_) * cycle_time_ms_;
  }

 private:
  int led_state_;
  uint32_t led_time_;
  uint16_t on_time_;
  uint16_t off_time_;
  bool is_initialized_;

  LED_Arduino()
      : LED(),
        led_state_{LOW},
        led_time_{0},
        on_time_(duty_on_ * cycle_time_ms_),
        off_time_((1.0f - duty_on_) * cycle_time_ms_),
        is_initialized_{false} {}
};

LED& LED::Instance() { return LED_Arduino::Instance(); }

}  // namespace peripherals
