#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

add_subdirectory(Utils)

set(RESOURCE_EMBEDDER_SCRIPT ${IGC_SOURCE_DIR}/BiFModule/resource_embedder.py)

# Args:
#   RES_LIST - generated list
#   REQUIRED_TARGET - target to link with
#
# Generates include option list for gcc/clang based on the target
# INTERFACE_INCLUDE_DIRECTORIES property.
# The output is "-I<some dir>;-I<some other dir>..."
function(get_target_include_opt_list RES_LIST REQUIRED_TARGET)
  set(INCLUDE_DIRS "$<TARGET_PROPERTY:${REQUIRED_TARGET},INTERFACE_INCLUDE_DIRECTORIES>")
  set(${RES_LIST} "$<$<BOOL:${INCLUDE_DIRS}>:-I$<JOIN:${INCLUDE_DIRS},$<SEMICOLON>-I>>" PARENT_SCOPE)
endfunction()

# Takes CM code compiles to spv then spv is translated to binary LLVM IR,
# binary LLVM IR is encoded as C global char array in generated cpp file.
# Args:
#   RES_FILE - variable name to put generated file path into
#   CMCL_SRC_PATH - path to CM source
#   BIF_NAME - name for all the generated files without extension
#   PTR_BIT_SIZE - bit size of a pointer, 32 or 64 are allowed
#
# Optional arguments:
#   DEPENDS - multivalue. Can be used to establish file-level dependency. Useful
#             if we want to have a dependency from auto-generated files that
#             are created by some other target(s).
#
# C global array name is concatenation of BIF_NAME, PTR_BIT_SIZE and RawData.
# Generated cpp file also has global int with the array size value,
# its name is concatenation of BIF_NAME, PTR_BIT_SIZE and RawData_size.
function(vc_embed_bif RES_FILE CMCL_SRC_PATH BIF_NAME PTR_BIT_SIZE)
  if((NOT (${PTR_BIT_SIZE} EQUAL 32)) AND (NOT (${PTR_BIT_SIZE} EQUAL 64)))
    message(FATAL_ERROR "vc_embed_bif: wrong argument: PTR_BIT_SIZE = ${PTR_BIT_SIZE}: ptr size can only be 32 or 64")
  endif()
  set(MANGLED_BIF_NAME ${BIF_NAME}${PTR_BIT_SIZE})
  set(BIF_CLANG_BC_NAME ${MANGLED_BIF_NAME}.clang.bc)
  set(BIF_CLANG_BC_PATH ${CMAKE_CURRENT_BINARY_DIR}/${BIF_CLANG_BC_NAME})
  set(BIF_CMCL_BC_NAME ${MANGLED_BIF_NAME}.cmcl.bc)
  set(BIF_CMCL_BC_PATH ${CMAKE_CURRENT_BINARY_DIR}/${BIF_CMCL_BC_NAME})
  set(BIF_OPT_BC_NAME ${MANGLED_BIF_NAME}.opt.bc)
  set(BIF_OPT_BC_PATH ${CMAKE_CURRENT_BINARY_DIR}/${BIF_OPT_BC_NAME})
  set(BIF_CPP_NAME ${MANGLED_BIF_NAME}.cpp)
  set(BIF_CPP_PATH ${CMAKE_CURRENT_BINARY_DIR}/${BIF_CPP_NAME})
  set(BIF_SYMBOL ${MANGLED_BIF_NAME}RawData)

  cmake_parse_arguments(PARSE_ARGV 4
                        EXTRA
                        ""
                        ""
                        "DEPENDS")

  if(EXTRA_DEPENDS)
    message("vc_embed_bif - ${MANGLED_BIF_NAME} has extra dependencies: "
            "${EXTRA_DEPENDS}")
  endif()

  get_target_include_opt_list(CMCL_INCLUDES CMCLHeaders)
  get_target_include_opt_list(VC_INCLUDES VCHeaders)
  get_target_include_opt_list(UTILS_HEADERS VCBifUtils)

  set(SPIR_TARGET spir)
  if(PTR_BIT_SIZE EQUAL 64)
    set(SPIR_TARGET spir64)
  endif()

  add_custom_command(OUTPUT ${BIF_CLANG_BC_PATH}
    COMMAND clang-tool -cc1 ${CMCL_INCLUDES} ${VC_INCLUDES} ${UTILS_HEADERS}
    -x cl -cl-std=clc++ -triple=${SPIR_TARGET}
    -O2 -disable-llvm-passes -emit-llvm-bc -o ${BIF_CLANG_BC_NAME} ${CMCL_SRC_PATH}
    COMMENT "vc_embed_bif: Compiling CMCL source ${CMCL_SRC_PATH} to BC ${BIF_CLANG_BC_NAME}"
    DEPENDS clang-tool ${CMCL_SRC_PATH} ${EXTRA_DEPENDS}
    COMMAND_EXPAND_LISTS)
  add_custom_command(OUTPUT ${BIF_CMCL_BC_PATH}
    COMMAND CMCLTranslatorTool -o ${BIF_CMCL_BC_NAME} ${BIF_CLANG_BC_NAME}
    COMMENT "vc_embed_bif: Translating CMCL builtins: ${BIF_CLANG_BC_NAME} -> ${BIF_CMCL_BC_NAME}"
    DEPENDS CMCLTranslatorTool ${BIF_CLANG_BC_PATH})
  add_custom_command(OUTPUT ${BIF_OPT_BC_PATH}
    COMMAND opt --O2 -o ${BIF_OPT_BC_NAME} ${BIF_CMCL_BC_NAME}
    COMMENT "vc_embed_bif: running opt with O2: ${BIF_CMCL_BC_NAME} -> ${BIF_OPT_BC_NAME}"
    DEPENDS opt ${BIF_CMCL_BC_PATH})
  add_custom_command(
    OUTPUT ${BIF_CPP_PATH}
    COMMAND ${PYTHON_EXECUTABLE} ${RESOURCE_EMBEDDER_SCRIPT} ${BIF_OPT_BC_NAME} ${BIF_CPP_NAME}
            ${BIF_SYMBOL} no_attr
    COMMENT "vc_embed_bif: encoding LLVM IR as C array data: ${BIF_OPT_BC_NAME} -> ${BIF_CPP_NAME}"
    DEPENDS ${PYTHON_EXECUTABLE} ${RESOURCE_EMBEDDER_SCRIPT} ${BIF_OPT_BC_PATH})

  set(${RES_FILE} ${BIF_CPP_PATH} PARENT_SCOPE)
