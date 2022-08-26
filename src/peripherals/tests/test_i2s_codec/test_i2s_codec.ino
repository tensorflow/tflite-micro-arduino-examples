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

#include <TensorFlowLite.h>

#include <cmath>
#include <functional>
#include <limits>

#include "peripherals/i2c_arduino.h"
#include "peripherals/i2s_nrf52840.h"
#include "peripherals/utility.h"
#include "peripherals/wm8960.h"

namespace {
peripherals::I2C& i2c = peripherals::I2C_Arduino::Instance0();
peripherals::IAudioCodec& codec = peripherals::WM8960::Instance(i2c);
peripherals::IAudioI2S& i2s = peripherals::I2S_nrf52840::Instance();

bool is_playing_sin_wave = false;
uint32_t play_sin_wave_index = 0;

bool Mute(peripherals::AudioFunction which, bool should_mute) {
  codec.Mute(which, should_mute);
  return true;
}

bool Start(peripherals::AudioFunction which) {
  is_playing_sin_wave = false;
  codec.Start(which);
  i2s.Start(which);
  return true;
}

bool Stop(peripherals::AudioFunction which) {
  i2s.Stop(which);
  codec.Stop(which);
  return true;
}

bool SetSampleWidth(peripherals::AudioSampleWidth width) {
  bool result = false;
  peripherals::AudioConfiguration config;
  config = i2s.GetCurrentConfiguration();
  config.sample_width = width;
  result = i2s.SetCurrentConfiguration(config);
  if (!result) {
    return false;
  }

  result = codec.SetCurrentConfiguration(config);
  return result;
}

bool SetSampleRate(peripherals::AudioSampleRate rate) {
  bool result = false;
  peripherals::AudioConfiguration config;
  config = i2s.GetCurrentConfiguration();
  config.play_rate = config.record_rate = rate;
  result = i2s.SetCurrentConfiguration(config);
  if (!result) {
    return false;
  }

  result = codec.SetCurrentConfiguration(config);
  return result;
}

bool SetVolume(peripherals::AudioFunction which, float percent) {
  codec.SetVolume(which, percent);
  return true;
}

bool PlaySawWave(bool start) {
  if (start) {
    play_sin_wave_index = 0;
    digitalWrite(LEDB, HIGH);
  }

  auto start_time = micros();

  constexpr size_t kNumSamples_16 = 256;
  constexpr size_t kNumSamples_8 = 512;
  constexpr float freq = 1000.0f;
  const int rate = i2s.GetCurrentConfiguration().play_rate;
  const peripherals::AudioSampleWidth width =
      i2s.GetCurrentConfiguration().sample_width;
  const float period = (1.0f / freq) * rate;
  static int16_t sample_buffer_16[kNumSamples_16][2];
  static int8_t sample_buffer_8[kNumSamples_8][2];
  const size_t kNumSamples =
      width == peripherals::kSize_16bit ? kNumSamples_16 : kNumSamples_8;

  for (size_t sample = 0; sample < kNumSamples; sample++) {
    float x = (sample + play_sin_wave_index) / period;
    float v = x - int(x);
    v -= 0.5;

    if (width == peripherals::kSize_16bit) {
      v *= std::numeric_limits<int16_t>::max();
      sample_buffer_16[sample][0] = v;       // left channel
      sample_buffer_16[sample][1] = 0x00F1;  // right channel
    } else {
      v *= std::numeric_limits<int8_t>::max();
      sample_buffer_8[sample][0] = v;     // left channel
      sample_buffer_8[sample][1] = 0x0E;  // right channel
    }
  }

  if (start) {
    auto end_time = micros();
    Serial.print(" (");
    Serial.print(end_time - start_time);
    Serial.print("us) ");
  }

  size_t write_count;
  if (width == peripherals::kSize_16bit) {
    write_count = i2s.WritePlayBuffer(sample_buffer_16, kNumSamples);
  } else {
    write_count = i2s.WritePlayBuffer(sample_buffer_8, kNumSamples);
  }

  if (write_count != kNumSamples) {
    digitalWrite(LEDB, LOW);
  }
  play_sin_wave_index += write_count;
  is_playing_sin_wave = true;

  return true;
}

void Handler(peripherals::AudioFunction which) {
  if (which != peripherals::kPlay) {
    return;
  }
  if (is_playing_sin_wave) {
    PlaySawWave(false);
  }
}

struct {
  std::function<bool(void)> func;
  const char* test;
} tests[] = {
    {std::bind(SetVolume, peripherals::kPlay, 75), "play volume 75"},

    {std::bind(SetSampleWidth, peripherals::kSize_8bit), "set sample width 8"},

    {std::bind(SetSampleRate, peripherals::kRate_8000), "set sample rate 8k"},
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(PlaySawWave, true), "play saw wave"},
    {std::bind(Mute, peripherals::kPlay, true), "mute play"},
    {std::bind(Mute, peripherals::kPlay, false), "unmute play"},

