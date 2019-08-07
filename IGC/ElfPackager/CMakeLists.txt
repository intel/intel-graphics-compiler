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
elseif(NOT IGC_BUILD__USING_SYSTEM_LLVM)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${LLVM_TOOLS_BINARY_DIR})
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${LLVM_TOOLS_BINARY_DIR})
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${LLVM_TOOLS_BINARY_DIR})
endif()

set(IGC_BUILD__PROJ__ElfPackager       "${IGC_BUILD__PROJ_NAME_PREFIX}elf_packager")
set(IGC_BUILD__PROJ__ElfPackager       "${IGC_BUILD__PROJ__ElfPackager}" PARENT_SCOPE)
set(IGC_BUILD__PROJ_LABEL__ElfPackager "${IGC_BUILD__PROJ__ElfPackager}")

set(IGC_BUILD__SRC__ElfPackager
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

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

add_custom_command(TARGET ${IGC_BUILD__PROJ__ElfPackager}
                   POST_BUILD
                   COMMAND $<TARGET_FILE:${IGC_BUILD__PROJ__ElfPackager}> -includeSizet -funcList ${CMAKE_CURRENT_SOURCE_DIR}/function_bin.txt ${IGC_BUILD__BIF_DIR}/OCLBiFImpl.bc ${IGC_BUILD__BIF_DIR}/igdclbif.bin
                  )


add_dependencies("${IGC_BUILD__PROJ__ElfPackager}" "${IGC_BUILD__PROJ__BiFModule_OCL}")

if(MSVC)
#set up standard defines from the common WDK path.
bs_set_wdk(${IGC_BUILD__PROJ__ElfPackager})
endif()

if(LLVM_ON_UNIX)
  add_subdirectory(linux)
  #NOTE: Transfer names of projects to parent scope.
  set(IGC_BUILD__PROJ__BiFLib_OCL "${IGC_BUILD__PROJ__BiFLib_OCL}" PARENT_SCOPE)
endif()
