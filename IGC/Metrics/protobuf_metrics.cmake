#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

if(IGC_METRICS)

  set(protobuf_generate_PROTOC_OUT_DIR "${IGC_OPTION__OUTPUT_DIR}/Metrics")
  file(MAKE_DIRECTORY ${protobuf_generate_PROTOC_OUT_DIR})
  set(IGC_METRICS_PROTO_SCHEMA "Metrics/proto_schema")
  # For protobuf compiler setup
  set(Protobuf_IMPORT_DIRS "${IGC_METRICS_PROTO_SCHEMA}")
  file(GLOB IGC_METRICS_PROTO_SCHEMAS ${IGC_METRICS_PROTO_SCHEMA}/*.proto)

  message(STATUS "IGC\\Metrics - Preparing data template for Metrics")

  foreach(PROTO_SCHEMA ${IGC_METRICS_PROTO_SCHEMAS})
    protobuf_generate_imm_cpp(IGC_METRICS_SRC IGC_METRICS_HDR ${PROTO_SCHEMA})
    list(APPEND IGC_METRICS_SRCS ${IGC_METRICS_SRC})
    list(APPEND IGC_METRICS_HDRS ${IGC_METRICS_HDR})
  endforeach()

  get_target_property(PROTOBUF_HDRS protobuf::libprotobuf INTERFACE_INCLUDE_DIRECTORIES)

  include_directories(${PROTOBUF_HDRS})

  foreach(IGC_METRICS_SRC ${IGC_METRICS_SRCS})
    if(MSVC)
      set_source_files_properties(${IGC_METRICS_SRC} PROPERTIES COMPILE_FLAGS /wd4244)
    else()
      set_source_files_properties(${IGC_METRICS_SRC} PROPERTIES COMPILE_FLAGS -Wno-error)
    endif()
  endforeach()

  add_compile_definitions(IGC_METRICS__PROTOBUF_ATTACHED)
else()
  message(STATUS "IGC\\Metrics - metrics are disabled")
  set(IGC_METRICS_SRCS "")
  set(IGC_METRICS_HDRS "")
endif()

list(APPEND IGC_METRICS_SRCS "Metrics/IGCMetric.cpp")
list(APPEND IGC_METRICS_SRCS "Metrics/IGCMetricImpl.cpp")
list(APPEND IGC_METRICS_HDRS "Metrics/IGCMetric.h")
list(APPEND IGC_METRICS_HDRS "Metrics/IGCMetricImpl.h")

add_library(igc_metric STATIC ${IGC_METRICS_SRCS} ${IGC_METRICS_HDRS})

add_dependencies(igc_metric intrinsics_gen)

if(IGC_METRICS)
  target_link_libraries(igc_metric protobuf::libprotobuf-lite)
endif()