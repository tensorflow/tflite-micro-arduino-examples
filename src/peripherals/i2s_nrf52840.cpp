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

#undef I2S_NRF52840_DEBUG
#undef I2S_NRF52840_DEBUG_INT

#include "i2s_nrf52840.h"

#include <Arduino.h>
#include <hal/nrf_i2s.h>

#include <algorithm>
#include <cstring>
#include <type_traits>

#include "peripherals.h"

namespace peripherals {

namespace {

inline void TimestampPrint(const char c) {
#ifdef I2S_NRF52840_DEBUG
  TimestampBuffer::Instance().Insert(c);
#endif  // I2S_NRF52840_DEBUG
}

}  // namespace

I2S_nrf52840& I2S_nrf52840::Instance() {
  static I2S_nrf52840 I2S_nrf52840_static;

  return I2S_nrf52840_static;
}

void I2S_nrf52840::SetCallbackHandler(const AudioCallback handler) {
  callback_handler_ = handler;
}

AudioConfiguration I2S_nrf52840::GetCurrentConfiguration() const {
  // current configuration is always valid
  return cached_config_;
}

bool I2S_nrf52840::SetCurrentConfiguration(const AudioConfiguration& config) {
  if (!Initialize()) {
    return false;
  }

  if (is_playing_) {
    StopPlay();
  }
  if (is_recording_) {
    StopRecord();
  }

  if (config.play_rate != config.record_rate) {
    return false;
  }

  switch (config.play_rate) {
    case kRate_8000:
      // fallthrough
    case kRate_11025:
      // fallthrough
    case kRate_12000:
      // fallthrough
    case kRate_16000:
      // fallthrough
    case kRate_22050:
      // fallthrough
    case kRate_24000:
      // fallthrough
    case kRate_32000:
      // fallthrough
    case kRate_44100:
      // fallthrough
    case kRate_48000:
      break;
    default:
      return false;
  }

  bool result = SetConfig(config);
  if (!result) {
    return false;
  }

  cached_config_.play_rate = config.play_rate;
  cached_config_.record_rate = config.record_rate;
  cached_config_.channel_config = config.channel_config;
  cached_config_.sample_width = config.sample_width;

  return true;
}

void I2S_nrf52840::Start(const AudioFunction which) {
  if (!Initialize()) {
    return;
  }

  switch (which) {
    case kPlay:
      if (!is_playing_) {
        StartPlay();
      }
      break;
    case kRecord:
      if (!is_recording_) {
        StartRecord();
      }
      break;
  }
}

void I2S_nrf52840::Stop(const AudioFunction which) {
  if (!Initialize()) {
    return;
  }

  switch (which) {
    case kPlay:
      if (is_playing_) {
        StopPlay();
      }
      break;
    case kRecord:
      if (is_recording_) {
        StopRecord();
      }
      break;
  }
}

bool I2S_nrf52840::HadPlayUnderrun() {
  if (!is_callback_handler_active_) {
    NVIC_DisableIRQ(I2S_IRQn);
  }
  bool result = had_underrun_;
  had_underrun_ = false;
  if (!is_callback_handler_active_) {
    NVIC_EnableIRQ(I2S_IRQn);
  }
  return result;
}

bool I2S_nrf52840::HadRecordOverrun() {
  if (!is_callback_handler_active_) {
    NVIC_DisableIRQ(I2S_IRQn);
  }
  bool result = had_overrun_;
  had_overrun_ = false;
  if (!is_callback_handler_active_) {
    NVIC_EnableIRQ(I2S_IRQn);
  }
  return result;
}

size_t I2S_nrf52840::WritePlayBuffer(const void* const from,
                                     const size_t samples) {
  if (!is_playing_) {
    return 0;
  }

  if (play_write_ptr_ == nullptr) {
    if (samples == 0) {
      // don't change play var state unless adding data to play buffer
      return 0;
    }
    // setup write pointer
    is_play_write_pending_ = true;
    while (is_play_write_pending_) {
      // wait for interrupt and next buffer
      delayMicroseconds(20);
    }
    // play_write_ptr_ now set by interrupt handler
  }

  size_t remain_bytes_1 = 0;
  size_t remain_bytes_2 = 0;
  volatile uint8_t* write_ptr_1 = nullptr;
  volatile uint8_t* write_ptr_2 = nullptr;
  const size_t requested_xfer_bytes = SamplesToBytes(samples);

  if (!is_callback_handler_active_) {
    NVIC_DisableIRQ(I2S_IRQn);
    if (!is_playing_) {
      NVIC_EnableIRQ(I2S_IRQn);
      return 0;
    }
  }

  // calculate available space
  if (play_write_ptr_ > play_current_dma_ptr_) {
    remain_bytes_1 = &play_buffer_[kBufferSize] - play_write_ptr_;
    remain_bytes_2 = play_current_dma_ptr_ - play_buffer_;
  } else {
    remain_bytes_1 = play_current_dma_ptr_ - play_write_ptr_;
  }

  // clamp to requested transfer size if needed
  if (requested_xfer_bytes < (remain_bytes_1 + remain_bytes_2)) {
    if (requested_xfer_bytes < remain_bytes_1) {
      remain_bytes_1 = requested_xfer_bytes;
      remain_bytes_2 = 0;
    } else {
      remain_bytes_2 = requested_xfer_bytes - remain_bytes_1;
    }
  }

  write_ptr_1 = play_write_ptr_;
  play_write_ptr_ += remain_bytes_1;
  if (play_write_ptr_ == &play_buffer_[kBufferSize]) {
    // wrap around
    play_write_ptr_ = play_buffer_;
  }
  write_ptr_2 = play_write_ptr_;
  play_write_ptr_ += remain_bytes_2;

  if (!is_callback_handler_active_) {
    NVIC_EnableIRQ(I2S_IRQn);
  }

  const uint8_t* from2 = static_cast<const uint8_t*>(from) + remain_bytes_1;
  std::memcpy(const_cast<uint8_t*>(write_ptr_1), from, remain_bytes_1);
  std::memcpy(const_cast<uint8_t*>(write_ptr_2), from2, remain_bytes_2);

  return BytesToSamples(remain_bytes_1 + remain_bytes_2);
}

size_t I2S_nrf52840::ReadRecordBuffer(void* const to, const size_t samples) {
  if (!is_recording_) {
    return 0;
  }
  if (record_read_ptr_ == nullptr) {
    return 0;
  }

  size_t remain_bytes_1 = 0;
  size_t remain_bytes_2 = 0;
  volatile uint8_t* read_ptr_1 = nullptr;
  volatile uint8_t* read_ptr_2 = nullptr;
  const size_t requested_xfer_bytes = SamplesToBytes(samples);

  if (!is_callback_handler_active_) {
    NVIC_DisableIRQ(I2S_IRQn);
    if (!is_recording_) {
      NVIC_EnableIRQ(I2S_IRQn);
      return 0;
    }
  }

  // calculate available data
  if (record_read_ptr_ > record_current_dma_ptr_) {
    remain_bytes_1 = &record_buffer_[kBufferSize] - record_read_ptr_;
    remain_bytes_2 = record_current_dma_ptr_ - record_buffer_;
  } else {
    remain_bytes_1 = record_current_dma_ptr_ - record_read_ptr_;
  }

  // clamp to requested transfer size if needed
  if (requested_xfer_bytes < (remain_bytes_1 + remain_bytes_2)) {
    if (requested_xfer_bytes < remain_bytes_1) {
      remain_bytes_1 = requested_xfer_bytes;
      remain_bytes_2 = 0;
    } else {
      remain_bytes_2 = requested_xfer_bytes - remain_bytes_1;
    }
  }

  read_ptr_1 = record_read_ptr_;
  record_read_ptr_ += remain_bytes_1;
  if (record_read_ptr_ == &record_buffer_[kBufferSize]) {
    // wrap around
    record_read_ptr_ = record_buffer_;
  }
  read_ptr_2 = record_read_ptr_;
  record_read_ptr_ += remain_bytes_2;

  if (!is_callback_handler_active_) {
    NVIC_EnableIRQ(I2S_IRQn);
  }

  uint8_t* const to2 = static_cast<uint8_t* const>(to) + remain_bytes_1;
  std::memcpy(to, const_cast<uint8_t*>(read_ptr_1), remain_bytes_1);
  std::memcpy(to2, const_cast<uint8_t*>(read_ptr_2), remain_bytes_2);

  return BytesToSamples(remain_bytes_1 + remain_bytes_2);
}

uint64_t I2S_nrf52840::SampleCount(const AudioFunction which) {
  if (!Initialize()) {
    return 0;
  }

  uint64_t result = 0;

  if (!is_callback_handler_active_) {
    NVIC_DisableIRQ(I2S_IRQn);
  }
  switch (which) {
    case kPlay:
      result = play_sample_count_;
      break;
    case kRecord:
      result = record_sample_count_;
      break;
  }
  if (!is_callback_handler_active_) {
    NVIC_EnableIRQ(I2S_IRQn);
  }

  return result;
}

size_t I2S_nrf52840::BufferAvailable(const AudioFunction which) {
  if (!Initialize()) {
    return 0;
  }

  size_t result = 0;

  if (!is_callback_handler_active_) {
    NVIC_DisableIRQ(I2S_IRQn);
  }
  switch (which) {
    case kPlay:
      if (!is_playing_) {
        result = sizeof(play_buffer_);
      } else if (play_write_ptr_ == nullptr) {
        result = sizeof(play_buffer_) - kBufferIncrement;
      } else if (play_write_ptr_ > play_current_dma_ptr_) {
        result =
            sizeof(play_buffer_) - (play_write_ptr_ - play_current_dma_ptr_);
      } else {
        result = play_current_dma_ptr_ - play_write_ptr_;
      }
      break;
    case kRecord:
      if (!is_recording_ || record_read_ptr_ == nullptr) {
        result = 0;
      } else if (record_read_ptr_ > record_current_dma_ptr_) {
        result = sizeof(record_buffer_) -
                 (record_read_ptr_ - record_current_dma_ptr_);
      } else {
        result = record_current_dma_ptr_ - record_read_ptr_;
      }
      break;
  }
  if (!is_callback_handler_active_) {
    NVIC_EnableIRQ(I2S_IRQn);
  }

  result = BytesToSamples(result);

  return result;
}

I2S_nrf52840::I2S_nrf52840()
    : IAudioI2S(),
      is_initialized_(false),
      is_playing_(false),
      is_recording_(false),
      had_overrun_(false),
      had_underrun_(false),
      is_callback_handler_active_(false),
      is_play_write_pending_(false),
      is_play_count_pending_(false),
      callback_handler_(nullptr),
      play_sample_count_(0),
      record_sample_count_(0),
      play_write_ptr_(nullptr),
      play_current_dma_ptr_(nullptr),
      play_next_dma_ptr_(nullptr),
      record_read_ptr_(nullptr),
      record_current_dma_ptr_(nullptr),
      record_next_dma_ptr_(nullptr),
      play_buffer_{},
      record_buffer_{},
      cached_config_{} {
  cached_config_.play_rate = kRate_16000;
  cached_config_.record_rate = kRate_16000;
  cached_config_.sample_width = kSize_16bit;
  cached_config_.channel_config = kStereo;
}

bool I2S_nrf52840::Initialize() {
  if (is_initialized_) {
    return true;
  }

#if defined(I2S_NRF52840_DEBUG) || defined(I2S_NRF52840_DEBUG_INT)
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(D5, OUTPUT);
  digitalWrite(LEDR, HIGH);
  digitalWrite(LEDG, HIGH);
  digitalWrite(LEDB, HIGH);
  digitalWrite(D4, LOW);
  digitalWrite(D5, LOW);
#endif  // I2S_NRF52840_DEBUG

  nrf_i2s_pins_set(NRF_I2S, kI2S_BIT_CLK, kI2S_LR_CLK,
                   NRF_I2S_PIN_NOT_CONNECTED, kI2S_DATA_OUT, kI2S_DATA_IN);
  nrf_i2s_transfer_set(NRF_I2S, kBufferIncrement / sizeof(uint32_t), nullptr,
                       nullptr);
  nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_RXPTRUPD);
  nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_TXPTRUPD);
  nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_STOPPED);
  nrf_i2s_int_enable(NRF_I2S, NRF_I2S_INT_RXPTRUPD_MASK |
                                  NRF_I2S_INT_TXPTRUPD_MASK |
                                  NRF_I2S_INT_STOPPED_MASK);
  nrf_i2s_enable(NRF_I2S);

  // set the I2S IRQ priority and enable
  auto f = +[]() { peripherals::I2S_nrf52840::Instance().InterruptHandler(); };
  NVIC_SetVector(I2S_IRQn, reinterpret_cast<uint32_t>(f));
  NVIC_SetPriority(I2S_IRQn, kI2S_IRQ_PRIORITY);
  NVIC_ClearPendingIRQ(I2S_IRQn);
  NVIC_EnableIRQ(I2S_IRQn);

  // set default config
  if (!SetConfig(cached_config_)) {
    return false;
  }

  is_initialized_ = true;
  return true;
}

