#===================== begin_copyright_notice ==================================

#Copyright (c) 2017 Intel Corporation

#Permission is hereby granted, free of charge, to any person obtaining a
#copy of this software and associated documentation files (the
#"Software"), to deal in the Software without restriction, including
#without limitation the rights to use, copy, modify, merge, publish,
#distribute, sublicense, and/or sell copies of the Software, and to
#permit persons to whom the Software is furnished to do so, subject to
#the following conditions:

#The above copyright notice and this permission notice shall be included
#in all copies or substantial portions of the Software.

#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#======================= end_copyright_notice ==================================


# ======================================================================================================
# ================================================ UTILS ===============================================
# ======================================================================================================

# =============================================== Tools ================================================

# In new Clang versions VME types are built-in. Keep this flag until all OS's update Clang version to 8
if(NOT DEFINED VME_TYPES_DEFINED)
  set(VME_TYPES_DEFINED TRUE)
endif()

# Order of chosing way how to take opencl-clang
#1. CCLANG_FROM_SYSTEM - use installed on system opencl-clang toolchain
#2. CCLANG_BUILD_PREBUILDS - use prebuilded opencl-clang toolchain
#   CCLANG_BUILD_PREBUILDS_DIR - set path to prebuilt cclang folder
#3. CCLANG_BUILD_INTREE_LLVM - use sources of opencl-clang toolchain

find_library(CCLANG_FROM_SYSTEM ${COMMON_CLANG_LIBRARY_NAME})

### Check if user manual setup some of flag
if(NOT CCLANG_BUILD_PREBUILDS)
  set(CCLANG_BUILD_PREBUILDS FALSE)
else()
  set(CCLANG_FROM_SYSTEM FALSE)
  set(CCLANG_BUILD_INTREE_LLVM FALSE)
endif()

if(NOT CCLANG_BUILD_INTREE_LLVM)
  set(CCLANG_BUILD_INTREE_LLVM FALSE)
else()
  set(CCLANG_BUILD_PREBUILDS FALSE)
  set(CCLANG_FROM_SYSTEM FALSE)
endif()
###

### Check by order first available way to link with opencl-clang
if(NOT CCLANG_FROM_SYSTEM)
  if(NOT CCLANG_BUILD_PREBUILDS_DIR)
    # Detect CPU architecture
    igc_arch_get_cpu(cpuSuffix)
    set(CCLANG_BUILD_PREBUILDS_DIR "${IGC_BUILD__GFX_DEV_SRC_DIR}/../../prebuild-opencl-clang/Release/${cpuSuffix}/")

    if(NOT EXISTS ${CCLANG_BUILD_PREBUILDS_DIR})
      set(CCLANG_BUILD_PREBUILDS_DIR "${IGC_BUILD__GFX_DEV_SRC_DIR}/../prebuild-opencl-clang/Release/${cpuSuffix}/")
    endif()
  endif()
  set(CCLANG_BUILD_INTREE_LLVM_DIR ${LLVM_SOURCE_DIR}/projects/opencl-clang)

  ### Check if user by choosing some way of linking with opencl-clang provided required folders
  if(${CCLANG_BUILD_PREBUILDS})
    if(NOT EXISTS ${CCLANG_BUILD_PREBUILDS_DIR})
      message(FATAL_ERROR "[IGC\\BiFModule] : User setup to use prebuilded opencl-clang but not found folder : ${CCLANG_BUILD_PREBUILDS_DIR}")
      set(CCLANG_BUILD_PREBUILDS FALSE)
    endif()
  elseif(${CCLANG_BUILD_INTREE_LLVM})
    if(NOT EXISTS ${CCLANG_BUILD_INTREE_LLVM_DIR})
      message(FATAL_ERROR "[IGC\\BiFModule] : User setup to use sources of opencl-clang but not found folder : ${CCLANG_BUILD_INTREE_LLVM_DIR}")
      set(CCLANG_BUILD_INTREE_LLVM FALSE)
    endif()
  ###

  ### User didn't define nothing, then pick the method 2 or 3.
  elseif(EXISTS ${CCLANG_BUILD_PREBUILDS_DIR})
    set(CCLANG_BUILD_PREBUILDS TRUE)
  elseif(EXISTS ${CCLANG_BUILD_INTREE_LLVM_DIR})
    message(STATUS "[IGC\\BiFModule] : opencl-clang will be taken from sources")
    set(CCLANG_BUILD_INTREE_LLVM TRUE)
  else()
    message(FATAL_ERROR "[IGC\\BiFModule] : Cannot find opencl-clang tool-chain, please provide sources or install it on system.")
  endif()
  ###

endif()
###

#1. CCLANG_FROM_SYSTEM - use installed on system opencl-clang toolchain
if(CCLANG_FROM_SYSTEM)
  message(STATUS "[IGC\\BiFModule] : opencl-clang will be taken from system")

  find_library(SYSTEM_COMMON_CLANG ${COMMON_CLANG_LIBRARY_NAME})

  add_library(opencl-clang-lib SHARED IMPORTED GLOBAL)
  set_property(TARGET opencl-clang-lib PROPERTY "IMPORTED_LOCATION" "${SYSTEM_COMMON_CLANG}")
  find_program(CLANG_GE7 clang-${LLVM_VERSION_MAJOR})
  if(CLANG_GE7)
    message(STATUS "[IGC\\BiFModule] Found clang-${LLVM_VERSION_MAJOR} executable: ${CLANG_GE7}")

    add_executable(clang-tool IMPORTED GLOBAL)
    set_property(TARGET clang-tool PROPERTY "IMPORTED_LOCATION" "${CLANG_GE7}")
    set(CL_OPTIONS "-finclude-default-header")
    if(LLVM_VERSION_MAJOR VERSION_EQUAL 7)
      message(WARNING "[IGC\\BiFModule] : clang-7 should be patched with VME patch (https://reviews.llvm.org/D51484). Assuming that it is. If not, please add -DVME_TYPES_DEFINED=FALSE.")
    endif()
  else(CLANG_GE7)
    message(FATAL_ERROR "[IGC\\BiFModule] : Couldn't find clang-${LLVM_VERSION_MAJOR} executable, please install it.")
  endif(CLANG_GE7)
