

// Copyright 2015 The Shaderc Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <unordered_map>

#include "SPIRV/spirv.hpp"

#include "common_shaders_for_test.h"
#include "shaderc/shaderc.h"

namespace {

using testing::Each;
using testing::HasSubstr;
using testing::Not;


// Determines the kind of output required from the compiler.
enum class OutputType {
  SpirvBinary,
  SpirvAssemblyText,
  PreprocessedText,
};

// Generate a compilation result object with the given compile,
// shader source, shader kind, input file name, entry point name, options,
// and for the specified output type.  The entry point name is only significant
// for HLSL compilation.
shaderc_compilation_result_t MakeCompilationResult(
    const shaderc_compiler_t compiler, const std::string& shader,
    shaderc_shader_kind kind, const char* input_file_name,
    const char* entry_point_name, const shaderc_compile_options_t options,
    OutputType output_type) {
  switch (output_type) {
    case OutputType::SpirvBinary:
      return shaderc_compile_into_spv(compiler, shader.c_str(), shader.size(),
                                      kind, input_file_name, entry_point_name,
                                      options);
      break;
    case OutputType::SpirvAssemblyText:
      return shaderc_compile_into_spv_assembly(
          compiler, shader.c_str(), shader.size(), kind, input_file_name,
          entry_point_name, options);
      break;
    case OutputType::PreprocessedText:
      return shaderc_compile_into_preprocessed_text(
          compiler, shader.c_str(), shader.size(), kind, input_file_name,
          entry_point_name, options);
      break;
  }
  // We shouldn't reach here.  But some compilers might not know that.
  // Be a little defensive and produce something.
  return shaderc_compile_into_spv(compiler, shader.c_str(), shader.size(), kind,
                                  input_file_name, entry_point_name, options);
}

// Returns true if shaderc_compilation_result_t from MakeCompilationResult is yummy
bool CompilationResultIsSuccess(const shaderc_compilation_result_t result) {
  return shaderc_result_get_compilation_status(result) ==
         shaderc_compilation_status_success;


// Returns true if the given result contains a SPIR-V module that contains
// at least the number of bytes of the header and the correct magic number.
bool ResultContainsValidSpv(shaderc_compilation_result_t result) {
  if (!CompilationResultIsSuccess(result)) return false;
  size_t length = shaderc_result_get_length(result);
  if (length < 20) return false;
  const uint32_t* bytes = static_cast<const uint32_t*>(
      static_cast<const void*>(shaderc_result_get_bytes(result)));
  return bytes[0] == spv::MagicNumber;
}
