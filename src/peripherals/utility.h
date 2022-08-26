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

#ifndef PERIPHERALS_UTILITY_H_
#define PERIPHERALS_UTILITY_H_

#include <cstddef>
#include <cstdint>

namespace peripherals {

extern void DelayMicroseconds(uint32_t delay);
extern void DelayMilliseconds(uint32_t delay);

extern uint32_t MicrosecondsCounter();
extern uint32_t MillisecondsCounter();

extern void DebugOutput(const char* s);

class TimestampBuffer {
 public:
  static TimestampBuffer& Instance();

  void Insert(const char c);
  void Show();

 private:
  TimestampBuffer();

  size_t insert_index_;
  size_t show_index_;

  static constexpr size_t kNumEntries = 100;
  struct {
    uint32_t timestamp_us_;
    char c_;
  } entries_[kNumEntries];
};

}  // namespace peripherals

#endif  // PERIPHERALS_UTILITY_H_