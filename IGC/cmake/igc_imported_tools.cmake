#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include_guard(DIRECTORY)

function(igc_imported_tool target executable)
  if(DEFINED IGC_TOOLS_BINARY_DIR)
    if(EXISTS "${IGC_TOOLS_BINARY_DIR}/${executable}")
     add_executable("${target}" IMPORTED GLOBAL)
     set_target_properties("${target}" PROPERTIES
        IMPORTED_LOCATION "${IGC_TOOLS_BINARY_DIR}/${executable}" )
    else()
      message(WARNING "Executable ${IGC_TOOLS_BINARY_DIR}/${executable} not found. Will not use imported target")
    endif()
  endif()
endfunction()