###
#2. CCLANG_BUILD_PREBUILDS - use prebuilded opencl-clang toolchain
elseif(${CCLANG_BUILD_PREBUILDS})
  message(STATUS "[IGC\\BiFModule] : opencl-clang will be taken from prebuilds")

  set(CLANG_TOOL_PATH "${CCLANG_BUILD_PREBUILDS_DIR}/clang${CMAKE_EXECUTABLE_SUFFIX}")
  set(LLVM_PACKAGE_VERSION "${LLVM_VERSION_MAJOR}.${LLVM_VERSION_MINOR}.${LLVM_VERSION_PATCH}${LLVM_VERSION_SUFFIX}")

  # Get clang-tool version
  execute_process(
    COMMAND ${CLANG_TOOL_PATH} -v
    ERROR_VARIABLE CLANG_TOOL_V_CALL)
  string(REGEX MATCH "clang version ([0-9]*\\.[0-9]*\\.[0-9]*[a-zA-Z0-9]*) " CLANG_TOOL_VERSION "${CLANG_TOOL_V_CALL}")
  set(CLANG_TOOL_VERSION "${CMAKE_MATCH_1}")

  # Check if llvm version for IGC is newer or equal with the clang-tool version
  if(${LLVM_PACKAGE_VERSION} VERSION_GREATER ${CLANG_TOOL_VERSION} OR
     ${LLVM_PACKAGE_VERSION} EQUAL ${CLANG_TOOL_VERSION})
    add_library(opencl-clang-lib SHARED IMPORTED GLOBAL)
    set_property(TARGET opencl-clang-lib PROPERTY "IMPORTED_LOCATION" "${CCLANG_BUILD_PREBUILDS_DIR}/${COMMON_CLANG_LIB_FULL_NAME}")

    add_executable(clang-tool IMPORTED GLOBAL)
    set_property(TARGET clang-tool PROPERTY "IMPORTED_LOCATION" "${CLANG_TOOL_PATH}")

    set(opencl-header "${CCLANG_BUILD_PREBUILDS_DIR}/opencl-c.h")
  else()
    message(FATAL_ERROR "[IGC\\BiFModule] : The clang-tool(${CLANG_TOOL_VERSION}) from prebuilts is newer than llvm(${LLVM_PACKAGE_VERSION}) version for IGC.")
  endif()
###
#3. CCLANG_BUILD_INTREE_LLVM - use sources of opencl-clang toolchain
elseif(${CCLANG_BUILD_INTREE_LLVM})
  message(STATUS "[IGC\\BiFModule] : opencl-clang will be taken from sources")

  add_library(opencl-clang-lib ALIAS ${COMMON_CLANG_LIBRARY_NAME})
  add_executable(clang-tool ALIAS clang)
  set(opencl-header "${LLVM_SOURCE_DIR}/tools/clang/lib/Headers/opencl-c.h")
endif()
###

if(UNIX)
  if(NOT ${CCLANG_BUILD_INTREE_LLVM})
    # Get common clang library soname
    get_target_property(CCLANG_LIB_PATH opencl-clang-lib "IMPORTED_LOCATION")
    execute_process(
      COMMAND readelf -d ${CCLANG_LIB_PATH}
      OUTPUT_VARIABLE CCLANG_READELF_CALL)
    string(REGEX MATCH "\\[${COMMON_CLANG_LIB_FULL_NAME}\\.([0-9](\\.[0-9]*)*[a-zA-Z0-9]*)\\]" CCLANG_SONAME_VERSION "${CCLANG_READELF_CALL}")
    set(CCLANG_SONAME_VERSION "${CMAKE_MATCH_1}")

    # Check if common clang library is newer than 5.0.0 version on which we have SPIR-V support
    if("${CCLANG_SONAME_VERSION}" VERSION_GREATER "5.0.0")
      set_property(TARGET opencl-clang-lib PROPERTY "IMPORTED_SONAME" "${COMMON_CLANG_LIB_FULL_NAME}.${CCLANG_SONAME_VERSION}")
    else()
      message(FATAL_ERROR "[IGC\\BiFModule] : Version ${CCLANG_SONAME_VERSION} of library ${COMMON_CLANG_LIB_FULL_NAME} is below version 5.0.0 (where it's starts support of SPIR-V), please upgrade this library at least to version 5.0.0")
    endif()
  endif()
  if (NOT CCLANG_FROM_SYSTEM)
    install(FILES $<TARGET_FILE:opencl-clang-lib> DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} COMPONENT igc-opencl)
    install(FILES $<TARGET_SONAME_FILE:opencl-clang-lib> DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR} COMPONENT igc-opencl)

    # Get header opencl-c.h directory
    get_filename_component(opencl-headers-dir ${opencl-header} DIRECTORY)

    set(opencl-headers "${opencl-header}")
    # Fix for the split of opencl-c.h into opencl-c.h and opencl-c-base.h
    # https://github.com/llvm/llvm-project/commit/af1c230e70aeb72ec9d6363f8d91e3c7db3ef9f2
    if(EXISTS "${opencl-headers-dir}/opencl-c-base.h")
      set(opencl-headers
       "${opencl-headers}"
       "${opencl-headers-dir}/opencl-c-base.h")
    endif()
    install(FILES ${opencl-headers} DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR} COMPONENT igc-opencl-devel)
  endif()
endif()

# End Clang section


# ========================================= Helper functions ===========================================

