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

TEMP_DIR=$(mktemp -d)
cd "${TEMP_DIR}"

echo Cloning tflite-micro repo to "${TEMP_DIR}"
git clone --depth 1 --single-branch "https://github.com/tensorflow/tflite-micro.git"
cd tflite-micro

make -f tensorflow/lite/micro/tools/make/Makefile clean_downloads

BASE_DIR="${TEMP_DIR}/tflm_tree"
OUTPUT_DIR="${TEMP_DIR}/tflm_arduino"
TARGET=cortex_m_generic
OPTIMIZED_KERNEL_DIR=cmsis_nn
TARGET_ARCH=project_generation

# Create the TFLM base tree
python3 tensorflow/lite/micro/tools/project_generation/create_tflm_tree.py \
  -e hello_world -e magic_wand -e micro_speech -e person_detection \
  --makefile_options="TARGET=${TARGET} OPTIMIZED_KERNEL_DIR=${OPTIMIZED_KERNEL_DIR} TARGET_ARCH=${TARGET_ARCH}" \
  "${BASE_DIR}"

# Create the final tree in ${OUTPUT_DIR} using the base tree in ${BASE_DIR}
# The create_tflm_arduino.py script takes care of cleaning ${OUTPUT_DIR}
cd "${ROOT_DIR}"
python3 "${SCRIPT_DIR}"/create_tflm_arduino.py \
  --output_dir="${OUTPUT_DIR}" \
  --base_dir="${BASE_DIR}"

# Now, at the root of the repo, remove files and subdirectories that will be
# updated from ${OUTPUT_DIR}
find "${OUTPUT_DIR}" -maxdepth 1 \! -path "${OUTPUT_DIR}" -printf "%f\n" | xargs -I "{}" rm -rf "{}"
# copy ${OUTPUT_DIR} to the repo
cp -aT "${OUTPUT_DIR}" "${ROOT_DIR}"

rm -rf "${TEMP_DIR}"
