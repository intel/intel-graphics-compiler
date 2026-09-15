;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers -platformCri -igc-emit-visa %s -simd-mode 16 -regkey DumpVISAASMToConsole,EnableLSC | FileCheck %s

; Test for splitting six DWORDs into 4+2.
; CHECK: .kernel "matmul_kernel_with_block_pointers_batched"
; CHECK: lsc_load_block2d.ugm
; CHECK: lsc_store.ugm {{.*}}:d32x8
; CHECK: lsc_store.ugm {{.*}}:d32x4
; CHECK: lsc_store.ugm {{.*}}:d32x2

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @matmul_kernel_with_block_pointers_batched(ptr %0, <8 x i32> %r0, <3 x i32> %globalOffset, ptr %privateBase, ptr addrspace(1) %indirectDataPointer, ptr addrspace(1) %scratchPointer) #0 {
  %2 = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 0, i32 -1, i32 -1, i32 -1, i32 0, i32 0, i32 16, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0)
  %3 = bitcast <16 x i16> %2 to <8 x i32>
  store <8 x i32> %3, ptr %0, align 32
  %4 = bitcast <16 x i16> %2 to <8 x i32>
  %5 = extractelement <8 x i32> %4, i32 0
  %6 = insertelement <6 x i32> undef, i32 %5, i32 0
  %7 = extractelement <8 x i32> %4, i32 1
  %8 = insertelement <6 x i32> %6, i32 %7, i32 1
  %9 = extractelement <8 x i32> %4, i32 2
  %10 = insertelement <6 x i32> %8, i32 %9, i32 2
  %11 = extractelement <8 x i32> %4, i32 3
  %12 = insertelement <6 x i32> %10, i32 %11, i32 3
  %13 = extractelement <8 x i32> %4, i32 4
  %14 = insertelement <6 x i32> %12, i32 %13, i32 4
  %15 = extractelement <8 x i32> %4, i32 5
  %16 = insertelement <6 x i32> %14, i32 %15, i32 5
  store <6 x i32> %16, ptr addrspace(1) null, align 32
  ret void
}

declare <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32) #1

attributes #0 = { convergent nounwind null_pointer_is_valid }
attributes #1 = { nounwind }

!IGCMetadata = !{!0}
!igc.functions = !{!14}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", ptr @matmul_kernel_with_block_pointers_batched}
!3 = !{!"FuncMDValue[0]", !4, !10, !17}
!4 = !{!"resAllocMD", !5}
!5 = !{!"argAllocMDList", !6, !9, !18, !19, !20, !21}
!6 = !{!"argAllocMDListVec[0]", !7, !8, !8}
!7 = !{!"type", i32 0}
!8 = !{!"extensionType", i32 -1}
!9 = !{!"argAllocMDListVec[1]", !7, !8, !8}
!10 = !{!"implicitArgInfoList", !11, !12, !13, !22, !23}
!11 = !{!"implicitArgInfoListVec[0]", !{!"argId", i32 0}}
!12 = !{!"implicitArgInfoListVec[1]", !{!"argId", i32 2}}
!13 = !{!"implicitArgInfoListVec[2]", !{!"argId", i32 13}}
!14 = !{ptr @matmul_kernel_with_block_pointers_batched, !15}
!15 = !{!16}
!16 = !{!"function_type", i32 0}
!17 = !{!"requiredSubGroupSize", i32 16}
!18 = !{!"argAllocMDListVec[2]", !7, !8, !8}
!19 = !{!"argAllocMDListVec[3]", !7, !8, !8}
!20 = !{!"argAllocMDListVec[4]", !7, !8, !8}
!21 = !{!"argAllocMDListVec[5]", !7, !8, !8}
!22 = !{!"implicitArgInfoListVec[3]", !{!"argId", i32 57}}
!23 = !{!"implicitArgInfoListVec[4]", !{!"argId", i32 58}}