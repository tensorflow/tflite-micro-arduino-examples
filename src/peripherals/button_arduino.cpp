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

#include "button.h"
#include "peripherals.h"
#include "utility.h"

namespace peripherals {

class Button_Arduino : Button {
 public:
  static Button& Instance() {
    static Button_Arduino button_static;

    if (!button_static.is_initialized_) {
      pinMode(kBUTTON_GPIO, INPUT);
      attachInterrupt(
          digitalPinToInterrupt(kBUTTON_GPIO),
          []() { button_static.ButtonChangeInterrupt(); }, CHANGE);
      button_static.is_initialized_ = true;
    }

    return button_static;
  }

  void ButtonChangeInterrupt() {
    ButtonState new_state = digitalRead(kBUTTON_GPIO) == LOW ? KDown : kUp;
    uint32_t current_time = peripherals::MillisecondsCounter();
    button_state_current_ = new_state;
    button_state_time_ = current_time;
    is_button_state_active_ = true;
  }

  ButtonPressState GetPressState() override {
    if (!is_button_state_active_) {
      return button_press_state_;
    }

    noInterrupts();
    uint32_t current_time = peripherals::MillisecondsCounter();
    if (current_time - button_state_time_ > kStableTime) {
      if (button_state_current_ == KDown) {
        if (button_press_state_ == kNone) {
          if ((current_time - button_state_time_ > kLongPressTime) &&
              !is_button_press_long_down_) {
            // kNone -> kLongPress
            button_press_state_ = kLongPressDown;
            is_button_press_long_down_ = true;
          }
        } else if (button_press_state_ == kLongPressDown) {
          // no change
          button_press_state_ = kNone;
        }
      } else {
        // button is up
        if (button_press_state_ == kNone && !is_button_press_long_down_) {
          // kNone -> kPressed
          button_press_state_ = kPressed;
        } else if (is_button_press_long_down_) {
          // kLongPressDown -> kLongPressUp
          button_press_state_ = kLongPressUp;
          is_button_press_long_down_ = false;
        } else {
          // kPressed or kLongPressUp -> kNone
          button_press_state_ = kNone;
          button_state_prev_ = kUp;
          is_button_state_active_ = false;
        }
      }
    }
    interrupts();

    return button_press_state_;
  }

  ButtonState GetState() override {
    if (is_button_state_active_) {
      noInterrupts();
      uint32_t current_time = peripherals::MillisecondsCounter();
      if (current_time - button_state_time_ > kStableTime) {
        button_state_prev_ = button_state_current_;
      }
      interrupts();
    }

    return button_state_prev_;
  }

 private:
  static constexpr uint32_t kLongPressTime = 900;  // milliseconds
  static constexpr uint32_t kStableTime = 50;      // milliseconds

  ButtonState button_state_current_;
  ButtonState button_state_prev_;
  ButtonPressState button_press_state_;
  uint32_t button_state_time_;
  bool is_button_state_active_;
  bool is_button_press_long_down_;
  bool is_initialized_;

  Button_Arduino()
      : Button(),
        button_state_current_{kUp},
        button_state_prev_{kUp},
        button_press_state_{kNone},
        button_state_time_{0},
        is_button_state_active_{false},
        is_button_press_long_down_{false},
        is_initialized_{false} {}
};

Button& Button::Instance() { return Button_Arduino::Instance(); }

}  // namespace peripherals