void I2S_nrf52840::InterruptHandler() {
  TimestampPrint('!');
#ifdef I2S_NRF52840_DEBUG_INT
  digitalWrite(LEDG, LOW);
  digitalWrite(D4, HIGH);
  bool unknown_interrupt = true;
#endif  // I2S_NRF52840_DEBUG_INT

  bool need_play_callback = false;
  bool need_record_callback = false;

  // Only check and clear one event per interrupt.  Otherwise the nrf52840
  // will generate additional spurious interrupts.

  if (nrf_i2s_event_check(NRF_I2S, NRF_I2S_EVENT_TXPTRUPD)) {
#ifdef I2S_NRF52840_DEBUG_INT
    unknown_interrupt = false;
#endif  // I2S_NRF52840_DEBUG_INT

    if (!is_playing_) {
      // spurious event as per errata #55 (nrf52840 version 1)
      TimestampPrint('t');
    } else {
      TimestampPrint('T');
      play_current_dma_ptr_ = play_next_dma_ptr_;
      play_next_dma_ptr_ += kBufferIncrement;
      if (play_next_dma_ptr_ == &play_buffer_[kBufferSize]) {
        play_next_dma_ptr_ = play_buffer_;
      }
      nrf_i2s_tx_buffer_set(NRF_I2S,
                            reinterpret_cast<uint32_t*>(
                                const_cast<uint8_t*>(play_next_dma_ptr_)));
      if (is_play_write_pending_) {
        is_play_write_pending_ = false;
        is_play_count_pending_ = true;
        play_write_ptr_ = play_next_dma_ptr_;
      } else if (is_play_count_pending_) {
        // now doing DMA to initial write pointer, dont start counting samples
        // until next interrupt
        is_play_count_pending_ = false;
      } else if (play_write_ptr_ != nullptr) {
        // increment sample count
        play_sample_count_ += BytesToSamples(kBufferIncrement);
      }
      // check for underrun
      if (SameBufferSegment(const_cast<uint8_t*>(play_write_ptr_),
                            const_cast<uint8_t*>(play_current_dma_ptr_))) {
        had_underrun_ = true;
      }
      need_play_callback = true;
    }
#ifdef I2S_NRF52840_DEBUG
    digitalWrite(LEDR, LOW);
#endif  // I2S_NRF52840_DEBUG
#ifdef I2S_NRF52840_DEBUG_INT
    digitalWrite(LEDG, HIGH);
#endif  // I2S_NRF52840_DEBUG_INT

    nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_TXPTRUPD);

  } else if (nrf_i2s_event_check(NRF_I2S, NRF_I2S_EVENT_RXPTRUPD)) {
#ifdef I2S_NRF52840_DEBUG_INT
    unknown_interrupt = false;
#endif  // I2S_NRF52840_DEBUG_INT

    if (!is_recording_) {
      // spurious event as per errata #55 (nrf52840 version 1)
      TimestampPrint('r');
    } else {
      TimestampPrint('R');
      record_current_dma_ptr_ = record_next_dma_ptr_;
      record_next_dma_ptr_ += kBufferIncrement;
      if (record_next_dma_ptr_ == &record_buffer_[kBufferSize]) {
        record_next_dma_ptr_ = record_buffer_;
      }
      nrf_i2s_rx_buffer_set(NRF_I2S,
                            reinterpret_cast<uint32_t*>(
                                const_cast<uint8_t*>(record_next_dma_ptr_)));
      // check for overrun
      if (SameBufferSegment(const_cast<uint8_t*>(record_read_ptr_),
                            const_cast<uint8_t*>(record_current_dma_ptr_))) {
        had_overrun_ = true;
      }
      // check to setup read pointer
      if (record_read_ptr_ == nullptr) {
        record_read_ptr_ = record_current_dma_ptr_;
      } else {
        // increment sample count
        record_sample_count_ += BytesToSamples(kBufferIncrement);
      }
      need_record_callback = true;
    }
#ifdef I2S_NRF52840_DEBUG
    digitalWrite(LEDG, LOW);
#endif  // I2S_NRF52840_DEBUG
#ifdef I2S_NRF52840_DEBUG_INT
    digitalWrite(LEDG, HIGH);
#endif  // I2S_NRF52840_DEBUG_INT
    nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_RXPTRUPD);

  } else if (nrf_i2s_event_check(NRF_I2S, NRF_I2S_EVENT_STOPPED)) {
#ifdef I2S_NRF52840_DEBUG_INT
    unknown_interrupt = false;
#endif  // I2S_NRF52840_DEBUG_INT

    TimestampPrint('S');
    // power consumption fix as per errata #194 (nrf52840 version 1)
    *reinterpret_cast<uint32_t*>(NRF_I2S_BASE + 0x38) = 1;
    *reinterpret_cast<uint32_t*>(NRF_I2S_BASE + 0x3C) = 1;
#ifdef I2S_NRF52840_DEBUG
    digitalWrite(LEDB, LOW);
#endif  // I2S_NRF52840_DEBUG
#ifdef I2S_NRF52840_DEBUG_INT
    digitalWrite(LEDG, HIGH);
#endif  // I2S_NRF52840_DEBUG_INT
    nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_STOPPED);
  }

  is_callback_handler_active_ = true;
  if (callback_handler_ != nullptr && need_play_callback) {
    callback_handler_(kPlay);
  }
  if (callback_handler_ != nullptr && need_record_callback) {
    callback_handler_(kRecord);
  }
  is_callback_handler_active_ = false;

