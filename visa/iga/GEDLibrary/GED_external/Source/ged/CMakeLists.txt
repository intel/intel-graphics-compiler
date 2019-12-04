#####
# - create examples of using GED
#####
cmake_minimum_required(VERSION 3.0)
project(ged)

#####
# determine target type
# - on linux, target type is specified via -DTARGET=[x86|x64]
# - on windows, target type is configured via 'cmake A win32|ia64'
#####
if ((NOT TARGET) OR (CMAKE_HOST_SYSTEM_NAME MATCHES Windows))
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(TARGET "intel64")
  else()
    set(TARGET "ia32")
  endif()
endif()
if ((NOT ${TARGET} STREQUAL "intel64") AND (NOT ${TARGET} STREQUAL "ia32"))
  message(FATAL_ERROR "ERROR: invalid target type, e.g. cmake -DTARGET=[ia32|intel64] ..")
endif()
message(INFO ": Building for ${TARGET} target on ${CMAKE_HOST_SYSTEM_NAME}")

#####
# - include rules for building GED library
#####
include(${CMAKE_SOURCE_DIR}/../../build/autogen-${TARGET}/CMakeLists.txt)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGED_BUILDING_EXECUTABLE")

add_executable(ged ged.cpp)
target_link_libraries(ged ${GED_LIBRARY})
target_include_directories(ged PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/.. ${GED_INCLUDE})