# Creates custom commands which build precompiled header for built-in functions.
#
# igc_bif_build_pch(
#     OUTPUT    outPchFilePath
#     TRIPLE    archTriple
#     HEADERS   srcHeaderFilePath [srcHeaderFilePath [...]]
#     [DEFINES  define [define [...]]]
#     [CHAINED  chainedPchFilePath]
#     [OPTIONS  [option [option [...]]]]
#   )
#
# @param outPchFilePath     Full path where output precompiled header should be written.
#                           Output file is only updated if it does not exist or it was changed.
# @param archTriple         Identifier for target architecture for compiled header.
# @param srcHeaderFilePath  Path to source header which will be precompiled (multiple can be specified).
# @param define             Additional preprocessor definition (multiple can be specified).
# @param chainedPchFilePath Path to precompiled header which will be chained with currently created
#                           precompiled header.
# @param option             Additional compilation option (multiple can be specified).
function(igc_bif_build_pch)
  set(_defines)
  set(_chainedPchFilePath)
  set(_chainedPchFilePathSet NO)
  set(_options)

  set(_parseState 0)
  foreach(_compileArg ${ARGN})
    string(REPLACE ";" "\;" _compileArg "${_compileArg}")

    # States: [0] "OUTPUT" [1] <param> [2] "TRIPLE" [3] <param> [4] "HEADERS" [5] <param> [6] *<param> [6]
    #         *1( "DEFINES" [7] <param> [8] *<param> [8] ) *1( "CHAINED" [9] <param> [10] ) *1( "OPTIONS" [11] *<param> [11] )
    # Transitions: 0 (OUTPUT) -> 1 -> 2 (TRIPLE) -> 3 -> 4 (HEADERS) -> 5 -> 6
    #              6 (DEFINES) -> 7 -> 8
    #              6 (CHAINED) -> 9 -> 10
    #              6 (OPTIONS) -> 11
    #              6 -> 6
    #              8 (CHAINED) -> 9 -> 10
    #              8 (OPTIONS) -> 11
    #              8 -> 8
    #              10 (OPTIONS) -> 11
    #              11 -> 11
    # Stop States: 6, 8, 10, 11
    if((_parseState EQUAL 0) AND (_compileArg MATCHES "^OUTPUT$"))
      set(_parseState 1)
    elseif(_parseState EQUAL 1)
      set(_outPchFilePath "${_compileArg}")
      set(_parseState 2)
    elseif((_parseState EQUAL 2) AND (_compileArg MATCHES "^TRIPLE$"))
      set(_parseState 3)
    elseif(_parseState EQUAL 3)
      set(_archTriple "${_compileArg}")
      set(_parseState 4)
    elseif((_parseState EQUAL 4) AND (_compileArg MATCHES "^HEADERS$"))
      set(_parseState 5)
    elseif(_parseState EQUAL 5)
      set(_srcHeaderFilePaths "${_compileArg}")
      set(_parseState 6)
    elseif(_parseState EQUAL 6)
      if(_compileArg MATCHES "^DEFINES$")
        set(_parseState 7)
      elseif(_compileArg MATCHES "^CHAINED$")
        set(_parseState 9)
      elseif(_compileArg MATCHES "^OPTIONS$")
        set(_parseState 11)
      else()
        list(APPEND _srcHeaderFilePaths "${_compileArg}")
      endif()
    elseif(_parseState EQUAL 7)
      set(_defines "${_compileArg}")
      set(_parseState 8)
    elseif(_parseState EQUAL 8)
      if(_compileArg MATCHES "^CHAINED$")
        set(_parseState 9)
      elseif(_compileArg MATCHES "^OPTIONS$")
        set(_parseState 11)
      else()
        list(APPEND _defines "${_compileArg}")
      endif()
    elseif(_parseState EQUAL 9)
      set(_chainedPchFilePath    "${_compileArg}")
      set(_chainedPchFilePathSet YES)
      set(_parseState 10)
    elseif((_parseState EQUAL 10) AND (_compileArg MATCHES "^OPTIONS$"))
      set(_parseState 11)
    elseif(_parseState EQUAL 11)
      list(APPEND _options "${_compileArg}")
    else()
      message(FATAL_ERROR "Invalid parameter token near \"${_compileArg}\".")
    endif()
  endforeach()
  if(NOT ((_parseState EQUAL 6) OR (_parseState EQUAL 8) OR (_parseState EQUAL 10) OR (_parseState EQUAL 11)))
    message(FATAL_ERROR "Invalid number of parameters.")
  endif()


  get_filename_component(_outPchFileName "${_outPchFilePath}" NAME)
  get_filename_component(_outPchFileDir  "${_outPchFilePath}" PATH)
  set(_pchTempFilePath "${_outPchFilePath}_pch.tmp")

  set(_defineFlags)
  foreach(_define ${_defines})
    string(REPLACE ";" "\;" _define "${_define}")
    list(APPEND _defineFlags "-D${_define}")
  endforeach()

  if(_chainedPchFilePathSet)
    set(_chainedPchFlags "-include-pch" "${_chainedPchFilePath}")
  else()
    set(_chainedPchFlags)
  endif()

  # Header precompilation is triggered by CLANG change, change of source headers or change in chained precompiled header.
  add_custom_command(
      OUTPUT "${_pchTempFilePath}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${_outPchFileDir}"
      COMMAND clang-tool -cc1 -x cl -fblocks -fpreserve-vec3-type -Os -opencl-builtins "-triple=${_archTriple}" -cl-std=CL2.0 -emit-pch -o "${_pchTempFilePath}" ${_chainedPchFlags} ${_defineFlags} ${_options} ${_srcHeaderFilePaths}
      DEPENDS clang-tool ${_chainedPchFilePath} ${_srcHeaderFilePaths}
      COMMENT "BiF: Building OpenCL precompiled header: \"${_outPchFileName}\""
    )
  add_custom_command(
      OUTPUT "${_outPchFilePath}"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_pchTempFilePath}" "${_outPchFilePath}"
      DEPENDS "${_pchTempFilePath}"
      COMMENT "BiF: Updating precompiled header \"${_outPchFileName}\" if changed."
    )
endfunction()

# ======================================================================================================

