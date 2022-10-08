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

#ifdef __IMXRT1062__

#include "ws_wm8960_audio_hat_imxrt1062.h"

#include "i2c_arduino.h"
#include "i2s_imxrt1062.h"
#include "wm8960.h"

namespace peripherals {

// Composite audio device implementation for:
// WaveShare WM8960 Audio Hat used with imxrt1062 SOC

AudioDevice& WS_WM8960_AudioHat_IMXRT1062::Instance() {
  static WS_WM8960_AudioHat_IMXRT1062 instance_static;

  return instance_static;
}

WS_WM8960_AudioHat_IMXRT1062::WS_WM8960_AudioHat_IMXRT1062()
    : AudioDevice(I2S_imxrt1062::Instance(),
                  WM8960::Instance(I2C_Arduino::Instance0())) {}

}  // namespace peripherals

#endif  // __IMXRT1062__
