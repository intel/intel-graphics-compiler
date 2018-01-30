cmake_minimum_required(VERSION 3.2.0 FATAL_ERROR)

add_subdirectory(IGC)

list(APPEND IGC__IGC_TARGETS "igc_dll")
list(APPEND IGC__IGC_TARGETS "IGA_DLL")
list(APPEND IGC__IGC_TARGETS "fcl_dll")
list(APPEND IGC__IGC_TARGETS "common_clang_dll")

set(IGC__IGC_TARGETS "${IGC__IGC_TARGETS}" PARENT_SCOPE)

