#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

add_executable(IBiF_matrix_generator IBiF_matrix_generator.cpp)

add_custom_command(
  TARGET IBiF_matrix_generator
  POST_BUILD
  COMMAND IBiF_matrix_generator
  COMMENT "Running IBiF_matrix_generator"
)
