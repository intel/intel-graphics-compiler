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

add_library(VCBifUtils INTERFACE)
target_include_directories(VCBifUtils INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(VCBifUtils VCEmuDivremBoilerplate)
