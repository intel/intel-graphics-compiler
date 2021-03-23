#=========================== begin_copyright_notice ============================
#
# Copyright (c) 2021 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
#============================ end_copyright_notice =============================

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

  get_target_include_opt_list(CMCL_INCLUDES CMCLHeaders)

  set(SPIR_TARGET spir)
  if(PTR_BIT_SIZE EQUAL 64)
    set(SPIR_TARGET spir64)
  endif()

  add_custom_command(OUTPUT ${BIF_CLANG_BC_PATH}
    COMMAND clang-tool -cc1 ${CMCL_INCLUDES} -x cl -cl-std=clc++ -triple=${SPIR_TARGET}
    -O2 -disable-llvm-passes -emit-llvm-bc -o ${BIF_CLANG_BC_NAME} ${CMCL_SRC_PATH}
    COMMENT "vc_embed_bif: Compiling CMCL source ${CMCL_SRC_PATH} to BC ${BIF_CLANG_BC_NAME}"
    DEPENDS clang-tool ${CMCL_SRC_PATH}
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
else()
  set(PRINTF_OCL_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/printf_ocl_genx.cpp)
  vc_embed_bif(PRINTF_OCL_32_CPP_PATH ${PRINTF_OCL_SRC_PATH} VCBiFPrintfOCL 32)
  vc_embed_bif(PRINTF_OCL_64_CPP_PATH ${PRINTF_OCL_SRC_PATH} VCBiFPrintfOCL 64)

  add_custom_target(VCBiFPreparation
    DEPENDS ${PRINTF_OCL_32_CPP_PATH}
            ${PRINTF_OCL_64_CPP_PATH}
    SOURCES ${PRINTF_OCL_SRC_PATH})

  add_library(VCEmbeddedBiF STATIC
    ${PRINTF_OCL_32_CPP_PATH}
    ${PRINTF_OCL_64_CPP_PATH})
  add_dependencies(VCEmbeddedBiF VCBiFPreparation)
  target_link_libraries(VCEmbeddedBiF VCHeaders)
endif()

