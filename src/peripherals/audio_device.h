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

#ifndef PERIPHERALS_AUDIO_DEVICE_H_
#define PERIPHERALS_AUDIO_DEVICE_H_

#include <cstddef>
#include <cstdint>

#include "audio_codec.h"
#include "audio_common.h"
#include "audio_i2s.h"

namespace peripherals {

// composite default audio device interface
class AudioDevice {
 public:
  // Get the current audio configuration.
  // The current audio configuration is always valid.
  //
  // Returns:
  //  copy of AudioConfiguration
  virtual AudioConfiguration GetCurrentConfiguration() const {
    return i2s_.GetCurrentConfiguration();
  }

  // Set the current audio configuration.
  // This method cannot be used within the audio callback handler.
  //
  // Returns:
  //  true if the audio configuration is valid
  //  false otherwise
  virtual bool SetCurrentConfiguration(const AudioConfiguration& new_config) {
    if (is_callback_handler_active_) {
      return false;
    }

    i2s_.Stop(kPlay);
    codec_.Stop(kPlay);
    i2s_.Stop(kRecord);
    codec_.Stop(kRecord);

    bool result = false;
    AudioConfiguration old_config;
    old_config = i2s_.GetCurrentConfiguration();
    result = i2s_.SetCurrentConfiguration(new_config);
    if (!result) {
      return false;
    }

    result = codec_.SetCurrentConfiguration(new_config);
    if (!result) {
      i2s_.SetCurrentConfiguration(old_config);
      return false;
    }

    return result;
  }

  // Start playback or recording.
  // This method cannot be used within the audio callback handler.
  //
  // Notes:
  //  When starting playback, the internal playback buffers are
  //  zero-filled and playback begins immediately
  virtual void Start(const AudioFunction which) {
    if (is_callback_handler_active_) {
      return;
    }
    codec_.Start(which);
    i2s_.Start(which);
  }

  // Stop playback or recording.
  // This method cannot be used within the audio callback handler.
  //
  virtual void Stop(const AudioFunction which) {
    if (is_callback_handler_active_) {
      return;
    }
    i2s_.Stop(which);
    codec_.Stop(which);
  }

  // Get and reset the current playback underrun state.
  //
  // Returns:
  //  true if there was a playback underrun
  //  false otherwise
  virtual bool HadPlayUnderrun() { return i2s_.HadPlayUnderrun(); }

  // Get and reset the current recording overrun state.
  //
  // Returns:
  //  true if there was a recording overrun
  //  false otherwise
  virtual bool HadRecordOverrun() { return i2s_.HadRecordOverrun(); }

  // Feed sample data to the internal playback buffers.
  // This method is non-blocking.
  // The number of samples transferred may be less than the requested amount.
  //
  // Returns:
  //  number of samples transferred
  virtual size_t WritePlayBuffer(const void* from, const size_t samples) {
    return i2s_.WritePlayBuffer(from, samples);
  }

  // Drain sample data from the internal recording buffers.
  // This method is non-blocking.
  // The number of samples transferred may be less than the requested amount.
  //
  // Returns:
  //  number of samples transferred
  virtual size_t ReadRecordBuffer(void* to, const size_t samples) {
    return i2s_.ReadRecordBuffer(to, samples);
  }

  // Get the playback/recording sample counter.
  // The counters remain available after calling the Stop method.
  //
  // Returns:
  //  count of samples played or recorded
  virtual uint64_t SampleCount(const AudioFunction which) const {
    return i2s_.SampleCount(which);
  }

  // Get the internal buffer space available.
  // For playback, this is the number of samples that can be fed to the
  // internal buffers.
  // For recording, this is the number of samples that can be drained
  // from the internal buffers.
  //
  // Returns:
  //  number of samples
  virtual size_t BufferAvailable(const AudioFunction which) const {
    return i2s_.BufferAvailable(which);
  }

  // Set the playback or recording volume as a percentage (0-100)
  virtual void SetVolume(const AudioFunction which, const float percent) {
    codec_.SetVolume(which, percent);
  }

  // Mute the playback or recording
  virtual void Mute(const AudioFunction which, const bool enable) {
    codec_.Mute(which, enable);
  }

  // Set the audio callback handler.
  // The callback handler will be executed on each internal buffer update,
  // when playback and/or recording is active.
  // This method cannot be used within the audio callback handler.
  //
  // Notes:
  //  The callback handler may be executed from within an interrupt handler.
  //  Therefore, time spent in the callback handler should be minimized.
  virtual void SetCallbackHandler(const AudioCallback handler) {
    if (is_callback_handler_active_) {
      return;
    }
    callback_handler_ = handler;
    if (handler == nullptr) {
      i2s_.SetCallbackHandler(nullptr);
    } else {
      AudioCallback cb = [this](const AudioFunction which) {
        this->CallbackHandler(which);
      };
      i2s_.SetCallbackHandler(cb);
    }
  }

 private:
  IAudioI2S& i2s_;
  IAudioCodec& codec_;
  bool is_callback_handler_active_;
  AudioCallback callback_handler_;

  void CallbackHandler(const AudioFunction which) {
    if (callback_handler_ != nullptr) {
      is_callback_handler_active_ = true;
      callback_handler_(which);
      is_callback_handler_active_ = false;
    }
  }

  AudioDevice() = delete;

 protected:
  explicit AudioDevice(IAudioI2S& i2s, IAudioCodec& codec)
      : i2s_{i2s},
        codec_{codec},
        is_callback_handler_active_{false},
        callback_handler_{nullptr} {}
};

}  // namespace peripherals

#endif  // PERIPHERALS_AUDIO_DEVICE_H_
