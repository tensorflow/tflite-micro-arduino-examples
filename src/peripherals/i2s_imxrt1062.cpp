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

#undef I2S_IMXRT1062_DEBUG
#undef I2S_IMXRT1062_DEBUG_INTR

#include "i2s_imxrt1062.h"

#include <Arduino.h>

#include <algorithm>
#include <cstring>
#include <type_traits>

#include "peripherals.h"

namespace peripherals {

#ifdef I2S_IMXRT1062_DEBUG_INTR
// debugging vars in the peripherals namespace
volatile uint32_t i2s_imxrt1062_dma_es;
#endif  // I2S_IMXRT1062_DEBUG_INTR

namespace {

inline void TimestampPrint(const char c) {
#if defined(I2S_IMXRT1062_DEBUG) || defined(I2S_IMXRT1062_DEBUG_INTR)
  TimestampBuffer::Instance().Insert(c);
#endif  // I2S_IMXRT1062_DEBUG
}

// TCSR_SR: reset status and FIFO
constexpr uint32_t I2S_TCSR_SR = (1 << 24);
// RCSR_SR: reset status and FIFO
constexpr uint32_t I2S_RCSR_SR = (1 << 24);
// TCSR_FEF: clear fifo error flag
constexpr uint32_t I2S_TCSR_FEF = (1 << 18);
// RCSR_FEF: clear fifo error flag
constexpr uint32_t I2S_RCSR_FEF = (1 << 18);
// TCR4_CHMOD: output zeros during masking or channel disable
constexpr uint32_t I2S_TCR4_CHMOD = (1 << 5);
// TX and RX fifo slots
constexpr size_t I2S_FIFO_SIZE = 32;

inline volatile void DSB() { asm("dsb"); }

}  // namespace

I2S_imxrt1062& I2S_imxrt1062::Instance() {
  static I2S_imxrt1062 I2S_imxrt1062_static DMAMEM;

  return I2S_imxrt1062_static;
}

void I2S_imxrt1062::SetCallbackHandler(const AudioCallback handler) {
  if (!is_initialized_) {
    return;
  }
  callback_handler_ = handler;
}

AudioConfiguration I2S_imxrt1062::GetCurrentConfiguration() const {
  // current configuration is always valid
  return cached_config_;
}

bool I2S_imxrt1062::SetCurrentConfiguration(const AudioConfiguration& config) {
  if (!is_initialized_) {
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

  switch (config.sample_width) {
    case kSize_8bit:
      // fallthrough
    case kSize_16bit:
      break;
    default:
      return false;
  }

  switch (config.channel_config) {
    case kStereo:
      // fallthrough
    case kMono:
      break;
    default:
      return false;
  }

  cached_config_.play_rate = config.play_rate;
  cached_config_.record_rate = config.record_rate;
  cached_config_.channel_config = config.channel_config;
  cached_config_.sample_width = config.sample_width;

  return true;
}

void I2S_imxrt1062::Start(const AudioFunction which) {
  if (!is_initialized_) {
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

void I2S_imxrt1062::Stop(const AudioFunction which) {
  if (!is_initialized_) {
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

bool I2S_imxrt1062::HadPlayUnderrun() {
  if (!is_initialized_) {
    return false;
  }
  DisableInterrupts(kPlay);
  bool result = had_underrun_;
  had_underrun_ = false;
  EnableInterrupts(kPlay);
  return result;
}

bool I2S_imxrt1062::HadRecordOverrun() {
  if (!is_initialized_) {
    return false;
  }
  DisableInterrupts(kRecord);
  bool result = had_overrun_;
  had_overrun_ = false;
  EnableInterrupts(kRecord);
  return result;
}

size_t I2S_imxrt1062::WritePlayBuffer(const void* const from,
                                      const size_t samples) {
  if (!is_initialized_ || !is_playing_) {
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

  DisableInterrupts(kPlay);

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

  EnableInterrupts(kPlay);

  const uint8_t* from2 = static_cast<const uint8_t*>(from) + remain_bytes_1;
  if (remain_bytes_1) {
    std::memcpy(const_cast<uint8_t*>(write_ptr_1), from, remain_bytes_1);
    arm_dcache_flush_delete(const_cast<uint8_t*>(write_ptr_1), remain_bytes_1);
  }
  if (remain_bytes_2) {
    std::memcpy(const_cast<uint8_t*>(write_ptr_2), from2, remain_bytes_2);
    arm_dcache_flush_delete(const_cast<uint8_t*>(write_ptr_2), remain_bytes_2);
  }

  return BytesToSamples(remain_bytes_1 + remain_bytes_2);
}

size_t I2S_imxrt1062::ReadRecordBuffer(void* const to, const size_t samples) {
  if (!is_initialized_ || !is_recording_) {
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

  DisableInterrupts(kRecord);

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

  EnableInterrupts(kRecord);

  // cache invalidation (delete) occurs in interrupt handler
  uint8_t* const to2 = static_cast<uint8_t* const>(to) + remain_bytes_1;
  std::memcpy(to, const_cast<uint8_t*>(read_ptr_1), remain_bytes_1);
  std::memcpy(to2, const_cast<uint8_t*>(read_ptr_2), remain_bytes_2);

  return BytesToSamples(remain_bytes_1 + remain_bytes_2);
}

uint64_t I2S_imxrt1062::SampleCount(const AudioFunction which) {
  if (!is_initialized_) {
    return 0;
  }

  uint64_t result = 0;

  DisableInterrupts(which);
  switch (which) {
    case kPlay:
      result = play_sample_count_;
      break;
    case kRecord:
      result = record_sample_count_;
      break;
  }
  EnableInterrupts(which);

  return result;
}

size_t I2S_imxrt1062::BufferAvailable(const AudioFunction which) {
  if (!is_initialized_) {
    return 0;
  }

  size_t result = 0;

  DisableInterrupts(which);
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
  EnableInterrupts(which);

  result = BytesToSamples(result);

  return result;
}

I2S_imxrt1062::I2S_imxrt1062()
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
      cached_config_{},
      dma_tx_(),
      dma_rx_() {
  cached_config_.play_rate = kRate_16000;
  cached_config_.record_rate = kRate_16000;
  cached_config_.sample_width = kSize_16bit;
  cached_config_.channel_config = kStereo;
}

bool I2S_imxrt1062::Initialize() {
  TimestampPrint('Z');

  if (is_initialized_) {
    return true;
  }

  // Check that both DMA channels were allocated in the constructor.
  // DMA channel allocation handles DMA clock enable and shared DMA config.
  if (dma_rx_.TCD == nullptr || dma_tx_.TCD == nullptr) {
    return false;
  }

  // set non-changing DMA TCD fields
  static_assert(kNumBuffers == 2, "DMA configured for 2 buffers");
  dma_tx_.interruptAtCompletion();
  dma_tx_.interruptAtHalf();
  dma_rx_.interruptAtCompletion();
  dma_rx_.interruptAtHalf();

  // enable DMA error interrupts
  DMA_SEEI = dma_tx_.channel;
  DMA_SEEI = dma_rx_.channel;

  // set the DMA IRQ priority and enable DMA interrupts
  NVIC_CLEAR_PENDING(IRQ_DMA_CH0 + dma_tx_.channel);
  auto f = +[]() { I2S_imxrt1062::Instance().DMA_InterruptHandler(kPlay); };
  dma_tx_.attachInterrupt(f, kI2S_IRQ_PRIORITY);
  NVIC_CLEAR_PENDING(IRQ_DMA_CH0 + dma_rx_.channel);
  f = +[]() { I2S_imxrt1062::Instance().DMA_InterruptHandler(kRecord); };
  dma_rx_.attachInterrupt(f, kI2S_IRQ_PRIORITY);

  // DMA IOMUX routing setup
  dma_tx_.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI2_TX);
  dma_rx_.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI2_RX);

  // enable DMA (arm request enable for each DMA channel)
  dma_tx_.enable();
  dma_rx_.enable();

  // turn on SAI2 (I2S) clock
  CCM_CCGR5 |= CCM_CCGR5_SAI2(CCM_CCGR_ON);

  // SAI2 (I2S) IOMUX routing setup
  CORE_PIN2_CONFIG = 2;                   // SAI2_TX_DATA
  CORE_PIN3_CONFIG = 2;                   // SAI2_TX_SYNC
  CORE_PIN4_CONFIG = 2;                   // SAI2_TX_BCLK
  CORE_PIN5_CONFIG = 2;                   // SAI2_RX_DATA
  IOMUXC_SAI2_RX_DATA0_SELECT_INPUT = 0;  // pin 5 is IN2 input
  IOMUXC_SAI2_TX_BCLK_SELECT_INPUT = 0;   // pin 4 is BCLK2 input
  IOMUXC_SAI2_TX_SYNC_SELECT_INPUT = 0;   // pin 3 is LRCLK2 input

  // GPIO pad configuration
  // TBD??? slew rates, speed, drive current, pull-up

  // setup non-changing SAI2 configuration registers
  // Only RCR1 and RCR3 can be modified while RCSR_RE is set
  // Only TCR1 and TCR3 can be modified while TCSR_TE is set

  // TCSR: soft reset
  I2S2_TCSR = I2S_TCSR_SR;
  // TCR1: half-empty TX watermark
  I2S2_TCR1 = I2S_FIFO_SIZE / 2;
  // TCR2: TX async mode,
  //   I2S bit clock polarity,
  //   LRCLK & BCLK are generated externally,
  //   SAI2 uses bus clock internally
  // Do not modify TCR2 while TCSR_TE is set
  I2S2_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_BCP | I2S_TCR2_MSEL(0);
  // TCR3: TX channel enable (FIFO request enable)
  I2S2_TCR3 = I2S_TCR3_TCE;
  // TCR4: always transmit 2 channels per frame onto I2S bus,
  //   use Output mode (zero fill channels),
  //   MSB first,
  //   frame sync early (I2S style frame sync),
  //   frame sync polarity active low (I2S style frame sync)
  // TBD??? does SYWD need to be set for externally generated LRCLK?
  I2S2_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_CHMOD | I2S_TCR4_MF | I2S_TCR4_FSE |
              I2S_TCR4_FSP;
  // TCR5: AudioConfiguration specific (see SetConfig() for values)
  // TMR: AudioConfiguration specific (see SetConfig() for values)

  // RCSR: soft reset
  I2S2_RCSR = I2S_RCSR_SR;
  // RCR1: AudioConfiguration specific, see StartDMA() for setup
  // RCR2: RX sync mode,
  //   I2S bit clock polarity,
  //   LRCLK & BCLK are generated externally,
  //   SAI2 uses bus clock internally
  // Do not modify RCR2 while RCSR_RE is set
  I2S2_RCR2 = I2S_RCR2_SYNC(1) | I2S_RCR2_BCP | I2S_RCR2_MSEL(0);
  // RCR3: RX channel enable (FIFO request enable)
  I2S2_RCR3 = I2S_RCR3_RCE;
  // RCR4: always receive 2 channels per frame onto I2S bus,
  //   MSB first,
  //   frame sync early (I2S style frame sync),
  //   frame sync polarity active low (I2S style frame sync)
  // TBD??? does SYWD need to be set for externally generated LRCLK?
  I2S2_RCR4 = I2S_RCR4_FRSZ(1) | I2S_RCR4_MF | I2S_RCR4_FSE | I2S_RCR4_FSP;
  // RCR5: AudioConfiguration specific, see StartDMA() for setup
  // RMR: AudioConfiguration specific, see StartDMA() for setup

  TimestampPrint('z');
  is_initialized_ = true;
  return true;
}

void I2S_imxrt1062::EnableInterrupts(const AudioFunction which) const {
  if (!is_callback_handler_active_) {
    const DMAChannel& dma = (which == kPlay) ? dma_tx_ : dma_rx_;
    NVIC_ENABLE_IRQ(dma.channel + IRQ_DMA_CH0);
  }
}

void I2S_imxrt1062::DisableInterrupts(const AudioFunction which) const {
  if (!is_callback_handler_active_) {
    const DMAChannel& dma = (which == kPlay) ? dma_tx_ : dma_rx_;
    NVIC_DISABLE_IRQ(dma.channel + IRQ_DMA_CH0);
  }
}

void I2S_imxrt1062::DMA_InterruptHandler(const AudioFunction which) {
  DSB();
  TimestampPrint('!');

  bool need_callback = false;
  DMAChannel& dma = (which == kPlay) ? dma_tx_ : dma_rx_;

  if (dma.error()) {
    TimestampPrint('E');
    dma.clearError();
#ifdef I2S_IMXRT1062_DEBUG_INTR
    i2s_imxrt1062_dma_es = DMA_ES;
    DMA_CEEI = dma.channel;
    DMA_CR |= DMA_CR_HALT;
    while (DMA_CR & DMA_CR_ACTIVE) {
      DelayMicroseconds(1);
    }
#endif  // I2S_IMXRT1062_DEBUG_INTR

  } else if (which == kPlay) {
    if (!is_playing_) {
      // TBD???: spurious interrupt?  should never happen
      TimestampPrint('t');
    } else {
      TimestampPrint('T');

      // TBD: sync current pointer to dma.TCD->SADDR
      // then compute next pointer from current pointer
      play_current_dma_ptr_ = play_next_dma_ptr_;
      play_next_dma_ptr_ += kBufferIncrement;
      if (play_next_dma_ptr_ == &play_buffer_[kBufferSize]) {
        play_next_dma_ptr_ = play_buffer_;
      }

      if (is_play_write_pending_) {
        is_play_write_pending_ = false;
        is_play_count_pending_ = true;
        play_write_ptr_ = play_next_dma_ptr_;
      } else if (is_play_count_pending_) {
        // now doing DMA from initial write pointer, dont start counting
        // samples until next interrupt
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
      need_callback = true;
    }

    dma.clearInterrupt();

  } else if (which == kRecord) {
    if (!is_recording_) {
      // TBD???: spurious interrupt?  should never happen
      TimestampPrint('r');
    } else {
      TimestampPrint('R');

      // check to setup read pointer
      if (record_read_ptr_ == nullptr) {
        record_read_ptr_ = record_current_dma_ptr_;
      }

      // invalidate data cache
      arm_dcache_delete(const_cast<uint8_t*>(record_current_dma_ptr_),
                        kBufferIncrement);
      // increment sample count
      record_sample_count_ += BytesToSamples(kBufferIncrement);

      // TBD: sync current pointer to dma.TCD->DADDR
      // then compute next pointer from current pointer
      record_current_dma_ptr_ = record_next_dma_ptr_;
      record_next_dma_ptr_ += kBufferIncrement;
      if (record_next_dma_ptr_ == &record_buffer_[kBufferSize]) {
        record_next_dma_ptr_ = record_buffer_;
      }

      // check for overrun
      if (SameBufferSegment(const_cast<uint8_t*>(record_read_ptr_),
                            const_cast<uint8_t*>(record_current_dma_ptr_))) {
        had_overrun_ = true;
      }

      need_callback = true;
    }

    dma.clearInterrupt();
  }

  if (callback_handler_ != nullptr && need_callback) {
    TimestampPrint('C');

    // don't allow nested callback from other DMA channel
    const AudioFunction other = (which == kPlay) ? kRecord : kPlay;
    DisableInterrupts(other);
    is_callback_handler_active_ = true;
    callback_handler_(which);
    is_callback_handler_active_ = false;
    EnableInterrupts(other);
  }

  TimestampPrint('=');
  DSB();
}

void I2S_imxrt1062::StartDMA(const AudioFunction which) {
  // Setup RX and TX DMA descriptors.
  // Set RX enable then set TX enable and also
  // enable RX or TX DMA requests.
  // DMA interrupts must already be disabled.

  uint16_t attr_xsize;
  uint32_t nbytes;
  uint16_t xoff;
  uint32_t num_channels;
  uint32_t xmr;
  const uint8_t fbt = cached_config_.sample_width - 1;
  const uint8_t word_x_width = cached_config_.sample_width - 1;

  switch (cached_config_.sample_width) {
    case kSize_8bit:
      attr_xsize = DMA_TCD_ATTR_SIZE_8BIT;
      nbytes = 1;
      xoff = 1;
      break;
    case kSize_16bit:
      attr_xsize = DMA_TCD_ATTR_SIZE_16BIT;
      nbytes = 2;
      xoff = 2;
      break;
    default:
      return;
  }

  switch (cached_config_.channel_config) {
    case kStereo:
      nbytes *= 2;
      num_channels = 2;
      xmr = 0;
      break;
    case kMono:
      // nbytes unchanged
      num_channels = 1;
      xmr = 1 << 1;  // mask right channel
      break;
    default:
      return;
  }

  play_current_dma_ptr_ = play_buffer_;
  play_next_dma_ptr_ = play_buffer_ + kBufferIncrement;

  record_current_dma_ptr_ = record_buffer_;
  record_next_dma_ptr_ = record_buffer_ + kBufferIncrement;
  // no need to zero record_buffer_ as it will be overwritten

  // setup DMA descriptors
  static_assert(kNumBuffers == 2, "DMA configured for 2 buffers");
  dma_rx_.TCD->SADDR = &I2S2_RDR0;
  dma_rx_.TCD->SOFF = 0;
  dma_rx_.TCD->ATTR =
      DMA_TCD_ATTR_SSIZE(attr_xsize) | DMA_TCD_ATTR_DSIZE(attr_xsize);
  dma_rx_.TCD->NBYTES = nbytes;
  dma_rx_.TCD->SLAST = 0;
  dma_rx_.TCD->DADDR = record_buffer_;
  dma_rx_.TCD->DOFF = xoff;
  dma_rx_.TCD->DLASTSGA = -sizeof(record_buffer_);
  dma_rx_.TCD->BITER = sizeof(record_buffer_) / nbytes;
  dma_rx_.TCD->CITER = dma_rx_.TCD->BITER;

  dma_tx_.TCD->SADDR = play_buffer_;
  dma_tx_.TCD->SOFF = xoff;
  dma_tx_.TCD->ATTR =
      DMA_TCD_ATTR_SSIZE(attr_xsize) | DMA_TCD_ATTR_DSIZE(attr_xsize);
  dma_tx_.TCD->NBYTES = nbytes;
  dma_tx_.TCD->SLAST = -sizeof(play_buffer_);
  dma_tx_.TCD->DADDR = &I2S2_TDR0;
  dma_tx_.TCD->DOFF = 0;
  dma_tx_.TCD->DLASTSGA = 0;
  dma_tx_.TCD->BITER = sizeof(play_buffer_) / nbytes;
  dma_tx_.TCD->CITER = dma_tx_.TCD->BITER;

  // AudioConfiguration specific I2S setup

  // RCR1: full frame (1 or 2 channel) RX watermark
  I2S2_RCR1 = num_channels - 1;
  // TCR5: first bit shifted for MSB first mode, word width
  I2S2_TCR5 = I2S_TCR5_FBT(fbt) | I2S_TCR5_W0W(word_x_width) |
              I2S_TCR5_WNW(word_x_width);
  // TMR: channel masking (zero output)
  I2S2_TMR = xmr;
  // RCR5: first bit shifted for MSB first mode, word width
  I2S2_RCR5 = I2S_RCR5_FBT(fbt) | I2S_RCR5_W0W(word_x_width) |
              I2S_RCR5_WNW(word_x_width);
  // RMR: channel masking (dropped on input)
  I2S2_RMR = xmr;

  // setup TCSR & RCSR
  if (which == kPlay) {
    I2S2_RCSR = I2S_RCSR_RE | I2S_RCSR_FEF | I2S_RCSR_FR;
    I2S2_TCSR = I2S_TCSR_FEF | I2S_TCSR_FR;
    for (size_t i = 0; i < I2S_FIFO_SIZE; i++) {
      I2S2_TDR0 = 0;
    }
    I2S2_TCSR = I2S_TCSR_TE | I2S_TCSR_FEF | I2S_TCSR_FRDE;
  } else {
    I2S2_RCSR = I2S_RCSR_RE | I2S_RCSR_FEF | I2S_RCSR_FR | I2S_TCSR_FRDE;
    I2S2_TCSR = I2S_TCSR_TE | I2S_TCSR_FEF | I2S_TCSR_FR;
  }
}

void I2S_imxrt1062::StopDMA() {
  // Disable TX and RX DMA requests.
  // Wait for any pending TX and RX DMA.
  // Wait for TX underrun (TX fifo empty).
  // Clear TX enable then clear RX enable.
  // Wait for TX and RX frame end.
  // Clear any pending TX and RX DMA interrupts.
  // DMA interrupts must already be disabled.
  I2S2_TCSR = I2S_TCSR_TE | I2S_TCSR_BCE;
  I2S2_RCSR = I2S_RCSR_RE | I2S_RCSR_BCE;
  while (dma_tx_.TCD->CSR & DMA_TCD_CSR_ACTIVE) {
    DelayMicroseconds(1);
  }
  while (dma_rx_.TCD->CSR & DMA_TCD_CSR_ACTIVE) {
    DelayMicroseconds(1);
  }

  while ((I2S2_TCSR & I2S_TCSR_FEF) == 0) {
    DelayMicroseconds(1);
  }

  I2S2_TCSR = 0;
  I2S2_RCSR = 0;
  while (I2S2_TCSR & I2S_TCSR_TE) {
    DelayMicroseconds(1);
  }
  while (I2S2_RCSR & I2S_RCSR_RE) {
    DelayMicroseconds(1);
  }

  dma_tx_.clearInterrupt();
  NVIC_CLEAR_PENDING(IRQ_DMA_CH0 + dma_tx_.channel);
  dma_rx_.clearInterrupt();
  NVIC_CLEAR_PENDING(IRQ_DMA_CH0 + dma_rx_.channel);
}

void I2S_imxrt1062::StartPlay() {
  // clear the play buffer
  std::memset(play_buffer_, 0, sizeof(play_buffer_));
  arm_dcache_flush_delete(play_buffer_, sizeof(play_buffer_));

  DisableInterrupts(kPlay);
  if (!is_recording_) {
    StartDMA(kPlay);
  } else {
    // The fifo should be empty from previous StopPlay().
    // Zero-fill the fifo.
    // Set fifo error flag (TCSR_FEF) to clear error and also
    // set fifo request DMA enable (TCSR_FRDE).
    for (size_t i = 0; i < I2S_FIFO_SIZE; i++) {
      I2S2_TDR0 = 0;
    }
    I2S2_TCSR = I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FEF | I2S_TCSR_FRDE;
  }
  play_write_ptr_ = nullptr;
  had_underrun_ = false;
  play_sample_count_ = 0;
  is_playing_ = true;
  TimestampPrint('O');
  EnableInterrupts(kPlay);
}

void I2S_imxrt1062::StartRecord() {
  DisableInterrupts(kRecord);
  if (!is_playing_) {
    StartDMA(kRecord);
  } else {
    // Wait for fifo overrun.
    // Set fifo error flag (RCSR_FEF) to clear error and also
    // set fifo request DMA enable (RCSR_FRDE) and also
    // set fifo reset (RCSR_FR).
    while ((I2S2_RCSR & I2S_RCSR_FEF) == 0) {
      DelayMicroseconds(1);
    }
    I2S2_RCSR =
        I2S_RCSR_RE | I2S_RCSR_BCE | I2S_RCSR_FEF | I2S_RCSR_FRDE | I2S_RCSR_FR;
  }
  record_read_ptr_ = nullptr;
  had_overrun_ = false;
  record_sample_count_ = 0;
  is_recording_ = true;
  TimestampPrint('I');
  EnableInterrupts(kRecord);
}

void I2S_imxrt1062::StopPlay() {
  DisableInterrupts(kPlay);
  if (!is_recording_) {
    StopDMA();
  } else {
    // Clear fifo request DMA enable (TCSR_FRDE).
    // Wait for pending DMA to complete.
    // Wait for fifo underrun (TX fifo empty).
    // Clear any pending TX DMA interrupt.
    I2S2_TCSR = I2S_TCSR_TE | I2S_TCSR_BCE;
    while (dma_tx_.TCD->CSR & DMA_TCD_CSR_ACTIVE) {
      DelayMicroseconds(1);
    }
    while ((I2S2_TCSR & I2S_TCSR_FEF) == 0) {
      DelayMicroseconds(1);
    }
    dma_tx_.clearInterrupt();
    NVIC_CLEAR_PENDING(IRQ_DMA_CH0 + dma_tx_.channel);
  }
  is_playing_ = false;
  is_play_write_pending_ = false;
  is_play_count_pending_ = false;
  TimestampPrint('o');
  EnableInterrupts(kPlay);
}

void I2S_imxrt1062::StopRecord() {
  DisableInterrupts(kPlay);
  if (!is_playing_) {
    StopDMA();
  } else {
    // Clear fifo request DMA enable (RCSR_FRDE).
    // Wait for pending DMA to complete.
    // Clear any pending RX DMA interrupt.
    I2S2_RCSR = I2S_RCSR_RE | I2S_RCSR_BCE;
    while (dma_rx_.TCD->CSR & DMA_TCD_CSR_ACTIVE) {
      DelayMicroseconds(1);
    }
    dma_rx_.clearInterrupt();
    NVIC_CLEAR_PENDING(IRQ_DMA_CH0 + dma_rx_.channel);
  }
  is_recording_ = false;
  TimestampPrint('i');
  EnableInterrupts(kPlay);
}

size_t I2S_imxrt1062::BytesToSamples(const size_t num_bytes) const {
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
    case kMono:
      break;
    case kStereo:
      // fallthrough
    default:
      result /= 2;
      break;
  }

  return result;
}

size_t I2S_imxrt1062::SamplesToBytes(const size_t num_samples) const {
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
    case kMono:
      break;
    case kStereo:
      // fallthrough
    default:
      result *= 2;
      break;
  }

  return result;
}

bool I2S_imxrt1062::SameBufferSegment(const uint8_t* a,
                                      const uint8_t* b) const {
  uint32_t c, d;

  c = reinterpret_cast<decltype(c)>(a);
  d = reinterpret_cast<decltype(d)>(b);
  return (c >> kBufferIncrementShift) == (d >> kBufferIncrementShift);
}

}  // namespace peripherals

#endif  // ARDUINO_ARCH_IMXRT1062