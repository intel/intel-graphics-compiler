#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2018-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

cmake_minimum_required(VERSION 2.8.12)

set(_autogenScript "${IGC_SOURCE_DIR}/common/autogen.py")
set(_autogenSource "${IGC_SOURCE_DIR}/common/MDFrameWork.h")
set(_autogenTarget "${IGC_CODEGEN_DIR}/MDNodeFunctions.gen")

add_custom_command(OUTPUT "${_autogenTarget}"
                   COMMAND "${PYTHON_EXECUTABLE}" ${_autogenScript} ${_autogenSource} ${_autogenTarget}
                   MAIN_DEPENDENCY ${_autogenSource}
                   DEPENDS ${_autogenScript}
                   COMMENT "Running autogen.py for autogenerating MDFrameWork Implementation.")

add_custom_target(MDAutogen
                  DEPENDS "${_autogenTarget}"
                  COMMENT "Building custom target for Metadata Framework")

set_property(TARGET MDAutogen
             PROPERTY PROJECT_LABEL "MetadataFrameWorkAutogen")