#ifdef I2S_NRF52840_DEBUG_INT
  if (unknown_interrupt) {
    digitalWrite(D5, HIGH);
    peripherals::DelayMicroseconds(1);
    digitalWrite(D5, LOW);
  }
  digitalWrite(D4, LOW);
#endif  // I2S_NRF52840_DEBUG_INT
#ifdef I2S_NRF52840_DEBUG
  peripherals::DelayMicroseconds(1000);
  digitalWrite(LEDR, HIGH);
  digitalWrite(LEDG, HIGH);
  digitalWrite(LEDB, HIGH);
#endif  // I2S_NRF52840_DEBUG
}

void I2S_nrf52840::StartDMA() {
  // start the DMA engine (record and play)
  // I2S interrupts must already be disabled

  play_current_dma_ptr_ = nullptr;
  play_next_dma_ptr_ = play_buffer_;
  nrf_i2s_tx_buffer_set(NRF_I2S, reinterpret_cast<uint32_t*>(
                                     const_cast<uint8_t*>(play_next_dma_ptr_)));
  record_current_dma_ptr_ = nullptr;
  record_next_dma_ptr_ = record_buffer_;
  // no need to zero record_buffer_ as it will be overwritten
  nrf_i2s_rx_buffer_set(
      NRF_I2S,
      reinterpret_cast<uint32_t*>(const_cast<uint8_t*>(record_next_dma_ptr_)));
  nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_RXPTRUPD);
  nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_TXPTRUPD);
  nrf_i2s_event_clear(NRF_I2S, NRF_I2S_EVENT_STOPPED);
  NRF_I2S->CONFIG.TXEN = 1;
  NRF_I2S->CONFIG.RXEN = 1;
  nrf_i2s_task_trigger(NRF_I2S, NRF_I2S_TASK_START);
}

