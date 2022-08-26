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

#ifndef PERIPHERALS_WM8960_REGS_H_
#define PERIPHERALS_WM8960_REGS_H_

#include <cstdint>

namespace peripherals {
namespace {

enum WM8960_Registers : uint8_t {
  // DAC soft mute
  kADC_DAC_CONTROL_1,
  // DAC soft mute mode
  kADC_DAC_CONTROL_2,
  kADDITIONAL_CONTROL_1,
  kADDITIONAL_CONTROL_2,
  kADDITIONAL_CONTROL_4,
  kADCL_SIGNAL_PATH,
  kADCR_SIGNAL_PATH,
  kLEFT_OUT_MIX,
  kRIGHT_OUT_MIX,

  kPOWER_MANAGEMENT_1,
  kPOWER_MANAGEMENT_2,
  kPOWER_MANAGEMENT_3,
  kANTI_POP_1,

  kAUDIO_INTERFACE_1,
  kAUDIO_INTERFACE_2,

  kRESET,

  // left microphone volume and mute (analog)
  // default 0dB (max +30dB min -17.25dB step 0.75dB)
  kLEFT_INPUT_VOLUME,
  // right microphone volume and mute (analog)
  // default 0dB (max +30dB min -17.25dB step 0.75dB)
  kRIGHT_INPUT_VOLUME,
  // left ADC volume (digital)
  // default 0dB (max +30dB min -97dB step 0.5dB)
  kLEFT_ADC_VOLUME,
  // right ADC volume (digital)
  // default 0dB (max +30dB min -97dB step 0.5dB)
  kRIGHT_ADC_VOLUME,

  // headphone left volume (analog)
  // default mute (max +6dB min -73dB step 1.0dB)
  kLOUT1_VOLUME,
  // headphone right volume (analog)
  // default mute (max +6dB min -73dB step 1.0dB)
  kROUT1_VOLUME,
  // left DAC volume (digital)
  // default 0dB (max 0dB min -127dB step 0.5dB)
  kLEFT_DAC_VOLUME,
  // right DAC volume (digital)
  // default 0dB (max 0dB min -127dB step 0.5dB)
  kRIGHT_DAC_VOLUME,
  // left speaker volume (analog)
  // default mute (max +6dB min -73dB step 1.0dB)
  kLEFT_SPEAKER_VOLUME,
  // right speaker volume (analog)
  // default mute (max +6dB min -73dB step 1.0dB)
  kRIGHT_SPEAKER_VOLUME,
  kCLASS_D_CONTROL_1,
  // max gain with 3.3V and 5V supply is 1.52x
  // ACGAIN must be less than or equal to DCGAIN
  // do not change while outputs enabled
  kCLASS_D_CONTROL_2,

