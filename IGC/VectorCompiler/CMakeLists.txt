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



# CMake Settings:
# - SPIRV_PREBUILD_DIR
# - SPIRV_SRC
# - INSTALL_SPIRVDLL
# - VC_INTRINSICS_SRC
set(IGC_BUILD__PROJ__VectorCompiler "${IGC_BUILD__PROJ_NAME_PREFIX}VectorCompiler")
set(IGC_BUILD__PROJ__VectorCompiler "${IGC_BUILD__PROJ__VectorCompiler}" PARENT_SCOPE)
set(IGC_BUILD__PROJ_VC_LIBS_TO_LINK VCCodeGen PARENT_SCOPE)

set(IGC_BUILD__PROJ_LABEL__VectorCompiler "${IGC_BUILD__PROJ__VectorCompiler}")

message(STATUS "+++ Source/IGC/VectorCompiler +++")
message(STATUS "[VC] Build proj: ${IGC_BUILD__PROJ__VectorCompiler}")


igc_arch_get_cpu(_cpuSuffix)

set(BUILD_EXTERNAL YES)

# --- LLVM ---
if(IGC_BUILD__USING_SYSTEM_LLVM OR (WIN32 AND LLVM_USE_PREBUILT))
  message(STATUS "[VC] Using system llvm")

  # Need to search for llvm-tblgen
  find_program(LLVM_TABLEGEN_EXE "llvm-tblgen"
    ${LLVM_TOOLS_BINARY_DIR}
    NO_DEFAULT_PATH
    )
  if(LLVM_TABLEGEN_EXE-NOTFOUND)
    message(FATAL_ERROR "[VC] llvm-tblgen is not found")
  endif()
  message(STATUS "[VC] Found tblgen: ${LLVM_TABLEGEN_EXE}")

  # find_package was called by igc cmake, no need to do it again.
else()
  # Prebuilt llvm does not have tblgen...
  if(LLVM_USE_PREBUILT)
    message(FATAL_ERROR "[VC] vector compiler with prebuilt llvm is not supported")
  endif()

  # In last scenario we are building with llvm so every target is defined
  # and LLVMConfig will only set needed variables.
  message(STATUS "[VC] Using llvm source build")
  set(LLVM_BUILD_DIR "${LLVM_SOURCE_DIR}/../build/src")
  set(LLVM_CMAKE_DIR "${LLVM_BUILD_DIR}/lib/cmake/llvm")
  message(STATUS "[VC] LLVMConfig.cmake location: ${LLVM_CMAKE_DIR}")
  find_package(LLVM REQUIRED
    HINTS ${LLVM_CMAKE_DIR}
    NO_DEFAULT_PATH
    )

  # We have executable target, use it.
  set(LLVM_TABLEGEN_EXE "llvm-tblgen")
  message(STATUS "[VC] Using executable target llvm-tlbgen for tablegenning")
  # IGC has its own special cmake for external llvm.
  # It sets LLVM_INCLUDE_DIRS instead of LLVM_INCLUDE_DIR.
  set(LLVM_INCLUDE_DIR ${LLVM_INCLUDE_DIRS})
endif()

# Now find_package was called in all cases and we have all needed variables.
set(CMAKE_MODULE_PATH
  ${LLVM_CMAKE_DIR}
  ${CMAKE_MODULE_PATH}
  )

cmake_policy(SET CMP0057 NEW)
# cm offline compiler requires -rdynamic flag to be absent
cmake_policy(SET CMP0065 NEW)

include(AddLLVM)

set(LLVM_MAIN_INCLUDE_DIR ${LLVM_INCLUDE_DIR})
include(TableGen)
# Set LLVM_TABLEGEN_FLAGS manually based on include dirs.
list(TRANSFORM LLVM_INCLUDE_DIR PREPEND "-I=" OUTPUT_VARIABLE LLVM_TABLEGEN_FLAGS)

message(STATUS "[VC] Including llvm headers: ${LLVM_INCLUDE_DIR}")
include_directories(${LLVM_INCLUDE_DIR})

# --- VISA ---

# HACK. We should use only visa/include without visa internal headers.
set(VISA_INCLUDE_DIRS ${IGC_BUILD__VISA_DIR})

# --- VC Intrinsics ---

add_compile_definitions(LLVM_VERSION_MAJOR=${LLVM_VERSION_MAJOR})

if(LLVM_ON_WIN32)
  add_compile_options(/experimental:external)
  foreach(INCDIR ${LLVM_INCLUDE_DIRS})
    add_compile_options("SHELL:/external:I ${INCDIR}")
  endforeach()
  add_compile_options(/external:W0)

  # disable 32/64 warnings
  add_compile_options(/wd4244)

  #  disable unary minus to unsigned type warning
  add_compile_options(/wd4146)

  # disable implicitly deleted dtor warning
  add_compile_options(/wd4624)
endif()

if(DEFINED VC_INTRINSICS_SRC)
  set(INTRSRC "${VC_INTRINSICS_SRC}/GenXIntrinsics")
endif()

if(NOT DEFINED INTRSRC)
  set(INTRSRC "${CMAKE_CURRENT_SOURCE_DIR}/../../../vc-intrinsics/GenXIntrinsics")
endif()

message(STATUS "[VC] Using vc-intrinsics source from: ${INTRSRC}")

# We are using prebuilt SPIRV and building intrinsics.
set(INTRBUILD "${CMAKE_CURRENT_BINARY_DIR}/intrbuild")


# Do not copy anything from prebuilts. libSPIRVDLL.so will be dynamically loaded at runtime.
add_subdirectory(${INTRSRC} ${INTRBUILD})
include_directories(${INTRSRC}/include ${INTRBUILD}/include)

include(cmake/spirv.cmake)
include(cmake/utils.cmake)

# --- VC Opt ---

add_subdirectory(include)
add_subdirectory(lib)