void I2S_nrf52840::StartPlay() {
  if (!is_callback_handler_active_) {
    NVIC_DisableIRQ(I2S_IRQn);
    if (is_playing_) {
      NVIC_EnableIRQ(I2S_IRQn);
      return;
    }
  }

  // clear the play buffer
  std::memset(play_buffer_, 0, sizeof(play_buffer_));

  if (!is_recording_) {
    StartDMA();
  } else {
    NRF_I2S->CONFIG.TXEN = 1;
  }
  play_write_ptr_ = nullptr;
  had_underrun_ = false;
  play_sample_count_ = 0;
  is_playing_ = true;
  TimestampPrint('O');
  if (!is_callback_handler_active_) {
    NVIC_EnableIRQ(I2S_IRQn);
  }
}

void I2S_nrf52840::StartRecord() {
  if (!is_callback_handler_active_) {
    NVIC_DisableIRQ(I2S_IRQn);
    if (is_recording_) {
      NVIC_EnableIRQ(I2S_IRQn);
      return;
    }
  }
  if (!is_playing_) {
    StartDMA();
  } else {
    NRF_I2S->CONFIG.RXEN = 1;
  }
  record_read_ptr_ = nullptr;
  had_overrun_ = false;
  record_sample_count_ = 0;
  is_recording_ = true;
  TimestampPrint('I');
  if (!is_callback_handler_active_) {
    NVIC_EnableIRQ(I2S_IRQn);
  }
}

