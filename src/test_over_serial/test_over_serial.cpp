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

#include "test_over_serial.h"

#include <climits>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <tuple>

#include "base64.h"
#include "tensorflow/lite/micro/micro_string.h"
#include "tensorflow/lite/micro/system_setup.h"

namespace test_over_serial {

// base64 decoded line length
// assumes kSerialMaxInputLength is divisible by 16
// (base64 encoded and decoded data in multiples of 4 bytes)
constexpr size_t kBase64MaxDecodeLength = (kSerialMaxInputLength * 3) / 4;

constexpr size_t k64BitUIntLength = 20;  // string length of 64bit uint
const char* const kCommandTest = "TEST";
const char* const kCommandData = "DATA";
const char* const kCommandDataAck = "DATA_ACK";
const char* const kCommandOk = "OK";
const char* const kCommandFail = "FAIL";
const char* const kDelimiter = " ";

class TestOverSerialImpl : public TestOverSerial {
 private:
  bool in_data_mode_;
  InputBuffer data_info_;

  void TokenizeInput(char* input, const char*& arg1, const char*& arg2,
                     const char*& arg3) {
    arg1 = strtok(input, kDelimiter);
    arg2 = strtok(nullptr, kDelimiter);
    arg3 = strtok(nullptr, kDelimiter);
  }

  const char* DataTypeToString() {
    switch (data_type_) {
      case kIMAGE_GRAYSCALE:
        return "image-grayscale";
      case kRAW_INT8:
        return "raw-int8";
      case kRAW_FLOAT:
        return "raw-float";
      case kAUDIO_PCM_16KHZ_MONO_S16:
        return "audio-pcm-16khz-mono-s16";
      default:
        break;
    }

    return "unknown";
  }

  size_t DataTypeToUnitSize() {
    switch (data_type_) {
      case kIMAGE_GRAYSCALE:
        return sizeof(uint8_t);
      default:
      case kRAW_INT8:
        return sizeof(int8_t);
      case kRAW_FLOAT:
        return sizeof(float);
      case kAUDIO_PCM_16KHZ_MONO_S16:
        return sizeof(int16_t);
    }
    // NOTREACHED
  }

  void ProcessInputData(const char* in_buffer, size_t in_length,
                        const InputHandler* handler) {
    uint8_t decoded_buffer[kBase64MaxDecodeLength];
    const char* result = "";

    int decoded_length = DecodeBase64(in_buffer, in_length, decoded_buffer);
    uint8_t** p = const_cast<uint8_t**>(&data_info_.data.uint8);
    *p = reinterpret_cast<uint8_t*>(decoded_buffer);
    if (decoded_length > 0 && (decoded_length % DataTypeToUnitSize()) == 0) {
      data_info_.length = decoded_length / DataTypeToUnitSize();
      size_t offset = data_info_.offset + data_info_.length;
      if (nullptr != handler && (offset <= data_info_.total)) {
        bool handler_result = (*handler)(&data_info_);
        if (!handler_result) {
          // abort input processing
          in_data_mode_ = false;
          DataReply(kCommandFail);
          return;
        } else {
          DataAckReply(decoded_length);
        }
      }
      data_info_.offset = offset;

      if (data_info_.offset < data_info_.total) {
        // not yet at end of data
        return;
      }

      in_data_mode_ = false;
      if (data_info_.offset > data_info_.total) {
        // received more data than expected
        result = kCommandFail;
      } else {
        // received all data
        result = kCommandOk;
      }
    } else {
      // illegal decode length or decode error
      in_data_mode_ = false;
      result = kCommandFail;
    }

    DataReply(result);
  }

  inline bool IsDataMode() { return in_test_mode_ && in_data_mode_; }

  void TestOkReply(size_t input_length) {
    char data_length[k64BitUIntLength + 1];
    MicroSnprintf(data_length, sizeof(data_length), "%u", input_length);
    Reply({kCommandOk, kCommandTest, data_length});
  }

  void DataAckReply(size_t decoded_length) {
    char data_length[k64BitUIntLength + 1];
    MicroSnprintf(data_length, sizeof(data_length), "%u", decoded_length);
    Reply({kCommandDataAck, data_length});
  }

  void DataReply(const char* result) {
    char data_length[k64BitUIntLength + 1];
    MicroSnprintf(data_length, sizeof(data_length), "%u",
                  data_info_.total * DataTypeToUnitSize());
    Reply({result, kCommandData, DataTypeToString(), data_length});
  }

  void Reply(const std::initializer_list<const char* const>& list) {
    SerialWrite("!");
    for (auto elem : list) {
      if (elem != *list.begin()) {
        SerialWrite(" ");
      }
      SerialWrite(elem);
    }
    SerialWrite("\n");
  }

  bool ProcessDataInfo(const char* const data_type,
                       const char* const data_length) {
    int8_t** p = const_cast<int8_t**>(&data_info_.data.int8);
    *p = nullptr;
    data_info_.length = 0;
    data_info_.offset = 0;
    data_info_.total = 0;

    if (strcmp(data_type, DataTypeToString()) == 0) {
      size_t total = std::strtoul(data_length, nullptr, 10);
      if (total == 0 || total == ULONG_MAX) {
        // unable to convert to unsigned long
        return false;
      }
      data_info_.total = total / DataTypeToUnitSize();
    } else {
      // mismatched data type
      return false;
    }

    return true;
  }

 public:
  TestOverSerialImpl()
      : TestOverSerial(), in_data_mode_(false), data_info_({}) {}

  void ProcessInput(const InputHandler* handler) override {
    size_t received;
    char* input_buffer;

    std::tie(received, input_buffer) = SerialReadLine(10);
    if (received == 0 || input_buffer == NULL) {
      return;
    }

    if (input_buffer[0] != '!') {
      if (IsDataMode()) {
        ProcessInputData(input_buffer, received, handler);
      } else {
        // unknown command
        // output FAIL with input_buffer
        Reply({kCommandFail, input_buffer});
      }
      return;
    }

    // process commands
    const char* command;
    const char* data_type;
    const char* data_length;
    TokenizeInput(input_buffer + 1, command, data_type, data_length);
    if (strcmp(command, kCommandTest) == 0) {
      // TEST command
      in_test_mode_ = true;
      TestOkReply(kBase64MaxDecodeLength);
    } else if (strcmp(command, kCommandData) == 0) {
      // DATA command
      if (nullptr == data_type) {
        Reply({kCommandFail, input_buffer});
        return;
      }
      if (nullptr == data_length) {
        Reply({kCommandFail, input_buffer});
        return;
      }
      if (!IsTestMode()) {
        Reply({kCommandFail, command, data_type, data_length});
        return;
      }
      if (!ProcessDataInfo(data_type, data_length)) {
        Reply({kCommandFail, command, data_type, data_length});
        return;
      }

      in_data_mode_ = true;
      // reply when all data received

    } else {
      // unknown command
      // output FAIL with input_buffer
      Reply({kCommandFail, input_buffer});
    }
  }
};

static TestOverSerialImpl singleton;

TestOverSerial::TestOverSerial()
    : in_test_mode_(false), data_type_(kRAW_INT8) {}

TestOverSerial& TestOverSerial::Instance(const TestDataType data_type) {
  singleton.data_type_ = data_type;
  return singleton;
}

}  // namespace test_over_serial
