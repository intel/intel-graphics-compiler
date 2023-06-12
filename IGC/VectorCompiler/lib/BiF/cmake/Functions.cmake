#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021-2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

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

# Takes CMCL source code and compiles it to LLVM bitcode.
# The compilation flow is as follows:
#   1. Each source file is processed by clang tool to obtain not optimized
#      bitcode.
#   2. If several source files are specified - the resulting bitcode files are
#      linked together (via llvm-link).
#   3. CMCL translator is executed (via CMCLTranslatorTool).
#   4. "O2" optimizations are run (via opt tool).
# Args:
#   RES_FILE - variable name to put generated file path into.
#   CMCL_SRC_PATH - multivalue. path to CMCL sources.
#   BIF_NAME - name for all the generated files without extension.
#   PTR_BIT_SIZE - bit size of a pointer, 32 or 64 are allowed.
#
# Optional arguments:
#   CLANG_INCLUDES - Argument representing extra include directories passed to
#                    clang.
#   CLANG_FLAGS - Argument representing extra flags used for clang invocation.
#   DEPENDS - multivalue. Can be used to establish file-level dependency.
#             Useful if we want to have a dependency from auto-generated files
#             that are created by some other target(s).
function(vc_build_bif TARGET RES_FILE CMCL_SRC_PATH BIF_NAME PTR_BIT_SIZE)
  cmake_parse_arguments(PARSE_ARGV 4
                        EXTRA
                        ""
                        "CLANG_INCLUDES;CLANG_FLAGS"
                        "DEPENDS")

  if((NOT (${PTR_BIT_SIZE} EQUAL 32)) AND (NOT (${PTR_BIT_SIZE} EQUAL 64)))
    message(FATAL_ERROR "vc_build_bif: wrong argument: PTR_BIT_SIZE = ${PTR_BIT_SIZE}: ptr size can only be 32 or 64")
  endif()
  set(MANGLED_BIF_NAME ${BIF_NAME}${PTR_BIT_SIZE})
  set(BIF_CLANG_BC_NAME_FINAL ${MANGLED_BIF_NAME}.clang.bc)
  set(BIF_CLANG_BC_PATH_FINAL ${CMAKE_CURRENT_BINARY_DIR}/${BIF_CLANG_BC_NAME_FINAL})
  set(BIF_CMCL_BC_NAME ${MANGLED_BIF_NAME}.cmcl.bc)
  set(BIF_CMCL_BC_PATH ${CMAKE_CURRENT_BINARY_DIR}/${BIF_CMCL_BC_NAME})
  set(BIF_OPT_BC_NAME ${MANGLED_BIF_NAME}.opt.bc)
  set(BIF_OPT_BC_PATH ${CMAKE_CURRENT_BINARY_DIR}/${BIF_OPT_BC_NAME})

  if(EXTRA_DEPENDS)
    message("vc_build_bif - ${MANGLED_BIF_NAME} has extra dependencies: "
            "${EXTRA_DEPENDS}")
  endif()

  get_target_include_opt_list(CMCL_INCLUDES CMCLHeaders)
  get_target_include_opt_list(VC_INCLUDES VCHeaders)

  set(SPIR_TARGET spir)
  if(PTR_BIT_SIZE EQUAL 64)
    set(SPIR_TARGET spir64)
  endif()
  set(BC_PATH_LIST "")
  list(LENGTH CMCL_SRC_PATH LENGTH_CMCL_SRC_PATH)

  foreach(CMCL_SRC IN LISTS CMCL_SRC_PATH)
    if (NOT IS_ABSOLUTE ${CMCL_SRC})
      set(CMCL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/${CMCL_SRC}")
    endif()
    file(RELATIVE_PATH SRC_NAME ${CMAKE_CURRENT_SOURCE_DIR} ${CMCL_SRC})
    if (LENGTH_CMCL_SRC_PATH GREATER 1)
      set(BIF_CLANG_BC_NAME ${SRC_NAME}.${PTR_BIT_SIZE}.clang.bc)
    else()
      set(BIF_CLANG_BC_NAME ${BIF_CLANG_BC_NAME_FINAL})
    endif()
    set(BIF_CLANG_BC_PATH ${CMAKE_CURRENT_BINARY_DIR}/${BIF_CLANG_BC_NAME})
    get_filename_component(BIF_CLANG_BC_PATH_DIR ${BIF_CLANG_BC_PATH} DIRECTORY)

    add_custom_command(OUTPUT "${BIF_CLANG_BC_PATH}"
      COMMAND ${CMAKE_COMMAND} -E make_directory ${BIF_CLANG_BC_PATH_DIR}
      COMMAND clang-tool -cc1 ${CMCL_INCLUDES} ${VC_INCLUDES}
               ${EXTRA_CLANG_INCLUDES} ${IGC_LLVM_DEPENDENT_CLANG_FLAGS} ${EXTRA_CLANG_FLAGS}
               -x cl -cl-std=clc++ -triple=${SPIR_TARGET}
               -O2 -disable-llvm-passes -emit-llvm-bc -o "${BIF_CLANG_BC_NAME}" ${CMCL_SRC}
      COMMENT "vc_build_bif: Compiling CMCL source ${CMCL_SRC} to BC ${BIF_CLANG_BC_NAME}"
      DEPENDS clang-tool "${CMCL_SRC}" ${EXTRA_DEPENDS}
      COMMAND_EXPAND_LISTS)
      list(APPEND BC_PATH_LIST ${BIF_CLANG_BC_PATH})
  endforeach()

  if (LENGTH_CMCL_SRC_PATH GREATER 1)
    add_custom_command(OUTPUT ${BIF_CLANG_BC_PATH_FINAL}
      COMMAND ${LLVM_LINK_EXE} ${BC_PATH_LIST}  -o ${BIF_CLANG_BC_NAME_FINAL}
      COMMENT "vc_build_bif: Link ${BC_PATH_LIST}  together to BC ${BIF_CLANG_BC_NAME_FINAL}"
      DEPENDS ${LLVM_LINK_EXE} ${BC_PATH_LIST}
      COMMAND_EXPAND_LISTS)
  endif()

  add_custom_target(${TARGET}
    COMMENT "vc_build_bif: Translating CMCL builtins:  ${BIF_CLANG_BC_NAME_FINAL} -> ${BIF_OPT_BC_NAME}"
    COMMAND CMCLTranslatorTool -o ${BIF_CMCL_BC_NAME} ${BIF_CLANG_BC_NAME_FINAL}
    COMMAND ${LLVM_OPT_EXE} ${IGC_LLVM_DEPENDENT_OPT_FLAGS} --O2 -o ${BIF_OPT_BC_NAME} ${BIF_CMCL_BC_NAME}
    DEPENDS CMCLTranslatorTool ${LLVM_OPT_EXE} ${BIF_CLANG_BC_PATH_FINAL}
    BYPRODUCTS ${BIF_OPT_BC_PATH}
    SOURCES ${CMCL_SRC_PATH})
  set(${RES_FILE} ${BIF_OPT_BC_NAME} PARENT_SCOPE)
endfunction()

# Takes binary data as an input (LLVM bitcode is expected) and converts input
# to an embeddable C(C++) source code.
# The data is encoded as a global C array.
#   The name has the following structure: {BIF_NAME, PTR_BIT_SIZE, "RawData" }.
# The size of array is stored as a global int.
#   The name for the variable is: {BIF_NAME, PTR_BIT_SIZE, "RawData_size" }.
# Args:
#   RES_FILE - variable name to put generated file path into.
#   BIF_OPT_BC_NAME  - path to the binary data that needs to be embedded.
#   MANGLED_BIF_NAME - the desired name for the embeddable source file.
# Path to resulting CPP source code is stored in the specified cmake variable.
function(vc_generate_embeddable_source TARGET RES_FILE BIF_OPT_BC_NAME MANGLED_BIF_NAME)
  set(BIF_CPP_NAME ${MANGLED_BIF_NAME}.cpp)
  set(BIF_CPP_PATH ${CMAKE_CURRENT_BINARY_DIR}/${BIF_CPP_NAME})
  set(BIF_OPT_BC_PATH ${CMAKE_CURRENT_BINARY_DIR}/${BIF_OPT_BC_NAME})
  set(BIF_SYMBOL ${MANGLED_BIF_NAME}RawData)

  add_custom_target(${TARGET}
    COMMENT "vc_generate_embeddable_source: encoding LLVM IR as C array data: ${BIF_OPT_BC_NAME} -> ${BIF_CPP_NAME}"
    COMMAND ${PYTHON_EXECUTABLE} ${RESOURCE_EMBEDDER_SCRIPT} ${BIF_OPT_BC_NAME} ${BIF_CPP_NAME} ${BIF_SYMBOL} no_attr
    DEPENDS ${PYTHON_EXECUTABLE} ${RESOURCE_EMBEDDER_SCRIPT} ${BIF_OPT_BC_PATH}
    BYPRODUCTS ${BIF_CPP_PATH})
  set(${RES_FILE} ${BIF_CPP_PATH} PARENT_SCOPE)
endfunction()

# Takes generic library as an input (in a form of LLVM bitcode) and
# generates code that allows VC compiler to obtain platform-specific
# precompiled library that can be consumed by VC BuiltinFunctions pass.
# This is done using the following steps:
# 1. Generic library is pre-compiled by *vcb* tool for every known architecture
#    supported by VC backend (unnecessary functions are discarded in the
#    process).
# 2. Generates code that allows VC backend to obtain a pre-combiled LLVM
#    bitcode for the specified architecture during runtime. Pre-compiled
#    bitcodes are stored internally as a C array. Bitcodes corresponding to
#    different architectures that have the same binary representation
#    (in a form of LLVM IR) are coalesced to reduce the size. This step is
#    done by VCB tool that is invoked with "-BiFUnique" argument.
# Args:
#   RES_FILE - variable name to put generated file path into.
#   BIF_OPT_BC_NAME  - path to generic library (in a form of bitcode).
#   MANGLED_BIF_NAME - the desired name for the generated source file.
# Path to resulting CPP source code is stored in the specified cmake variable.
function(vc_generate_optimized_bif TARGET RES_FILE BIF_OPT_BC_NAME MANGLED_BIF_NAME)
  set(BIF_CPP_NAME ${MANGLED_BIF_NAME}.cpp)
  set(BIF_CPP_PATH ${CMAKE_CURRENT_BINARY_DIR}/${BIF_CPP_NAME})
  set(BIF_OPT_BC_PATH ${CMAKE_CURRENT_BINARY_DIR}/${BIF_OPT_BC_NAME})
  set(BIF_SYMBOL ${MANGLED_BIF_NAME}RawData)
  set(BIF_CONF_NAME "${MANGLED_BIF_NAME}.conf")
  set(BIF_CONF_PATH  ${CMAKE_CURRENT_BINARY_DIR}/${BIF_CONF_NAME})
  set(PLTF_BC_PATH_LIST "")

  foreach(PLTF IN LISTS SUPPORTED_VC_PLATFORMS)
    set(PLTF_BC_NAME "${MANGLED_BIF_NAME}_${PLTF}.vccg.bc")
    set(PLTF_BC_PATH ${CMAKE_CURRENT_BINARY_DIR}/${PLTF_BC_NAME})
    add_custom_target("${TARGET}-${PLTF}-BC"
      COMMENT "vc_generate_optimized_bif: compile optimized BiF for ${PLTF}"
      COMMAND ${VCB_EXE} -o ${PLTF_BC_NAME} -cpu ${PLTF} ${BIF_OPT_BC_NAME}
      DEPENDS ${VCB_EXE} ${BIF_OPT_BC_PATH}
      BYPRODUCTS ${PLTF_BC_NAME})
    list(APPEND PLTF_BC_PATH_LIST ${PLTF_BC_PATH})
  endforeach()

  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/builtins.conf.in"
      "${BIF_CONF_PATH}" @ONLY)
  add_custom_target(${TARGET}
      COMMENT "vc_generate_optimized_bif: create hashed version of optimized functions"
      COMMAND ${VCB_EXE} -BiFUnique -symb ${BIF_SYMBOL} -o ${BIF_CPP_NAME} ${BIF_CONF_NAME}
      DEPENDS ${VCB_EXE} ${BIF_CONF_PATH} ${PLTF_BC_PATH_LIST}
      BYPRODUCTS ${BIF_CPP_PATH})
  set(${RES_FILE} ${BIF_CPP_PATH} PARENT_SCOPE)
