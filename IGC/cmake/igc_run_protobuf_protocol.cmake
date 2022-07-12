#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

message(STATUS "Running ${protobuf_generate_LANGUAGE} protocol buffer compiler on ${proto}")
execute_process(
  COMMAND ${protobuf-comp} --${protobuf_generate_LANGUAGE}_out ${dll_export_decl}${protobuf_generate_PROTOC_OUT_DIR} ${dll_desc_out} ${protobuf_include_path} ${abs_file}
  WORKING_DIRECTORY ${WRK_DIR_PROTO}
  RESULT_VARIABLE rv
  OUTPUT_VARIABLE ov
  ERROR_VARIABLE ev
)

if(${rv} EQUAL "0")
  message(STATUS "Protobuf compiler : successfully finished the compilation of the schema, ${abs_file}")
else()
  message("${protobuf-comp} --${protobuf_generate_LANGUAGE}_out ${dll_export_decl}${protobuf_generate_PROTOC_OUT_DIR} ${dll_desc_out} ${protobuf_include_path} ${abs_file}")
  message("Result='${rv}'")
  message("Output='${ov}'")
  message("Error='${ev}'")
  message(FATAL_ERROR "Protobuf compiler : failed to compile schema ${abs_file}")
endif()