  kCLOCKING_1,
  kCLOCKING_2,
  kPLL_1,
  kPLL_2,
  kPLL_3,
  kPLL_4,
  kREGISTER_LAST_ = kPLL_4
};

struct WM8960_Field {
  const uint16_t mask;
  const uint8_t shift;
  WM8960_Registers reg;
};

constexpr uint16_t kCLASS_D_CONTROL_1_RESERVED = 0x037;
constexpr uint16_t kCLASS_D_CONTROL_2_RESERVED = 0x080;

constexpr int kSYSCLK_12MHz = 12288000;
constexpr int kSYSCLK_11MHz = 11289600;

constexpr WM8960_Field CLKSEL = {0b1, 0, kCLOCKING_1};
constexpr WM8960_Field SYSCLKDIV = {0b11, 1, kCLOCKING_1};
constexpr WM8960_Field DACDIV = {0b111, 3, kCLOCKING_1};
constexpr WM8960_Field ADCDIV = {0b111, 6, kCLOCKING_1};
constexpr uint8_t kCLKSEL_PLL = 0b1;         // SYSCLK from PLL
constexpr uint8_t kSYSCLKDIV_2 = 0b10;       // SYSCLK pre-divider
constexpr uint8_t kDAC_ADC_DIV_1_0 = 0b000;  // SYSCLK / (1.0 * 256)
constexpr uint8_t kDAC_ADC_DIV_1_5 = 0b001;  // SYSCLK / (1.5 * 256)
constexpr uint8_t kDAC_ADC_DIV_2_0 = 0b010;  // SYSCLK / (2.0 * 256)
constexpr uint8_t kDAC_ADC_DIV_3_0 = 0b011;  // SYSCLK / (3.0 * 256)
constexpr uint8_t kDAC_ADC_DIV_4_0 = 0b100;  // SYSCLK / (4.0 * 256)
constexpr uint8_t kDAC_ADC_DIV_6_0 = 0b110;  // SYSCLK / (6.0 * 256)

constexpr WM8960_Field BCLKDIV = {0b1111, 0, kCLOCKING_2};
constexpr WM8960_Field DCLKDIV = {0b111, 6, kCLOCKING_2};
constexpr uint8_t kBCLKDIV_1 = 0b0000;   // BCLK = SYSCLK / 1
constexpr uint8_t kBCLKDIV_2 = 0b0010;   // BCLK = SYSCLK / 2
constexpr uint8_t kBCLKDIV_3 = 0b0011;   // BCLK = SYSCLK / 3
constexpr uint8_t kBCLKDIV_4 = 0b0100;   // BCLK = SYSCLK / 4
constexpr uint8_t kBCLKDIV_6 = 0b0110;   // BCLK = SYSCLK / 6
constexpr uint8_t kBCLKDIV_8 = 0b0111;   // BCLK = SYSCLK / 8
constexpr uint8_t kBCLKDIV_11 = 0b1000;  // BCLK = SYSCLK / 11
constexpr uint8_t kBCLKDIV_12 = 0b1001;  // BCLK = SYSCLK / 12
constexpr uint8_t kBCLKDIV_16 = 0b1010;  // BCLK = SYSCLK / 16
constexpr uint8_t kBCLKDIV_22 = 0b1011;  // BCLK = SYSCLK / 22
constexpr uint8_t kBCLKDIV_24 = 0b1100;  // BCLK = SYSCLK / 24
constexpr uint8_t kBCLKDIV_32 = 0b1101;  // BCLK = SYSCLK / 32
constexpr struct {
  int div;
  uint8_t value;
} kBCLKDIV_Map[] = {{32, kBCLKDIV_32}, {24, kBCLKDIV_24}, {22, kBCLKDIV_22},
                    {16, kBCLKDIV_16}, {12, kBCLKDIV_12}, {11, kBCLKDIV_11},
                    {8, kBCLKDIV_8},   {6, kBCLKDIV_6},   {4, kBCLKDIV_4},
                    {3, kBCLKDIV_3},   {2, kBCLKDIV_2},   {1, kBCLKDIV_1}};
constexpr uint8_t kDCLKDIV_16 = 0b111;  // CLASS D CLK = SYSCLK / 16

constexpr WM8960_Field PLLN = {0b1111, 0, kPLL_1};
constexpr WM8960_Field PLLPRESCALE = {0b1, 4, kPLL_1};
constexpr WM8960_Field SDM = {0b1, 5, kPLL_1};
constexpr uint8_t kPLLN_11_2896_MHZ = 7;  // PLL N for 11.2896MHz SYSCLK
constexpr uint8_t kPLLN_12_288_MHZ = 8;   // PLL N for 12.288MHz SYSCLK
constexpr uint8_t kPLLPRESCALE_2 = 0b1;   // MCLK / 2
constexpr uint8_t kSDM_INTEGER_MODE = 0b0;
constexpr uint8_t kSDM_FRACTIONAL_MODE = 0b1;

constexpr WM8960_Field PLLK_23_16 = {0x0FF, 0, kPLL_2};
constexpr WM8960_Field PLLK_15_8 = {0x0FF, 0, kPLL_3};
constexpr WM8960_Field PLLK_7_0 = {0x0FF, 0, kPLL_4};
constexpr uint32_t kPLLK_11_2896_MHZ = 0x86C226;  // PLL K for 11.2896MHz SYSCLK
constexpr uint32_t kPLLK_12_288_MHZ = 0x3126E9;   // PLL K for 12.288MHz SYSCLK

constexpr WM8960_Field IPVU = {0b1, 8, kRIGHT_INPUT_VOLUME};
constexpr WM8960_Field LINMUTE = {0b1, 7, kLEFT_INPUT_VOLUME};
constexpr WM8960_Field LIZC = {0b1, 6, kLEFT_INPUT_VOLUME};
constexpr WM8960_Field LINVOL = {0x3F, 0, kLEFT_INPUT_VOLUME};
constexpr WM8960_Field RINMUTE = {0b1, 7, kRIGHT_INPUT_VOLUME};
constexpr WM8960_Field RIZC = {0b1, 6, kRIGHT_INPUT_VOLUME};
constexpr WM8960_Field RINVOL = {0x3F, 0, kRIGHT_INPUT_VOLUME};
constexpr uint8_t kIPVU_UPDATE = 0b1;
constexpr uint8_t kL_R_INMUTE_ENABLE = 0b1;
constexpr uint8_t kL_R_IZC_ENABLE = 0b1;
constexpr uint8_t kL_R_INVOL_MAX = 0x3F;  // +30dB
constexpr uint8_t kL_R_INVOL_MIN = 0x00;  // -17.25dB

constexpr WM8960_Field OUT1VU = {0b1, 8, kROUT1_VOLUME};
constexpr WM8960_Field LO1ZC = {0b1, 7, kLOUT1_VOLUME};
constexpr WM8960_Field LOUT1VOL = {0x7F, 0, kLOUT1_VOLUME};
constexpr WM8960_Field RO1ZC = {0b1, 7, kROUT1_VOLUME};
constexpr WM8960_Field ROUT1VOL = {0x7F, 0, kROUT1_VOLUME};
constexpr uint8_t kOUT1VU_UPDATE = 0b1;
constexpr uint8_t kL_R_O1ZC_ENABLE = 0b1;
constexpr uint8_t kL_R_OUT1VOL_MAX = 0x7F;  // +6dB
constexpr uint8_t kL_R_OUT1VOL_MIN = 0x30;  // -73dB

constexpr WM8960_Field DACMU = {0b1, 3, kADC_DAC_CONTROL_1};
constexpr uint8_t kDACMU_MUTE = 0b1;

constexpr WM8960_Field DACSMM = {0b1, 3, kADC_DAC_CONTROL_2};
constexpr WM8960_Field DACMR = {0b1, 2, kADC_DAC_CONTROL_2};
constexpr uint8_t kDACSMM_RAMP = 0b1;
constexpr uint8_t kDACMR_FAST = 0b0;
constexpr uint8_t kDACMR_SLOW = 0b1;

constexpr WM8960_Field MS = {0b1, 6, kAUDIO_INTERFACE_1};
constexpr WM8960_Field WL = {0b11, 2, kAUDIO_INTERFACE_1};
constexpr WM8960_Field FORMAT = {0b11, 0, kAUDIO_INTERFACE_1};
constexpr uint8_t kMS_MASTER = 0b1;  // I2S master mode
constexpr uint8_t kWL_16 = 0b00;
constexpr uint8_t kWL_20 = 0b01;
constexpr uint8_t kWL_24 = 0b10;
constexpr uint8_t kWL_32 = 0b11;
constexpr uint8_t kFORMAT_LEFT = 0b01;
constexpr uint8_t kFORMAT_I2S = 0b10;

constexpr WM8960_Field WL8 = {0b1, 5, kAUDIO_INTERFACE_2};
constexpr uint8_t kWL8_ENABLE = 0b1;

constexpr WM8960_Field TSDEN = {0b1, 8, kADDITIONAL_CONTROL_1};
constexpr WM8960_Field VSEL = {0b11, 6, kADDITIONAL_CONTROL_1};
constexpr WM8960_Field DMONOMIX = {0b1, 4, kADDITIONAL_CONTROL_1};
constexpr WM8960_Field TOCLKSEL = {0b1, 1, kADDITIONAL_CONTROL_1};
constexpr WM8960_Field TOEN = {0b1, 0, kADDITIONAL_CONTROL_1};
constexpr WM8960_Field LRCM = {0b1, 2, kADDITIONAL_CONTROL_2};
constexpr WM8960_Field TSENSEN = {0b1, 1, kADDITIONAL_CONTROL_4};
constexpr uint8_t kTSDEN_ENABLE = 0b1;     // thermal shutdown enable
constexpr uint8_t kVSEL_3_3V = 0b11;       // optimize AVDD for 3.3V
constexpr uint8_t kDMONOMIX_STEREO = 0b0;  // DAC stero
constexpr uint8_t kDMONOMIX_MONO = 0b1;    // DAC mono mix and -6dB
constexpr uint8_t kTOCLKSEL_SLOW = 0b0;    // volume zero-cross slow timeout
constexpr uint8_t kTOCLKSEL_FAST = 0b1;    // volume zero-cross fast timeout
constexpr uint8_t kTOEN_ENABLE = 0b1;      // volume zero-cross timeout enable
constexpr uint8_t kLRCM_BOTH = 0b1;        // DAC and ADC LRCLK disable pairing
constexpr uint8_t kTSENSEN_ENABLE = 0b1;   // temperature sensor enable

constexpr WM8960_Field VMIDSEL = {0b11, 7, kPOWER_MANAGEMENT_1};
constexpr WM8960_Field VREF = {0b1, 6, kPOWER_MANAGEMENT_1};
constexpr WM8960_Field AINL = {0b1, 5, kPOWER_MANAGEMENT_1};
constexpr WM8960_Field AINR = {0b1, 4, kPOWER_MANAGEMENT_1};
constexpr WM8960_Field ADCL = {0b1, 3, kPOWER_MANAGEMENT_1};
constexpr WM8960_Field ADCR = {0b1, 2, kPOWER_MANAGEMENT_1};
constexpr WM8960_Field MICB = {0b1, 1, kPOWER_MANAGEMENT_1};
constexpr WM8960_Field DIGENB = {0b1, 0, kPOWER_MANAGEMENT_1};
constexpr WM8960_Field DACL = {0b1, 8, kPOWER_MANAGEMENT_2};
constexpr WM8960_Field DACR = {0b1, 7, kPOWER_MANAGEMENT_2};
constexpr WM8960_Field LOUT1 = {0b1, 6, kPOWER_MANAGEMENT_2};
constexpr WM8960_Field ROUT1 = {0b1, 5, kPOWER_MANAGEMENT_2};
constexpr WM8960_Field SPKL = {0b1, 4, kPOWER_MANAGEMENT_2};
constexpr WM8960_Field SPKR = {0b1, 3, kPOWER_MANAGEMENT_2};
constexpr WM8960_Field PLL_EN = {0b1, 0, kPOWER_MANAGEMENT_2};
constexpr WM8960_Field LMIC = {0b1, 5, kPOWER_MANAGEMENT_3};
constexpr WM8960_Field RMIC = {0b1, 4, kPOWER_MANAGEMENT_3};
constexpr WM8960_Field LOMIX = {0b1, 3, kPOWER_MANAGEMENT_3};
constexpr WM8960_Field ROMIX = {0b1, 2, kPOWER_MANAGEMENT_3};
constexpr WM8960_Field HPSTBY = {0b1, 0, kANTI_POP_1};
constexpr WM8960_Field SOFT_ST = {0b1, 2, kANTI_POP_1};
constexpr uint8_t kVMIDSEL_DISABLE = 0b00;
constexpr uint8_t kVMIDSEL_ENABLE = 0b01;
constexpr uint8_t kVMIDSEL_STANDBY = 0b10;
constexpr uint8_t kVMIDSEL_FAST_START = 0b11;
constexpr uint8_t kDIGENB_DISABLE = 0b01;  // MCLK disable
constexpr uint8_t kPOWER_MANAGEMENT_POWER_ON = 0b1;
constexpr uint8_t kHPSTBY_NORMAL = 0b0;   // headphone amp normal mode
constexpr uint8_t kHPSTBY_STANDBY = 0b1;  // headphone amp standby mode
constexpr uint8_t kSOFT_ST_ENABLE = 0b1;  // VMID soft start enable

constexpr WM8960_Field LMN1 = {0b1, 8, kADCL_SIGNAL_PATH};
constexpr WM8960_Field LMIC2B = {0b1, 3, kADCL_SIGNAL_PATH};
constexpr WM8960_Field RMN1 = {0b1, 8, kADCR_SIGNAL_PATH};
constexpr WM8960_Field RMIC2B = {0b1, 3, kADCR_SIGNAL_PATH};
constexpr uint8_t kSIGNAL_PATH_CONNECT = 0b1;

constexpr WM8960_Field LD2LO = {0b1, 8, kLEFT_OUT_MIX};
constexpr WM8960_Field RD2RO = {0b1, 8, kRIGHT_OUT_MIX};
constexpr uint8_t kOUT_MIX_ENABLE = 0b1;

constexpr WM8960_Field SPKVU = {0b1, 8, kRIGHT_SPEAKER_VOLUME};
constexpr WM8960_Field SPKLZC = {0b1, 7, kLEFT_SPEAKER_VOLUME};
constexpr WM8960_Field SPKLVOL = {0x7F, 0, kLEFT_SPEAKER_VOLUME};
constexpr WM8960_Field SPKRZC = {0b1, 7, kRIGHT_SPEAKER_VOLUME};
constexpr WM8960_Field SPKRVOL = {0x7F, 0, kRIGHT_SPEAKER_VOLUME};
constexpr uint8_t kSPKVU_UPDATE = 0b1;
constexpr uint8_t kSPK_L_R_ZC_ENABLE = 0b1;
constexpr uint8_t kSPK_L_R_VOL_MAX = 0x7F;  // +6dB
constexpr uint8_t kSPK_L_R_VOL_MIN = 0x30;  // -73dB

constexpr WM8960_Field SPK_OP_EN = {0b11, 6, kCLASS_D_CONTROL_1};
constexpr uint8_t kSPK_OP_EN_OFF = 0b00;
constexpr uint8_t kSPK_OP_EN_LEFT = 0b01;
constexpr uint8_t kSPK_OP_EN_RIGHT = 0b10;
constexpr uint8_t kSPK_OP_EN_BOTH = 0b11;

constexpr WM8960_Field AC_GAIN = {0b111, 0, kCLASS_D_CONTROL_2};
constexpr WM8960_Field DC_GAIN = {0b111, 3, kCLASS_D_CONTROL_2};
constexpr uint8_t kAC_DC_GAIN_0_00 = 0b000;  // 0.00x = +0dB
constexpr uint8_t kAC_DC_GAIN_1_27 = 0b001;  // 1.27x = +2.1dB
constexpr uint8_t kAC_DC_GAIN_1_40 = 0b010;  // 1.40x = +2.9dB

constexpr WM8960_Field ADCVU = {0b1, 8, kRIGHT_ADC_VOLUME};
constexpr WM8960_Field LADCVOL = {0xFF, 0, kLEFT_ADC_VOLUME};
constexpr WM8960_Field RADCVOL = {0xFF, 0, kRIGHT_ADC_VOLUME};
constexpr uint8_t kADCVU_UPDATE = 0b1;
constexpr uint8_t kL_R_ADCVOL_MAX = 0xFF;  // +30dB
constexpr uint8_t kL_R_ADCVOL_MIN = 0x01;  // -97dB

constexpr WM8960_Field DACVU = {0b1, 8, kRIGHT_DAC_VOLUME};
constexpr WM8960_Field LDACVOL = {0xFF, 0, kLEFT_DAC_VOLUME};
constexpr WM8960_Field RDACVOL = {0xFF, 0, kRIGHT_DAC_VOLUME};
constexpr uint8_t kDACVU_UPDATE = 0b1;
constexpr uint8_t kL_R_DACVOL_MAX = 0xFF;  // +0dB
constexpr uint8_t kL_R_DACVOL_MIN = 0x01;  // -127dB

}  // namespace

}  // namespace peripherals

#endif  // PERIPHERALS_WM8960_REGS_H_
