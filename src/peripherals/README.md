# TensorFlow Lite Micro Peripherals

This directory provides drivers with generic interfaces to peripherals used
with the TFLM examples.

Currently supported peripherals:
* Waveshare WM8960 Audio HAT
* Button on Waveshare WM8960 Audio HAT
* Default LED on Nano 33 BLE Sense

## Table of contents
<!--ts-->
* [TensorFlow Lite Micro Peripherals](#tensorflow-lite-micro-peripherals)
   * [Waveshare WM8960 Audio HAT](#waveshare-wm8960-audio-hat)
      * [Supported Capabilities](#supported-capabilities)
      * [Audio Board Pin Connections](#audio-board-pin-connections)
      * [Use with Tiny Machine Learning Kit (TMLK)](#use-with-tiny-machine-learning-kit-tmlk)
         * [Parts List and Tools Required](#parts-list-and-tools-required)
         * [Assembly](#assembly)
         * [Notes](#notes)
      * [Testing](#testing)
   * [Button](#button)
   * [LEDs](#leds)
<!--te-->

## Waveshare WM8960 Audio HAT

The [Waveshare WM8960 Audio HAT](https://www.waveshare.com/wm8960-audio-hat.htm) 
utilizes the WM8960 codec.
This audio board provides a stereo 3.5mm earphone jack and 1W speaker outputs
as well as stereo microphone inputs.

The earphone output is best used with 24 ohm (or higher) impedance headphones.

The USB power source must be capable of supplying a minimum of 500 milli-amps
in order to use the audio board with speakers attached.

### Supported Capabilities

This audio board has the following supported capabilities:
* stereo input
* stereo output
* earphone output
* 1W (max) speaker output
* 8 or 16 bits per channel
* standard samples rates from 8kHz to 48kHz
* simultaneous record and playback

### Audio Board Pin Connections

Even numbered pins are on the connector outside edge.
Odd numbered pins are on the connector inside edge.
With back facing up and text upright, pin 1 is on the left inside edge.

Male connectors are used for signals, female for power:

| | | | | | | | | | | | | | | | | | | | | |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
|**Pin**| 1 | 3 | 5 | 7 | 9 | 11 | 13 | 15 | 17 | 19 | 21 | 23 | 25 | 27 | 29 | 31 | 33 | 35 | 37 | 39 |
|**Color**| | Red | Orange | | | Yellow | | | | | | | | | | | | Blue | | |
|**Function**| 3.3V | I2C_DATA | I2C_CLK | | Ground | BUTTON <br> 3.3V pull-up | | | 3.3V | | | | Ground | | | | | I2S_LRCLK | | Ground |
|**Direction**| NC | IN/OUT | IN | | GND | OUT | | | NC | | | | GND | | | | | OUT | | GND |
| | | | | | | | | | | | | | | | | | | | | |

|| | | | | | | | | | | | | | | | | | | | |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
|**Pin**| 2 | 4 | 6 | 8 | 10 | 12 | 14 | 16 | 18 | 20 | 22 | 24 | 26 | 28 | 30 | 32 | 34 | 36 | 38 | 40 |
|**Color**| White | | Black | | | Green | | | | | | | | | | | | | Purple | Gray |
|**Function**| 5V | 5V | Ground | | | I2S_BCLK | Ground | | | Ground | | | | | Ground | | Ground | | I2S_ADC | I2S_DAC |
|**Direction**| IN | IN | GND | | | OUT | GND | | | GND | | | | | GND | | GND | | OUT | IN |
|| | | | | | | | | | | | | | | | | | | | |

[![top](/docs/TMLK_WS_wm8960_images/IMG_1356.JPG)](/docs/TMLK_WS_wm8960_images/IMG_1356.JPG)
[![bottom](/docs/TMLK_WS_wm8960_images/IMG_1357.JPG)](/docs/TMLK_WS_wm8960_images/IMG_1357.JPG)

### Use with Tiny Machine Learning Kit (TMLK)

Wiring color in the tables matches the images in this document.
Jumper wire vendors may have other color combinations.

Power is supplied to the audio board by the green terminal block on the TMLK
shield.  `VIN` of the terminal block supplies +5V.

Note: `VIN` is connected to `VUSB` on the Nano 33 BLE Sense, thus when the
Nano 33 BLE Sense is powered by the USB cable, `VIN` will be an output.

Terminal block pins for power:

| Pin | Color | Function |
| --- | ---   | ---      | 
| VIN | White | 5V       |
| GND | Black | Ground   |
|     |       |          |

Dual row 20 pin connector for signaling:

| Pin | Color | Function                 | Direction |     | Pin | Color | Function | Direction |
| --- | ---   | ---                      | ---       | --- | --- | ---   | ---      | ---       |
| 1   |       | 3.3V                     | OUT       |     | 2   |       | Ground   | GND       |
| 3   | Orange| I2C_CLK                  | OUT       |     | 4   | Red   | I2C_DATA | IN/OUT    |
| 5   | Yellow| Button <br> 3.3V pull-up | IN        |     | 6   |       |          |           |
| 7   |       |                          |           |     | 8   | Green | I2S_BCLK | IN        |
| 9   |       |                          |           |     | 10  |       |          |           |
| 11  |       |                          |           |     | 12  | Purple| I2S_ADC  | IN        |
| 13  | Gray  | I2S_DAC                  | OUT       |     | 14  |       |          |           |
| 15  |       |                          |           |     | 16  | Blue  | I2S_LRCLK| IN        |
| 17  |       |                          |           |     | 18  |       |          |           |
| 19  |       |                          |           |     | 20  |       |          |           |
|     |       |                          |           |     |     |       |          |           |

[![pins](/docs/TMLK_WS_wm8960_images/IMG_1355.JPG)](/docs/TMLK_WS_wm8960_images/IMG_1355.JPG)

#### Parts List and Tools Required

The following tools are required:
* 2.5mm (or smaller) slotted screwdriver

Parts can be ordered from a number of vendors and distributors:
* DigiKey
* Mouser
* ProtoSupplies
* Adafruit
* SparkFun
* Amazon
* Arduino
* Waveshare

The following is the list of parts:
* [Tiny Machine Learning Kit](https://store-usa.arduino.cc/products/arduino-tiny-machine-learning-kit)
* [Waveshare WM8960 Audio HAT](https://www.waveshare.com/wm8960-audio-hat.htm) 
* 20cm male-male jumper wire, 2.54mm pitch (requires 2)
* 10cm male-female jumper wire, 2.54mm pitch (requires 7)
* M2.5 25mm male-female nylon standoffs with nuts, (requires 6 standoffs, 8 nuts)

The jumper wires are usually ordered in a pack of 40.

The nylon standoffs are usually ordered in a box assortment.

#### Assembly

1. Remove OV7675 camera from dual row 20 pin connector in middle of TMLK shield
2. Add nylon standoffs to the TMLK shield mounting holes
3. Add nylon standoffs to 3 of 4 mounting holes on the audio board
(see image).
The two nylon standoffs at the end of the audio board will need an extra nut
on the bottom side of the board to get the correct height.
4. The audio board attaches to the TMLK shield using a single nylon standoff,
with the audio board on top of the TMLK shield
(see image below)
5. On the TMLK shield, unscrew the green terminal block connections until the
metal gate is fully in the down (open) position
6. Insert black 20cm male-male jumper wire into the `GND` connector of the
green terminal block.
7. Insert white 20cm male-male jumper wire into the `VIN` connector of the
green terminal block
7. Turn the screws on the green terminal block connectors until the jumper
wire pins are firmly held in place
8. Connect the white jumper wire to pin 2 on the bottom of the audio board.
9. Connect the black jumper wire to pin 6 on the bottom of the audio board.
10. Check there is a one pin gap between the white and black jumper connections
on the bottom of the audio board (see [image](/docs/TMLK_WS_wm8960_images/IMG_1362.JPG)).
Failure to do this may result in a **FIRE**.  **Fire** is **bad**.
11. Connect the signal pins using the pin connection tables and images above

Fully assembled:

[![side view](/docs/TMLK_WS_wm8960_images/IMG_1363.JPG)](/docs/TMLK_WS_wm8960_images/IMG_1363.JPG)

Additional images of fully assembled device:

[<img src="../../docs/TMLK_WS_wm8960_images/IMG_1360.JPG" width="200" />](/docs/TMLK_WS_wm8960_images/IMG_1360.JPG)
[<img src="../../docs/TMLK_WS_wm8960_images/IMG_1361.JPG" width="200" />](/docs/TMLK_WS_wm8960_images/IMG_1361.JPG)
[<img src="../../docs/TMLK_WS_wm8960_images/IMG_1362.JPG" width="200" />](/docs/TMLK_WS_wm8960_images/IMG_1362.JPG)

#### Notes

* VIN/GND on TMLK board with all wires connected to WM8960
  * 4.66V with USB isolator
  * 4.65V without USB isolator
* WM8960 crystal is 23.9996MHz
* Oscilloscope measurements for 16KHz sample rate, 16 bits per channel, two channels
  * LR clock is 15.9998KHz
  * Bit clock is 511.992KHz

### Testing

The [audio_play_record](/src/peripherals/examples/audio_play_record/audio_play_record.ino)
test application is an audio API usage example.

1. Open the Arduino IDE
2. Open the `File` menu and navigate to: 
`Examples -> Arduino_TensorFlowLite -> src -> peripherals -> audio_play_record`
3. Load the `audio_play_record` application onto the Nano 33 BLE Sense
4. The application is running when the default LED on the Nano 33 BLE Sense
is blinking.
5. Press and hold the button on the audio board (not the TMLK shield button)
until the LED stops blinking.  
Audio is now being recorded.
There is enough memory for approx. 3 seconds of recording.
Release the button to stop recording or wait until the LED is again blinking.
6. Press and release the button on the audio board to play the
previously recorded audio

## Button

A generic button event API is provided in the [button.h](/src/peripherals/button.h)
header file.

The physical button will vary between peripherals and platforms.  The GPIO pin
that connects to the button is defined in the [peripherals.h](/src/peripherals/peripherals.h)
header file.

The generic button API provides access to the debounced up/down state of the
button as well as the following button events:

* Button-Press
* Button-Long-Press-Down
* Button-Long-Press-Up

## LEDs

A generic LED API is provided in the [led.h](/src/peripherals/led.h) header file.

The physical default LED will vary between peripherals and platforms.
The GPIO pin that connects to the default LED is defined in the
[peripherals.h](/src/peripherals/peripherals.h) header file.

The generic LED API provides the following functionality:

* Turning the LED on/off
* Blinking the LED at an application defined duty cycle and rate
