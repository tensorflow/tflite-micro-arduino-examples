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

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "tensorflow/lite/micro/debug_log.h"
#include "tensorflow/lite/micro/system_setup.h"

namespace {
peripherals::AudioDevice* audio = nullptr;

constexpr peripherals::AudioSampleRate kSampleRate = peripherals::kRate_16000;
constexpr peripherals::AudioSampleWidth kNumBitsPerChannel =
    peripherals::kSize_16bit;

constexpr size_t kNumSamples = 48000;
constexpr size_t kNumZeroSamples = kSampleRate / 10;  // 100 milliseconds
constexpr size_t kNumChannels = 2;
static_assert(kNumChannels == 2 || kNumChannels == 1);

int16_t record_play_buffer[kNumSamples][kNumChannels];

constexpr size_t kNumBytesPerChannel = kNumBitsPerChannel / 8;
static_assert(kNumBytesPerChannel == sizeof(record_play_buffer[0][0]));

size_t play_sample_index;
size_t record_sample_index;

enum ProgramState { kPlaying, kRecording, kIdle } current_state;

}  // namespace

void setup() {
  // initialize
  tflite::InitializeTarget();

  audio = AUDIO_DEVICE_WS_WM8960_AUDIO_HAT;
  peripherals::AudioConfiguration config;
  config = audio->GetCurrentConfiguration();
  if (kNumChannels == 2) {
    config.channel_config = peripherals::kStereo;
  } else {
    config.channel_config = peripherals::kMono;
  }
  config.play_rate = config.record_rate = kSampleRate;
  config.sample_width = kNumBitsPerChannel;
  if (!audio->SetCurrentConfiguration(config)) {
    while (true) {
      // forever
      DebugLog("Audio configuration failed\n");
      continue;
    }
  }

  audio->SetVolume(peripherals::kPlay, 100.0f);
  audio->SetVolume(peripherals::kRecord, 90.0f);

  current_state = kIdle;

  peripherals::LED::Instance().SetBlinkParams(0.33f, 500);

  DebugLog("Setup complete\n");
}

void loop() {
  peripherals::ButtonPressState button_state =
      peripherals::Button::Instance().GetPressState();

  if (current_state == kRecording) {
    // if recording, check if button released to stop recording
    // if recording, check if buffer full to stop recording
    // otherwise empty the recording buffer
    if (button_state == peripherals::kLongPressUp ||
        record_sample_index == kNumSamples) {
      audio->Stop(peripherals::kRecord);
      current_state = kIdle;
      peripherals::LED::Instance().Show(false);
      DebugLog("Stopped recording\n");
    } else {
      auto num_read =
          audio->ReadRecordBuffer(&record_play_buffer[record_sample_index][0],
                                  kNumSamples - record_sample_index);
      record_sample_index += num_read;
    }
  } else if (current_state == kPlaying) {
    // if playing, check if end of playback to stop playback
    // otherwise feed playback buffer
    if (audio->SampleCount(peripherals::kPlay) >= record_sample_index) {
      audio->Stop(peripherals::kPlay);
      current_state = kIdle;
      peripherals::LED::Instance().Show(false);
      DebugLog("Stopped playing\n");
    } else if (play_sample_index == record_sample_index) {
      // zero fill after end of recorded samples
      size_t num_fill = kNumZeroSamples;
      std::remove_reference<decltype(record_play_buffer[0][0])>::type
          zero[kNumChannels] = {};
      while (num_fill > 0) {
        auto num_written = audio->WritePlayBuffer(zero, 1);
        num_fill -= num_written;
      }
    } else {
      // feed playback buffer
      auto num_written =
          audio->WritePlayBuffer(&record_play_buffer[play_sample_index][0],
                                 record_sample_index - play_sample_index);
      play_sample_index += num_written;
    }
  } else {
    if (button_state == peripherals::kPressed) {
      // if not recording and not playing, check for button short press to
      // start playback
      play_sample_index = 0;
      audio->Start(peripherals::kPlay);
      current_state = kPlaying;
      peripherals::LED::Instance().Show(true);
      DebugLog("Playing...\n");
    } else if (button_state == peripherals::kLongPressDown) {
      // if not recording and not playing, check for button long press to
      // start recording
      record_sample_index = 0;
      audio->Start(peripherals::kRecord);
      current_state = kRecording;
      peripherals::LED::Instance().Show(true);
      DebugLog("Recording...\n");
    }
  }

  // if idle, blink builtin LED
  if (current_state == kIdle) {
    peripherals::LED::Instance().Blink();
  }
}
