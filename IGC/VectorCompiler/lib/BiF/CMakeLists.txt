#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include(cmake/Functions.cmake)

set(RESOURCE_EMBEDDER_SCRIPT ${IGC_SOURCE_DIR}/BiFModule/resource_embedder.py)
set(VCB_EXE "vcb" CACHE STRING "")

set(PRINTF_COMMON ${CMAKE_CURRENT_SOURCE_DIR}/Print/common.h)
vc_embed_bif(PRINTF_OCL_32_TYPED_PTRS Print/ocl.cpp VCBiFPrintfOCL 32 FALSE DEPENDS ${PRINTF_COMMON})
vc_embed_bif(PRINTF_OCL_32_OPAQUE_PTRS Print/ocl.cpp VCBiFPrintfOCL 32 TRUE DEPENDS ${PRINTF_COMMON})
vc_embed_bif(PRINTF_OCL_64_TYPED_PTRS Print/ocl.cpp VCBiFPrintfOCL 64 FALSE DEPENDS ${PRINTF_COMMON})
vc_embed_bif(PRINTF_OCL_64_OPAQUE_PTRS Print/ocl.cpp VCBiFPrintfOCL 64 TRUE DEPENDS ${PRINTF_COMMON})
vc_embed_bif(PRINTF_ZE_32_TYPED_PTRS Print/ze.cpp VCBiFPrintfZE 32 FALSE DEPENDS ${PRINTF_COMMON})
vc_embed_bif(PRINTF_ZE_32_OPAQUE_PTRS Print/ze.cpp VCBiFPrintfZE 32 TRUE DEPENDS ${PRINTF_COMMON})
vc_embed_bif(PRINTF_ZE_64_TYPED_PTRS Print/ze.cpp VCBiFPrintfZE 64 FALSE DEPENDS ${PRINTF_COMMON})
vc_embed_bif(PRINTF_ZE_64_OPAQUE_PTRS Print/ze.cpp VCBiFPrintfZE 64 TRUE DEPENDS ${PRINTF_COMMON})

get_target_include_opt_list(IGC_SPIRV_HEADERS_INCLUDES IGCSPIRVHeaders)

set(SPIRV_BUILTIN_SOURCES
  Spirv/atomic.cpp
  Spirv/assert.cpp
  Spirv/barrier.cpp
  Spirv/exec.cpp
  Spirv/math.cpp)

vc_embed_bif(SPIRV_BUILTINS_TYPED_PTRS "${SPIRV_BUILTIN_SOURCES}" VCSPIRVBuiltins 64 FALSE
  CLANG_INCLUDES ${IGC_SPIRV_HEADERS_INCLUDES}
  CLANG_FLAGS ${CLANG_FLAGS_FOR_SPIRV_BUILTINS})
vc_embed_bif(SPIRV_BUILTINS_OPAQUE_PTRS "${SPIRV_BUILTIN_SOURCES}" VCSPIRVBuiltins 64 TRUE
  CLANG_INCLUDES ${IGC_SPIRV_HEADERS_INCLUDES}
  CLANG_FLAGS ${CLANG_FLAGS_FOR_SPIRV_BUILTINS})

set(BUILTIN_SOURCES
  Library/Math/F16/fptoi.cpp
  Library/Math/F16/itofp.cpp

  Library/Math/F32/fdiv.cpp
  Library/Math/F32/frem.cpp
  Library/Math/F32/fptoi.cpp
  Library/Math/F32/itofp.cpp

  Library/Math/F64/rsqrt.cpp
  Library/Math/F64/fdiv.cpp
  Library/Math/F64/fptoi.cpp
  Library/Math/F64/frem.cpp
  Library/Math/F64/fsqrt.cpp
  Library/Math/F64/itofp.cpp

  Library/Math/Integer/i64divrem.cpp
  Library/Math/Integer/sdivrem.cpp
  Library/Math/Integer/udivrem.cpp

  Library/Atomics/Local/binop.cpp)

vc_embed_optimized_bif(BUILTINS_TYPED_PTRS "${BUILTIN_SOURCES}" VCBuiltins 64 FALSE)
vc_embed_optimized_bif(BUILTINS_OPAQUE_PTRS "${BUILTIN_SOURCES}" VCBuiltins 64 TRUE)

set(BIF_OUTPUT_TYPED_PTRS ${PRINTF_OCL_32_TYPED_PTRS} ${PRINTF_OCL_64_TYPED_PTRS}
                          ${PRINTF_ZE_32_TYPED_PTRS} ${PRINTF_ZE_64_TYPED_PTRS}
                          ${BUILTINS_TYPED_PTRS} ${SPIRV_BUILTINS_TYPED_PTRS})
set(BIF_OUTPUT_OPAQUE_PTRS ${PRINTF_OCL_32_OPAQUE_PTRS} ${PRINTF_OCL_64_OPAQUE_PTRS}
                           ${PRINTF_ZE_32_OPAQUE_PTRS} ${PRINTF_ZE_64_OPAQUE_PTRS}
                           ${BUILTINS_OPAQUE_PTRS} ${SPIRV_BUILTINS_OPAQUE_PTRS})

add_custom_target(VCBiFPreparation DEPENDS ${BIF_OUTPUT_TYPED_PTRS} ${BIF_OUTPUT_OPAQUE_PTRS})
if(IGC_OPTION__API_ENABLE_OPAQUE_POINTERS)
  add_library(VCEmbeddedBiF OBJECT ${BIF_OUTPUT_OPAQUE_PTRS})
else()
  add_library(VCEmbeddedBiF OBJECT ${BIF_OUTPUT_TYPED_PTRS})
endif()
add_dependencies(VCEmbeddedBiF VCBiFPreparation)
target_link_libraries(VCEmbeddedBiF VCHeaders)
