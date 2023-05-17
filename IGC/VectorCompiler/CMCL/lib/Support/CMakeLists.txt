#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021-2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(CM_CL_SUPPORT_SOURCES
  BuiltinTranslator.cpp
  TranslatorPass.cpp
  InitializePasses.cpp
  )

add_library(CMCLSupport STATIC ${CM_CL_SUPPORT_SOURCES})

# LLVM dependencies
igc_get_llvm_targets(LLVM_LIBS
  Core
  Support
  )

target_link_libraries(CMCLSupport
  CMCLInterface
  LLVMGenXIntrinsics
  VCInternalIntrinsics
  ${LLVM_LIBS}
  )

# Generating builtin translation info from the description.
set(TRANSLATION_INFO_NAME TranslationInfo.inc)
set(TRANSLATION_INFO_PATH ${CMAKE_CURRENT_BINARY_DIR}/${TRANSLATION_INFO_NAME})
set(GENERATOR_SCRIPT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/GenerateTranslationInfo.py)
set(BUILTIN_DESCRIPTION_NAME TranslationDescription.json)
set(BUILTIN_DESCRIPTION_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${BUILTIN_DESCRIPTION_NAME})
add_custom_command(OUTPUT ${TRANSLATION_INFO_PATH}
  COMMAND ${PYTHON_EXECUTABLE} ${GENERATOR_SCRIPT_PATH} --desc ${BUILTIN_DESCRIPTION_PATH}
          --output ${TRANSLATION_INFO_PATH}
  COMMENT "CMCL: Generating include file ${TRANSLATION_INFO_NAME} from JSON "
          "description ${BUILTIN_DESCRIPTION_NAME}"
  DEPENDS ${PYTHON_EXECUTABLE} ${GENERATOR_SCRIPT_PATH} ${BUILTIN_DESCRIPTION_PATH})

add_custom_target(CMCLTranslationCodeGeneration
    DEPENDS ${TRANSLATION_INFO_PATH}
    SOURCES ${BUILTIN_DESCRIPTION_PATH} ${GENERATOR_SCRIPT_PATH})

add_dependencies(CMCLSupport CMCLTranslationCodeGeneration)
target_include_directories(CMCLSupport PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
