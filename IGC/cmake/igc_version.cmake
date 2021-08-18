#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

set(IGC_ROOT_SOURCE_DIR "${IGC_BUILD__IGC_SRC_DIR}/../..")

find_program(GIT NAMES git)
if(GIT)
  if(IS_DIRECTORY ${IGC_ROOT_SOURCE_DIR}/.git)
    set(GIT_arg --git-dir=${IGC_ROOT_SOURCE_DIR}/.git rev-parse HEAD)
    execute_process(
                    COMMAND ${GIT} ${GIT_arg}
                    OUTPUT_VARIABLE IGC_REVISION
                    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  endif()
endif()