# Creates custom command which builds LLVM Bitcode file for built-in functions.
#
# igc_bif_build_bc(
#     OUTPUT               outBcFilePath
#     TRIPLE               archTriple
#     SOURCES              srcFilePath [srcFilePath [...]]
#     [LANGUAGE            srcLanguage]
#     [PRECOMPILED         pchFilePath]
#     [FORCE_INCLUDE       incFilePath [incFilePath [...]]]
#     [INCLUDE_DIRECTORIES includeDir [includeDir [...]]]
#     [DEFINES             define [define [...]]]
#     [DEPENDS             dependency [dependency [...]]]
#     [OPTIMIZE            useOpt]
#     [OPTIONS             lang [option [...]]]
#     [OPTIONS             lang [option [...]]]
#     [...]
#     [OPTIONS             lang [option [...]]]
#   )
#
# @param outBcFilePath      Full path where output bitcode file should be written.
#                           Output file is only updated if it does not exist or it was changed.
# @param archTriple         Identifier for target architecture for compiled sources.
# @param srcFilePath        Path to source file which will be compiled into .bc (multiple can be specified).
#                           If multiple files are specified, each file is converted to .bc and output .bc
#                           is created by linking intermediate .bc files.
# @param srcLanguage        Language used to compile/build source files. The following are supported:
#                            - "DEFAULT" - determined by extension (.ll -> "LL"; .bc -> "BC"; .* (other) -> "CL").
#                            - "CL"      - Sources treated as OpenCL C sources (CLANG will be used).
#                            - "LL"      - Sources treated as LLVM IR text files (LLVM-AS will be used).
#                            - "BC"      - Sources treated as LLVM Bitcode files (LLVM-LINK will be used).
#                           If not specified, "DEFAULT" is used.
# @param pchFilePath        Includes precompiled header. Ignored for other files than "CL".
# @param incFilePath        Includes header for each compiled source. Ignored for other files than "CL".
# @param includeDir         Adds include directory. Ignored for other files than "CL".
# @param define             Additional preprocessor definition (multiple can be specified). Ignored for other files than "CL".
# @param useOpt             Use optmizer (boolean value). Only final created .bc is optimized (intermediate .bc created during
#                           build will not be put into optimizer).  Default is FALSE.
# @param dependency         Additional files / targets that source files depends on (multiple can be specified).
# @param lang               Language for which options are defined (currently "CL", "LL", "BC" and "OPT" for optimizer;
#                           "DEFAULT" will add options to all lanuages).
# @param option             Additional compilation option (multiple can be specified).
function(igc_bif_build_bc)
  set(__allowedLanguages "DEFAULT" "CL" "LL" "BC")
  set(__allowedOptLanguages ${__allowedLanguages} "OPT")

  set(_srcLanguage "DEFAULT")
  set(_pchFilePath)
  set(_pchFilePathSet NO)
  set(_incFilePaths)
  set(_includeDirs)
  set(_defines)
  set(_dependencies)
  set(_useOpt NO)
  foreach(_allowedOptLang ${__allowedOptLanguages})
    set("_options_${_allowedOptLang}")
  endforeach()

  set(_parseState 0)
  foreach(_compileArg ${ARGN})
    string(REPLACE ";" "\;" _compileArg "${_compileArg}")

    # States: [0] "OUTPUT" [1] <param> [2] "TRIPLE" [3] <param> [4] "SOURCES" [5] <param> [6] *<param> [6]
    #         *1( "LANGUAGE" [7] <param> [8] ) *1( "PRECOMPILED" [9] <param> [10] ) *1( "FORCE_INCLUDE" [11] <param> [12] *<param> [12] )
    #         *1( "INCLUDE_DIRECTORIES" [13] <param> [14] *<param> [14] ) *1( "DEFINES" [15] <param> [16] *<param> [16] ) *1( "DEPENDS" [17] <param> [18] *<param> [18] )
    #         *1( "OPTIMIZE" [19] <param> [20] ) *( "OPTIONS" [21] <param> [22] *<param> [22] )
    # Transitions: 0 (OUTPUT) -> 1 -> 2 (TRIPLE) -> 3 -> 4 (SOURCES) -> 5 -> 6
    #              6 (LANGUAGE) -> 7 -> 8
    #              6 (PRECOMPILED) -> 9 -> 10
    #              6 (FORCE_INCLUDE) -> 11 -> 12
    #              6 (INCLUDE_DIRECTORIES) -> 13 -> 14
    #              6 (DEFINES) -> 15 -> 16
    #              6 (DEPENDS) -> 17 -> 18
    #              6 (OPTIMIZE) -> 19 -> 20
    #              6 (OPTIONS) -> 21 -> 22
    #              6 -> 6
    #              8 (PRECOMPILED) -> 9 -> 10
    #              8 (FORCE_INCLUDE) -> 11 -> 12
    #              8 (INCLUDE_DIRECTORIES) -> 13 -> 14
    #              8 (DEFINES) -> 15 -> 16
    #              8 (DEPENDS) -> 17 -> 18
    #              8 (OPTIMIZE) -> 19 -> 20
    #              8 (OPTIONS) -> 21 -> 22
    #              10 (FORCE_INCLUDE) -> 11 -> 12
    #              10 (INCLUDE_DIRECTORIES) -> 13 -> 14
    #              10 (DEFINES) -> 15 -> 16
    #              10 (DEPENDS) -> 17 -> 18
    #              10 (OPTIMIZE) -> 19 -> 20
    #              10 (OPTIONS) -> 21 -> 22
    #              12 (INCLUDE_DIRECTORIES) -> 13 -> 14
    #              12 (DEFINES) -> 15 -> 16
    #              12 (DEPENDS) -> 17 -> 18
    #              12 (OPTIMIZE) -> 19 -> 20
    #              12 (OPTIONS) -> 21 -> 22
    #              12 -> 12
    #              14 (DEFINES) -> 15 -> 16
    #              14 (DEPENDS) -> 17 -> 18
    #              14 (OPTIMIZE) -> 19 -> 20
    #              14 (OPTIONS) -> 21 -> 22
    #              14 -> 14
    #              16 (DEPENDS) -> 17 -> 18
    #              16 (OPTIMIZE) -> 19 -> 20
    #              16 (OPTIONS) -> 21 -> 22
    #              16 -> 16
    #              18 (OPTIMIZE) -> 19 -> 20
    #              18 (OPTIONS) -> 21 -> 22
    #              18 -> 18
    #              20 (OPTIONS) -> 21 -> 22
    #              22 (OPTIONS) -> 21 -> 22
    #              22 -> 22
    # Stop States: 6, 8, 10, 12, 14, 16, 18, 20, 22
    if((_parseState EQUAL 0) AND (_compileArg MATCHES "^OUTPUT$"))
      set(_parseState 1)
    elseif(_parseState EQUAL 1)
      set(_outBcFilePath "${_compileArg}")
      set(_parseState 2)
    elseif((_parseState EQUAL 2) AND (_compileArg MATCHES "^TRIPLE$"))
      set(_parseState 3)
    elseif(_parseState EQUAL 3)
      set(_archTriple "${_compileArg}")
      set(_parseState 4)
    elseif((_parseState EQUAL 4) AND (_compileArg MATCHES "^SOURCES$"))
      set(_parseState 5)
    elseif(_parseState EQUAL 5)
      set(_srcFilePaths "${_compileArg}")
      set(_parseState 6)
    elseif(_parseState EQUAL 6)
      if(_compileArg MATCHES "^LANGUAGE$")
        set(_parseState 7)
      elseif(_compileArg MATCHES "^PRECOMPILED$")
        set(_parseState 9)
      elseif(_compileArg MATCHES "^FORCE_INCLUDE$")
        set(_parseState 11)
      elseif(_compileArg MATCHES "^INCLUDE_DIRECTORIES$")
        set(_parseState 13)
      elseif(_compileArg MATCHES "^DEFINES$")
        set(_parseState 15)
      elseif(_compileArg MATCHES "^DEPENDS$")
        set(_parseState 17)
      elseif(_compileArg MATCHES "^OPTIMIZE$")
        set(_parseState 19)
      elseif(_compileArg MATCHES "^OPTIONS$")
        set(_parseState 21)
      else()
        list(APPEND _srcFilePaths "${_compileArg}")
      endif()
    elseif(_parseState EQUAL 7)
      list(FIND __allowedLanguages "${_compileArg}" _foundLanguageIdx)
      if(_foundLanguageIdx LESS 0)
        message(FATAL_ERROR "Language \"${_compileArg}\" is not supported. Supported languages: ${__allowedLanguages}")
      endif()
      set(_srcLanguage "${_compileArg}")
      set(_parseState 8)
    elseif(_parseState EQUAL 8)
      if(_compileArg MATCHES "^PRECOMPILED$")
        set(_parseState 9)
      elseif(_compileArg MATCHES "^FORCE_INCLUDE$")
        set(_parseState 11)
      elseif(_compileArg MATCHES "^INCLUDE_DIRECTORIES$")
        set(_parseState 13)
      elseif(_compileArg MATCHES "^DEFINES$")
        set(_parseState 15)
      elseif(_compileArg MATCHES "^DEPENDS$")
        set(_parseState 17)
      elseif(_compileArg MATCHES "^OPTIMIZE$")
        set(_parseState 19)
      elseif(_compileArg MATCHES "^OPTIONS$")
        set(_parseState 21)
      else()
        message(FATAL_ERROR "Invalid parameter token near \"${_compileArg}\".")
      endif()
    elseif(_parseState EQUAL 9)
      set(_pchFilePath "${_compileArg}")
      set(_pchFilePathSet YES)
      set(_parseState 10)
    elseif(_parseState EQUAL 10)
      if(_compileArg MATCHES "^FORCE_INCLUDE$")
        set(_parseState 11)
      elseif(_compileArg MATCHES "^INCLUDE_DIRECTORIES$")
        set(_parseState 13)
      elseif(_compileArg MATCHES "^DEFINES$")
        set(_parseState 15)
      elseif(_compileArg MATCHES "^DEPENDS$")
        set(_parseState 17)
      elseif(_compileArg MATCHES "^OPTIMIZE$")
        set(_parseState 19)
      elseif(_compileArg MATCHES "^OPTIONS$")
        set(_parseState 21)
      else()
        message(FATAL_ERROR "Invalid parameter token near \"${_compileArg}\".")
      endif()
    elseif(_parseState EQUAL 11)
      set(_incFilePaths "${_compileArg}")
      set(_parseState 12)
    elseif(_parseState EQUAL 12)
      if(_compileArg MATCHES "^INCLUDE_DIRECTORIES$")
        set(_parseState 13)
      elseif(_compileArg MATCHES "^DEFINES$")
        set(_parseState 15)
      elseif(_compileArg MATCHES "^DEPENDS$")
        set(_parseState 17)
      elseif(_compileArg MATCHES "^OPTIMIZE$")
        set(_parseState 19)
      elseif(_compileArg MATCHES "^OPTIONS$")
        set(_parseState 21)
      else()
        list(APPEND _incFilePaths "${_compileArg}")
      endif()
    elseif(_parseState EQUAL 13)
      set(_includeDirs "${_compileArg}")
      set(_parseState 14)
    elseif(_parseState EQUAL 14)
      if(_compileArg MATCHES "^DEFINES$")
        set(_parseState 15)
      elseif(_compileArg MATCHES "^DEPENDS$")
        set(_parseState 17)
      elseif(_compileArg MATCHES "^OPTIMIZE$")
        set(_parseState 19)
      elseif(_compileArg MATCHES "^OPTIONS$")
        set(_parseState 21)
      else()
        list(APPEND _includeDirs "${_compileArg}")
      endif()
    elseif(_parseState EQUAL 15)
      set(_defines "${_compileArg}")
      set(_parseState 16)
    elseif(_parseState EQUAL 16)
      if(_compileArg MATCHES "^DEPENDS$")
        set(_parseState 17)
      elseif(_compileArg MATCHES "^OPTIMIZE$")
        set(_parseState 19)
      elseif(_compileArg MATCHES "^OPTIONS$")
        set(_parseState 21)
      else()
        list(APPEND _defines "${_compileArg}")
      endif()
    elseif(_parseState EQUAL 17)
      set(_dependencies "${_compileArg}")
      set(_parseState 18)
    elseif(_parseState EQUAL 18)
      if(_compileArg MATCHES "^OPTIMIZE$")
        set(_parseState 19)
      elseif(_compileArg MATCHES "^OPTIONS$")
        set(_parseState 21)
      else()
        list(APPEND _dependencies "${_compileArg}")
      endif()
    elseif(_parseState EQUAL 19)
      set(_useOpt "${_compileArg}")
      set(_parseState 20)
    elseif((_parseState EQUAL 20) AND (_compileArg MATCHES "^OPTIONS$"))
      set(_parseState 21)
    elseif(_parseState EQUAL 21)
      list(FIND __allowedOptLanguages "${_compileArg}" _foundLanguageIdx)
      if(_foundLanguageIdx LESS 0)
        message(WARNING "Options language \"${_compileArg}\" is not supported. These options will be omitted. Supported languages: ${__allowedOptLanguages}")
        set(_optionsLang)
      else()
        set(_optionsLang "${_compileArg}")
      endif()
      set(_parseState 22)
    elseif(_parseState EQUAL 22)
      if(_compileArg MATCHES "^OPTIONS$")
        set(_parseState 21)
      elseif(DEFINED _optionsLang)
        list(APPEND "_options_${_optionsLang}" "${_compileArg}")
      endif()
    else()
      message(FATAL_ERROR "Invalid parameter token near \"${_compileArg}\".")
    endif()
  endforeach()
  if(NOT ((_parseState EQUAL 6) OR (_parseState EQUAL 8) OR (_parseState EQUAL 10) OR (_parseState EQUAL 12) OR (_parseState EQUAL 14) OR (_parseState EQUAL 16) OR (_parseState EQUAL 18) OR (_parseState EQUAL 20) OR (_parseState EQUAL 22)))
    message(FATAL_ERROR "Invalid number of parameters.")
  endif()


  get_filename_component(_outBcFileName "${_outBcFilePath}" NAME)
  get_filename_component(_outBcFileDir  "${_outBcFilePath}" PATH)

  # Grouping source files by language.
  foreach(_allowedLang ${__allowedLanguages})
    set("_srcFilePaths_${_allowedLang}")
  endforeach()
  if(_srcLanguage MATCHES "^DEFAULT$")
    foreach(_srcFilePath ${_srcFilePaths})
      string(REPLACE ";" "\;" _srcFilePath "${_srcFilePath}")
      if(_srcFilePath MATCHES "\\.[lL][lL]$")
        set(_srcFileLang "LL")
      elseif(_srcFilePath MATCHES "\\.[bB][cC]$")
        set(_srcFileLang "BC")
      else()
        set(_srcFileLang "CL")
      endif()
      list(APPEND "_srcFilePaths_${_srcFileLang}" "${_srcFilePath}")
    endforeach()
  else()
    set("_srcFilePaths_${_srcLanguage}" ${_srcFilePaths})
  endif()

  if(_pchFilePathSet)
    set(_pchFlags "-include-pch" "${_pchFilePath}")
  else()
    set(_pchFlags)
  endif()

  set(_incFileFlags)
  foreach(_incFilePath ${_incFilePaths})
    string(REPLACE ";" "\;" _incFilePath "${_incFilePath}")
    file(TO_CMAKE_PATH "${_incFilePath}" _incFilePath)
    list(APPEND _incFileFlags "-include" "${_incFilePath}")
  endforeach()

  set(_includeDirsFlags)
  foreach(_includeDir ${_includeDirs})
    string(REPLACE ";" "\;" _includeDir "${_includeDir}")
    list(APPEND _includeDirsFlags "-I" "${_includeDir}")
  endforeach()

  set(_defineFlags)
  foreach(_define ${_defines})
    string(REPLACE ";" "\;" _define "${_define}")
    list(APPEND _defineFlags "-D${_define}")
  endforeach()


  # Adding custom build commands for each language.
  set(_bcFiles)
  set(_bcFileId 0)
  foreach(_srcFilePath ${_srcFilePaths_CL})
    string(REPLACE ";" "\;" _srcFilePath "${_srcFilePath}")

    get_filename_component(_srcFileName      "${_srcFilePath}" NAME)
    get_filename_component(_srcFileNameWoExt "${_srcFilePath}" NAME_WE)
    set(_bcIntFilePath  "${_outBcFilePath}_${_srcFileNameWoExt}__cl__${_bcFileId}.bc")
    set(_bcTempFilePath "${_bcIntFilePath}.tmp")

    file(TO_CMAKE_PATH "${_bcIntFilePath}" _bcIntFilePath)
    file(TO_CMAKE_PATH "${_bcTempFilePath}" _bcTempFilePath)

    math(EXPR _bcFileId "${_bcFileId} + 1")

    # OpenCL source compilation is triggered by CLANG change, change of source files, change of precompiled header, change of
    # forcibly included headers or change of additional dependencies.
    add_custom_command(
        OUTPUT "${_bcTempFilePath}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_outBcFileDir}"
        COMMAND clang-tool -cc1 -x cl -fblocks -fpreserve-vec3-type -opencl-builtins "-triple=${_archTriple}" -w -emit-llvm-bc -o "${_bcTempFilePath}" ${_pchFlags} ${_incFileFlags} ${_includeDirsFlags} ${_defineFlags} ${_options_DEFAULT} ${_options_CL} "${_srcFilePath}"
        DEPENDS clang-tool ${_pchFilePath} ${_incFilePaths} "${_srcFilePath}" ${_dependencies}
        COMMENT "BiF: \"${_outBcFileName}\": Compiling OpenCL source: \"${_srcFileName}\""
      )
    add_custom_command(
        OUTPUT "${_bcIntFilePath}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_bcTempFilePath}" "${_bcIntFilePath}"
        DEPENDS "${_bcTempFilePath}"
        COMMENT "BiF: \"${_outBcFileName}\": Updating intermediate .bc for \"${_srcFileName}\" if changed."
      )

    list(APPEND _bcFiles "${_bcIntFilePath}")
  endforeach()

  foreach(_srcFilePath ${_srcFilePaths_LL})
    string(REPLACE ";" "\;" _srcFilePath "${_srcFilePath}")

    get_filename_component(_srcFileName      "${_srcFilePath}" NAME)
    get_filename_component(_srcFileNameWoExt "${_srcFilePath}" NAME_WE)
    set(_bcIntFilePath  "${_outBcFilePath}_${_srcFileNameWoExt}__ll__${_bcFileId}.bc")
    set(_bcTempFilePath "${_bcIntFilePath}.tmp")
    math(EXPR _bcFileId "${_bcFileId} + 1")

    # LLVM assembling is triggered by host LLVM package change (where tools are), change of source files or change of additional dependencies.
    # Makes sure that LLVM is unzipped before.
    add_custom_command(
        OUTPUT "${_bcTempFilePath}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_outBcFileDir}"
        COMMAND llvm-as -o "${_bcTempFilePath}" ${_options_DEFAULT} ${_options_LL} "${_srcFilePath}"
        DEPENDS "${_srcFilePath}" ${_dependencies}
        COMMENT "BiF: \"${_outBcFileName}\": Compiling OpenCL source: \"${_srcFileName}\""
      )
    add_custom_command(
        OUTPUT "${_bcIntFilePath}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_bcTempFilePath}" "${_bcIntFilePath}"
        DEPENDS "${_bcTempFilePath}"
        COMMENT "BiF: \"${_outBcFileName}\": Updating intermediate .bc for \"${_srcFileName}\" if changed."
      )

    list(APPEND _bcFiles "${_bcIntFilePath}")
  endforeach()

  foreach(_srcFilePath ${_srcFilePaths_BC})
    string(REPLACE ";" "\;" _srcFilePath "${_srcFilePath}")
    list(APPEND _bcFiles "${_srcFilePath}")
  endforeach()

  # Determining whether implicit link is required.
  list(LENGTH _bcFiles _bcFilesCount)
  if(_bcFilesCount GREATER 1)
    set(_bcIntFilePath  "${_outBcFilePath}__link__${_bcFileId}.bc")
    set(_bcTempFilePath "${_bcIntFilePath}.tmp")
    math(EXPR _bcFileId "${_bcFileId} + 1")

    # LLVM linking is triggered by host LLVM package change (where tools are), change of intermediate .bc files or change of additional dependencies.
    # Makes sure that LLVM is unzipped before.
    add_custom_command(
        OUTPUT "${_bcTempFilePath}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_outBcFileDir}"
        COMMAND llvm-link -o "${_bcTempFilePath}" ${_options_DEFAULT} ${_options_BC} ${_bcFiles}
        DEPENDS ${_bcFiles} ${_dependencies}
        COMMENT "BiF: \"${_outBcFileName}\": Linking intermediate .bc into output .bc."
      )
    add_custom_command(
        OUTPUT "${_bcIntFilePath}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_bcTempFilePath}" "${_bcIntFilePath}"
        DEPENDS "${_bcTempFilePath}"
        COMMENT "BiF: \"${_outBcFileName}\": Updating output .bc if changed (after linking)."
      )

    set(_bcFiles "${_bcIntFilePath}")
  elseif(_bcFilesCount LESS 1)
    return()
  endif()

  if(_useOpt)
    set(_bcIntFilePath  "${_outBcFilePath}__opt__${_bcFileId}.bc")
    set(_bcTempFilePath "${_bcIntFilePath}.tmp")

    # LLVM optmizer is triggered by host OPT change, change of .bc file to optimize or change of additional dependencies.
    # Makes sure that LLVM is unzipped before.
    add_custom_command(
        OUTPUT "${_bcTempFilePath}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_outBcFileDir}"
        COMMAND opt -O2 -o "${_bcTempFilePath}" ${_options_DEFAULT} ${_options_OPT} ${_bcFiles}
        DEPENDS ${_bcFiles} ${_dependencies}
        COMMENT "BiF: \"${_outBcFileName}\": Optmizing output .bc."
      )
    add_custom_command(
        OUTPUT "${_bcIntFilePath}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_bcTempFilePath}" "${_bcIntFilePath}"
        DEPENDS "${_bcTempFilePath}"
        COMMENT "BiF: \"${_outBcFileName}\": Updating output .bc if changed (after optimization)."
      )

    set(_bcFiles "${_bcIntFilePath}")
  endif()

  # Final copy.
  #igc_tool_get_activity_commandline(_copyOutCmd IGC_TARGET__TOOLS_COPY CopyPreserve ${_bcFiles} "${_outBcFilePath}")
  add_custom_command(
      OUTPUT "${_outBcFilePath}"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${_bcFiles} "${_outBcFilePath}"
      DEPENDS ${_bcFiles}
      COMMENT "BiF: \"${_outBcFileName}\": Copying output .bc."
    )
