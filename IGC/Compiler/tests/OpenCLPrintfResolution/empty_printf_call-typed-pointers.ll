;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt -igc-opencl-printf-resolution -S -disable-output < %s
; ------------------------------------------------
; OpenCLPrintfResolution
; ------------------------------------------------
;
; Was reduced from ocl test kernel:
; kernel void empty_printf_call(global int* out)
; {
;     int ret = printf("");
;     *out = ret;
; }
;
; ------------------------------------------------

@.str = internal unnamed_addr addrspace(2) constant [1 x i8] zeroinitializer, align 1, !spirv.Decorations !0

; Function Attrs: nounwind
define spir_kernel void @empty_printf_call(i32 addrspace(1)* %out, <8 x i32> %r0, <8 x i32> %payloadHeader, i8 addrspace(2)* %constBase, i8* %privateBase, i8 addrspace(1)* %printfBuffer) #0 {
entry:
  %empty_str = getelementptr inbounds [1 x i8], [1 x i8] addrspace(2)* @.str, i64 0, i64 0
  %call = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %empty_str) #0
  store i32 %call, i32 addrspace(1)* %out, align 4
  ret void
}

; Function Attrs: nounwind
declare spir_func i32 @printf(i8 addrspace(2)*, ...) #0

attributes #0 = { nounwind }

!igc.functions = !{!0}
!IGCMetadata = !{!9}

!0 = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, i8 addrspace(2)*, i8*, i8 addrspace(1)*)* @empty_printf_call, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!9 = !{!"ModuleMD", !10}
!10 = !{!"FuncMD", !11, !12}
!11 = distinct !{!"FuncMDMap[0]", void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, i8 addrspace(2)*, i8*, i8 addrspace(1)*)* @empty_printf_call}
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
