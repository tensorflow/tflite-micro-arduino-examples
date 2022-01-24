# Lint as: python3
# Copyright 2021 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================
"""Script to run Arduino example tests over serial port"""

import argparse
from json.decoder import JSONDecodeError
import threading
from types import SimpleNamespace
import serial
import base64
import sys
import queue
import re
import time
import traceback
import wave
import json
from PIL import Image
from datetime import datetime
from enum import Enum, unique
from pathlib import Path
from typing import List, Pattern, Union


class TestOverSerial:
  """
  Test Arduino examples by sending data over serial port
  to Nano 33 BLE Sense device
  """

  #
  # private types
  #

  @unique
  class DataType(str, Enum):
    """
    Test data type Enum.
    """
    IMAGE = "image-grayscale"
    AUDIO = "audio-pcm-16khz-mono-s16"
    RAW_INT8 = "raw-int8"
    RAW_FLOAT = "raw-float"

    def __str__(self) -> str:
      return str(self.value)

  @unique
  class ConfigKeys(str, Enum):
    """
    Test configurationn keys Enum.
    """
    TYPE = "data type"
    DELAY = "delay after"
    DATA = "test data"
    FILE = "file name"
    PATTERN = "regex"
    EXPR = "expr"
    LABEL = "label"
    QQVGA = "qqvga size"

    def __str__(self) -> str:
      return str(self.value)

  #
  # private methods
  #

  def __init__(self, config) -> None:
    self._results = TestOverSerial.TestResults(total=0,
                                               completed=0,
                                               passed=0,
                                               failed=0)
    self._config = config

  def _send_test_command(self) -> bool:
    serial_wrapper = SerialWrapper()
    result_match = ResultMatch()
    main = Main()
    command = "!TEST"
    result = serial_wrapper.writeline(command, timeout=1.0)
    if not result:
      # write timeout
      main.fatal("Timeout while sending TEST command")
    else:
      main.log_console(command, prefix=">>>")
    success = r"!OK TEST (\d+)"
    fail = "!FAIL TEST"
    patterns = [success, fail]
    result = result_match.wait_match(patterns, timeout=5.0)
    if result is None:
      # match timeout
      main.fatal("Timeout while waiting for TEST command reply")
    elif len(result) == 2 and result[0][:-len(result[1]) - 1] in success:
      # TEST command success
      return (True, int(result[1]))
    elif result[0] == fail and len(result) == 1:
      # TEST command failed
      return (False, 0)
    else:
      # unknown response
      main.fatal("Unknown TEST command reply")

  def _send_data(self, data_type: "TestOverSerial.DataType", datum: bytes,
                 decode_length: int) -> bool:
    serial_wrapper = SerialWrapper()
    result_match = ResultMatch()
    main = Main()

    # send DATA command
    command = f"DATA {str(data_type)} {len(datum)}"
    result = serial_wrapper.writeline(f"!{command}", timeout=1.0)
    if not result:
      # write timeout
      main.fatal("Timeout while sending DATA command")
    else:
      main.log_console(command, prefix=">>>")

    # Send base64 encoded data.
    # Maximum line size (not including trailing '\n' character) is determined
    # by <decode_length> * 4 / 3
    remain = len(datum)
    index = 0
    fail = f"!FAIL {command}"
    success = r"!DATA_ACK (\d+)"
    while remain > 0:
      base64_data = base64.b64encode(datum[index:index + decode_length])
      result = serial_wrapper.writeline(base64_data.decode(), timeout=1.0)
      if not result:
        # write timeout
        main.fatal("Write timeout during data transfer")
      # check for transfer abort
      result = result_match.wait_match([fail, success], timeout=1.0)
      if result is None:
        main.fatal("Timeout waiting for DATA ack")
      elif result[0] == fail:
        return False
      elif len(result) == 2 and result[0][:-len(result[1]) - 1] in success:
        # DATA ack response
        remain_decode_length = min(remain, decode_length)
        if int(result[1]) != remain_decode_length:
          # bad DATA ack response line length
          main.fatal(
              f"DATA ack line length {result[1]} != {remain_decode_length}")
      else:
        # unknown response
        main.fatal("Unknown DATA ack response")
      remain -= decode_length
      index += decode_length

    # get DATA command reply
    success = f"!OK {command}"
    patterns = [success, fail]
    result = result_match.wait_match(patterns, timeout=1.0)
    if result is None:
      # match timeout
      main.fatal("Timeout while waiting for DATA command reply")
    elif len(result) != 1:
      # wrong number of regex matches
      main.fatal("Invalid DATA command reply")
    elif result[0] == success:
      return True
    elif result[0] == fail:
      return False
    else:
      # unknown response
      main.fatal("Unknown DATA command reply")

  def _load_image_grayscale(self, path: Path, use_qqvga: bool) -> bytes:
    ksize_qqvga = (160, 120)
    ksize_96x96 = (96, 96)
    if use_qqvga:
      new_size = ksize_qqvga
    else:
      new_size = ksize_96x96
    try:
      with Image.open(path) as im:
        if im.mode != "L":
          im = im.convert("L")  # convert to grayscale
        if im.size != new_size:
          im = im.resize(new_size)
        return im.tobytes()
    except (FileNotFoundError, OSError) as ex:
      Main().fatal(f"Failed to open image <{str(path)}> ({str(ex)})")

  def _load_wave(self, path: Path) -> bytes:
    try:
      wave_file: wave.Wave_read
      with wave.open(str(path)) as wave_file:
        samples = wave_file.readframes(wave_file.getnframes())
        return samples
    except wave.Error as ex:
      Main().fatal(f"Failed to open wave audio <{str(path)}> ({str(ex)})")

  def _safe_eval(self, expr: str, groups: List[str], label: str) -> bool:
    if expr == "":
      return True

    for index in range(len(groups)):
      try:
        f = float(groups[index])
        groups[index] = f
      except ValueError:
        pass

    local_vars = {
        "groups": groups,
        "label": label,
    }
    return bool(eval(  # pylint: disable=eval-used
        expr, None, local_vars))

  #
  # public types
  #

  @unique
  class Examples(str, Enum):
    """
    Test example Enum.
    """
    PERSON = "person_detection"
    WAND = "magic_wand"
    SPEECH = "micro_speech"
    HELLO = "hello_world"

    def __str__(self) -> str:
      return str(self.value)

  class TestResults(SimpleNamespace):
    pass

  #
  # public methods
  #

  @property
  def results(self) -> "TestOverSerial.TestResults":
    """
    The test results property.

    Returns:
      An instance of TestOverSerial.TestResults containing the following
      values:
        total: Total number of tests to run.
        completed: Number of tests that were completed.
        passed: Number of tests that passed.
        failed: Number of tests that failed.
    """
    return self._results

  def start_tests(self) -> None:
    """
    Begin running the tests as per the example test configuration.

    Raises:
      RuntimeError if an unrecoverable error occurs during the test run.
    """
    main = Main()
    result, decode_line_length = self._send_test_command()
    if not result:
      main.fatal("Unable to place device into test mode")

    try:
      config = self._config
      data_type = str(config[TestOverSerial.ConfigKeys.TYPE])
      delay_after = float(config[TestOverSerial.ConfigKeys.DELAY])
      test_list = list(config[TestOverSerial.ConfigKeys.DATA])
      self._results.total = len(test_list)
      test_index = 1
      for test_info in test_list:
        file_path = Path(test_info[TestOverSerial.ConfigKeys.FILE])
        label = str(test_info[TestOverSerial.ConfigKeys.LABEL])
        pattern = str(test_info[TestOverSerial.ConfigKeys.PATTERN])
        eval_expr = str(test_info[TestOverSerial.ConfigKeys.EXPR])
        if data_type == TestOverSerial.DataType.IMAGE:
          datum = self._load_image_grayscale(
              file_path, use_qqvga=test_info[TestOverSerial.ConfigKeys.QQVGA])
        elif data_type == TestOverSerial.DataType.AUDIO:
          datum = self._load_wave(file_path)
        else:
          main.fatal(f"Data type <{data_type}> not yet supported")

        main.log_console(
            f"Test #{test_index} for label <{label}> file <{file_path.name}>")
        result = self._send_data(data_type,
                                 datum=datum,
                                 decode_length=decode_line_length)
        if not result:
          main.fatal(f"Test #{test_index} ABORTED during data transfer")
        groups = ResultMatch().wait_match([pattern], timeout=3.0)
        if groups is None:
          # timeout waiting for regex pattern
          main.fatal(f"Test #{test_index} TIMEOUT waiting for inference result")
        result = self._safe_eval(eval_expr, groups=groups, label=label)
        if result:
          self._results.passed += 1
          main.log_console(f"Test #{test_index} PASSED")
        else:
          self._results.failed += 1
          main.log_console(f"Test #{test_index} FAILED")

        self._results.completed += 1

        if delay_after > 0 and test_index < self._results.total:
          main.log_console(f"Waiting {delay_after}s before next test")
          time.sleep(delay_after)

        test_index += 1
    except (KeyError, ValueError, IndexError, TypeError) as ex:
      main.fatal(f"Invalid configuration data ({str(ex)})")


