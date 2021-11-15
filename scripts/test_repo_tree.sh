#!/usr/bin/env bash
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
#
# Tests the local git repo to make sure the Arduino examples can successfully
# be built using the Arduino CLI.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."
cd "${ROOT_DIR}"

source "${SCRIPT_DIR}"/helper_functions.sh

readable_run "${SCRIPT_DIR}"/install_arduino_cli.sh

# test_arduino_libarary.sh must be passed a normalized path,
# thus the cd ${ROOT_DIR} above is required for ${PWD} here.
readable_run "${SCRIPT_DIR}"/test_arduino_library.sh \
  "${PWD}"
