#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2020-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================



add_library(AdaptorCM INTERFACE)

igc_get_llvm_targets(LLVM_LIBS
  Support
)

target_link_libraries(AdaptorCM
  INTERFACE
  ${LLVM_LIBS}
)

target_compile_definitions(AdaptorCM
  INTERFACE
  "-DCMFE_WRAPPER_NAME=\"${INSTALL_CMFE_NAME}\""
  )

target_include_directories(AdaptorCM INTERFACE "${CMFE_INCLUDE_DIRS}")