class ResultMatch:
  """
  Match serial port input against desired results in the
  form of regular expressions.
  This class is a singleton.
  """

  #
  # private class vars
  #

  _instance: "ResultMatch" = None

  #
  # private methods
  #

  def __new__(cls) -> "ResultMatch":
    if ResultMatch._instance is not None:
      return ResultMatch._instance
    else:
      return super(ResultMatch, cls).__new__(cls)

  def __init__(self) -> None:
    # ensure __init__ is only called once for singleton
    if ResultMatch._instance is not None:
      return
    else:
      ResultMatch._instance = self
    self._queue: queue.Queue = queue.Queue()

  def _pop(self, timeout: Union[float, None]) -> Union[str, None]:
    try:
      return self._queue.get(timeout=timeout)
    except queue.Empty:
      return None

  #
  # public methods
  #

  def push(self, datum: str) -> None:
    """
    Push a serial input string on the queue

    Args:
      datum: String
    """
    self._queue.put(datum)

  def wait_match(self,
                 patterns: List[str],
                 timeout: Union[float, None] = None) -> Union[None, List[str]]:
    """
    Given a list of regex patterns, match them against the strings from the
    serial input queue and return the first match.

    Args:
      patterns: List of regex patterns.
      timeout: Float specifying the number of seconds to wait for a match,
        or None to wait forever.

    Returns:
      List of strings, one for each regex group match.
      None is returned on timeout.
    """
    regex_list: List[Pattern] = []
    for s in patterns:
      # add global capture to expression
      s = "(" + s + ")"
      regex_list.append(re.compile(s))

    past = datetime.now()
    while True:
      elapsed = (datetime.now() - past).total_seconds()
      if timeout is not None and elapsed > timeout:
        return None
      serial_input = self._pop(timeout)
      if serial_input is None:
        return None
      for regex in regex_list:
        match_object = regex.fullmatch(serial_input)
        if match_object is not None:
          return list(match_object.groups())


