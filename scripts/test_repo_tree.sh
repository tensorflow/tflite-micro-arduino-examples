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
# Creates the project file distributions for the TensorFlow Lite Micro test and
# example targets aimed at embedded platforms.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."
cd "${ROOT_DIR}"
echo BASH_SOURCE=${BASH_SOURCE[0]} SCRIPT_DIR=${SCRIPT_DIR} ROOT_DIR=${ROOT_DIR}

source "${SCRIPT_DIR}"/helper_functions.sh

readable_run "${SCRIPT_DIR}"/install_arduino_cli.sh

readable_run "${SCRIPT_DIR}"/test_arduino_library.sh \
  "${ROOT_DIR}"
