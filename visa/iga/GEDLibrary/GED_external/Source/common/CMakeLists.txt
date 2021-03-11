#===================== begin_copyright_notice ==================================

#Copyright (c) 2017 Intel Corporation

#Permission is hereby granted, free of charge, to any person obtaining a
#copy of this software and associated documentation files (the
#"Software"), to deal in the Software without restriction, including
#without limitation the rights to use, copy, modify, merge, publish,
#distribute, sublicense, and/or sell copies of the Software, and to
#permit persons to whom the Software is furnished to do so, subject to
#the following conditions:

#The above copyright notice and this permission notice shall be included
#in all copies or substantial portions of the Software.

#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#======================= end_copyright_notice =============================
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
