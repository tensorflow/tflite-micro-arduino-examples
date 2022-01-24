# Test-Over-Serial support module

This support module allows for testing of
[Tensorflow Lite Micro](https://www.tensorflow.org/lite/microcontrollers) (TFLM)
Arduino [examples](/examples) without the use of board specific peripherals (cameras, microphones, etc).
All data necessary to conduct an inference operation by the example application is supplied over the Arduino serial interface.

Currently supported TFLM example applications are:
- person_detection
- micro_speech

## Table of contents
<!--ts-->
   * [Overview](#overview)
   * [Required serial interface support](#required-serial-interface-support)
   * [Commands](#commands)
   * [C++ API](#c-api)
      * [Input handler callback](#input-handler-callback)
   * [Testing with test_over_serial.py script](#testing-with-test_over_serialpy-script)
      * [Linux and ModemManager](#linux-and-modemmanager)
      * [Example test configuration data](#example-test-configuration-data)
<!--te-->

## Overview

The Arduino TFLM example application executes the normal inference operation using data from the board specific peripherals.  When commanded over the serial interface to enter test mode, normal inference operations are halted.  The example application remains in test mode awaiting data to arrive over the serial interface.  When sufficient data is available, an inference operation is run.  The example application then remains in test mode awaiting the next set of data from the serial interface.

This module does not interfere with the example application sending output to the serial interface.  Thus the application can continue to have debug or other output.  This module will consume all serial input once test mode is active.

## Required serial interface support

For the Test-Over-Serial module to work, serial interface access is required using a light-weight API.  The header file [system_setup.h](/src/tensorflow/lite/micro/system_setup.h) declares the API, and [system_setup.cpp](/src/tensorflow/lite/micro/system_setup.cpp) provides the board specific implementation of the API.

Each line (command or data or response) sent over the serial interface must be newline (`'\n'`) terminated.

No 3rd party Arduino libraries are required to use the Test-Over-Serial support module.

## Commands

The Test-Over-Serial support module recognizes to the following commands received over the serial interface:
- **!TEST** Notification that the test mode has started.  The application should cease all inference operations and await test data.  The response to this command is **!OK TEST _max-binary-data-per-line_**.
    - The `max-binary-data-per-line` parameter is the maximum number of bytes of binary data to base-64 encode per line sent over the serial interface
- **!DATA _data-type_ &nbsp;_binary-data-size_** Notification of the start of the data transfer.  When all data is received, the **!OK DATA _data-type_ &nbsp;_binary-data-size_** response is sent.  After each line of data received, the **!DATA_ACK _bytes-received_** response is sent.  Binary data must be base-64 encoded before being sent over the serial interface.
    - The `binary-data-size` parameter is always in units of bytes
    - The `data-type` parameter specifies the type of data (audio, image, etc.)
    - The `bytes-received` parameter is the number of bytes of base-64 decoded binary data received

Any failure condition detected by the module is responded to by sending **!FAIL _command-that-failed_**.

## C++ API

An example application can make use of the Test-Over-Serial module through the [C++ API](/src/test_over_serial/test_over_serial.h):
- `TestOverSerial& Instance(const TestDataType data_type)` Returns an instance of the `TestOverSerial` class.  This class is a singleton.
    - `data_type` is the type of data the application wants to receive (audio, image, etc.)
- `bool IsTestMode(void)` Returns `true` if the `!TEST` command has been received.
- `void ProcessInput(const InputHandler*)` The serial interface input processor.  All commands and data received over the serial interface are responded to by this routine.  When receiving data, for each line received, the `InputHandler` callback is executed.

### Input handler callback

The `InputHandler` callback is used for application specific handling of received data.  A single parameter is passed to the callback, a pointer to the following data structure:
```
struct InputBuffer {
  DataPtr data;   // input buffer pointer
  size_t length;  // input buffer length
  size_t offset;  // offset from start of input
  size_t total;   // total data that will be transferred
};
```
All members of this data structure will have values in units of the `data-type`.  For example, 16-bit mono audio data will consist of 2 bytes per audio `sample`.  The units of all values in the data structure would therefore be a count of `sample(s)`, not bytes.

The input buffer data will have been base-64 decoded prior to the `InputHandler` callback being executed.

The `InputHandler` callback must return `true` to allow data transfer to continue.  Returning `false` will abort the data transfer and a `!FAIL DATA...` response will be sent to the serial interface.

## Testing with test_over_serial.py script

The [test_over_serial.py](/scripts/test_over_serial.py) script is provided for automated testing of the supported Arduino example applications.

Only the `--example` option is required, which specifies which example application to test.  The `--verbose` option controls the amount of output seen.  The `--verbose` values can be:
- `all` See all output including any output sent by the Arduino application
- `test` Restrict output to just the test results

Example command line (must be run from the top of the tflite-micro-arduino-examples repository directory):
```
tflite-micro-arduino-examples$ python3 scripts/test_over_serial.py --example person_detection --verbose test
10:21:03.746946  Test start:
10:21:04.016295  Test #1 for label <person> file <person.bmp>
10:21:05.280497  Test #1 FAILED
10:21:05.280648  Waiting 4.0s before next test
10:21:09.288489  Test #2 for label <no person> file <no_person.bmp>
10:21:10.550045  Test #2 PASSED
10:21:10.550116  Waiting 4.0s before next test
10:21:14.556783  Test #3 for label <person> file <person.bmp>
10:21:16.328190  Test #3 PASSED
10:21:16.328250  Waiting 4.0s before next test
10:21:20.334084  Test #4 for label <no person> file <no_person.bmp>
10:21:22.115927  Test #4 PASSED
10:21:22.222481  Test end: total 4 completed 4 passed 3 failed 1
```
### Linux and `ModemManager`

If `!TEST` command timeouts are seen when running the test script, it may be that `ModemManager` is interfering with the serial interface.  `ModemManager` will need to be disabled or removed.

To disable (Ubuntu Linux):
```
systemctl disable ModemManager.service
systemctl stop ModemManager.service
```

To remove (Ubuntu Linux):
```
sudo apt-get purge modemmanager
```

### Example test configuration data

The test script relies on configuration data in a JSON compatible form. An example configuration:
```
{
    "person_detection":
    {
        "data type":
            "image-grayscale",
        "delay after":
            4.0,
        "test data":
        [
            {
                "file name": "examples/person_detection/data/person.bmp",
                "label": "person",
                "regex": r"Person score: (\d+\.\d+)% "
                            r"No person score: (\d+\.\d+)%",
                "expr": "groups[1] > groups[2]",
                "qqvga size": False,
            },

        ]
    }
}
```

The configuration fields are as follows:
- `data type` One of the following values:
    - `image-grayscale`
    - `audio-pcm-16khz-mono-s16`
    - `raw-int8`
    - `raw-float`
- `delay after` Float value setting the number of seconds to delay after each test
- `test data` Array of `test-data-element(s)`

The `test-data-element` fields are as follows:
- `file name` Name of the data file to be sent to the serial interface.  This file should reside within the `data` sub-directory of the example source code.
- `label` The prediction label the model inference should produce
- `regex` A [regular-expression](https://docs.python.org/3/library/re.html) that captures portions of the Arduino application output
- `expr` A Python expression that evaluates to a boolean.  The expression should return `True` if all test conditions have been met.  The expression has two variables available:
    - `groups` A list of strings and/or floats matching the `regex` capture(s).
    - `label` A string containing the prediction label
- `qqvga size` (Image element only) If `true` then resize the image data to QQVGA (160x120) dimensions.  Otherwise, the image data is resized to fit the model tensor dimensions.

All fields in the configuration data are **required**.