class SerialWrapper:
  """
  Wrapper class for serial port access.
  This class is a singleton.
  """

  #
  # private class vars
  #

  _instance: "SerialWrapper" = None

  #
  # private methods
  #

  def __new__(cls) -> "SerialWrapper":
    if SerialWrapper._instance is not None:
      return SerialWrapper._instance
    else:
      return super(SerialWrapper, cls).__new__(cls)

  def __init__(self) -> None:
    # ensure __init__ is only called once for singleton
    if SerialWrapper._instance is not None:
      return
    else:
      SerialWrapper._instance = self
    self._serial: serial.Serial = None

  #
  # public methods
  #

  def initialize(self, port: Union[str, None]) -> None:
    """
    Open and initialize the serial port.

    Args:
      port: String containing the full path to the serial port.
        If the value is None, a default for the OS is used.

    Raises:
      RuntimeError if the serial port cannot be opened.
    """
    if self._serial is not None:
      return
    if port is not None:
      pass
    elif sys.platform == "linux":
      port = "/dev/ttyACM0"
    elif sys.platform == "darwin":
      port = "/dev/cu.usbmodem14201"
    else:
      Main().fatal("Serial port not supported for this OS")

    try:
      self._serial = serial.Serial(port=port, exclusive=True)
    except ValueError as ex:
      Main().fatal(f"Bad serial port parameters ({str(ex)})")
    except serial.SerialException as ex:
      Main().fatal(f"Unable to open serial port ({str(ex)})")

  def readline(self, timeout: Union[float, None]) -> str:
    """
    Read characters from the serial port, up to and including the newline
    ('\\n') character.

    Args:
      timeout: Float specifying the number of seconds to wait for a newline
        character, or None to wait forever.

    Returns:
      A string of characters read including the newline character.
      If the timeout occurs before reading a newline, all currently read
      characters are returned.
    """
    self._serial.timeout = timeout
    result_bytes: bytes = self._serial.read_until()
    return result_bytes.decode()

  def writeline(self, data: str, timeout: Union[float, None]) -> bool:
    """
    Write a string of characters to the serial port.  A newline ('\\n')
    character is then also written to the serial port.

    Args:
      data: String to write to serial port.
      timeout: Float specifying the number of seconds to wait for the write
        to complete, or None to wait forever.

    Returns:
      True if the write operation succeeded.
      False if the write operation timed-out.

    Raises:
      RuntimeError if the number of characters written does not match the
      length of <data>.
    """
    self._serial.write_timeout = timeout
    try:
      count = self._serial.write(data.encode())
      if count != len(data):
        Main().fatal(f"Short write {count} of {len(data)}")
      self._serial.write(b"\n")
      return True
    except serial.SerialTimeoutException:
      return False

  def close(self) -> None:
    """
    Close the serial port.
    """
    if self._serial is None:
      return
    else:
      self._serial.close()
      self._serial = None


