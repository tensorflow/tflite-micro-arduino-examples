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

#ifndef PERIPHERALS_WM8960_H_
#define PERIPHERALS_WM8960_H_

#include "audio_codec.h"
#include "i2c.h"
#include "wm8960_regs.h"

namespace peripherals {

class WM8960 final : public IAudioCodec {
 public:
  static IAudioCodec& Instance(I2C&);

 private:
  bool is_initialized_;
  bool is_playing_;
  bool is_recording_;
  bool is_mclk_disabled_;
  bool is_play_muted_;
  bool is_record_muted_;
  I2C& i2c_;
  AudioConfiguration cached_config_;
  uint16_t register_cache_[kREGISTER_LAST_ + 1];

  bool Initialize();
  void WriteRegister(const WM8960_Registers reg);
  void UpdateField(const WM8960_Field& field, const uint16_t value);
  void Play(const bool enable);
  void Record(const bool enable);
  void PlayMute(const bool enable);
  void RecordMute(const bool enable);
  void SetPlayVolume(const float percent);
  void SetRecordVolume(const float percent);
  void SetConfig(const AudioConfiguration& config);
  void PowerDown(const bool want_power_down);

  virtual AudioConfiguration GetCurrentConfiguration() const override;
  virtual bool SetCurrentConfiguration(
      const AudioConfiguration& config) override;

  virtual void Mute(const AudioFunction which, const bool enable) override;
  virtual void Start(const AudioFunction which) override;
  virtual void Stop(const AudioFunction which) override;
  virtual void SetVolume(const AudioFunction which,
                         const float percent) override;

  virtual bool HasSampleRate(const AudioFunction which,
                             const AudioSampleRate rate) const override;
  virtual bool HasChannelConfig(
      const AudioChannelConfig channel) const override;
  virtual bool HasFunction(const AudioFunction which) const override;
  virtual bool HasSampleWidth(const AudioSampleWidth width) const override;
  virtual bool HasSimultaneousPlayRecord() const override;
  virtual bool HasVolume(const AudioFunction which) const override;
  virtual bool HasMute(const AudioFunction which) const override;

  WM8960() = delete;
  explicit WM8960(I2C& i2c);
};

}  // namespace peripherals

#endif  // PERIPHERALS_WM8960_H_
