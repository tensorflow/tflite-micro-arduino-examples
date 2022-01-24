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

#include "base64.h"

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace test_over_serial {

namespace {
constexpr uint8_t kIndex_A = 0;
constexpr uint8_t kIndex_a = 26;
constexpr uint8_t kIndex_0 = 52;
constexpr uint8_t kIndex_Plus = 62;
constexpr uint8_t kIndex_Slash = 63;
constexpr uint8_t kIndex_Pad = 127;
constexpr uint8_t kIndex_Illegal = 128;

inline uint8_t ConvertToBase64(char c) {
  if (c >= 'A' && c <= 'Z') {
    return (c - 'A') + kIndex_A;
  } else if (c >= 'a' && c <= 'z') {
    return (c - 'a') + kIndex_a;
  } else if (c >= '0' && c <= '9') {
    return (c - '0') + kIndex_0;
  } else if (c == '+') {
    return kIndex_Plus;
  } else if (c == '/') {
    return kIndex_Slash;
  } else if (c == '=') {
    return kIndex_Pad;
  } else {
    return kIndex_Illegal;
  }
}

}  // namespace

// DecodeBase64
// Decode a base64 encoding specified by <input> and <length>.
// The decoded bytes are stored in <output>, which must be large
// enough to hold all the decoded bytes.
// Returns the number of bytes decoded.
// Returns -1 on decode error or if <output> is too small to contain
// all decoded data.
int DecodeBase64(const char* input, size_t input_length, size_t output_length,
                 uint8_t* output) {
  constexpr int kBase64Bits = 6;
  size_t output_index = 0;
  uint8_t current_value = 0;
  int bits_to_fill = 8;  // <current_value> starting with most sig. bit

  for (size_t input_index = 0; input_index < input_length; input_index++) {
    if (std::isspace(input[input_index])) {
      // skip whitespace
      continue;
    }
    const uint8_t value = ConvertToBase64(input[input_index]);
    if (value == kIndex_Illegal) {
      // illegal character in input
      return -1;
    } else if (value == kIndex_Pad) {
      // end of input reached
      break;
    } else {
      // update output
      if (bits_to_fill >= kBase64Bits) {
        bits_to_fill -= kBase64Bits;
        current_value |= value << bits_to_fill;
      } else {
        bits_to_fill -= kBase64Bits;
        current_value |= value >> -bits_to_fill;
      }
      if (bits_to_fill <= 0) {
        // current_value ready to be stored in <output>
        if (output_index >= output_length) {
          // output buffer too small
          return -1;
        }
        output[output_index++] = current_value;
        bits_to_fill += 8;
        current_value = value << bits_to_fill;
      }
    }
  }

  return output_index;
}

}  // namespace test_over_serial
