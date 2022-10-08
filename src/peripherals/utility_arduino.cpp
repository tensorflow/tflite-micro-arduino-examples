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

#include <limits>

#include "peripherals.h"
#include "utility.h"

namespace peripherals {

void Initialize() {
  // Some boards (ex. Teensy 4.x) are very sensitive to misconfigured
  // pins that are connected to pull-ups.  This usually results in additional
  // noise being propagated throughout the SOC, interfering with other modules
  // like I2C.
  pinMode(kLED_DEFAULT_GPIO, OUTPUT);
  pinMode(kBUTTON_GPIO, INPUT);
  pinMode(kI2S_BIT_CLK, INPUT);
  pinMode(kI2S_LR_CLK, INPUT);
  pinMode(kI2S_DATA_IN, INPUT);
  pinMode(kI2S_DATA_OUT, OUTPUT);
}

void DelayMicroseconds(uint32_t delay) {
  constexpr uint16_t kArduinoAccurateDelay = 16383;

  while (delay > kArduinoAccurateDelay) {
    delayMicroseconds(kArduinoAccurateDelay);
    delay -= kArduinoAccurateDelay;
  }
  delayMicroseconds(delay);
}

void DelayMilliseconds(uint32_t amount) {
  // Arduino millisecond delay cannot be used within an interrupt handler.
  // Use microsecond delay instead.
  constexpr uint32_t kArduinoMaxMilliseconds =
      std::numeric_limits<decltype(kArduinoMaxMilliseconds)>::max() / 1000;
  while (amount > kArduinoMaxMilliseconds) {
    DelayMicroseconds(kArduinoMaxMilliseconds * 1000);
    amount -= kArduinoMaxMilliseconds;
  }
  DelayMicroseconds(amount * 1000);
}

uint32_t MicrosecondsCounter() { return micros(); }

uint32_t MillisecondsCounter() { return millis(); }

void DebugOutput(const char* s) { Serial.println(s); }

TimestampBuffer::TimestampBuffer()
    : insert_index_{0}, show_index_{0}, entries_{} {}

TimestampBuffer& TimestampBuffer::Instance() {
  static TimestampBuffer TimestampBuffer_static;

  return TimestampBuffer_static;
}

void TimestampBuffer::Insert(const char c) {
  size_t next_index = (insert_index_ + 1) % kNumEntries;
  if (next_index == show_index_) {
    // insert overflow
    if (entries_[insert_index_].c_ != '\0') {
      entries_[insert_index_].c_ = '\0';
      entries_[insert_index_].timestamp_us_ = micros();
    }
  } else {
    // insert new timestamp
    entries_[insert_index_].c_ = c;
    entries_[insert_index_].timestamp_us_ = micros();
    insert_index_ = (insert_index_ + 1) % kNumEntries;
  }
}

void TimestampBuffer::Show() {
  uint32_t prev_time;
  if (show_index_ == 0) {
    prev_time = entries_[kNumEntries - 1].timestamp_us_;
  } else {
    prev_time = entries_[show_index_ - 1].timestamp_us_;
  }

  while (show_index_ != insert_index_) {
    if (entries_[show_index_].c_ == '\0') {
      // overflow message
      Serial.print(entries_[show_index_].timestamp_us_);
      Serial.print(": ");
      Serial.println("*** TimestampBuffer Overflow ***");
      break;
    } else {
      uint32_t show_time = entries_[show_index_].timestamp_us_;
      Serial.print(show_time);
      Serial.print(" (");
      Serial.print(show_time - prev_time);
      Serial.print("): ");
      Serial.println(entries_[show_index_].c_);
      show_index_ = (show_index_ + 1) % kNumEntries;
      prev_time = show_time;
    }
  }
  show_index_ = insert_index_;
}

}  // namespace peripherals