void I2S_nrf52840::StopPlay() {
  if (!is_callback_handler_active_) {
    NVIC_DisableIRQ(I2S_IRQn);
    if (!is_playing_) {
      NVIC_EnableIRQ(I2S_IRQn);
      return;
    }
  }
  NRF_I2S->CONFIG.TXEN = 0;
  if (!is_recording_) {
    nrf_i2s_task_trigger(NRF_I2S, NRF_I2S_TASK_STOP);
  }
  is_playing_ = false;
  is_play_write_pending_ = false;
  is_play_count_pending_ = false;
  TimestampPrint('o');
  if (!is_callback_handler_active_) {
    NVIC_EnableIRQ(I2S_IRQn);
  }
}

void I2S_nrf52840::StopRecord() {
  if (!is_callback_handler_active_) {
    NVIC_DisableIRQ(I2S_IRQn);
    if (!is_recording_) {
      NVIC_EnableIRQ(I2S_IRQn);
      return;
    }
  }
  NRF_I2S->CONFIG.RXEN = 0;
  if (!is_playing_) {
    nrf_i2s_task_trigger(NRF_I2S, NRF_I2S_TASK_STOP);
  }
  is_recording_ = false;
  TimestampPrint('i');
  if (!is_callback_handler_active_) {
    NVIC_EnableIRQ(I2S_IRQn);
  }
}