    {std::bind(SetSampleRate, peripherals::kRate_11025),
     "set sample rate 11.025k"},
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(PlaySawWave, true), "play saw wave"},
    {std::bind(SetSampleRate, peripherals::kRate_12000), "set sample rate 12k"},
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(PlaySawWave, true), "play saw wave"},
    {std::bind(SetSampleRate, peripherals::kRate_16000), "set sample rate 16k"},
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(PlaySawWave, true), "play saw wave"},
    {std::bind(SetSampleRate, peripherals::kRate_22050),
     "set sample rate 22.050k"},
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(PlaySawWave, true), "play saw wave"},
    {std::bind(SetSampleRate, peripherals::kRate_24000), "set sample rate 24k"},
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(PlaySawWave, true), "play saw wave"},
    {std::bind(SetSampleRate, peripherals::kRate_32000), "set sample rate 32k"},
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(PlaySawWave, true), "play saw wave"},
    {std::bind(SetSampleRate, peripherals::kRate_44100),
     "set sample rate 44.1k"},
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(PlaySawWave, true), "play saw wave"},
    {std::bind(SetSampleRate, peripherals::kRate_48000), "set sample rate 48k"},
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(PlaySawWave, true), "play saw wave"},

    {std::bind(SetSampleWidth, peripherals::kSize_16bit),
     "set sample width 16"},

    {std::bind(SetSampleRate, peripherals::kRate_8000), "set sample rate 8k"},
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(PlaySawWave, true), "play saw wave"},
    {std::bind(SetSampleRate, peripherals::kRate_11025),
     "set sample rate 11.025k"},
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(PlaySawWave, true), "play saw wave"},
    {std::bind(SetSampleRate, peripherals::kRate_12000), "set sample rate 12k"},
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(PlaySawWave, true), "play saw wave"},
    {std::bind(SetSampleRate, peripherals::kRate_16000), "set sample rate 16k"},
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(PlaySawWave, true), "play saw wave"},
    {std::bind(SetSampleRate, peripherals::kRate_22050),
     "set sample rate 22.050k"},
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(PlaySawWave, true), "play saw wave"},
    {std::bind(SetSampleRate, peripherals::kRate_24000), "set sample rate 24k"},
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(PlaySawWave, true), "play saw wave"},
    {std::bind(SetSampleRate, peripherals::kRate_32000), "set sample rate 32k"},
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(PlaySawWave, true), "play saw wave"},
    {std::bind(SetSampleRate, peripherals::kRate_44100),
     "set sample rate 44.1k"},
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(PlaySawWave, true), "play saw wave"},
    {std::bind(SetSampleRate, peripherals::kRate_48000), "set sample rate 48k"},
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(PlaySawWave, true), "play saw wave"},

    {std::bind(Stop, peripherals::kPlay), "stop play"},

#ifdef notdef
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(PlaySawWave), "play saw wave"},
    {std::bind(SetVolume, peripherals::kPlay, 95), "play volume 95"},
    {std::bind(Stop, peripherals::kPlay), "stop play"},
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(PlaySawWave), "play saw wave"},
    {std::bind(SetVolume, peripherals::kPlay, 95), "play volume 95"},
    {std::bind(Stop, peripherals::kPlay), "stop play"},
#endif

#ifdef notdef
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(Start, peripherals::kRecord), "start record"},
    {std::bind(CaptureRecording), "capture recording"},
    {std::bind(Stop, peripherals::kRecord), "stop record"},
    {std::bind(Stop, peripherals::kPlay), "stop play"},

    {std::bind(Start, peripherals::kRecord), "start record"},
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(PlaySawWave), "play saw wave"},
    {std::bind(Stop, peripherals::kPlay), "stop play"},
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(Stop, peripherals::kPlay), "stop play"},
    {std::bind(Stop, peripherals::kRecord), "stop record"},
#endif

#ifdef notdef
    {std::bind(Mute, peripherals::kPlay, true), "mute play"},
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(Stop, peripherals::kPlay), "stop play"},
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(Mute, peripherals::kPlay, false), "unmute play"},
    {std::bind(SetVolume, peripherals::kPlay, 0), "play volume 0"},
    {std::bind(SetVolume, peripherals::kPlay, 50), "play volume 50"},
    {std::bind(Stop, peripherals::kPlay), "stop play"},
    {std::bind(Start, peripherals::kPlay), "start play"},
    {std::bind(Start, peripherals::kRecord), "start record"},
    {std::bind(SetVolume, peripherals::kRecord, 0), "record volume 0"},
    {std::bind(SetVolume, peripherals::kRecord, 50), "record volume 50"},
    {std::bind(Mute, peripherals::kRecord, true), "mute record"},
    {std::bind(Mute, peripherals::kRecord, false), "unmute record"},
    {std::bind(Stop, peripherals::kRecord), "stop record"},
    {std::bind(Stop, peripherals::kPlay), "stop play"},
#endif  // notdef

    {nullptr, nullptr},
};

int test_index = 0;
bool waiting_input = false;

}  // namespace

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LEDB, OUTPUT);
  digitalWrite(LEDB, HIGH);
  Serial.begin(9600);
  while (!Serial) {
    // wait
  }

  i2s.SetCallbackHandler(Handler);

  Serial.println("Start of Tests");
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  peripherals::DelayMilliseconds(250);
  peripherals::TimestampBuffer::Instance().Show();
  digitalWrite(LED_BUILTIN, LOW);
  peripherals::DelayMilliseconds(250);
  peripherals::TimestampBuffer::Instance().Show();

  if (i2s.HadPlayUnderrun()) {
    Serial.print(" (UNDERRUN) ");
  }

  if (test_index < 0) {
    return;
  } else if (tests[test_index].func == nullptr) {
    test_index = -1;
    Serial.println("End of Tests");
    return;
  }

  if (waiting_input) {
    while (Serial.available()) {
      int c = Serial.read();
      if (c == '\n') {
        auto start_time = micros();
        bool result = tests[test_index++].func();
        auto end_time = micros();
        if (result) {
          Serial.print("done");
        } else {
          Serial.print("FAIL");
        }
        Serial.print(" (");
        Serial.print(end_time - start_time);
        Serial.println("us)");
        waiting_input = false;
        break;
      }
    }
  } else {
    Serial.print(tests[test_index].test);
    Serial.print("...");
    waiting_input = true;
  }
}