endfunction()

if (IGC_OPTION__VC_DISABLE_BIF)
  target_compile_definitions(VCHeaders INTERFACE IGC_VC_DISABLE_BIF)
# Adding it just to get binary dir property in lit and eventually xfail.
  add_custom_target(VCBiFPreparation)
else()

  set(PRINTF_NOT_CM_COMMON_H_PATH ${CMAKE_CURRENT_SOURCE_DIR}/printf_not_cm_common.h)
  set(PRINTF_OCL_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/printf_ocl_genx.cpp)
  set(EMU_DIVREM_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/emulation_divrem.cpp)

  vc_embed_bif(PRINTF_OCL_32_CPP_PATH ${PRINTF_OCL_SRC_PATH} VCBiFPrintfOCL 32
               DEPENDS ${PRINTF_NOT_CM_COMMON_H_PATH})
  vc_embed_bif(PRINTF_OCL_64_CPP_PATH ${PRINTF_OCL_SRC_PATH} VCBiFPrintfOCL 64
               DEPENDS ${PRINTF_NOT_CM_COMMON_H_PATH})

  get_target_property(DIVREM_EXTRA_SOURCES VCEmuDivremBoilerplate SOURCES)
  vc_embed_bif(EMU_DIVREM_CPP_PATH ${EMU_DIVREM_SRC_PATH} VCEmulation 64
               DEPENDS "${DIVREM_EXTRA_SOURCES}")

  add_custom_target(VCBiFPreparation
    DEPENDS ${PRINTF_OCL_32_CPP_PATH}
            ${PRINTF_OCL_64_CPP_PATH}
            ${EMU_DIVREM_CPP_PATH}
    SOURCES ${PRINTF_OCL_SRC_PATH}
            ${EMU_DIVREM_SRC_PATH})

  add_library(VCEmbeddedBiF STATIC
    ${PRINTF_OCL_32_CPP_PATH}
    ${PRINTF_OCL_64_CPP_PATH}
    ${EMU_DIVREM_CPP_PATH})

  add_dependencies(VCBiFPreparation VCBifUtils)
  add_dependencies(VCEmbeddedBiF VCBiFPreparation)
  target_link_libraries(VCEmbeddedBiF VCHeaders)
endif()