bool I2S_nrf52840::SetConfig(const AudioConfiguration& config) {
  nrf_i2s_swidth_t sample_width;
  nrf_i2s_channels_t channels;

  switch (config.channel_config) {
    case kStereo:
      channels = NRF_I2S_CHANNELS_STEREO;
      break;
    default:
      return false;
  }

  switch (config.sample_width) {
    case kSize_8bit:
      sample_width = NRF_I2S_SWIDTH_8BIT;
      break;
    case kSize_16bit:
      sample_width = NRF_I2S_SWIDTH_16BIT;
      break;
    default:
      return false;
  }

  bool result = nrf_i2s_configure(
      NRF_I2S, NRF_I2S_MODE_SLAVE, NRF_I2S_FORMAT_I2S, NRF_I2S_ALIGN_LEFT,
      sample_width, channels, NRF_I2S_MCK_DISABLED,
      static_cast<nrf_i2s_ratio_t>(0) /* ratio not used */);
  return result;
}

size_t I2S_nrf52840::BytesToSamples(const size_t num_bytes) const {
  size_t result = num_bytes;

  switch (cached_config_.sample_width) {
    case kSize_8bit:
      break;
    case kSize_16bit:
      // fallthrough
    default:
      result /= 2;
      break;
  }

  switch (cached_config_.channel_config) {
    case kStereo:
      // fallthrough
    default:
      result /= 2;
      break;
  }

  return result;
}

size_t I2S_nrf52840::SamplesToBytes(const size_t num_samples) const {
  size_t result = num_samples;

  switch (cached_config_.sample_width) {
    case kSize_8bit:
      break;
    case kSize_16bit:
      // fallthrough
    default:
      result *= 2;
      break;
  }

  switch (cached_config_.channel_config) {
    case kStereo:
      // fallthrough
    default:
      result *= 2;
      break;
  }

  return result;
}

bool I2S_nrf52840::SameBufferSegment(const uint8_t* a, const uint8_t* b) const {
  uint32_t c, d;

  c = reinterpret_cast<decltype(c)>(a);
  d = reinterpret_cast<decltype(d)>(b);
  return (c >> kBufferIncrementShift) == (d >> kBufferIncrementShift);
}

}  // namespace peripherals
