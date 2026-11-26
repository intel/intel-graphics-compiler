;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
;
; RUN: igc_opt --opaque-pointers -ocl -platformbmg -igc-emit-visa %s -regkey EnablePassInlineData=1 2>&1 | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; Test that an error is reported when the cross-thread data size does not fit in 16 bits
; (runtime limit). %big starts after inline data (32) and ends at 70032, aligned to 70048 > 65535.
; A single big struct is a shortcut: ErrorCheck (explicit arguments > 32 KB) would reject it
; in a full compile. The limit can be reached by many small arguments,
; e.g. char, each placed in a 4-byte slot.

; CHECK: error: in kernel 'pulled_too_big': Total size of kernel arguments exceeds cross-thread data limit! Size: 70048, limit: 65535

%struct.Big = type { [17500 x i32] }

define spir_kernel void @pulled_too_big(ptr addrspace(1) %out, ptr byval(%struct.Big) %big, <8 x i32> %r0, <8 x i32> %payloadHeader) {
entry:
  %bigp = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 1)
  %v = load i32, ptr addrspace(5) %bigp, align 4
  store i32 %v, ptr addrspace(1) %out, align 4
  ret void
}

declare ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32)

!igc.functions = !{!0}
!IGCMetadata = !{!3}

!0 = !{ptr @pulled_too_big, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = !{!"FuncMDMap[0]", ptr @pulled_too_big}
!6 = !{!"FuncMDValue[0]", !7, !8, !20, !30}
!7 = !{!"functionType", !"KernelFunction"}
!8 = !{!"resAllocMD", !9}
!9 = !{!"argAllocMDList", !10, !14, !15, !16}
!10 = !{!"argAllocMDListVec[0]", !11, !12, !13}
!11 = !{!"type", i32 0}
!12 = !{!"extensionType", i32 -1}
!13 = !{!"indexType", i32 -1}
!14 = !{!"argAllocMDListVec[1]", !11, !12, !13}
!15 = !{!"argAllocMDListVec[2]", !11, !12, !13}
!16 = !{!"argAllocMDListVec[3]", !11, !12, !13}
!20 = !{!"implicitArgInfoList", !21, !23}
!21 = !{!"implicitArgInfoListVec[0]", !22}
!22 = !{!"argId", i32 0}
!23 = !{!"implicitArgInfoListVec[1]", !24}
!24 = !{!"argId", i32 1}
!30 = !{!"pulledArgs", !31}
!31 = !{!"pulledArgsVec[0]", i32 1}
