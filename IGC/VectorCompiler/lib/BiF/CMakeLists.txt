#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021-2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include(cmake/Functions.cmake)

set(RESOURCE_EMBEDDER_SCRIPT ${IGC_SOURCE_DIR}/BiFModule/resource_embedder.py)
set(VCB_EXE "vcb" CACHE STRING "")

set(PRINTF_NOT_CM_COMMON_H_PATH ${CMAKE_CURRENT_SOURCE_DIR}/printf_not_cm_common.h)

vc_embed_bif(PRINTF_OCL_32_CPP_PATH printf_ocl_genx.cpp VCBiFPrintfOCL 32
             DEPENDS ${PRINTF_NOT_CM_COMMON_H_PATH})
vc_embed_bif(PRINTF_OCL_64_CPP_PATH printf_ocl_genx.cpp VCBiFPrintfOCL 64
             DEPENDS ${PRINTF_NOT_CM_COMMON_H_PATH})
vc_embed_bif(PRINTF_ZE_32_CPP_PATH printf_ze_genx.cpp VCBiFPrintfZE 32
             DEPENDS ${PRINTF_NOT_CM_COMMON_H_PATH})
vc_embed_bif(PRINTF_ZE_64_CPP_PATH printf_ze_genx.cpp VCBiFPrintfZE 64
             DEPENDS ${PRINTF_NOT_CM_COMMON_H_PATH})

#Define the macro if Khronos SPIR-V translator is used.
if(IGC_OPTION__USE_KHRONOS_SPIRV_TRANSLATOR_IN_SC)
  set(CLANG_FLAGS_FOR_SPIRV_BUILTINS "-D__USE_KHRONOS_SPIRV_TRANSLATOR_IN_SC__")
endif()

get_target_include_opt_list(IGC_SPIRV_HEADERS_INCLUDES IGCSPIRVHeaders)

set(SPIRV_BUILTIN_SOURCES
  spirv_atomic_builtins_genx.cpp
  spirv_exec_builtins_genx.cpp
  spirv_math_builtins_genx.cpp)
vc_embed_bif(SPIRV_BUILTINS_CPP_PATH
             "${SPIRV_BUILTIN_SOURCES}"
             VCSPIRVBuiltins 64
             CLANG_INCLUDES ${IGC_SPIRV_HEADERS_INCLUDES}
             CLANG_FLAGS ${CLANG_FLAGS_FOR_SPIRV_BUILTINS})

set(BUILTIN_SOURCES
  Library/Math/F16/fptoi.cpp
  Library/Math/F16/itofp.cpp

  Library/Math/F32/fptoi.cpp
  Library/Math/F32/itofp.cpp

  Library/Math/F64/fdiv.cpp
  Library/Math/F64/fptoi.cpp
  Library/Math/F64/fsqrt.cpp
  Library/Math/F64/itofp.cpp

  Library/Math/Integer/i64divrem.cpp
  Library/Math/Integer/sdivrem.cpp
  Library/Math/Integer/udivrem.cpp

  Library/Atomics/Local/binop.cpp)

vc_embed_optimized_bif(BUILTINS_CPP_PATH "${BUILTIN_SOURCES}" VCBuiltins 64)

add_custom_target(VCBiFPreparation
  DEPENDS ${PRINTF_OCL_32_CPP_PATH}
          ${PRINTF_OCL_64_CPP_PATH}
          ${PRINTF_ZE_32_CPP_PATH}
          ${PRINTF_ZE_64_CPP_PATH}
          ${BUILTINS_CPP_PATH}
          ${SPIRV_BUILTINS_CPP_PATH})

add_library(VCEmbeddedBiF OBJECT
  ${PRINTF_OCL_32_CPP_PATH}
  ${PRINTF_OCL_64_CPP_PATH}
  ${PRINTF_ZE_32_CPP_PATH}
  ${PRINTF_ZE_64_CPP_PATH}
  ${BUILTINS_CPP_PATH}
  ${SPIRV_BUILTINS_CPP_PATH})
target_link_libraries(VCEmbeddedBiF VCHeaders)