class InputHandler:
  """
  Input handler for data from serial port.  Runs in a thread.
  This class is a singleton.
  """
  #
  # private class vars
  #

  _instance: "InputHandler" = None

  #
  # private methods
  #

  def __new__(cls) -> "InputHandler":
    if InputHandler._instance is not None:
      return InputHandler._instance
    else:
      return super(InputHandler, cls).__new__(cls)

  def __init__(self) -> None:
    # ensure __init__ is only called once for singleton
    if InputHandler._instance is None:
      InputHandler._instance = self
      self._canceled = False
      self._thread: threading.Thread = None

  def _input_handler(self) -> None:
    result_match = ResultMatch()
    serial_wrapper = SerialWrapper()
    main = Main()
    buffered_line = ""
    while not self._canceled:
      line = serial_wrapper.readline(timeout=0.1)
      if not line.endswith("\n"):
        buffered_line += line
      else:
        # strip "\n" off end.
        # strip "\r" off end generated by Tensorflow Lite Micro
        # debug/error macro.
        completed_line = (buffered_line + line).rstrip("\n\r")
        main.log_console(completed_line, prefix="<<<")
        result_match.push(completed_line)
        buffered_line = ""

  #
  # public methods
  #

  def stop(self) -> None:
    """
    Signal the InputHandler thread to stop, and wait for the thread to exit.
    """
    self._canceled = True
    if self._thread is not None:
      self._thread.join()
      self._thread = None

  def start(self) -> None:
    """
    Start the InputHandler thread.
    """
    self._thread = threading.Thread(target=self._input_handler)
    self._canceled = False
    self._thread.start()


