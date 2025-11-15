#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2019-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

add_definitions(-D_CRT_SECURE_NO_WARNINGS -DISTDLIB_UMD)

# have all configurations output a .pdb.
if (MSVC)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG")
  set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /OPT:REF /OPT:ICF")
  set(CMAKE_SHARED_LINKER_FLAGS_RELEASE-INTERNAL "${CMAKE_SHARED_LINKER_FLAGS_RELEASE-INTERNAL} /OPT:REF /OPT:ICF")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
endif()

set(IGC_BUILD__PROJ__IRBuilderGenerator       "${IGC_BUILD__PROJ_NAME_PREFIX}IRBuilderGenerator")
set(IGC_BUILD__PROJ__IRBuilderGenerator       "${IGC_BUILD__PROJ__IRBuilderGenerator}" PARENT_SCOPE)
set(IGC_BUILD__PROJ_LABEL__IRBuilderGenerator "${IGC_BUILD__PROJ__IRBuilderGenerator}")
set(IGC_BUILD__OUTPUT_NAME__IRBuilderGenerator "IRBuilderGenerator_exe")

set(IGC_BUILD__SRC__IRBuilderGenerator
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/BuilderUtils.h"
    )

igc_imported_tool("${IGC_BUILD__PROJ__IRBuilderGenerator}" "${IGC_BUILD__OUTPUT_NAME__IRBuilderGenerator}")

if(NOT TARGET ${IGC_BUILD__PROJ__IRBuilderGenerator})
  add_executable("${IGC_BUILD__PROJ__IRBuilderGenerator}"
      ${IGC_BUILD__SRC__IRBuilderGenerator}
    )

  # Need to change the output name because the executable has the same name as the directory
  # it is in which collides on linux because executables don't have extensions.
  set_target_properties("${IGC_BUILD__PROJ__IRBuilderGenerator}" PROPERTIES
      FOLDER "Tools"
      OUTPUT_NAME "${IGC_BUILD__OUTPUT_NAME__IRBuilderGenerator}")

  target_link_libraries(${IGC_BUILD__PROJ__IRBuilderGenerator}
      ${IGC_BUILD__LLVM_LIBS_TO_LINK})

  target_include_directories(${IGC_BUILD__PROJ__IRBuilderGenerator} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../"
  )
endif()

if(MSVC)
#set up standard defines from the common WDK path.
bs_set_wdk(${IGC_BUILD__PROJ__IRBuilderGenerator})
endif()

