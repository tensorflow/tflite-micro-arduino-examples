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

#include "wm8960.h"

#include "utility.h"

namespace peripherals {

IAudioCodec& WM8960::Instance(I2C& i2c) {
  static WM8960 wm8960_static(i2c);

  return wm8960_static;
}

WM8960::WM8960(I2C& i2c)
    : IAudioCodec(),
      is_initialized_(false),
      is_playing_(false),
      is_recording_(false),
      is_mclk_disabled_(false),
      is_play_muted_(false),
      is_record_muted_(false),
      i2c_(i2c),
      cached_config_{},
      register_cache_{} {
  cached_config_.play_rate = kRate_16000;
  cached_config_.record_rate = kRate_16000;
  cached_config_.sample_width = kSize_16bit;
  cached_config_.channel_config = kStereo;
}

bool WM8960::Initialize() {
  if (is_initialized_) {
    return true;
  }

  if (!i2c_.Initialize()) {
    return false;
  }

  WriteRegister(kRESET);
  DelayMilliseconds(1);
  // update register cache to reflect reset state
  UpdateField(DACMU, kDACMU_MUTE);
  UpdateField(LINMUTE, kL_R_INMUTE_ENABLE);
  UpdateField(RINMUTE, kL_R_INMUTE_ENABLE);
  // reset does not setup a low-power state
  PowerDown(true);

  // setup non-changing values
  UpdateField(CLKSEL, kCLKSEL_PLL);
  UpdateField(SYSCLKDIV, kSYSCLKDIV_2);
  WriteRegister(kCLOCKING_1);
  UpdateField(DCLKDIV, kDCLKDIV_16);
  WriteRegister(kCLOCKING_2);
  UpdateField(PLLPRESCALE, kPLLPRESCALE_2);
  UpdateField(SDM, kSDM_FRACTIONAL_MODE);
  WriteRegister(kPLL_1);
  UpdateField(LIZC, kL_R_IZC_ENABLE);
  WriteRegister(kLEFT_INPUT_VOLUME);
  UpdateField(RIZC, kL_R_IZC_ENABLE);
  UpdateField(IPVU, kIPVU_UPDATE);
  WriteRegister(kRIGHT_INPUT_VOLUME);
  UpdateField(LO1ZC, kL_R_O1ZC_ENABLE);
  WriteRegister(kLOUT1_VOLUME);
  UpdateField(RO1ZC, kL_R_O1ZC_ENABLE);
  UpdateField(OUT1VU, kOUT1VU_UPDATE);
  WriteRegister(kROUT1_VOLUME);
  UpdateField(DACSMM, kDACSMM_RAMP);
  UpdateField(DACMR, kDACMR_FAST);
  WriteRegister(kADC_DAC_CONTROL_2);
  UpdateField(FORMAT, kFORMAT_I2S);
  UpdateField(MS, kMS_MASTER);
  WriteRegister(kAUDIO_INTERFACE_1);
  UpdateField(VSEL, kVSEL_3_3V);
  UpdateField(TOCLKSEL, kTOCLKSEL_FAST);
  UpdateField(TOEN, kTOEN_ENABLE);
  WriteRegister(kADDITIONAL_CONTROL_1);
  UpdateField(LRCM, kLRCM_BOTH);
  WriteRegister(kADDITIONAL_CONTROL_2);
  UpdateField(LMN1, kSIGNAL_PATH_CONNECT);
  UpdateField(LMIC2B, kSIGNAL_PATH_CONNECT);
  WriteRegister(kADCL_SIGNAL_PATH);
  UpdateField(RMN1, kSIGNAL_PATH_CONNECT);
  UpdateField(RMIC2B, kSIGNAL_PATH_CONNECT);
  WriteRegister(kADCR_SIGNAL_PATH);
  UpdateField(LD2LO, kOUT_MIX_ENABLE);
  WriteRegister(kLEFT_OUT_MIX);
  UpdateField(RD2RO, kOUT_MIX_ENABLE);
  WriteRegister(kRIGHT_OUT_MIX);
  UpdateField(SPKLZC, kSPK_L_R_ZC_ENABLE);
  WriteRegister(kLEFT_SPEAKER_VOLUME);
  UpdateField(SPKRZC, kSPK_L_R_ZC_ENABLE);
  UpdateField(SPKVU, kSPKVU_UPDATE);
  WriteRegister(kRIGHT_SPEAKER_VOLUME);
  UpdateField(AC_GAIN, kAC_DC_GAIN_1_40);
  UpdateField(DC_GAIN, kAC_DC_GAIN_1_40);
  WriteRegister(kCLASS_D_CONTROL_2);

  // set ADC digital volume
  UpdateField(LADCVOL, kL_R_ADCVOL_MAX - (30 * 2));  // 0dB
  WriteRegister(kLEFT_ADC_VOLUME);
  UpdateField(RADCVOL, kL_R_ADCVOL_MAX - (30 * 2));  // 0dB
  UpdateField(ADCVU, kADCVU_UPDATE);
  WriteRegister(kRIGHT_ADC_VOLUME);

  // set DAC digital volume
  UpdateField(LDACVOL, kL_R_DACVOL_MAX);  // 0dB
  WriteRegister(kLEFT_DAC_VOLUME);
  UpdateField(RDACVOL, kL_R_DACVOL_MAX);  // 0dB
  UpdateField(DACVU, kDACVU_UPDATE);
  WriteRegister(kRIGHT_DAC_VOLUME);

  // set default config and volume
  SetConfig(cached_config_);
  SetPlayVolume(0.75f);
  SetRecordVolume(0.75f);

  is_initialized_ = true;

  return true;
}

void WM8960::WriteRegister(const WM8960_Registers reg) {
  uint8_t register_address = 0x0F;  // reset
  uint16_t value = 0;

  switch (reg) {
    case kLEFT_INPUT_VOLUME:
      register_address = 0x00;
      break;
    case kRIGHT_INPUT_VOLUME:
      register_address = 0x01;
      break;
    case kLOUT1_VOLUME:
      register_address = 0x02;
      break;
    case kROUT1_VOLUME:
      register_address = 0x03;
      break;
    case kCLOCKING_1:
      register_address = 0x04;
      break;
    case kADC_DAC_CONTROL_1:
      register_address = 0x05;
      break;
    case kADC_DAC_CONTROL_2:
      register_address = 0x06;
      break;
    case kAUDIO_INTERFACE_1:
      register_address = 0x07;
      break;
    case kCLOCKING_2:
      register_address = 0x08;
      break;
    case kAUDIO_INTERFACE_2:
      register_address = 0x09;
      break;
    case kLEFT_DAC_VOLUME:
      register_address = 0x0A;
      break;
    case kRIGHT_DAC_VOLUME:
      register_address = 0x0B;
      break;
    case kRESET:
      register_address = 0x0F;
      break;
    case kLEFT_ADC_VOLUME:
      register_address = 0x15;
      break;
    case kRIGHT_ADC_VOLUME:
      register_address = 0x16;
      break;
    case kADDITIONAL_CONTROL_1:
      register_address = 0x17;
      break;
    case kADDITIONAL_CONTROL_2:
      register_address = 0x18;
      break;
    case kPOWER_MANAGEMENT_1:
      register_address = 0x19;
      break;
    case kPOWER_MANAGEMENT_2:
      register_address = 0x1A;
      break;
    case kANTI_POP_1:
      register_address = 0x1C;
      break;
    case kADCL_SIGNAL_PATH:
      register_address = 0x20;
      break;
    case kADCR_SIGNAL_PATH:
      register_address = 0x21;
      break;
    case kLEFT_OUT_MIX:
      register_address = 0x22;
      break;
    case kRIGHT_OUT_MIX:
      register_address = 0x25;
      break;
    case kLEFT_SPEAKER_VOLUME:
      register_address = 0x28;
      break;
    case kRIGHT_SPEAKER_VOLUME:
      register_address = 0x29;
      break;
    case kPOWER_MANAGEMENT_3:
      register_address = 0x2F;
      break;
    case kADDITIONAL_CONTROL_4:
      register_address = 0x30;
      break;
    case kCLASS_D_CONTROL_1:
      register_address = 0x31;
      value |= kCLASS_D_CONTROL_1_RESERVED;
      break;
    case kCLASS_D_CONTROL_2:
      register_address = 0x33;
      value |= kCLASS_D_CONTROL_2_RESERVED;
      break;
    case kPLL_1:
      register_address = 0x34;
      break;
    case kPLL_2:
      register_address = 0x35;
      break;
    case kPLL_3:
      register_address = 0x36;
      break;
    case kPLL_4:
      register_address = 0x37;
      break;
  }

  value |= register_address << 9;
  value |= register_cache_[reg] & 0x1FF;

  constexpr uint8_t kCODEC_I2C_ADDRESS = 0x1A;
  i2c_.Write(kCODEC_I2C_ADDRESS, value);
}

void WM8960::UpdateField(const WM8960_Field& field, uint16_t value) {
  uint16_t new_value =
      register_cache_[field.reg] & ~(field.mask << field.shift);
  new_value |= (value & field.mask) << field.shift;
  register_cache_[field.reg] = new_value;
}

void WM8960::Play(const bool enable) {
  bool was_muted = is_play_muted_;
  PlayMute(true);
  is_playing_ = enable;
  PowerDown(!enable);
  PlayMute(was_muted);
}

void WM8960::Record(const bool enable) {
  bool was_muted = is_record_muted_;
  RecordMute(true);
  is_recording_ = enable;
  PowerDown(!enable);
  RecordMute(was_muted);
}

void WM8960::PlayMute(const bool enable) {
  UpdateField(DACMU, enable ? kDACMU_MUTE : ~kDACMU_MUTE);
  WriteRegister(kADC_DAC_CONTROL_1);
  is_play_muted_ = enable;
}

void WM8960::RecordMute(const bool enable) {
  uint8_t mute_value = enable ? kL_R_INMUTE_ENABLE : ~kL_R_INMUTE_ENABLE;
  UpdateField(LINMUTE, mute_value);
  WriteRegister(kLEFT_INPUT_VOLUME);
  UpdateField(RINMUTE, mute_value);
  // IPVU already set
  WriteRegister(kRIGHT_INPUT_VOLUME);
  is_record_muted_ = enable;
}

void WM8960::SetPlayVolume(const float percent) {
  uint8_t volume_value;
  // set headphone
  volume_value = static_cast<uint8_t>((percent * kL_R_OUT1VOL_MAX) +
                                      ((1.0f - percent) * kL_R_OUT1VOL_MIN));
  UpdateField(LOUT1VOL, volume_value);
  WriteRegister(kLOUT1_VOLUME);
  UpdateField(ROUT1VOL, volume_value);
  // OUT1VU already set
  WriteRegister(kROUT1_VOLUME);
  // set speaker
  volume_value = static_cast<uint8_t>((percent * kSPK_L_R_VOL_MAX) +
                                      ((1.0f - percent) * kSPK_L_R_VOL_MIN));
  UpdateField(SPKLVOL, volume_value);
  WriteRegister(kLEFT_SPEAKER_VOLUME);
  UpdateField(SPKRVOL, volume_value);
  // SPKVU already set
  WriteRegister(kRIGHT_SPEAKER_VOLUME);
}

void WM8960::SetRecordVolume(const float percent) {
  uint8_t volume_value;
  // set microphone
  volume_value = static_cast<uint8_t>((percent * kL_R_INVOL_MAX) +
                                      ((1.0f - percent) * kL_R_INVOL_MIN));
  UpdateField(LINVOL, volume_value);
  WriteRegister(kLEFT_INPUT_VOLUME);
  UpdateField(RINVOL, volume_value);
  // IPVU already set
  WriteRegister(kRIGHT_INPUT_VOLUME);
}

void WM8960::SetConfig(const AudioConfiguration& config) {
  int sysclk = 0;

  switch (config.play_rate) {
    case kRate_11025:
      sysclk = kSYSCLK_11MHz;
      UpdateField(DACDIV, kDAC_ADC_DIV_4_0);
      UpdateField(ADCDIV, kDAC_ADC_DIV_4_0);
      break;
    case kRate_22050:
      sysclk = kSYSCLK_11MHz;
      UpdateField(DACDIV, kDAC_ADC_DIV_2_0);
      UpdateField(ADCDIV, kDAC_ADC_DIV_2_0);
      break;
    case kRate_44100:
      sysclk = kSYSCLK_11MHz;
      UpdateField(DACDIV, kDAC_ADC_DIV_1_0);
      UpdateField(ADCDIV, kDAC_ADC_DIV_1_0);
      break;
    case kRate_8000:
      sysclk = kSYSCLK_12MHz;
      UpdateField(DACDIV, kDAC_ADC_DIV_6_0);
      UpdateField(ADCDIV, kDAC_ADC_DIV_6_0);
      break;
    case kRate_12000:
      sysclk = kSYSCLK_12MHz;
      UpdateField(DACDIV, kDAC_ADC_DIV_4_0);
      UpdateField(ADCDIV, kDAC_ADC_DIV_4_0);
      break;
    case kRate_16000:
      sysclk = kSYSCLK_12MHz;
      UpdateField(DACDIV, kDAC_ADC_DIV_3_0);
      UpdateField(ADCDIV, kDAC_ADC_DIV_3_0);
      break;
    case kRate_24000:
      sysclk = kSYSCLK_12MHz;
      UpdateField(DACDIV, kDAC_ADC_DIV_2_0);
      UpdateField(ADCDIV, kDAC_ADC_DIV_2_0);
      break;
    case kRate_32000:
      sysclk = kSYSCLK_12MHz;
      UpdateField(DACDIV, kDAC_ADC_DIV_1_5);
      UpdateField(ADCDIV, kDAC_ADC_DIV_1_5);
      break;
    case kRate_48000:
      sysclk = kSYSCLK_12MHz;
      UpdateField(DACDIV, kDAC_ADC_DIV_1_0);
      UpdateField(ADCDIV, kDAC_ADC_DIV_1_0);
      break;
  }
  WriteRegister(kCLOCKING_1);

  if (sysclk == kSYSCLK_11MHz) {
    UpdateField(PLLN, kPLLN_11_2896_MHZ);
    UpdateField(PLLK_23_16, kPLLK_11_2896_MHZ >> 16);
    UpdateField(PLLK_15_8, kPLLK_11_2896_MHZ >> 8);
    UpdateField(PLLK_7_0, kPLLK_11_2896_MHZ & PLLK_7_0.mask);
  } else {
    UpdateField(PLLN, kPLLN_12_288_MHZ);
    UpdateField(PLLK_23_16, kPLLK_12_288_MHZ >> 16);
    UpdateField(PLLK_15_8, kPLLK_12_288_MHZ >> 8);
    UpdateField(PLLK_7_0, kPLLK_12_288_MHZ & PLLK_7_0.mask);
  }
  WriteRegister(kPLL_1);
  WriteRegister(kPLL_2);
  WriteRegister(kPLL_3);
  WriteRegister(kPLL_4);

  // find slowest BCLK that fits in a single frame
  uint8_t bclk_div = kBCLKDIV_1;
  int bclk_div_want = sysclk / (config.play_rate * config.sample_width * 2);
  for (auto item : kBCLKDIV_Map) {
    if (bclk_div_want >= item.div) {
      bclk_div = item.value;
      break;
    }
  }
  UpdateField(BCLKDIV, bclk_div);
  WriteRegister(kCLOCKING_2);

  switch (config.sample_width) {
    case kSize_8bit:
      UpdateField(WL8, kWL8_ENABLE);
      break;
    case kSize_16bit:
      // fallthrough
    default:
      UpdateField(WL8, ~kWL8_ENABLE);
      UpdateField(WL, kWL_16);
      break;
  }
  WriteRegister(kAUDIO_INTERFACE_1);
  WriteRegister(kAUDIO_INTERFACE_2);

  switch (config.channel_config) {
    case kStereo:
      // fallthrough
    default:
      UpdateField(DMONOMIX, kDMONOMIX_STEREO);
      break;
  }
  WriteRegister(kADDITIONAL_CONTROL_1);
}

void WM8960::PowerDown(const bool want_power_down) {
  uint8_t power_enable = want_power_down ? ~kPOWER_MANAGEMENT_POWER_ON
                                         : kPOWER_MANAGEMENT_POWER_ON;
  if (want_power_down) {
    // power down
    if (!is_playing_) {
      UpdateField(SPK_OP_EN, kSPK_OP_EN_OFF);
      WriteRegister(kCLASS_D_CONTROL_1);
      UpdateField(HPSTBY, kHPSTBY_STANDBY);
      WriteRegister(kANTI_POP_1);
      UpdateField(DACL, power_enable);
      UpdateField(DACR, power_enable);
      UpdateField(LOUT1, power_enable);
      UpdateField(ROUT1, power_enable);
      UpdateField(SPKL, power_enable);
      UpdateField(SPKR, power_enable);
      WriteRegister(kPOWER_MANAGEMENT_2);
      UpdateField(LOMIX, power_enable);
      UpdateField(ROMIX, power_enable);
      WriteRegister(kPOWER_MANAGEMENT_3);
    }
    if (!is_recording_) {
      UpdateField(AINL, power_enable);
      UpdateField(AINR, power_enable);
      UpdateField(ADCL, power_enable);
      UpdateField(ADCR, power_enable);
      UpdateField(MICB, power_enable);
      WriteRegister(kPOWER_MANAGEMENT_1);
      UpdateField(LMIC, power_enable);
      UpdateField(RMIC, power_enable);
      WriteRegister(kPOWER_MANAGEMENT_3);
    }
    if (!is_playing_ && !is_recording_) {
      // turn off the over-temp protection
      UpdateField(TSDEN, ~kTSDEN_ENABLE);
      WriteRegister(kADDITIONAL_CONTROL_1);
      UpdateField(TSENSEN, ~kTSENSEN_ENABLE);
      WriteRegister(kADDITIONAL_CONTROL_4);
      // turn off PLL then MCLK
      DelayMilliseconds(1);  // as per datasheet
      UpdateField(PLL_EN, power_enable);
      WriteRegister(kPOWER_MANAGEMENT_2);
      UpdateField(DIGENB, kDIGENB_DISABLE);
      WriteRegister(kPOWER_MANAGEMENT_1);
      // can now turn off VREF and VMIDSEL
      UpdateField(VREF, power_enable);
      UpdateField(VMIDSEL, kVMIDSEL_DISABLE);
      WriteRegister(kPOWER_MANAGEMENT_1);
      UpdateField(SOFT_ST, ~kSOFT_ST_ENABLE);
      WriteRegister(kANTI_POP_1);
      is_mclk_disabled_ = true;
    }
  } else {
    // power up
    if (is_mclk_disabled_) {
      UpdateField(SOFT_ST, kSOFT_ST_ENABLE);
      WriteRegister(kANTI_POP_1);
      // turn on VREF and VMIDSEL
      UpdateField(VREF, power_enable);
      UpdateField(VMIDSEL, kVMIDSEL_ENABLE);
      WriteRegister(kPOWER_MANAGEMENT_1);
      // turn on PLL then MCLK
      UpdateField(PLL_EN, power_enable);
      WriteRegister(kPOWER_MANAGEMENT_2);
      UpdateField(DIGENB, ~kDIGENB_DISABLE);
      WriteRegister(kPOWER_MANAGEMENT_1);
      DelayMilliseconds(1);  // as per datasheet
      // turn on the over-temp protection
      UpdateField(TSDEN, kTSDEN_ENABLE);
      WriteRegister(kADDITIONAL_CONTROL_1);
      UpdateField(TSENSEN, kTSENSEN_ENABLE);
      WriteRegister(kADDITIONAL_CONTROL_4);
      is_mclk_disabled_ = false;
    }
    if (is_playing_) {
      UpdateField(DACL, power_enable);
      UpdateField(DACR, power_enable);
      UpdateField(LOUT1, power_enable);
      UpdateField(ROUT1, power_enable);
      UpdateField(SPKL, power_enable);
      UpdateField(SPKR, power_enable);
      WriteRegister(kPOWER_MANAGEMENT_2);
      UpdateField(LOMIX, power_enable);
      UpdateField(ROMIX, power_enable);
      WriteRegister(kPOWER_MANAGEMENT_3);
      UpdateField(SPK_OP_EN, kSPK_OP_EN_BOTH);
      WriteRegister(kCLASS_D_CONTROL_1);
      UpdateField(HPSTBY, kHPSTBY_NORMAL);
      WriteRegister(kANTI_POP_1);
    }
    if (is_recording_) {
      UpdateField(AINL, power_enable);
      UpdateField(AINR, power_enable);
      UpdateField(ADCL, power_enable);
      UpdateField(ADCR, power_enable);
      UpdateField(MICB, power_enable);
      WriteRegister(kPOWER_MANAGEMENT_1);
      UpdateField(LMIC, power_enable);
      UpdateField(RMIC, power_enable);
      WriteRegister(kPOWER_MANAGEMENT_3);
    }
  }
}

AudioConfiguration WM8960::GetCurrentConfiguration() const {
  // the cached configuration is always valid
  return cached_config_;
}

bool WM8960::SetCurrentConfiguration(const AudioConfiguration& config) {
  if (!Initialize()) {
    return false;
  }

  if (is_playing_) {
    Play(false);
  }
  if (is_recording_) {
    Record(false);
  }

  if (config.play_rate != config.record_rate) {
    return false;
  }
  if (!HasSampleRate(kPlay, config.play_rate)) {
    return false;
  }
  if (!HasChannelConfig(config.channel_config)) {
    return false;
  }
  if (!HasSampleWidth(config.sample_width)) {
    return false;
  }

  SetConfig(config);
  cached_config_.play_rate = config.play_rate;
  cached_config_.record_rate = config.record_rate;
  cached_config_.channel_config = config.channel_config;
  cached_config_.sample_width = config.sample_width;

  return true;
}

void WM8960::Mute(const AudioFunction which, const bool enable) {
  if (!Initialize()) {
    return;
  }
  switch (which) {
    case kPlay:
      if (is_play_muted_ != enable) {
        PlayMute(enable);
      }
      break;
    case kRecord:
      if (is_record_muted_ != enable) {
        RecordMute(enable);
      }
      break;
  }
}

void WM8960::Start(const AudioFunction which) {
  if (!Initialize()) {
    return;
  }
  switch (which) {
    case kPlay:
      if (!is_playing_) {
        Play(true);
      }
      break;
    case kRecord:
      if (!is_recording_) {
        Record(true);
      }
      break;
  }
}

void WM8960::Stop(const AudioFunction which) {
  if (!Initialize()) {
    return;
  }
  switch (which) {
    case kPlay:
      if (is_playing_) {
        Play(false);
      }
      break;
    case kRecord:
      if (is_recording_) {
        Record(false);
      }
      break;
  }
}

void WM8960::SetVolume(const AudioFunction which, const float percent) {
  if (!Initialize()) {
    return;
  }
  switch (which) {
    case kPlay:
      SetPlayVolume(percent / 100.0f);
      break;
    case kRecord:
      SetRecordVolume(percent / 100.0f);
      break;
  }
}

bool WM8960::HasSampleRate(const AudioFunction which,
                           const AudioSampleRate rate) const {
  switch (rate) {
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
      return true;
  }

  return false;
}

bool WM8960::HasChannelConfig(const AudioChannelConfig channel) const {
  switch (channel) {
    case kStereo:
      return true;
    default:
      break;
  }

  return false;
}

bool WM8960::HasFunction(const AudioFunction which) const { return true; }

bool WM8960::HasSampleWidth(const AudioSampleWidth width) const {
  switch (width) {
    case kSize_8bit:
      // fallthrough
    case kSize_16bit:
      return true;
    default:
      break;
  }

  return false;
}

bool WM8960::HasSimultaneousPlayRecord() const { return false; }

bool WM8960::HasVolume(const AudioFunction which) const { return true; }

bool WM8960::HasMute(const AudioFunction which) const { return true; }

}  // namespace peripherals