endfunction()

# ======================================================================================================

# Returns list common OpenCL C files from selected directories:
# - sources (.cl)
# - headers (.h)
#
# @param retVarName         Name of placeholder variable where results will be stored.
# @param [includeDir [...]] Full path to directory which will be searched for common files.
function(igc_bif_find_cl_files retVarName)
  set(_filePaths)
  foreach(_includeDir ${ARGN})
    string(REPLACE ";" "\;" _includeDir "${_includeDir}")
    file(GLOB_RECURSE _sourceFilePaths "${_includeDir}/*.[cC][lL]")
    file(GLOB_RECURSE _headerFilePaths "${_includeDir}/*.[hH]")
    list(APPEND _filePaths ${_sourceFilePaths} ${_headerFilePaths})
  endforeach()
  set("${retVarName}" ${_filePaths} PARENT_SCOPE)
endfunction()

# ======================================================================================================
# ======================================================================================================
# ======================================================================================================

# ======================================================================================================
# ===================================== BUILD STEP CONFIGURATION =======================================
# ======================================================================================================

# ======================================== Precompiled headers =========================================
if(NOT CCLANG_FROM_SYSTEM)
  set(IGC_BUILD__BIF_OCL_INCLUDES ${opencl-header})
  set(PCH_OPTIONS "-include ${opencl-header}")
