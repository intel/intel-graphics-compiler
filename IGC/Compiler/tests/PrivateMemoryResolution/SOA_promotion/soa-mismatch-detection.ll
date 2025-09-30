;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys,llvm-16-plus
;
; RUN: igc_opt --ocl --igc-private-mem-resolution --regkey EnablePrivMemNewSOATranspose=1 --regkey EnableOpaquePointersBackend=1 -S %s | FileCheck %s
;
; This test is testing "MismatchDetected" algorithm in LowerGEPForPrivMem.cpp
; The purpose of this test is to validate whether various combinations of allocas/geps/load/stores
; are not mismatched and then failing due to asserts or causing miscalculations at runtime.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%"struct.ispc::vec_t" = type { i32, i32, i32 }

; Function Attrs: nofree nosync nounwind
define spir_kernel void @test(ptr nocapture writeonly %d, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr nocapture readnone %privateBase) {
entry:
; CHECK:    [[SIMDSIZE:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
  %arr = alloca [136 x i8], align 16
  br label %b0

b0:
; This is not valid case since i8 and float have different sizes
; CHECK-NOT: error
  %index = getelementptr inbounds i8, ptr %arr, i64 120
  %load = load <4 x float>, ptr %index, align 8
  br label %exit

exit:
; This is valid case: because i32 and float have 32 bits
; CHECK: insertelement <2 x float> {{.*}}, float {{.*}}, i32 0
; CHECK: insertelement <2 x float> {{.*}}, float {{.*}}, i32 1
  %arr2 = alloca [1024 x i32], align 4
  %l = load <2 x float>, ptr %arr2

; This case is not valid because i32 and i8 have different sizes
; CHECK: %load2 = load i8, ptr {{.*}}
  %arr3 = alloca [512 x i32]
  %load2 = load i8, ptr %arr3

; This case tests whether load the size of allocas scalar type isn't marked as possible SOA layout
; CHECK:    [[LOAD:%.*]] = load <2 x i32>, ptr {{.*}}, align 4
  %arr4 = alloca [2 x double], align 8
  %gep4 = getelementptr inbounds [2 x double], ptr %arr4, i64 0, i64 0
  %load3 = load <2 x i32>, ptr %gep4, align 4

; Case Alloca->Store->Gep->Store: This case is not valid due to different sizes

; CHECK:    store <4 x i32> zeroinitializer, ptr {{.*}}
; CHECK:    [[OFFSET_GEP:%.*]] = getelementptr i8, ptr {{.*}}, i32 16
; CHECK:    store i32 0, ptr [[OFFSET_GEP]], align 4

  %offset.i.i.i.i = alloca [8 x %"struct.ispc::vec_t"], align 4
  store <4 x i32> zeroinitializer, ptr %offset.i.i.i.i, align 4
  %offset_gep = getelementptr i8, ptr %offset.i.i.i.i, i32 16
  store i32 0, ptr %offset_gep, align 4

  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!6}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[1]", ptr @test}
!5 = !{!"FuncMDValue[1]", !2}
!6 = !{ptr @test, !408}
!408 = !{!409, !410}
!409 = !{!"function_type", i32 0}
!410 = !{!"implicit_arg_desc", !411, !412, !413, !414, !415, !416, !417}
!411 = !{i32 0}
!412 = !{i32 1}
!413 = !{i32 7}
!414 = !{i32 8}
!415 = !{i32 9}
!416 = !{i32 10}
!417 = !{i32 13}
