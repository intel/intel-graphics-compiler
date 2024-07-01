#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2018-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

source_group(Headers
  FILES
    ${IGC_SOURCE_DIR}/AdaptorOCL/CLElfLib/ElfReader.h
    ${IGC_SOURCE_DIR}/AdaptorOCL/CLElfLib/ElfWriter.h
    ${IGC_SOURCE_DIR}/AdaptorOCL/CLElfLib/CLElfTypes.h
)

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

set(IGC_BUILD__PROJ__ElfPackager       "${IGC_BUILD__PROJ_NAME_PREFIX}elf_packager")
set(IGC_BUILD__PROJ__ElfPackager       "${IGC_BUILD__PROJ__ElfPackager}" PARENT_SCOPE)
set(IGC_BUILD__PROJ_LABEL__ElfPackager "${IGC_BUILD__PROJ__ElfPackager}")

set(IGC_BUILD__SRC__ElfPackager
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

igc_imported_tool("${IGC_BUILD__PROJ__ElfPackager}" "${IGC_BUILD__PROJ_NAME_PREFIX}elf_packager")
if(NOT TARGET ${IGC_BUILD__PROJ__ElfPackager})
  add_executable("${IGC_BUILD__PROJ__ElfPackager}"
      ${IGC_BUILD__SRC__ElfPackager}
      ${IGC_SOURCE_DIR}/AdaptorOCL/CLElfLib/ElfWriter.cpp
      ${IGC_SOURCE_DIR}/AdaptorOCL/CLElfLib/ElfReader.cpp
      ${IGC_SOURCE_DIR}/AdaptorOCL/CLElfLib/ElfWriter.h
      ${IGC_SOURCE_DIR}/AdaptorOCL/CLElfLib/ElfReader.h
    )

  add_dependencies("${IGC_BUILD__PROJ__ElfPackager}" "${IGC_BUILD__PROJ__GenISAIntrinsics}")

  target_link_libraries(${IGC_BUILD__PROJ__ElfPackager}
      ${IGC_BUILD__LLVM_LIBS_TO_LINK})

  include_directories (
    ${IGC_SOURCE_DIR}/AdaptorOCL/CLElfLib/
  )

  if(NOT ANDROID)
    add_custom_command(TARGET ${IGC_BUILD__PROJ__ElfPackager}
                      POST_BUILD
                      COMMAND ${IGC_BUILD__PROJ__ElfPackager} -includeSizet -funcList ${CMAKE_CURRENT_SOURCE_DIR}/function_bin.txt ${IGC_BUILD__BIF_DIR}/OCLBiFImpl.bc ${IGC_BUILD__BIF_DIR}/igdclbif.bin
                      )
  endif()


  add_dependencies("${IGC_BUILD__PROJ__ElfPackager}" "${IGC_BUILD__PROJ__BiFModule_OCL}")
endif()

if(MSVC)
#set up standard defines from the common WDK path.
bs_set_wdk(${IGC_BUILD__PROJ__ElfPackager})
endif()

if(LLVM_ON_UNIX)
  add_subdirectory(linux)
  #NOTE: Transfer names of projects to parent scope.
  set(IGC_BUILD__PROJ__BiFLib_OCL "${IGC_BUILD__PROJ__BiFLib_OCL}" PARENT_SCOPE)
endif()
