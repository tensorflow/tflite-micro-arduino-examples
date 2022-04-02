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
#ifndef TENSORFLOW_LITE_MICRO_BASE64_H_
#define TENSORFLOW_LITE_MICRO_BASE64_H_

#include <cstddef>
#include <cstdint>

namespace test_over_serial {

// DecodeBase64
// Decode a base64 encoding specified by <input> and <length>.
// The decoded bytes are stored in <output>, which must be large
// enough to hold all the decoded bytes.
// Returns the number of bytes decoded.
// Returns -1 on decode error or if <output> is too small to contain
// all decoded data.
int DecodeBase64(const char* input, size_t input_length, size_t output_length,
                 uint8_t* output);

template <size_t N>
inline int DecodeBase64(const char* input, size_t length,
                        uint8_t (&output)[N]) {
  return DecodeBase64(input, length, N, output);
}

}  // namespace test_over_serial

#endif  // TENSORFLOW_LITE_MICRO_BASE64_H_
