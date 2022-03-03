#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================



# CMake Settings:
# - VC_INTRINSICS_SRC
set(IGC_BUILD__PROJ__VectorCompiler "${IGC_BUILD__PROJ_NAME_PREFIX}VectorCompiler")
set(IGC_BUILD__PROJ__VectorCompiler "${IGC_BUILD__PROJ__VectorCompiler}" PARENT_SCOPE)
# Ordering matters here.
# FIXME: set proper dependencies for VCCodeGen and others.
set(IGC_BUILD__PROJ_VC_LIBS_TO_LINK VCIGCDeps PARENT_SCOPE)

set(IGC_BUILD__PROJ_LABEL__VectorCompiler "${IGC_BUILD__PROJ__VectorCompiler}")

message(STATUS "+++ Source/IGC/VectorCompiler +++")
message(STATUS "[VC] Build proj: ${IGC_BUILD__PROJ__VectorCompiler}")

if(LLVM_VERSION_MAJOR LESS 9)
  message(FATAL_ERROR "OpenCL C++ is available only since LLVM-9. Cannot build VC BiF without this feature.")
endif()

# --- Options ---

if(LLVM_ON_WIN32)
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

# --- VC platforms ---
include(cmake/supported_platforms_list.cmake)

# --- VC Intrinsics ---
include(cmake/handle_vc_intrinsics.cmake)

# --- VC Opt ---

add_subdirectory(include)
add_subdirectory(lib)

# Common utilities that depend on other IGC components.
# These have to be separated because of circular dependencies between
# some components in IGC.
add_subdirectory(igcdeps)


add_subdirectory(utils)

# --- CMCL  ---
add_subdirectory(CMCL)
