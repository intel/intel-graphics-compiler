#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(IGC_BUILD__PROJ__IBiF_matrix_generator       "${IGC_BUILD__PROJ_NAME_PREFIX}IBiF_matrix_generator")

igc_imported_tool("${IGC_BUILD__PROJ__IBiF_matrix_generator}" "IBiF_matrix_generator")

if(NOT TARGET ${IGC_BUILD__PROJ__IBiF_matrix_generator})
  add_executable("${IGC_BUILD__PROJ__IBiF_matrix_generator}" IBiF_matrix_generator.cpp)
endif()

# Generate header using local or prebuilt tool
set(IBiF_MATRIX_HEADER "${CMAKE_CURRENT_BINARY_DIR}/IBiF_matrix_generated.h")
add_custom_command(
  OUTPUT ${IBiF_MATRIX_HEADER}
  COMMAND "$<TARGET_FILE:${IGC_BUILD__PROJ__IBiF_matrix_generator}>"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Running IBiF_matrix_generator"
  DEPENDS ${IGC_BUILD__PROJ__IBiF_matrix_generator}
)

add_custom_target(IBiF_matrix_generator_run
  DEPENDS ${IBiF_MATRIX_HEADER}
)