class Main:
  """
  Script "main()" and support methods.
  This class is a singleton.
  """

  #
  # private class vars
  #

  _instance: "Main" = None

  #
  # private types
  #

  @unique
  class Verbosity(str, Enum):
    """
    Logging verbosity Enum.
    """
    ALL = "all"
    TEST_ONLY = "test"

    def __str__(self) -> str:
      return str(self.value)

  #
  # private methods
  #

  def __new__(cls) -> "Main":
    if Main._instance is not None:
      return Main._instance
    else:
      return super(Main, cls).__new__(cls)

  def __init__(self) -> None:
    # ensure __init__ is only called once for singleton
    if Main._instance is not None:
      return
    else:
      Main._instance = self
      self._args = self._parse_arguments().parse_args()
      self._config = None

  def _parse_arguments(self) -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Test Arduino example running on device, "
        "with inference data supplied over serial port")
    parser.add_argument("--example",
                        type=str,
                        required=True,
                        choices=[
                            str(TestOverSerial.Examples.PERSON),
                            str(TestOverSerial.Examples.SPEECH),
                            str(TestOverSerial.Examples.WAND),
                            str(TestOverSerial.Examples.HELLO)
                        ],
                        help="Arduino example to test")
    parser.add_argument(
        "--verbose",
        type=str,
        default=str(Main.Verbosity.TEST_ONLY),
        choices=[str(Main.Verbosity.ALL),
                 str(Main.Verbosity.TEST_ONLY)],
        help="Verbosity of the console output")
    parser.add_argument(
        "--port",
        type=str,
        default=None,
        metavar="PATH",
        help="Full path of the serial port.  "
        "If not specified an appropriate default will be chosen.")
    parser.add_argument(
        "--config",
        type=Path,
        default=None,
        metavar="PATH",
        help="Full path of the configuration file.  "
        "If not specified an appropriate default will be chosen.")
    return parser

  def _load_config(self):
    if self._args.config is None:
      self._args.config = Path(
        f"examples/{self._args.example}/data/serial_test_config.json")
    try:
      with open(self._args.config) as fp:
        self._config = json.load(fp)
    except OSError as ex:
      self.fatal(f"Unable to open config file ({str(ex)})")
    except JSONDecodeError as ex:
      self.fatal(f"Bad format for config file ({str(ex)})")

  #
  # public methods
  #

  def main(self) -> bool:
    """
    Script entry point.

    Returns:
      True if all tests have passed.
      False if any test has failed or all tests are not completed.
    """
    self._load_config()
    try:
      self.log_console("Test start:")
      test = TestOverSerial(self._config)
      serial_wrapper = SerialWrapper()
      input_handler = InputHandler()
      serial_wrapper.initialize(self._args.port)
      input_handler.start()
      test.start_tests()
    finally:
      # cleanup before exit
      input_handler.stop()
      serial_wrapper.close()
      results = test.results
      self.log_console(
          f"Test end: total {results.total} completed {results.completed} "
          f"passed {results.passed} failed {results.failed}")

    if results.failed > 0 or results.total != results.completed:
      return False
    else:
      return True

  def fatal(self, message: str) -> None:
    """
    Send a message to standard output and then raise an exception.

    Args:
      message: String to be sent to standard output, and that will appear
        as part of the exception.

    Raises:
      RuntimeException.
    """
    print()  # ensure logging and exception occur on new line
    self.log_console(message, prefix="FATAL")
    raise RuntimeError(message)

  def log_console(self, message: str, prefix: str = "") -> None:
    """
    Send a message to standard output.  Verbosity of this method is
    determined by the "--verbose" command line option.

    Args:
      message: String to be sent to standard output.
      prefix: String to be output before <message>.
    """
    if prefix != "" and self._args.verbose != Main.Verbosity.ALL:
      return
    time_str = datetime.now().strftime("%H:%M:%S.%f")
    print(time_str, prefix, message, sep=" ")


if __name__ == "__main__":
  try:
    if Main().main():
      sys.exit(0)
    else:
      sys.exit(1)
  except RuntimeError as ex:
    traceback.print_tb(sys.exc_info()[2])
    print(str(ex))
    sys.exit(1)
