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

set(IGC_BUILD__PROJ__RTStackReflection       "${IGC_BUILD__PROJ_NAME_PREFIX}RTStackReflection")
set(IGC_BUILD__PROJ__RTStackReflection       "${IGC_BUILD__PROJ__RTStackReflection}" PARENT_SCOPE)
set(IGC_BUILD__PROJ_LABEL__RTStackReflection "${IGC_BUILD__PROJ__RTStackReflection}")

set(IGC_BUILD__SRC__RTStackReflection
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/reflection.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/shared.h"
    )

set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/reflection.cpp" PROPERTIES HEADER_FILE_ONLY TRUE)

igc_imported_tool("${IGC_BUILD__PROJ__RTStackReflection}" RTStackReflection)

if(NOT TARGET ${IGC_BUILD__PROJ__RTStackReflection})
  add_executable("${IGC_BUILD__PROJ__RTStackReflection}"
      ${IGC_BUILD__SRC__RTStackReflection}
    )

  set_target_properties("${IGC_BUILD__PROJ__RTStackReflection}" PROPERTIES FOLDER "Tools")

  target_link_libraries(${IGC_BUILD__PROJ__RTStackReflection}
      ${IGC_BUILD__LLVM_LIBS_TO_LINK})

  target_include_directories(${IGC_BUILD__PROJ__RTStackReflection} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../"
  )
endif()

if(MSVC)
#set up standard defines from the common WDK path.
bs_set_wdk(${IGC_BUILD__PROJ__RTStackReflection})
endif()