else()
  set(PCH_OPTIONS ${CL_OPTIONS})
endif()

set(IGC_BUILD__BIF_RS_PCH_SRC "${IGC_OPTION__BIF_SRC_RS_DIR}/rs_pch.h")
set(IGC_BUILD__BIF_RS_PCH32   "${IGC_BUILD__BIF_DIR}/rs32.pch")
set(IGC_BUILD__BIF_RS_PCH64   "${IGC_BUILD__BIF_DIR}/rs64.pch")

igc_bif_build_pch(
    OUTPUT  "${IGC_BUILD__BIF_RS_PCH32}"
    TRIPLE  spir
    HEADERS "${IGC_BUILD__BIF_RS_PCH_SRC}"
    DEFINES "__32bit__=1" "cl_khr_fp64"
    OPTIONS ${PCH_OPTIONS}
  )

igc_bif_build_pch(
    OUTPUT  "${IGC_BUILD__BIF_RS_PCH64}"
    TRIPLE  spir64
    HEADERS "${IGC_BUILD__BIF_RS_PCH_SRC}"
    OPTIONS ${PCH_OPTIONS}
  )

# ======================================================================================================
#concat files function
# @param in_file        input file.
# @param out_file       output file.
function(cat in_file out_file)
  file(READ ${in_file} CONTENTS)
  file(APPEND ${out_file} "${CONTENTS}")
