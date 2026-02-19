;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-opencl-printf-resolution -S  < %s | FileCheck %s
; ------------------------------------------------
; OpenCLPrintfResolution
; ------------------------------------------------

%struct.A.base = type <{ i8 addrspace(4)*, i32 }>
%struct.B = type { %struct.A.base, [4 x i8] }

@.str = internal unnamed_addr addrspace(2) constant [19 x i8] c"constructed A: %p\0A\00", align 1, !spirv.Decorations !0
@.priv.__global = internal addrspace(1) global %struct.B zeroinitializer

; Function Attrs: nounwind
define spir_kernel void @struct_printf_call(i32 addrspace(1)* %out, <8 x i32> %r0, <8 x i32> %payloadHeader, i8 addrspace(2)* %constBase, i8* %privateBase, i8 addrspace(1)* %printfBuffer) #0 {
entry:
  %str = getelementptr inbounds [19 x i8], [19 x i8] addrspace(2)* @.str, i64 0, i64 0
  %bc = bitcast %struct.B addrspace(1)* @.priv.__global to i8 addrspace(1)*
  %asc = addrspacecast i8 addrspace(1)* %bc to i8 addrspace(4)*
; CHECK: [[TMP1:%[A-z0-9]*]] = bitcast %struct.B addrspace(1)* @.priv.__global to i8 addrspace(1)*
; CHECK: [[TMP2:%[A-z0-9]*]] = addrspacecast i8 addrspace(1)* [[TMP1]] to i8 addrspace(4)*
; CHECK: [[TMP3:%[A-z0-9]*]] = ptrtoint i8 addrspace(4)* [[TMP2]] to i64
; CHECK-NOT: [[TMP4:%[A-z0-9]*]] = ptrtoint %struct.B addrspace(1)* @.priv.__global to i64
; CHECK: store i64 [[TMP3]]
  %call = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %str, i8 addrspace(4)* %asc) #0
  store i32 %call, i32 addrspace(1)* %out, align 4
  ret void
}

; Function Attrs: nounwind
declare spir_func i32 @printf(i8 addrspace(2)*, ...) #0

attributes #0 = { nounwind }

!igc.functions = !{!0}
!IGCMetadata = !{!9}

!0 = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, i8 addrspace(2)*, i8*, i8 addrspace(1)*)* @struct_printf_call, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4, !5, !6, !7, !8}
!4 = !{i32 0}
!5 = !{i32 1}
!6 = !{i32 11}
!7 = !{i32 13}
!8 = !{i32 14}
!9 = !{!"ModuleMD", !10}
!10 = !{!"FuncMD", !11, !12}
!11 = distinct !{!"FuncMDMap[0]", void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, i8 addrspace(2)*, i8*, i8 addrspace(1)*)* @struct_printf_call}
!12 = !{!"FuncMDValue[0]", !13, !14, !15}
!13 = !{!"localOffsets"}
!14 = !{!"funcArgs"}
!15 = !{!"functionType", !"KernelFunction"}
