#=========================== begin_copyright_notice ============================
#
# Copyright (C) 2018-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
#============================ end_copyright_notice =============================

include "llvm/TableGen/SearchableTable.td"
include "llvm/IR/Intrinsics.td"

def AMDGPUImageDMaskIntrinsicTable : GenericTable {
  let FilterClass = "AMDGPUImageDMaskIntrinsic";
  let Fields = ["Intr"];

  let PrimaryKey = ["Intr"];
  let PrimaryKeyName = "getAMDGPUImageDMaskIntrinsic";
  let PrimaryKeyEarlyOut = 1;
}
