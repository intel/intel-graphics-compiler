/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cm-cl/exec.h>

using namespace cm;

CM_NODEBUG CM_INLINE ulong __spirv_BuiltInGlobalInvocationId(int dim) {
  return cm::exec::get_local_id(dim) +
         static_cast<ulong>(cm::exec::get_group_id(dim)) *
             cm::exec::get_local_size(dim);
}

CM_NODEBUG CM_INLINE ulong __spirv_BuiltInWorkgroupSize(int dim) {
  return cm::exec::get_local_size(dim);
}

CM_NODEBUG CM_INLINE ulong __spirv_BuiltInLocalInvocationId(int dim) {
  return cm::exec::get_local_id(dim);
}

CM_NODEBUG CM_INLINE ulong __spirv_BuiltInWorkgroupId(int dim) {
  return cm::exec::get_group_id(dim);
}

CM_NODEBUG CM_INLINE ulong __spirv_BuiltInGlobalSize(int dim) {
  return static_cast<ulong>(cm::exec::get_local_size(dim)) *
         cm::exec::get_group_count(dim);
}

CM_NODEBUG CM_INLINE ulong __spirv_BuiltInNumWorkgroups(int dim) {
  return cm::exec::get_group_count(dim);
}
