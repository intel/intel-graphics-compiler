;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, llvm-16-plus
;
; Verify that nested pointer PHIs are both split.
;
; RUN: igc_opt --opaque-pointers --ocl --platformdg2 --igc-split-phis-of-alloca-pointers \
; RUN:   --regkey EnablePHIOfAllocaPtrSplit=1 --regkey EnableSOAAcrossCalls=1 -S %s | FileCheck %s

; CHECK-LABEL: define spir_kernel void @k(
; CHECK-NOT:   phi ptr
; CHECK:     m1:
; CHECK:       [[ISA:%.*]] = phi i1 [ true, %ina ], [ false, %inb ]
; CHECK:       [[ISB:%.*]] = phi i1 [ false, %ina ], [ true, %inb ]
; CHECK:     m2:
; CHECK:       [[ISP1:%.*]] = phi i1 [ true, %m1 ], [ false, %x ]
; CHECK:       [[ISX:%.*]] = phi i1 [ false, %m1 ], [ true, %x ]
; CHECK:     use:
; CHECK:       [[EA:%.*]] = getelementptr inbounds float, ptr %a, i32 %i
; CHECK:       [[PA:%.*]] = and i1 [[ISA]], [[ISP1]]
; CHECK:       [[VA:%.*]] = call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr [[EA]], i64 4, i1 [[PA]], float poison)
; CHECK:       [[NODE:%.*]] = addrspacecast ptr addrspace(1) %node to ptr
; CHECK:       [[PB:%.*]] = and i1 [[ISB]], [[ISP1]]
; CHECK:       [[VB:%.*]] = call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr [[NODE]], i64 4, i1 [[PB]], float poison)
; CHECK:       [[VP1:%.*]] = select i1 [[ISA]], float [[VA]], float [[VB]]
; CHECK:       [[EX:%.*]] = getelementptr inbounds float, ptr %a, i32 %j
; CHECK:       [[VX:%.*]] = call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr [[EX]], i64 4, i1 [[ISX]], float poison)
; CHECK:       [[V:%.*]] = select i1 [[ISP1]], float [[VP1]], float [[VX]]
; CHECK:       store float [[V]], ptr addrspace(1) %out, align 4
; CHECK-NOT:   phi ptr
define spir_kernel void @k(ptr addrspace(1) %out, ptr addrspace(1) %node, i32 %i, i32 %j, i1 %c1, i1 %c2, i1 %c3) {
entry:
  %a = alloca [64 x float], align 4
  br i1 %c1, label %ina, label %inb
ina:
  %ea = getelementptr inbounds float, ptr %a, i32 %i
  br label %m1
inb:
  %nodep = addrspacecast ptr addrspace(1) %node to ptr
  br label %m1
m1:
  %p1 = phi ptr [ %ea, %ina ], [ %nodep, %inb ]
  br i1 %c2, label %x, label %m2
x:
  %ex = getelementptr inbounds float, ptr %a, i32 %j
  br label %m2
m2:
  %p2 = phi ptr [ %p1, %m1 ], [ %ex, %x ]
  br i1 %c3, label %use, label %exit
use:
  %v = load float, ptr %p2, align 4
  store float %v, ptr addrspace(1) %out, align 4
  br label %exit
exit:
  ret void
}

!igc.functions = !{!0}
!0 = !{ptr @k, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