endfunction()

# ============================================ Building .bc =============================================

# All:
#     -cc1 -x cl -opencl-builtins -w -emit-llvm-bc
#     -include .\IGCBiFModule\opencl_cth.h
#     -I .\IGCBiFModule\shared -I .\IGCBiFModule\PointerSize
#     -triple=(spir or spir64 - consistent with PCH)
### -include-pch OCL_32                 -D__EXECUTION_MODEL_DEBUG=1 -D__OPENCL_C_VERSION__=200                                      .\IGCBiFModule\shared\IBiF_Impl.cl                   -o .\IBiF_Impl_int.bc
### -include-pch OCL_32 -cl-enable-half -D__EXECUTION_MODEL_DEBUG=1 -D__OPENCL_C_VERSION__=120 -Dcl_khr_fp16 -I                     .\IGCBiFModule\PointerSize\IBiF_size_t.cl            -o .\IGCsize_t_32.bc
### -include-pch OCL_64 -cl-enable-half -D__EXECUTION_MODEL_DEBUG=1 -D__OPENCL_C_VERSION__=120 -Dcl_khr_fp16 -I                     .\IGCBiFModule\PointerSize\IBiF_size_t.cl            -o .\IGCsize_t_64.bc

# Other:
#     %LLVM_LINK_EXE% .\IBiF_Impl_int.bc .\IGCBiF_Impl_ll.bc -o .\OCLBiFImpl.bc


file(GLOB_RECURSE _PRE_RELEASE_CL "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/PreRelease/*.cl")

set(_concatScript   "${IGC_SOURCE_DIR}/BiFModule/concat.py")
set(IGC_BUILD__BIF_OCL_FORCE_INC "${IGC_BUILD__BIF_DIR}/opencl_cth.h")
set(IGC_BUILD__BIF_OCL_SHARED_INC  "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/opencl_cth_released.h")
set(IGC_BUILD__BIF_OCL_SHARED_INC_PRE_RELEASE    "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/PreRelease/opencl_cth_pre_release.h")
set(IGC_BUILD__BIF_OCL_INCLUDES ${IGC_BUILD__BIF_OCL_INCLUDES} ${IGC_BUILD__BIF_OCL_FORCE_INC})



add_custom_command(
    OUTPUT "${IGC_BUILD__BIF_OCL_FORCE_INC}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${IGC_BUILD__BIF_DIR}"
    COMMAND "${PYTHON_EXECUTABLE}"
    ARGS ${_concatScript} -new ${IGC_BUILD__BIF_OCL_FORCE_INC} ${IGC_BUILD__BIF_OCL_SHARED_INC} ${IGC_BUILD__BIF_OCL_SHARED_INC_PRE_RELEASE}
    DEPENDS ${_concatScript}
    DEPENDS ${IGC_BUILD__BIF_OCL_SHARED_INC}
    DEPENDS ${IGC_BUILD__BIF_OCL_SHARED_INC_PRE_RELEASE}
    COMMENT "Running concat.py for autogenerating opencl_cth.h"
    )

set(IGC_BUILD__BIF_OCL_COMMON_INC_DIRS
    "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL"
    "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/PointerSize"
    "${IGC_OPTION__BIF_SRC_OCL_DIR}/Headers"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_shared/device_enqueue"
  )

set(IGC_BUILD__BIF_SPIRV_COMMON_INC_DIRS
    "${IGC_OPTION__BIF_SRC_OCL_DIR}/Implementation"
    "${IGC_OPTION__BIF_SRC_OCL_DIR}/Headers"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/ocl_igc_interface/device_enqueue"
  )

file(GLOB_RECURSE _MATH_SRC_BC      "${IGC_OPTION__BIF_SRC_OCL_DIR}/*.bc")

file(GLOB_RECURSE SVML_FILES "${IGC_OPTION__BIF_SRC_OCL_DIR}/Implementation/BinaryReleaseOnly/*.cl" )

set(FLAG "")
if(${VME_TYPES_DEFINED})
    list(APPEND FLAG "__VME_TYPES_DEFINED__")
endif(${VME_TYPES_DEFINED})

igc_bif_find_cl_files(IGC_BUILD__BIF_OCL_COMMON_DEPENDS ${IGC_BUILD__BIF_OCL_COMMON_INC_DIRS} "${IGC_OPTION__BIF_SRC_OCL_DIR}/Implementation")

set(KHR_DEFINES "cl_khr_f16" "cl_khrfp64" "cl_khr_gl_msaa_sharing" "cl_khr_mipmap_image" "cl_khr_depth_images" "cl_intel_subgroups_short" "cl_intel_subgroups_char" "cl_intel_subgroups_long" "cl_intel_64bit_global_atomics_placeholder")

igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/IBiF_Impl_int.bc"
    TRIPLE               spir64
    SOURCES              "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/IBiF_Impl.cl"
    FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
    INCLUDE_DIRECTORIES  ${IGC_BUILD__BIF_OCL_COMMON_INC_DIRS}
    DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=200" "__IGC_BUILD__" ${KHR_DEFINES} ${FLAG}
    DEPENDS              ${IGC_BUILD__BIF_OCL_COMMON_DEPENDS}
    OPTIONS CL           ${CL_OPTIONS} -cl-std=CL2.0
  )

igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/IBiF_Impl_int_spirv.bc"
    TRIPLE               spir64
    SOURCES              "${IGC_OPTION__BIF_SRC_OCL_DIR}/Implementation/IBiF_Impl.cl"
    FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
    INCLUDE_DIRECTORIES  ${IGC_BUILD__BIF_SPIRV_COMMON_INC_DIRS}
    DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=200" "__IGC_BUILD__" ${KHR_DEFINES} ${FLAG}
    DEPENDS              ${IGC_BUILD__BIF_OCL_COMMON_DEPENDS}
    OPTIONS CL           ${CL_OPTIONS} -cl-std=CL2.0
  )
  if ( NOT "${_PRE_RELEASE_CL}" STREQUAL "" )
   igc_bif_build_bc(
      OUTPUT               "${IGC_BUILD__BIF_DIR}/IBiF_PreRelease_int.bc"
      TRIPLE               spir64
      SOURCES              ${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/PreRelease/IBIF_PreRelease_Impl.cl
      FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
      INCLUDE_DIRECTORIES  ${IGC_BUILD__BIF_OCL_COMMON_INC_DIRS}
      DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=200" "__IGC_BUILD__" ${KHR_DEFINES} "cl_intel_device_side_avc_vme_enable" "cl_intel_device_side_avc_motion_estimation" ${FLAG}
      DEPENDS              ${IGC_BUILD__BIF_OCL_COMMON_DEPENDS}
      OPTIONS CL           ${CL_OPTIONS} -cl-std=CL2.0
     )
 endif()
igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/IBiF_spirv_size_t_32.bc"
    TRIPLE               spir
    SOURCES              "${IGC_OPTION__BIF_SRC_OCL_DIR}/Implementation/pointersize.cl"
    FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
    INCLUDE_DIRECTORIES  ${IGC_BUILD__BIF_OCL_COMMON_INC_DIRS}
    DEFINES              "__32bit__=1" "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=200" "__IGC_BUILD__" ${KHR_DEFINES} ${FLAG}
    DEPENDS              ${IGC_BUILD__BIF_OCL_COMMON_DEPENDS}
    OPTIONS CL           ${CL_OPTIONS} -cl-std=CL2.0
  )
igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/IBiF_spirv_size_t_64.bc"
    TRIPLE               spir64
    SOURCES              "${IGC_OPTION__BIF_SRC_OCL_DIR}/Implementation/pointersize.cl"
    FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
    INCLUDE_DIRECTORIES  ${IGC_BUILD__BIF_OCL_COMMON_INC_DIRS}
    DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=200" "__IGC_BUILD__" ${KHR_DEFINES} ${FLAG}
    DEPENDS              ${IGC_BUILD__BIF_OCL_COMMON_DEPENDS}
    OPTIONS CL           ${CL_OPTIONS} -cl-std=CL2.0
  )
igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/IGCsize_t_32_int.bc"
    TRIPLE               spir
    SOURCES              "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/PointerSize/IBiF_size_t.cl"
    FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
    INCLUDE_DIRECTORIES  ${IGC_BUILD__BIF_OCL_COMMON_INC_DIRS}
    DEFINES              "__32bit__=1" "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=200" "__IGC_BUILD__" ${KHR_DEFINES} ${FLAG}
    DEPENDS              ${IGC_BUILD__BIF_OCL_COMMON_DEPENDS}
    OPTIONS CL           ${CL_OPTIONS} -cl-std=CL2.0
  )
igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/IGCsize_t_64_int.bc"
    TRIPLE               spir64
    SOURCES              "${IGC_OPTION__BIF_SRC_OCL_DIR}/Languages/OpenCL/PointerSize/IBiF_size_t.cl"
    FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
    INCLUDE_DIRECTORIES  ${IGC_BUILD__BIF_OCL_COMMON_INC_DIRS}
    DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=200" "__IGC_BUILD__" ${KHR_DEFINES} ${FLAG}
    DEPENDS              ${IGC_BUILD__BIF_OCL_COMMON_DEPENDS}
    OPTIONS CL           ${CL_OPTIONS} -cl-std=CL2.0
  )
igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/IGCsize_t_32.bc"
    TRIPLE               spir
    SOURCES              "${IGC_BUILD__BIF_DIR}/IGCsize_t_32_int.bc"
                         "${IGC_BUILD__BIF_DIR}/IBiF_spirv_size_t_32.bc"
    FORCE_INCLUDE         ${IGC_BUILD__BIF_OCL_INCLUDES}
    DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=120" ${KHR_DEFINES} ${FLAG}
    DEPENDS              ${IGC_BUILD__BIF_OCL_COMMON_DEPENDS}
    OPTIONS CL           ${CL_OPTIONS}
  )
igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/IGCsize_t_64.bc"
    TRIPLE               spir64
    SOURCES              "${IGC_BUILD__BIF_DIR}/IGCsize_t_64_int.bc"
                         "${IGC_BUILD__BIF_DIR}/IBiF_spirv_size_t_64.bc"
    FORCE_INCLUDE         ${IGC_BUILD__BIF_OCL_INCLUDES}
    DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=120" ${KHR_DEFINES} ${FLAG}
    DEPENDS              ${IGC_BUILD__BIF_OCL_COMMON_DEPENDS}
    OPTIONS CL           ${CL_OPTIONS}
  )

if ("${FLAG}" STREQUAL "")
    unset(FLAG)
endif()

# TODO:
# Math source( svml files) is not released as a part of open source bundle. Instead, we release .bc.
# We basically look for .bc and only if .bc is not present we build from source.
#Currently, not sure where exactly the .bc will be placed. But ones we have all
# the info, we need to make sure all the dependencies are handled. Like for example if any
# file uses svml files, we need to prototype the headers and during linking it gets resolved.

if ("${_PRE_RELEASE_CL}" STREQUAL "" AND ( NOT "${_MATH_SRC_BC}" STREQUAL "" ) )
  igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/OCLBiFImpl.bc"
    TRIPLE               spir64
    SOURCES              "${IGC_BUILD__BIF_DIR}/IBiF_Impl_int.bc"
                         "${IGC_BUILD__BIF_DIR}/IBiF_Impl_int_spirv.bc"
                         "${IGC_BUILD__BIF_DIR}/IBiF_Math_Src_spirv.bc"
    FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
    DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=120" ${KHR_DEFINES}
    OPTIONS CL           ${CL_OPTIONS}
  )
else()
  igc_bif_build_bc(
    OUTPUT               "${IGC_BUILD__BIF_DIR}/OCLBiFImpl.bc"
    TRIPLE               spir64
    SOURCES              "${IGC_BUILD__BIF_DIR}/IBiF_Impl_int.bc"
                         "${IGC_BUILD__BIF_DIR}/IBiF_Impl_int_spirv.bc"
                         "${IGC_BUILD__BIF_DIR}/IBiF_PreRelease_int.bc"
    FORCE_INCLUDE        ${IGC_BUILD__BIF_OCL_INCLUDES}
    DEFINES              "__EXECUTION_MODEL_DEBUG=1" "__OPENCL_C_VERSION__=120" ${KHR_DEFINES}
    OPTIONS CL           ${CL_OPTIONS}
  )
endif()

# =========================================== Custom targets ============================================

set(IGC_BUILD__PROJ__BiFModule_OCL       "${IGC_BUILD__PROJ_NAME_PREFIX}BiFModuleOcl")
set(IGC_BUILD__PROJ__BiFModule_OCL       "${IGC_BUILD__PROJ__BiFModule_OCL}" PARENT_SCOPE)
set(IGC_BUILD__PROJ_LABEL__BiFModule_OCL "BiFModule-OCL")

add_custom_target("${IGC_BUILD__PROJ__BiFModule_OCL}"
    DEPENDS clang-tool "${IGC_BUILD__BIF_DIR}/OCLBiFImpl.bc" "${IGC_BUILD__BIF_DIR}/IGCsize_t_32.bc" "${IGC_BUILD__BIF_DIR}/IGCsize_t_64.bc" "${IGC_BUILD__BIF_DIR}/IBiF_Impl_int_spirv.bc"
    SOURCES ${IGC_BUILD__BIF_OCL_COMMON_DEPENDS}
  )
set_property(TARGET "${IGC_BUILD__PROJ__BiFModule_OCL}" PROPERTY PROJECT_LABEL "${IGC_BUILD__PROJ_LABEL__BiFModule_OCL}")

# ======================================================================================================
# ======================================================================================================
# ======================================================================================================