endfunction()

# Takes CMCL source code, compiles it and produces an embeddable .cpp file
# containing the resulting LLVM bitcode (as a C array).
# See vc_build_bif and vc_generate_embeddable_source for more details.
# Path to resulting CPP source code is stored in the specified cmake variable.
function(vc_embed_bif RES_FILE CMCL_SRC_PATH BIF_NAME PTR_BIT_SIZE)

    cmake_parse_arguments(PARSE_ARGV 4
                          EXTRA
                          ""
                          "CLANG_INCLUDES;CLANG_FLAGS"
                          "DEPENDS")
    set(MANGLED_BIF_NAME ${BIF_NAME}${PTR_BIT_SIZE})
    vc_build_bif("${BIF_NAME}${PTR_BIT_SIZE}-BC" RES_FILE_VC_EMBED "${CMCL_SRC_PATH}" "${BIF_NAME}" "${PTR_BIT_SIZE}"
        CLANG_INCLUDES "${EXTRA_CLANG_INCLUDES}"
        CLANG_FLAGS "${EXTRA_CLANG_FLAGS}"
        DEPENDS "${EXTRA_DEPENDS}")
    vc_generate_embeddable_source("${BIF_NAME}${PTR_BIT_SIZE}" RES_FILE_EMBED_GEN "${RES_FILE_VC_EMBED}" "${MANGLED_BIF_NAME}")
    set(${RES_FILE} ${RES_FILE_EMBED_GEN} PARENT_SCOPE)
endfunction()

# Takes a list of source files that represent generic emulation library
# and generates source code that allows VC to obtain pre-compiled
# platform-optimized version of the emulation library during the runtime.
# See vc_build_bif and vc_generate_optimized_bif for details
# Path to resulting CPP source code is stored in the specified cmake variable.
function(vc_embed_optimized_bif RES_FILE CMCL_SRC_PATH BIF_NAME PTR_BIT_SIZE)
    set(MANGLED_BIF_NAME ${BIF_NAME}${PTR_BIT_SIZE})
    vc_build_bif("${BIF_NAME}${PTR_BIT_SIZE}-BC" RES_FILE_VC_EMBED "${CMCL_SRC_PATH}" "${BIF_NAME}" "${PTR_BIT_SIZE}")
    vc_generate_optimized_bif("${BIF_NAME}${PTR_BIT_SIZE}" RES_FILE_EMBED_GEN "${RES_FILE_VC_EMBED}"
        "${MANGLED_BIF_NAME}")
    set(${RES_FILE} ${RES_FILE_EMBED_GEN} PARENT_SCOPE)
endfunction()
