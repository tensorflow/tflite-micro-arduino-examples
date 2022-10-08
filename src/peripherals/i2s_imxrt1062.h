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

#ifndef PERIPHERALS_I2S_IMXRT1062_H_
#define PERIPHERALS_I2S_IMXRT1062_H_

#include "DMAChannel.h"
#include "audio_i2s.h"

namespace peripherals {

class I2S_imxrt1062 final : public IAudioI2S {
 public:
  static I2S_imxrt1062& Instance();

 private:
  void DMA_InterruptHandler(const AudioFunction which);
  void EnableInterrupts(const AudioFunction which) const;
  void DisableInterrupts(const AudioFunction which) const;

  void SetCallbackHandler(const AudioCallback handler) override;

  AudioConfiguration GetCurrentConfiguration() const override;
  bool SetCurrentConfiguration(const AudioConfiguration& config) override;

  void Start(const AudioFunction which) override;
  void Stop(const AudioFunction which) override;
  bool HadPlayUnderrun() override;
  bool HadRecordOverrun() override;

  size_t WritePlayBuffer(const void* const from, const size_t samples) override;
  size_t ReadRecordBuffer(void* const to, const size_t samples) override;
  uint64_t SampleCount(const AudioFunction which) override;
  size_t BufferAvailable(const AudioFunction which) override;

  bool Initialize() override;

  I2S_imxrt1062();

  void StartPlay();
  void StartRecord();
  void StartDMA(const AudioFunction which);
  void StopDMA();
  void StopPlay();
  void StopRecord();
  size_t BytesToSamples(const size_t num_bytes) const;
  size_t SamplesToBytes(const size_t num_samples) const;
  bool SameBufferSegment(const uint8_t* a, const uint8_t* b) const;

  static constexpr size_t kBufferIncrementShift = 10;
  static constexpr size_t kBufferIncrement = 1 << kBufferIncrementShift;
  static constexpr size_t kNumBuffers = 2;
  static constexpr size_t kBufferSize = kBufferIncrement * kNumBuffers;

  bool is_initialized_;
  bool is_playing_;
  bool is_recording_;
  volatile bool had_overrun_;
  volatile bool had_underrun_;
  volatile bool is_callback_handler_active_;
  volatile bool is_play_write_pending_;
  volatile bool is_play_count_pending_;
  AudioCallback callback_handler_;
  volatile uint64_t play_sample_count_;
  volatile uint64_t record_sample_count_;
  volatile uint8_t* play_write_ptr_;
  volatile uint8_t* play_current_dma_ptr_;
  volatile uint8_t* play_next_dma_ptr_;
  volatile uint8_t* record_read_ptr_;
  volatile uint8_t* record_current_dma_ptr_;
  volatile uint8_t* record_next_dma_ptr_;
  // 32 byte alignment for ARM cache flush/delete
  alignas(32) uint8_t play_buffer_[I2S_imxrt1062::kBufferSize];
  alignas(32) uint8_t record_buffer_[I2S_imxrt1062::kBufferSize];
  AudioConfiguration cached_config_;
  DMAChannel dma_tx_;
  DMAChannel dma_rx_;
};

}  // namespace peripherals

#endif  // PERIPHERALS_I2S_IMXRT1062_H_
