#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

if((${LLVM_VERSION_MAJOR} LESS 16))
    return()
endif()

set(PLUGIN_SOURCES
    Plugin.cpp
)

add_library(NewPMPlugin MODULE ${PLUGIN_SOURCES})

install(TARGETS NewPMPlugin
  LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
  COMPONENT igc-core
  )

target_include_directories(NewPMPlugin
  PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include/"
)
