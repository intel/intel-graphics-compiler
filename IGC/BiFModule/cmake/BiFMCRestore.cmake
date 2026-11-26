#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

# Restores the BiF package from the prebuild cache if it can and writes "hit" or "miss"
# to IGC_BIF_STATE. The file changes only with the decision, so the build steps rerun only then.
#
# -DIGC_BIF_CONFIG=<file> -DIGC_BIF_STATE=<file> -DIGC_BIF_STAMP=<file>

include("${IGC_BIF_CONFIG}")
include(${CMAKE_CURRENT_LIST_DIR}/BiFMCConst.cmake)

function(igc_bif_set_state state)
  set(current "")
  if(EXISTS "${IGC_BIF_STATE}")
    file(READ "${IGC_BIF_STATE}" current)
  endif()
  if(NOT current STREQUAL state)
    file(WRITE "${IGC_BIF_STATE}" "${state}")
  endif()
  file(TOUCH "${IGC_BIF_STAMP}")
endfunction()


execute_process( COMMAND ${CMAKE_COMMAND} -E compare_files ${BiFModule_PREBUILD_SHA_PATH} ${BiFModule_SRC_SHA_PATH}
                 RESULT_VARIABLE compare_result
)

# When debugging or performing experiments with e.g LLVM Core codebase
# The BiF was being rebuilt when there was no reason to do so.
# For example: You've just set up IGC workspace from scratch and fully built it.
# Then you decided to add additional log / print / whatever to e.g InstCombiner.
# Such change does not affect the result of InstCombiner, but the BiF was being rebuild anyway,
# which takes time, which very significantly slows down the development and increases feedback loop.

# This env variable allows developer to prevent BiF from being created/recreated and is intended for debug/experimental purposes only.

if($ENV{IGC_DBG_SKIP_BIF_GENERATION})
  message("[IGC\\BiFModuleCache] Skipping BiF building/rebuilding due to env. variable ['IGC_DBG_SKIP_BIF_GENERATION'] being set. This is intended for debug/experimental purposes only, use with caution.")
  igc_bif_set_state(hit)
  return()
endif()

if(compare_result EQUAL 0)
  message("[IGC\\BiFModuleCache] - Reusing the BiFCache")
  execute_process(COMMAND "${CMAKE_COMMAND}" -E remove_directory "${IGC_BUILD__BIF_DIR}")
  execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_directory "${BiFModule_PreBuild_PATH}" "${IGC_BUILD__BIF_DIR}")
  igc_bif_set_state(hit)
else()
  message("[IGC\\BiFModuleCache] - Building BiF from source")
  igc_bif_set_state(miss)
endif()
