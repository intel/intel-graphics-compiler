;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check new implicit arg is added for __builtin_IB_get_snap_wa_reqd when
; sampler is bindless (offset is or'ed with 1).

; REQUIRES: opaque-ptr-fix, llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-image-func-analysis -igc-serialize-metadata -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

%spirv.Sampler = type opaque

declare i32 @__builtin_IB_get_snap_wa_reqd(i32)

define i32 @foo(%spirv.Sampler addrspace(2)* %sampler) nounwind {
  %1 = ptrtoint %spirv.Sampler addrspace(2)* %sampler to i64
  %2 = or i64 %1, 1
  %3 = trunc i64 %2 to i32
  %id = call i32 @__builtin_IB_get_snap_wa_reqd(i32 %3)
  ret i32 %id
}

!igc.functions = !{!0}
!0 = !{i32 (%spirv.Sampler addrspace(2)*)* @foo, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

;CHECK: !{!"implicitArgInfoList"
;CHECK: !{!"argId", i32 31}
