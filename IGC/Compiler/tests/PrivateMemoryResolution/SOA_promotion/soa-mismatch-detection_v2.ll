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

%"class.oneapi::dpl::linear_congruential_engine.137" = type { %"class.sycl::_V1::vec.98" }
%"class.sycl::_V1::vec.98" = type { %"struct.std::array.102" }
%"struct.std::array.102" = type { [16 x i64] }

; Function Attrs: nofree nosync nounwind
define spir_kernel void @test(ptr nocapture writeonly %d, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr nocapture readnone %privateBase) {
entry:
    ; CHECK:        [[SIMDSIZE:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
    ; CHECK-NOT:    add i32 %baseAddOffset

    ; CHECK:        getelementptr <8 x i32>
    ; CHECK-NEXT:   load <8 x i32>

    %alloca1 = alloca %"class.oneapi::dpl::linear_congruential_engine.137", align 128
    %gep1 = getelementptr <8 x i32>, ptr %alloca1, i32 3
    %load1 = load <8 x i32>, ptr %gep1, align 32

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
