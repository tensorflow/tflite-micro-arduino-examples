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

#ifndef PERIPHERALS_AUDIO_COMMON_H_
#define PERIPHERALS_AUDIO_COMMON_H_

#include <functional>

namespace peripherals {

enum AudioFunction { kPlay, kRecord };

enum AudioChannelConfig {
  kStereo,            // stereo input to output onto left and right
  kMono,              // mono input to output duplicated onto left and right
  kMonoLeft,          // mono input to output onto left only
  kMonoRight,         // mono input to output onto right only
  kMonoLeftRightMix,  // stereo input mono-mixed to output onto left and right
};

enum AudioSampleRate {
  kRate_8000 = 8000,
  kRate_11025 = 11025,
  kRate_12000 = 12000,
  kRate_16000 = 16000,
  kRate_22050 = 22050,
  kRate_24000 = 24000,
  kRate_32000 = 32000,
  kRate_44100 = 44100,
  kRate_48000 = 48000,
};

enum AudioSampleWidth {
  kSize_8bit = 8,
  kSize_16bit = 16,
  kSize_20bit = 20,
  kSize_24bit = 24,
  kSize_32bit = 32,
};

struct AudioConfiguration {
  AudioSampleRate play_rate;
  AudioSampleRate record_rate;
  AudioSampleWidth sample_width;
  AudioChannelConfig channel_config;
};

using AudioCallback = std::function<void(const AudioFunction which)>;

}  // namespace peripherals

#endif  // PERIPHERALS_AUDIO_COMMON_H_
