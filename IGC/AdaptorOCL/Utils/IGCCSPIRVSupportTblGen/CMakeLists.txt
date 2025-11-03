#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include(TableGen)

add_tablegen(igcc-spirv-support-tblgen IGCC_YAML_DOCS_TBLGEN
    IGCCSPIRVSupportTblGen.cpp
)

set(LLVM_LINK_COMPONENTS
  Support
  TableGen
)

llvm_update_compile_flags(igcc-spirv-support-tblgen)

# Always link static TableGen explicitly — never rely on libLLVM.so here
# TableGen is never part of libLLVM.so (ref: llvm-project/llvm/tools/llvm-shlib/CMakeLists.txt)
target_link_libraries(igcc-spirv-support-tblgen PRIVATE
  LLVMSupport
  LLVMTableGen
)
