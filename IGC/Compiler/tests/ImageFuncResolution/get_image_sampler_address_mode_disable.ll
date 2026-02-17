;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-image-func-resolution %s -S -o - | FileCheck %s

; Check that __builtin_IB_get_address_mode, __builtin_IB_get_snap_wa_reqd and
; __builtin_IB_is_normalized_coords builtins are replaced with 0 when sampler
; isn't successfully tracked.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%spirv.Image._void_0_0_0_0_0_0_0 = type opaque

define spir_kernel void @test(i64 %const_reg_qword) {
entry:
  %astype.i = inttoptr i64 %const_reg_qword to %spirv.Image._void_0_0_0_0_0_0_0 addrspace(1)*
  %0 = ptrtoint %spirv.Image._void_0_0_0_0_0_0_0 addrspace(1)* %astype.i to i64
  %sampler_offset = add i64 %0, 128
  %1 = or i64 %sampler_offset, 1
  %conv = trunc i64 %1 to i32
  %call1 = call spir_func i32 @__builtin_IB_get_address_mode(i32 %conv)
  %call2 = call spir_func i32 @__builtin_IB_get_snap_wa_reqd(i32 %conv)
  %call3 = call spir_func i32 @__builtin_IB_is_normalized_coords(i32 %conv)

; CHECK: %add1 = add i32 0, 0
; CHECK: %add2 = add i32 %add1, 0

  %add1 = add i32 %call1, %call2
  %add2 = add i32 %add1, %call3
  ret void
}

declare spir_func i32 @__builtin_IB_get_address_mode(i32)

declare spir_func i32 @__builtin_IB_get_snap_wa_reqd(i32)

declare spir_func i32 @__builtin_IB_is_normalized_coords(i32)

!igc.functions = !{!0}
!IGCMetadata = !{!2}

!0 = !{void (i64)* @test, !1}
!1 = !{}
!2 = !{!"ModuleMD", !3, !7}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", void (i64)* @test}
!5 = !{!"FuncMDValue[0]", !6}
!6 = !{!"localOffsets"}
!7 = !{!"UseBindlessImage", i1 true}
