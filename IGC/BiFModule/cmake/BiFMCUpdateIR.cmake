#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# Regenerates the precompiled bfloat16 built-ins (IBiF_BFloat.ll), which are
# used when clang does not support __bf16, from the modules after "--".
#
# -DLLVM_LINK=<tool> -DLLVM_DIS=<tool> -DOPAQUE_ARG=<flag> -DOUTPUT=<.ll> -DSTAMP=<file>

set(modules)
set(found FALSE)
math(EXPR last "${CMAKE_ARGC} - 1")
foreach(i RANGE ${last})
  if(found)
    list(APPEND modules "${CMAKE_ARGV${i}}")
  elseif(CMAKE_ARGV${i} STREQUAL "--")
    set(found TRUE)
  endif()
endforeach()

set(linked "${STAMP}.bc")
execute_process(COMMAND "${LLVM_LINK}" ${OPAQUE_ARG} -o "${linked}" ${modules} RESULT_VARIABLE result)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "[IGC\\BiFModule] - Linking the bfloat16 modules failed")
endif()
execute_process(COMMAND "${LLVM_DIS}" ${OPAQUE_ARG} -o - "${linked}" OUTPUT_VARIABLE ir RESULT_VARIABLE result)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "[IGC\\BiFModule] - Disassembling the bfloat16 modules failed")
endif()
string(REGEX REPLACE "[^\n]*(ModuleID|source_filename)[^\n]*\n" "" ir "${ir}")
file(WRITE "${OUTPUT}" "${ir}")
file(TOUCH "${STAMP}")
