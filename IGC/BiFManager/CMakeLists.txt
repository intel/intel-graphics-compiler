#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

add_definitions(-D_CRT_SECURE_NO_WARNINGS -DISTDLIB_UMD)
get_property (RESOURCE_FILES_ALL_LOCAL GLOBAL PROPERTY RESOURCE_FILES_ALL)
get_property (RESOURCE_FILES_RS_LOCAL GLOBAL PROPERTY RESOURCE_FILES_RS)

# have all configurations output a .pdb.
if (MSVC)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG")
  set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /OPT:REF /OPT:ICF")
  set(CMAKE_SHARED_LINKER_FLAGS_RELEASE-INTERNAL "${CMAKE_SHARED_LINKER_FLAGS_RELEASE-INTERNAL} /OPT:REF /OPT:ICF")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
endif()

set(IGC_BUILD__PROJ__BiFManager       "${IGC_BUILD__PROJ_NAME_PREFIX}BiFManager")
set(IGC_BUILD__PROJ__BiFManager_EXE   "${IGC_BUILD__PROJ_NAME_PREFIX}BiFManager-bin")
set(IGC_BUILD__PROJ__BiFManager_EXE   "${IGC_BUILD__PROJ__BiFManager_EXE}" PARENT_SCOPE)
set(IGC_BUILD__PROJ__BiFManager       "${IGC_BUILD__PROJ__BiFManager}" PARENT_SCOPE)
set(IGC_BUILD__PROJ_LABEL__BiFManager "${IGC_BUILD__PROJ__BiFManager}")

add_library("${IGC_BUILD__PROJ__BiFManager}"
    "${CMAKE_CURRENT_SOURCE_DIR}/BiFManagerHandler.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/BiFManagerCommon.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/BiFManagerHandler.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/BiFManagerCommon.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../AdaptorOCL/OCL/LoadBuffer.cpp"
)

add_dependencies("${IGC_BUILD__PROJ__BiFManager}" "${IGC_BUILD__PROJ__BiFManagerSplitTask}")
target_include_directories("${IGC_BUILD__PROJ__BiFManager}" PRIVATE ${IGC_BUILD__BIF_DIR})


add_executable("${IGC_BUILD__PROJ__BiFManager_EXE}"
${IGC_WrapperLLVM_HDR}
${IGC_WrapperLLVM_SRC}
"${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/BiFManagerTool.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/BiFManagerCommon.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/BiFManagerTool.hpp"
"${CMAKE_CURRENT_SOURCE_DIR}/BiFManagerCommon.hpp")
target_link_libraries("${IGC_BUILD__PROJ__BiFManager_EXE}"
    ${IGC_BUILD__LLVM_LIBS_TO_LINK}
  )
target_compile_definitions(${IGC_BUILD__PROJ__BiFManager_EXE} PRIVATE BiFManagerBIN)

add_dependencies("${IGC_BUILD__PROJ__BiFManager}"
    "${IGC_BUILD__PROJ__BiFManager_EXE}")

if(MSVC)
#set up standard defines from the common WDK path.
bs_set_wdk(${IGC_BUILD__PROJ__BiFManager})
endif()
