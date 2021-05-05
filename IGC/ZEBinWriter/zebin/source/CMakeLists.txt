#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(ZE_INFO_SOURCE_FILE
    ${CMAKE_CURRENT_SOURCE_DIR}/autogen/ZEInfoYAML.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ZEELFObjectBuilder.cpp
    PARENT_SCOPE
)
set(ZE_INFO_INCLUDE_FILE
    ${CMAKE_CURRENT_SOURCE_DIR}/ZEELF.h
    ${CMAKE_CURRENT_SOURCE_DIR}/autogen/ZEInfo.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/autogen/ZEInfoYAML.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ZEELFObjectBuilder.hpp
    PARENT_SCOPE
)
