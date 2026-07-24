;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-opencl-printf-resolution -S < %s | FileCheck %s

; A format string in private memory (an alloca) does not resolve to a global, so
; it is lowered inline: runtime strlen, marker, memcpy from the private source.

define spir_kernel void @test(ptr addrspace(1) %out, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr addrspace(2) %constBase, ptr %privateBase, ptr addrspace(1) %printfBuffer) #0 {
  %fmt = alloca [8 x i8], align 1
  %call = call spir_func i32 (ptr, ...) @printf(ptr %fmt, i32 42) #0
  ret void
}

; CHECK-LABEL: @test(
; CHECK:       strlen.loop:
; CHECK:         [[CH:%.*]] = load i8
; CHECK:         icmp eq i8 [[CH]], 0
; CHECK:       call i32 @__builtin_IB_atomic_add_global_i32
; CHECK:         or i64 {{.*}}, -9223372036854775808
; CHECK:         store i64 {{.*}}, ptr addrspace(1)
; CHECK:         call void @llvm.memcpy.p1.p0.i32

declare spir_func i32 @printf(ptr, ...) #0

attributes #0 = { nounwind }

!igc.functions = !{!0}
!IGCMetadata = !{!9}

!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!9 = !{!"ModuleMD", !10}
!10 = !{!"FuncMD", !11, !12}
!11 = distinct !{!"FuncMDMap[0]", ptr @test}
!12 = !{!"FuncMDValue[0]", !13, !14, !15, !26}
!13 = !{!"localOffsets"}
!14 = !{!"funcArgs"}
!15 = !{!"functionType", !"KernelFunction"}
!16 = !{!"argId", i32 0}
!17 = !{!"implicitArgInfoListVec[0]", !16}
!18 = !{!"argId", i32 1}
!19 = !{!"implicitArgInfoListVec[1]", !18}
!20 = !{!"argId", i32 11}
!21 = !{!"implicitArgInfoListVec[2]", !20}
!22 = !{!"argId", i32 13}
!23 = !{!"implicitArgInfoListVec[3]", !22}
!24 = !{!"argId", i32 14}
!25 = !{!"implicitArgInfoListVec[4]", !24}
!26 = !{!"implicitArgInfoList", !17, !19, !21, !23, !25}
