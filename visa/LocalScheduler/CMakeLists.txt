# This file sets up the following libraries and executables:
# Target name                  |     CMake project name             |   Supported Platforms
#------------------------------+------------------------------------+---------------------------
# LocalSheduler.lib            |     LocalScheduler                 |  Windows
# libLocalScheduler.a          |     LocalScheduler                 |  Linux, Android

if(WIN32)
cmake_minimum_required(VERSION 3.1)
cmake_policy(SET CMP0043 OLD)
else()
cmake_minimum_required(VERSION 2.8)
endif(WIN32)

# Set up windows mobile build flags to use dynamic multi-threaded runtime (/MD)
# Set up windows(other) build flags to use static multi-threaded runtime (/MT)
win_static_runtime()

# ###############################################################
# LocalScheduler (lib)
# ###############################################################

include_directories(../include)
if(ANDROID AND MEDIA_IGA)
  set(NDK_Libstdcxx $ENV{ANDROID_NDK}/sources/cxx-stl/llvm-libc++/libcxx)
  set(Extra_Android_Inc ${NDK_Libstdcxx}/include ${ANDROID_NDK}/sources/android/support/include
    ${NDK_Libstdcxx}/include/backward)
  include_directories(${Extra_Android_Inc})
endif(ANDROID AND MEDIA_IGA)

  set(LocalScheduler_SOURCES
    Dependencies_G4IR.cpp
    LocalScheduler_G4IR.cpp
    LatencyTable.cpp
    G4_Sched.cpp)

  set(LocalScheduler_HEADERS
    Dependencies_G4IR.h
    LatencyTable.h
    LocalScheduler_G4IR.h)

if (WIN32 AND NOT IGC_BUILD)
  add_compile_options(/WX)
endif (WIN32 AND NOT IGC_BUILD)

#disable exceptions, windows only for now
if(WIN32)
  string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  add_definitions(-D_HAS_EXCEPTIONS=0)
endif()

add_library(LocalScheduler STATIC ${LocalScheduler_SOURCES} ${LocalScheduler_HEADERS})

if(IGC_BUILD AND MSVC)
#set up standard defines from the common WDK path.
bs_set_wdk(LocalScheduler)
endif()

add_dependencies(LocalScheduler check_headers)
source_group("Header Files" FILES ${LocalScheduler_HEADERS})
if( WIN32 AND ("${BUILD_WINDOWS_MOBILE}" STREQUAL "TRUE") )
      set_target_properties( LocalScheduler PROPERTIES COMPILE_DEFINITIONS _ATL_NO_WIN_SUPPORT )
endif()
