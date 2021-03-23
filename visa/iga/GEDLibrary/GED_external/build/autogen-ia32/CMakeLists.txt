#=========================== begin_copyright_notice ============================
#
# Copyright (c) 2017-2021 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
#============================ end_copyright_notice =============================

# GED_external/build/autogen-ia32
set(GED_autogenia32_cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_api.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_collectors_tables.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_compaction_tables.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_decoding_tables.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_encoding_masks_tables.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_enumerations.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_enum_interpreters.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_enum_tables.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_fragments.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_ins_field.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_mapping_fragments.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_mapping_tables.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_model_gen10.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_model_gen11.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_model_gen7.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_model_gen7_5.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_model_gen8.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_model_gen8_1.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_model_gen9.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_model_gentgl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_restrictions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/version.cpp
  PARENT_SCOPE
)
# GED_external/build/autogen-ia32
set(GED_autogenia32_h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_collectors_tables.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_compaction_tables.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_decoding_tables.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_encoding_masks_tables.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_enumerations.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_enumerations_internal.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_enum_interpreters.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_enum_tables.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_fragments.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_ins_field.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_ins_field_internal.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_mapping_fragments.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_mapping_tables.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_model_gen10.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_model_gen11.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_model_gen7.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_model_gen7_5.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_model_gen8.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_model_gen8_1.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_model_gen9.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_model_gentgl.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ged_restrictions.h
  PARENT_SCOPE
)
