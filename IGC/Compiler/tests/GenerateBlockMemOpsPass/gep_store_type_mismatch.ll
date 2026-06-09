;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt %s --opaque-pointers --platformpvc --generate-block-mem-ops -S --regkey EnableOpaquePointersBackend=1 | FileCheck %s
; CHECK-NOT: call void @llvm.genx.GenISA.simdBlockWrite

; Make sure that the gep (arrayidx) whose result type (%struct.work_size_data) doesn't match the store type (i32)
; behaves like the gep (arrayidx2) matching the type and they both don't generate simdBlockWrite instruction.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%struct.work_size_data = type { i32, i32, i32 }

define spir_kernel void @foo(ptr addrspace(1) %data, <8 x i32> %r0, <3 x i32> %globalOffset, <3 x i32> %globalSize, <3 x i32> %localSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr %privateBase, i32 %bufferOffset) {
entry:
  %idxprom = zext i16 %localIdX to i64
  %arrayidx = getelementptr %struct.work_size_data, ptr addrspace(1) %data, i64 %idxprom
  store i32 0, ptr addrspace(1) %arrayidx, align 4
  %arrayidx2 = getelementptr %struct.work_size_data, ptr addrspace(1) %data, i64 %idxprom, i32 0
  store i32 0, ptr addrspace(1) %arrayidx2, align 4
  ret void
}

!igc.functions = !{!0}

!0 = !{ptr @foo, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!15 = !{!"argId", i32 0}
!16 = !{!"implicitArgInfoListVec[0]", !15}
!17 = !{!"argId", i32 2}
!18 = !{!"implicitArgInfoListVec[1]", !17}
!19 = !{!"argId", i32 5}
!20 = !{!"implicitArgInfoListVec[2]", !19}
!21 = !{!"argId", i32 6}
!22 = !{!"implicitArgInfoListVec[3]", !21}
!23 = !{!"argId", i32 7}
!24 = !{!"implicitArgInfoListVec[4]", !23}
!25 = !{!"argId", i32 8}
!26 = !{!"implicitArgInfoListVec[5]", !25}
!27 = !{!"argId", i32 9}
!28 = !{!"implicitArgInfoListVec[6]", !27}
!29 = !{!"argId", i32 10}
!30 = !{!"implicitArgInfoListVec[7]", !29}
!31 = !{!"argId", i32 13}
!32 = !{!"implicitArgInfoListVec[8]", !31}
!33 = !{!"argId", i32 15}
!34 = !{!"explicitArgNum", i32 0}
!35 = !{!"implicitArgInfoListVec[9]", !33, !34}
!36 = !{!"implicitArgInfoList", !16, !18, !20, !22, !24, !26, !28, !30, !32, !35}
!37 = !{!"FuncMDMap[0]", ptr @foo}
!38 = !{!"FuncMDValue[0]", !36}
!39 = !{!"FuncMD", !37, !38}
!40 = !{!"ModuleMD", !39}
!IGCMetadata = !{!40}

