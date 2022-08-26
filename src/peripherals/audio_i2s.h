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

#ifndef PERIPHERALS_AUDIO_I2S_H_
#define PERIPHERALS_AUDIO_I2S_H_

#include <cstddef>
#include <cstdint>

#include "audio_common.h"

namespace peripherals {

class IAudioI2S {
 public:
  virtual AudioConfiguration GetCurrentConfiguration() const = 0;
  virtual bool SetCurrentConfiguration(const AudioConfiguration& config) = 0;

  virtual void Start(const AudioFunction which) = 0;
  virtual void Stop(const AudioFunction which) = 0;
  virtual bool HadPlayUnderrun() = 0;
  virtual bool HadRecordOverrun() = 0;

  virtual size_t WritePlayBuffer(const void* from, const size_t samples) = 0;
  virtual size_t ReadRecordBuffer(void* to, const size_t samples) = 0;
  virtual uint64_t SampleCount(const AudioFunction which) = 0;
  virtual size_t BufferAvailable(const AudioFunction which) = 0;

  virtual void SetCallbackHandler(const AudioCallback handler) = 0;
};

}  // namespace peripherals

#endif  // PERIPHERALS_AUDIO_I2S_H_
