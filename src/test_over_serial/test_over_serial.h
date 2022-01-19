/* Copyright 2021 The TensorFlow Authors. All Rights Reserved.

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
#ifndef TENSORFLOW_LITE_MICRO_TEST_OVER_SERIAL_H_
#define TENSORFLOW_LITE_MICRO_TEST_OVER_SERIAL_H_

#include <cstddef>
#include <cstdint>
#include <functional>

namespace test_over_serial {

enum TestDataType {
  kRAW_INT8,                  // int8
  kRAW_FLOAT,                 // float32
  kIMAGE_GRAYSCALE,           // uint8
  kAUDIO_PCM_16KHZ_MONO_S16,  // PCM@16KHz, mono, int16
};

union DataPtr {
  const int8_t* const int8;
  const uint8_t* const uint8;
  const float* const float32;
  const int16_t* const int16;
};

// length, offset and total use the chosen TestDataType units
struct InputBuffer {
  DataPtr data;   // input buffer pointer
  size_t length;  // input buffer length
  size_t offset;  // offset from start of input
  size_t total;   // total data that will be transferred
};

// The input handler should return:
// <false> to abort input processing
// <true> to continue input processing
using InputHandler = std::function<bool(const InputBuffer* const)>;

class TestOverSerial {
 protected:
  bool in_test_mode_;
  TestDataType data_type_;

  TestOverSerial();

 public:
  static TestOverSerial& Instance(const TestDataType data_type);
  inline bool IsTestMode(void) { return in_test_mode_; }
  virtual void ProcessInput(const InputHandler*) = 0;
};

}  // namespace test_over_serial

#endif  // TENSORFLOW_LITE_MICRO_TEST_OVER_SERIAL_H_
