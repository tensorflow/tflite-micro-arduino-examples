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

#ifndef PERIPHERALS_WS_WM8960_AUDIO_HAT_NRF52840_H_
#define PERIPHERALS_WS_WM8960_AUDIO_HAT_NRF52840_H_

#include "audio_device.h"

namespace peripherals {

// Composite audio device implementation for:
// WaveShare WM8960 Audio Hat used with nrf52840 SOC
class WS_WM8960_AudioHat_NRF52840 final : public AudioDevice {
 public:
  static AudioDevice& Instance();

 private:
  WS_WM8960_AudioHat_NRF52840();
};

}  // namespace peripherals

#endif  // PERIPHERALS_WS_WM8960_AUDIO_HAT_NRF52840_H_
