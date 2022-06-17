#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2022 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

find_package(Git)
if(NOT Git_FOUND)
  error("Git executable not found.")
endif()

execute_process(
  COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD .
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/proto_schema
  OUTPUT_VARIABLE IGCMetricsVer
  RESULT_VARIABLE IGCMetricsVer_RetCode
  )

if(${IGCMetricsVer_RetCode} EQUAL 0)
  message(STATUS "IGC\\Metrics - Data layout version ${IGCMetricsVer}")
else()
  set(IGCMetricsVer "0")
  message(STATUS "IGC\\Metrics - Data layout version not known - set to 0")
endif()

file(WRITE ${IGCMetricsVerFile} "#define IGCMetricsVer ${IGCMetricsVer}")