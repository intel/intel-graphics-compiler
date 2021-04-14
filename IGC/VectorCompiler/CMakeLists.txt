#=========================== begin_copyright_notice ============================
#
# Copyright (c) 2000-2021 Intel Corporation
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



# CMake Settings:
# - SPIRV_PREBUILD_DIR
# - SPIRVDLL_SRC
# - SPIRV_SRC
# - INSTALL_SPIRVDLL
# - VC_INTRINSICS_SRC
set(IGC_BUILD__PROJ__VectorCompiler "${IGC_BUILD__PROJ_NAME_PREFIX}VectorCompiler")
set(IGC_BUILD__PROJ__VectorCompiler "${IGC_BUILD__PROJ__VectorCompiler}" PARENT_SCOPE)
# Ordering matters here.
# FIXME: set proper dependencies for VCCodeGen and others.
set(IGC_BUILD__PROJ_VC_LIBS_TO_LINK VCIGCDeps VCCodeGen PARENT_SCOPE)

set(IGC_BUILD__PROJ_LABEL__VectorCompiler "${IGC_BUILD__PROJ__VectorCompiler}")

message(STATUS "+++ Source/IGC/VectorCompiler +++")
message(STATUS "[VC] Build proj: ${IGC_BUILD__PROJ__VectorCompiler}")

# --- LLVM ---

set(CMAKE_MODULE_PATH
  ${LLVM_CMAKE_DIR}
  ${CMAKE_MODULE_PATH}
  )

set(LLVM_TABLEGEN_EXE "llvm-tblgen")

include(AddLLVM)
include(TableGen)
# Set LLVM_TABLEGEN_FLAGS manually based on include dirs.
list(TRANSFORM LLVM_INCLUDE_DIRS PREPEND "-I=" OUTPUT_VARIABLE LLVM_TABLEGEN_FLAGS)

# --- Options ---

add_compile_definitions(LLVM_VERSION_MAJOR=${LLVM_VERSION_MAJOR})

if(LLVM_ON_WIN32)
  if (MSVC_IDE)
    add_compile_options(/experimental:external)
    foreach(INCDIR ${LLVM_INCLUDE_DIRS})
      add_compile_options("SHELL:/external:I ${INCDIR}")
    endforeach()
    add_compile_options(/external:W0)
  endif()

  # disable 32/64 warnings
  add_compile_options(/wd4244)

  #  disable unary minus to unsigned type warning
  add_compile_options(/wd4146)

  # disable implicitly deleted dtor warning
  add_compile_options(/wd4624)

  # Disable double-inline warning coming from StringRef.h:898
  # For some reason it is not filtered by /external:I option
  add_compile_options(/wd4141)
endif()

# --- VC Intrinsics ---

if(DEFINED VC_INTRINSICS_SRC)
  set(INTRSRC "${VC_INTRINSICS_SRC}/GenXIntrinsics")
endif()

if(NOT DEFINED INTRSRC)
  set(INTRSRC "${CMAKE_CURRENT_SOURCE_DIR}/../../../vc-intrinsics/GenXIntrinsics")
endif()

message(STATUS "[VC] Using vc-intrinsics source from: ${INTRSRC}")
# Trick intrinsics.
set(BUILD_EXTERNAL YES)
# We are using prebuilt SPIRV and building intrinsics.
set(INTRBUILD "${CMAKE_CURRENT_BINARY_DIR}/intrbuild")
add_subdirectory(${INTRSRC} ${INTRBUILD} EXCLUDE_FROM_ALL)

include(cmake/spirv.cmake)

set(IGC_OPTION__VC_DISABLE_BIF_DEFAULT OFF)
if(LLVM_VERSION_MAJOR LESS 9)
  set(IGC_OPTION__VC_DISABLE_BIF_DEFAULT ON)
endif()
option(IGC_OPTION__VC_DISABLE_BIF
  "disable BiF generation for VC (disables some BE features)"
  ${IGC_OPTION__VC_DISABLE_BIF_DEFAULT})

# --- VC Opt ---

add_subdirectory(include)
add_subdirectory(lib)

# Common utilities that depend on other IGC components.
# These have to be separated because of circular dependencies between
# some components in IGC.
add_subdirectory(igcdeps)


# --- CMCL  ---
add_subdirectory(CMCL)
