#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# Runs one step of the BiF build: the command after "--", unless the BiF
# package was restored from the prebuild cache (BiFMCRestore.cmake).
#
# -DIGC_BIF_STATE=<file>   "hit" when the package was restored
# -DIGC_BIF_CREATE=<paths> outputs to create when the step is skipped, so that
#                          the build tool sees them up to date
# -DIGC_BIF_KEEP=<paths>   outputs restored from the cache, touched when skipped
# Paths are separated by "|".

string(REPLACE "|" ";" IGC_BIF_CREATE "${IGC_BIF_CREATE}")
string(REPLACE "|" ";" IGC_BIF_KEEP "${IGC_BIF_KEEP}")
file(READ "${IGC_BIF_STATE}" state)
if(state STREQUAL "hit")
  foreach(output IN LISTS IGC_BIF_CREATE)
    file(TOUCH "${output}")
  endforeach()
  foreach(output IN LISTS IGC_BIF_KEEP)
    file(TOUCH_NOCREATE "${output}")
  endforeach()
  return()
endif()

set(command)
set(found FALSE)
math(EXPR last "${CMAKE_ARGC} - 1")
foreach(i RANGE ${last})
  if(found)
    list(APPEND command "${CMAKE_ARGV${i}}")
  elseif(CMAKE_ARGV${i} STREQUAL "--")
    set(found TRUE)
  endif()
endforeach()

execute_process(COMMAND ${command} RESULT_VARIABLE result)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "[IGC\\BiFModule] - Command failed (${result}): ${command}")
endif()
