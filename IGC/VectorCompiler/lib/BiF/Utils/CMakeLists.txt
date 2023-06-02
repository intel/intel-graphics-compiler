#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(EMU_DIVREM_BOILERPLATE_FILE ${CMAKE_CURRENT_BINARY_DIR}/emulation_divrem_boilerplate.h)
set(EMU_DIVREM_BOILERPLATE_GEN ${CMAKE_CURRENT_SOURCE_DIR}/emulation_divrem_boilerplate.py)

add_custom_command(OUTPUT ${EMU_DIVREM_BOILERPLATE_FILE}
    COMMAND ${PYTHON_EXECUTABLE} ${EMU_DIVREM_BOILERPLATE_GEN} ${EMU_DIVREM_BOILERPLATE_FILE}
    COMMENT "generation of boilerplate for integer division emulation"
    DEPENDS ${EMU_DIVREM_BOILERPLATE_GEN})

add_custom_target(VCEmuDivremBoilerplate SOURCES ${EMU_DIVREM_BOILERPLATE_FILE})

set(EMU_FDIV_BOILERPLATE_FILE ${CMAKE_CURRENT_BINARY_DIR}/emulation_fdiv_boilerplate.h)
set(EMU_FDIV_BOILERPLATE_GEN ${CMAKE_CURRENT_SOURCE_DIR}/emulation_fdiv_boilerplate.py)

add_custom_command(OUTPUT ${EMU_FDIV_BOILERPLATE_FILE}
    COMMAND ${PYTHON_EXECUTABLE} ${EMU_FDIV_BOILERPLATE_GEN} ${EMU_FDIV_BOILERPLATE_FILE}
    COMMENT "generation of boilerplate for double division emulation"
    DEPENDS ${EMU_FDIV_BOILERPLATE_GEN})

add_custom_target(VCEmuFDivBoilerplate SOURCES ${EMU_FDIV_BOILERPLATE_FILE})

set(EMU_FSQRT_BOILERPLATE_FILE ${CMAKE_CURRENT_BINARY_DIR}/emulation_fsqrt_boilerplate.h)
set(EMU_FSQRT_BOILERPLATE_GEN ${CMAKE_CURRENT_SOURCE_DIR}/emulation_fsqrt_boilerplate.py)

add_custom_command(OUTPUT ${EMU_FSQRT_BOILERPLATE_FILE}
    COMMAND ${PYTHON_EXECUTABLE} ${EMU_FSQRT_BOILERPLATE_GEN} ${EMU_FSQRT_BOILERPLATE_FILE}
    COMMENT "generation of boilerplate for double sqrt emulation"
    DEPENDS ${EMU_FSQRT_BOILERPLATE_GEN})

add_custom_target(VCEmuFSqrtBoilerplate SOURCES ${EMU_FSQRT_BOILERPLATE_FILE})

add_library(VCBifUtils INTERFACE)
target_include_directories(VCBifUtils INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(VCBifUtils VCEmuDivremBoilerplate)
add_dependencies(VCBifUtils VCEmuFDivBoilerplate)
add_dependencies(VCBifUtils VCEmuFSqrtBoilerplate)
