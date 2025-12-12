#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(IGC_BUILD__PROJ__IBiF_matrix_generator       "${IGC_BUILD__PROJ_NAME_PREFIX}IBiF_matrix_generator")
set(IGC_BUILD__PROJ__IBiF_matrix_generator       "${IGC_BUILD__PROJ__IBiF_matrix_generator}" PARENT_SCOPE)

add_executable("${IGC_BUILD__PROJ__IBiF_matrix_generator}" IBiF_matrix_generator.cpp)

add_custom_command(
  TARGET "${IGC_BUILD__PROJ__IBiF_matrix_generator}"
  POST_BUILD
  COMMAND IBiF_matrix_generator
  COMMENT "Running IBiF_matrix_generator"
)
