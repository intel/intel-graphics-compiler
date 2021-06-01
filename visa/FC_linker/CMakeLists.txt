#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

project(FC_EXE)

if (LINK_AS_STATIC_LIB)
  win_static_runtime()
else (LINK_AS_STATIC_LIB)
  win_dynamic_runtime()
endif (LINK_AS_STATIC_LIB)

set(FC_EXE_CPP
  ${CMAKE_CURRENT_SOURCE_DIR}/cm_fc_ld.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/DepGraph.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/PatchInfoDumper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/PatchInfoReader.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/PatchInfoLinker.cpp)

set(FC_EXE_HPP
  ${CMAKE_CURRENT_SOURCE_DIR}/cm_fc_ld.h
  ${CMAKE_CURRENT_SOURCE_DIR}/DepGraph.h
  ${CMAKE_CURRENT_SOURCE_DIR}/PatchInfoDumper.h
  ${CMAKE_CURRENT_SOURCE_DIR}/PatchInfoLinker.h
  ${CMAKE_CURRENT_SOURCE_DIR}/PatchInfoReader.h
  ${CMAKE_CURRENT_SOURCE_DIR}/PatchInfoRecord.h)

source_group("HeaderFiles" FILES ${FC_EXE_HPP})
source_group("SourceFiles" FILES ${FC_EXE_CPP})


if(MSVC)
  add_compile_options(/EHsc)
  add_compile_options(/w14062)
  add_compile_options(/w14189) # unreachable code
  add_compile_options(/w14244) # implicit narrowing
  add_compile_options(/w14245) # signed/unsigned conversion
  add_compile_options(/w14389) # signed/unsigned compare
  add_compile_options(/w14505) # unreferenced local function (static)
  add_compile_options(/WX)
else(MSVC)
  add_compile_options(-fexceptions)
endif(MSVC)

if(IGC_BUILD AND MSVC)
    Set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${IGC_BUILD__TOOLS_OUTPUT_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${IGC_BUILD__TOOLS_OUTPUT_DIR})
endif(IGC_BUILD AND MSVC)

add_executable(FC_EXE ${FC_EXE_CPP} ${FC_EXE_HPP})
if(MSVC_GENERATOR)
  set(FC_EXE_LINK_FLAGS "/INCREMENTAL:NO")
else()
  set(FC_EXE_LINK_FLAGS "")
endif()

if (MSVC)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "")
   if(CMAKE_SIZEOF_VOID_P EQUAL 8) # Only copy 64 build
      install(TARGETS FC_EXE RUNTIME
              DESTINATION ../Test_Tools/Release/x64/Standalones          CONFIGURATIONS Release)
      install(TARGETS FC_EXE RUNTIME
              DESTINATION ../Test_Tools/Release-Internal/x64/Standalones CONFIGURATIONS ReleaseInternal)
      install(TARGETS FC_EXE RUNTIME
              DESTINATION ../Test_Tools/Debug/x64/Standalones            CONFIGURATIONS Debug)
      # Only copy pdb for Debug mode
      install(FILES $<TARGET_PDB_FILE:FC_EXE>
              DESTINATION ../Test_Tools/Debug/x64/Standalones            CONFIGURATIONS Debug)
  endif(CMAKE_SIZEOF_VOID_P EQUAL 8)
elseif (UNIX)
  install(TARGETS FC_EXE RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR} COMPONENT igc-media)
endif (MSVC)

set_target_properties(FC_EXE PROPERTIES
    OUTPUT_NAME                                "fc${TARGET_MODIFIER}"
    COMPILE_DEFINITIONS                        "${FC_EXE_DEFINITIONS}"
    PDB_NAME                                   "fc_exe"
    LINK_FLAGS                                 "${FC_EXE_LINK_FLAGS}"
    FOLDER                                     "FCProjs"
  )

target_include_directories(FC_EXE PUBLIC "../include")

if(NOT WIN32)
  set_target_properties(FC_EXE PROPERTIES PREFIX "")
  target_link_libraries(FC_EXE PUBLIC)
  if(NOT ANDROID)
    target_link_libraries(FC_EXE PUBLIC "-lrt")
  endif()
else()
  target_link_libraries(FC_EXE PUBLIC)
endif()
