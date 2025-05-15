;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; To test using vector as the final value. The sub-values have constant, which
; is taken as either float or int.

; REQUIRES: llvm-14-plus, regkeys

; RUN: igc_opt --opaque-pointers %s -S -inputocl -igc-ldstcombine -regkey=EnableLdStCombine=1 \
; RUN:           -platformbmg \
; RUN: | FileCheck %s

; CHECK-LABEL: define spir_kernel void @test_vector
; CHECK:       call void @llvm.genx.GenISA.PredicatedStore.p1.v2f64(ptr addrspace(1) %{{.*}}, <2 x double>
; CHECK:       call void @llvm.genx.GenISA.PredicatedStore.p1.v2i64(ptr addrspace(1) %{{.*}}, <2 x i64>
; CHECK:       ret

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%struct.dw1_t = type { i64, double }

; Function Attrs: convergent nounwind
define spir_kernel void @test_vector(i8 addrspace(1)* %d, i64 addrspace(1)* %sl, double addrspace(1)* %sd, i32 %offset, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ) #0 {
entry:
  %conv.i.i = zext i16 %localIdX to i64
  %arrayidx = getelementptr inbounds double, double addrspace(1)* %sd, i64 %conv.i.i
  %0 = load double, double addrspace(1)* %arrayidx, align 8
  %arrayidx1 = getelementptr inbounds i64, i64 addrspace(1)* %sl, i64 %conv.i.i
  %1 = load i64, i64 addrspace(1)* %arrayidx1, align 8
  %2 = bitcast i8 addrspace(1)* %d to %struct.dw1_t addrspace(1)*
  %x0.sroa.0.0..sroa_idx = getelementptr inbounds %struct.dw1_t, %struct.dw1_t addrspace(1)* %2, i64 %conv.i.i, i32 0
  call void @llvm.genx.GenISA.PredicatedStore.p1i64.i64(i64 addrspace(1)* %x0.sroa.0.0..sroa_idx, i64 65793, i64 8, i1 true)
  %x0.sroa.4.0..sroa_idx5 = getelementptr inbounds %struct.dw1_t, %struct.dw1_t addrspace(1)* %2, i64 %conv.i.i, i32 1
  call void @llvm.genx.GenISA.PredicatedStore.p1f64.f64(double addrspace(1)* %x0.sroa.4.0..sroa_idx5, double %0, i64 8, i1 true)
  %conv = sext i32 %offset to i64
  %add = add nsw i64 %conv.i.i, %conv
  %x1.sroa.0.0..sroa_idx = getelementptr inbounds %struct.dw1_t, %struct.dw1_t addrspace(1)* %2, i64 %add, i32 0
  call void @llvm.genx.GenISA.PredicatedStore.p1i64.i64(i64 addrspace(1)* %x1.sroa.0.0..sroa_idx, i64 %1, i64 8, i1 true)
  %x1.sroa.4.0..sroa_idx2 = getelementptr inbounds %struct.dw1_t, %struct.dw1_t addrspace(1)* %2, i64 %add, i32 1
  call void @llvm.genx.GenISA.PredicatedStore.p1f64.f64(double addrspace(1)* %x1.sroa.4.0..sroa_idx2, double 1.000000e+00, i64 8, i1 true)
  ret void
}

declare void @llvm.genx.GenISA.PredicatedStore.p1i64.i64(i64 addrspace(1)*, i64, i64, i1)
declare void @llvm.genx.GenISA.PredicatedStore.p1f64.f64(double addrspace(1)*, double, i64, i1)


attributes #0 = { convergent nounwind "less-precise-fpmad"="true" "null-pointer-is-valid"="true" }

!igc.functions = !{!341}

!341 = !{void (i8 addrspace(1)*, i64 addrspace(1)*, double addrspace(1)*, i32, <8 x i32>, <8 x i32>, i16, i16, i16)* @test_vector, !342}
!342 = !{!343, !344}
!343 = !{!"function_type", i32 0}
!344 = !{!"implicit_arg_desc", !345, !346, !347, !348, !349}
!345 = !{i32 0}
!346 = !{i32 1}
!347 = !{i32 8}
!348 = !{i32 9}
!349 = !{i32 10}
