/* Copyright 2021 The TensorFlow Authors. All Rights Reserved.

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

#include "tensorflow/lite/micro/kernels/softmax.h"

#include "third_party/cmsis/CMSIS/NN/Include/arm_nnfunctions.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/kernels/internal/common.h"
#include "tensorflow/lite/kernels/internal/quantization_util.h"
#include "tensorflow/lite/kernels/internal/reference/softmax.h"
#include "tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "tensorflow/lite/kernels/kernel_util.h"
#include "tensorflow/lite/kernels/op_macros.h"
#include "tensorflow/lite/micro/kernels/kernel_util.h"

namespace tflite {
namespace {

void SoftmaxQuantized(const TfLiteEvalTensor* input, TfLiteEvalTensor* output,
                      const SoftmaxParams& op_data) {
  if (input->type == kTfLiteInt8) {
    if (output->type == kTfLiteInt16) {
      tflite::reference_ops::Softmax(
          op_data, tflite::micro::GetTensorShape(input),
          tflite::micro::GetTensorData<int8_t>(input),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<int16_t>(output));
    } else {
      const auto input_shape = tflite::micro::GetTensorShape(input);
      const auto output_shape = tflite::micro::GetTensorShape(output);
      const int trailing_dim = input_shape.DimensionsCount() - 1;
      const int outer_size =
          MatchingFlatSizeSkipDim(input_shape, trailing_dim, output_shape);
      const int depth =
          MatchingDim(input_shape, trailing_dim, output_shape, trailing_dim);

      arm_softmax_s8(tflite::micro::GetTensorData<int8_t>(input), outer_size,
                     depth, op_data.input_multiplier, op_data.input_left_shift,
                     op_data.diff_min,
                     tflite::micro::GetTensorData<int8_t>(output));
    }
  } else {
    tflite::reference_ops::SoftmaxInt16(
        op_data, tflite::micro::GetTensorShape(input),
        tflite::micro::GetTensorData<int16_t>(input),
        tflite::micro::GetTensorShape(output),
        tflite::micro::GetTensorData<int16_t>(output));
  }
}

TfLiteStatus SoftmaxEval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteEvalTensor* input = tflite::micro::GetEvalInput(context, node, 0);
  TfLiteEvalTensor* output = tflite::micro::GetEvalOutput(context, node, 0);

  TFLITE_DCHECK(node->user_data != nullptr);
  const SoftmaxParams data =
      *static_cast<const SoftmaxParams*>(node->user_data);

  switch (input->type) {
    case kTfLiteFloat32: {
      tflite::reference_ops::Softmax(
          data, tflite::micro::GetTensorShape(input),
          tflite::micro::GetTensorData<float>(input),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<float>(output));
      return kTfLiteOk;
    }
    case kTfLiteInt8:
    case kTfLiteInt16: {
      SoftmaxQuantized(input, output, data);
      return kTfLiteOk;
    }
    default:
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                         TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
  }
}

}  // namespace

TfLiteRegistration Register_SOFTMAX() {
  return {/*init=*/SoftmaxInit,
          /*free=*/nullptr,
          /*prepare=*/SoftmaxPrepare,
          /*invoke=*/SoftmaxEval,
          /*profiling_string=*/nullptr,
          /*builtin_code=*/0,
          /*custom_name=*/nullptr,
          /*version=*/0};
}

}  // namespace tflite
