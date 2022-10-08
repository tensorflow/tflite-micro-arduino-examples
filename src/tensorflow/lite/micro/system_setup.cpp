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

#include "tensorflow/lite/micro/system_setup.h"

#include <limits>

#include "peripherals/peripherals.h"
#include "tensorflow/lite/micro/debug_log.h"

#define DEBUG_SERIAL_OBJECT (SerialUSB)

extern "C" void DebugLog(const char* s) { DEBUG_SERIAL_OBJECT.print(s); }

namespace tflite {

constexpr ulong kSerialMaxInitWait = 4000;  // milliseconds

void InitializeTarget() {
  peripherals::Initialize();

  DEBUG_SERIAL_OBJECT.begin(9600);
  ulong start_time = millis();
  while (!DEBUG_SERIAL_OBJECT) {
    // allow for Arduino IDE Serial Monitor synchronization
    if (millis() - start_time > kSerialMaxInitWait) {
      break;
    }
  }
}

}  // namespace tflite

namespace test_over_serial {

// Change baud rate on default serial port
void SerialChangeBaudRate(const int baud) {
  DEBUG_SERIAL_OBJECT.begin(baud);
  ulong start_time = millis();
  while (!DEBUG_SERIAL_OBJECT) {
    // allow for Arduino IDE Serial Monitor synchronization
    if (millis() - start_time > tflite::kSerialMaxInitWait) {
      break;
    }
  }
}

template <size_t N>
class _LineBuffer {
 public:
  bool NeedReset() { return need_reset_; }
  void NeedReset(bool value) { need_reset_ = value; }
  void Clear() {
    need_reset_ = false;
    index_ = 0;
  }
  char* Buffer() { return buffer_; }
  size_t Available() { return index_; }
  size_t AvailableToStore() { return N - index_; }
  void StoreChar(char c) {
    if (index_ < N) {
      buffer_[index_++] = c;
    }
  }

 private:
  bool need_reset_ = false;
  size_t index_ = 0;
  char buffer_[N];
};

// SerialReadLine
// Read a set of ASCII characters from the default
// serial port.  Data is read up to the first newline ('\\n') character.
// This function uses an internal buffer which is automatically reset.
// The buffer will not contain the newline character.
// The buffer will be zero ('\\0') terminated.
// The <timeout> value is in milliseconds.  Any negative value means that
// the wait for data will be forever.
// Returns std::pair<size_t, char*>.
// The first pair element is the number of characters in buffer not including
// the newline character or zero terminator.
// Returns {0, NULL} if the timeout occurs.
std::pair<size_t, char*> SerialReadLine(int timeout) {
  static _LineBuffer<kSerialMaxInputLength + 1> line_buffer;

  if (line_buffer.NeedReset()) {
    line_buffer.Clear();
  }

  ulong start_time = millis();

  while (true) {
    int value = DEBUG_SERIAL_OBJECT.read();
    if (value >= 0) {
      if (value == '\n') {
        // read a newline character
        line_buffer.StoreChar('\0');
        line_buffer.NeedReset(true);
        break;
      } else {
        // read other character
        line_buffer.StoreChar(value);
        if (line_buffer.AvailableToStore() == 1) {
          // buffer is full
          line_buffer.StoreChar('\0');
          line_buffer.NeedReset(true);
          break;
        }
      }
    }
    if (timeout < 0) {
      // wait forever
      continue;
    } else if (millis() - start_time >= static_cast<ulong>(timeout)) {
      // timeout
      return std::make_pair(0UL, reinterpret_cast<char*>(NULL));
    }
  }

  return std::make_pair(line_buffer.Available() - 1, line_buffer.Buffer());
}

// SerialWrite
// Write the ASCII characters in <buffer> to the default serial port.
// The <buffer> must be zero terminated.
void SerialWrite(const char* buffer) { DEBUG_SERIAL_OBJECT.print(buffer); }

}  // namespace test_over_serial
