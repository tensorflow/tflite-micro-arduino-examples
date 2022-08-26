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

#include <functional>

#include "peripherals/i2c_arduino.h"
#include "peripherals/peripherals.h"
#include "peripherals/wm8960.h"

namespace {
peripherals::I2C& i2c = peripherals::I2C_Arduino::Instance0();
peripherals::IAudioCodec& codec = peripherals::WM8960::Instance(i2c);

bool Mute(peripherals::AudioFunction which, bool should_mute) {
  codec.Mute(which, should_mute);
  return true;
}

bool Start(peripherals::AudioFunction which) {
  codec.Start(which);
  return true;
}

bool Stop(peripherals::AudioFunction which) {
  codec.Stop(which);
  return true;
}

bool SetSampleRate(peripherals::AudioSampleRate rate) {
  peripherals::AudioConfiguration config;
  config = codec.GetCurrentConfiguration();
  config.play_rate = config.record_rate = rate;
  bool result = codec.SetCurrentConfiguration(config);
  return result;
}

bool SetVolume(peripherals::AudioFunction which, float percent) {
  codec.SetVolume(which, percent);
  return true;
}

struct {
  std::function<bool(void)> func;
  const char* test;
} tests[] = {
    {std::bind(SetSampleRate, peripherals::kRate_16000), "set sample rate 16k"},
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
    {nullptr, nullptr}};

int test_index = 0;
bool waiting_input = false;

}  // namespace

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  while (!Serial) {
    // wait
  }

  Serial.println("Start of Tests");
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  peripherals::DelayMilliseconds(500);
  digitalWrite(LED_BUILTIN, LOW);
  peripherals::DelayMilliseconds(500);

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
