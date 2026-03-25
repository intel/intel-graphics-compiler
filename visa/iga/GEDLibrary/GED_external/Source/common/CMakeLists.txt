#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# GED_external/Source/common
set(GED_common_cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_basic_types.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_compact_mapping_table.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_enum_types_internal.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_ins_decoding_table.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_ins_encoding_masks.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_ins_position_fragment.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_ins_restrictions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_interpreter_types.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_int_utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_option_parser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_string_utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_types_internal.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_validation_utils.cpp
  PARENT_SCOPE
)
# GED_external/Source/common
set(GED_common_h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_base.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_basic_types.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_compact_mapping_table.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_enum_types.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_enum_types_internal.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_ins_decoding_table.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_ins_encoding_masks.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_ins_position_fragment.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_ins_restrictions.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_interpreter_types.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_int_utils.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_option_parser.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_string_utils.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_types_internal.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_validation_utils.h
  PARENT_SCOPE
)
