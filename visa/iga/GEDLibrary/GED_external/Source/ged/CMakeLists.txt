#####
# - create examples of using GED
#####
cmake_minimum_required(VERSION 3.15)
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
set(GED_CXX_FLAGS ${CMAKE_CXX_FLAGS} -DGED_BUILDING_EXECUTABLE -DGED_DISASSEMBLY=1)
include(${CMAKE_SOURCE_DIR}/../../build/autogen-${TARGET}/CMakeLists.txt)

add_executable(gedexe ged.cpp)
set_target_properties(gedexe PROPERTIES OUTPUT_NAME ged)
target_link_libraries(gedexe ${GED_LIBRARY})
target_include_directories(gedexe PRIVATE ${GED_INCLUDE} ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/..)
target_compile_options(gedexe PRIVATE ${GED_CXX_FLAGS})
target_link_options(gedexe PRIVATE ${GED_EXE_LINKER_FLAGS})


