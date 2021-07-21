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
ROOT_DIR=${SCRIPT_DIR}/../../../../..
cd "${ROOT_DIR}"

source tensorflow/lite/micro/tools/ci_build/helper_functions.sh

readable_run make -f tensorflow/lite/micro/tools/make/Makefile clean_downloads

BASE_DIR=/tmp/tflm_tree
OUTPUT_DIR=/tmp/tflm_arduino
ZIP_FILE="${OUTPUT_DIR}.zip"
#TARGET=arduino
#OPTIMIZED_KERNEL_DIR=cmsis_nn

readable_run python3 tensorflow/lite/micro/tools/project_generation/create_tflm_arduino.py \
  --output_dir=${OUTPUT_DIR} \
  --base_dir=${BASE_DIR}

readable_run tensorflow/lite/micro/tools/ci_build/install_arduino_cli.sh

readable_run tensorflow/lite/micro/tools/ci_build/test_arduino_library.sh \
  ${ZIP_FILE}
