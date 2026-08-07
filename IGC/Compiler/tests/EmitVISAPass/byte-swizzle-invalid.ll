;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: system-windows
; REQUIRES: debug, regkeys
; RUN: split-file %s %t
; RUN: igc_opt -platformbmg -igc-emit-visa -simd-mode 16 %t/nonconstant.ll --regkey=EnableAssertEvaluation=1 --regkey=EnableAssertProgramTermination=0 --regkey=EnableLogAssertToStderr=1 2>&1 | FileCheck %s --check-prefix=NONCONSTANT
; RUN: igc_opt -platformbmg -igc-emit-visa -simd-mode 16 %t/out-of-range.ll --regkey=EnableAssertEvaluation=1 --regkey=EnableAssertProgramTermination=0 --regkey=EnableLogAssertToStderr=1 2>&1 | FileCheck %s --check-prefix=OUT-OF-RANGE

; Invalid masks must assert in debug builds before instruction emission starts
; with a partially validated mask.

; NONCONSTANT: Byte swizzle indices must be constant
; OUT-OF-RANGE: Byte swizzle indices must be in the range [0, 3]

;--- nonconstant.ll
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test(i16 %first, i16 %second, <4 x i32> %indices, i32 addrspace(1)* %out) {
  %swizzled = call <4 x i8> @llvm.genx.GenISA.byte.swizzle(i16 %first, i16 %second, <4 x i32> %indices)
  %packed = bitcast <4 x i8> %swizzled to i32
  store i32 %packed, i32 addrspace(1)* %out, align 4
  ret void
}

declare <4 x i8> @llvm.genx.GenISA.byte.swizzle(i16, i16, <4 x i32>)

!igc.functions = !{!0}
!IGCMetadata = !{!3}
!0 = !{void (i16, i16, <4 x i32>, i32 addrspace(1)*)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = !{!"FuncMDMap[0]", void (i16, i16, <4 x i32>, i32 addrspace(1)*)* @test}
!6 = !{!"FuncMDValue[0]", !7}
!7 = !{!"resAllocMD", !8}
!8 = !{!"argAllocMDList", !9, !10, !11, !12}
!9 = !{!"argAllocMDListVec[0]", !13, !14, !15}
!10 = !{!"argAllocMDListVec[1]", !13, !14, !15}
!11 = !{!"argAllocMDListVec[2]", !13, !14, !15}
!12 = !{!"argAllocMDListVec[3]", !13, !14, !15}
!13 = !{!"type", i32 0}
!14 = !{!"extensionType", i32 -1}
!15 = !{!"indexType", i32 -1}

;--- out-of-range.ll
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test(i16 %first, i16 %second, i32 addrspace(1)* %out) {
  %swizzled = call <4 x i8> @llvm.genx.GenISA.byte.swizzle(i16 %first, i16 %second, <4 x i32> <i32 0, i32 1, i32 2, i32 4>)
  %packed = bitcast <4 x i8> %swizzled to i32
  store i32 %packed, i32 addrspace(1)* %out, align 4
  ret void
}

declare <4 x i8> @llvm.genx.GenISA.byte.swizzle(i16, i16, <4 x i32>)

!igc.functions = !{!0}
!IGCMetadata = !{!3}
!0 = !{void (i16, i16, i32 addrspace(1)*)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = !{!"FuncMDMap[0]", void (i16, i16, i32 addrspace(1)*)* @test}
!6 = !{!"FuncMDValue[0]", !7}
!7 = !{!"resAllocMD", !8}
!8 = !{!"argAllocMDList", !9, !10, !11}
!9 = !{!"argAllocMDListVec[0]", !12, !13, !14}
!10 = !{!"argAllocMDListVec[1]", !12, !13, !14}
!11 = !{!"argAllocMDListVec[2]", !12, !13, !14}
!12 = !{!"type", i32 0}
!13 = !{!"extensionType", i32 -1}
!14 = !{!"indexType", i32 -1}
