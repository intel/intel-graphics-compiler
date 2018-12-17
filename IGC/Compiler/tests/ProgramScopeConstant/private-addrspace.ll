;===================== begin_copyright_notice ==================================

;Copyright (c) 2017 Intel Corporation

;Permission is hereby granted, free of charge, to any person obtaining a
;copy of this software and associated documentation files (the
;"Software"), to deal in the Software without restriction, including
;without limitation the rights to use, copy, modify, merge, publish,
;distribute, sublicense, and/or sell copies of the Software, and to
;permit persons to whom the Software is furnished to do so, subject to
;the following conditions:

;The above copyright notice and this permission notice shall be included
;in all copies or substantial portions of the Software.

;THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
;OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
;MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
;IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
;CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
;TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
;SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


;======================= end_copyright_notice ==================================
; RUN: igc_opt -igc-programscope-constant-analysis -igc-programscope-constant-resolve -verify -S < %s | FileCheck %s --check-prefix=CHECK-RESOLVE
target triple = "igil_32_GEN8"

@.str = private unnamed_addr constant [59 x i8] c"AABABABABABABABABABABABABABABABABABABABABABABABABABABABABA\00", align 1

; Function Attrs: alwaysinline nounwind
define void @ocl_test_kernel(i32 addrspace(1)* %ocl_test_results) #0 {
entry:
; CHECK-RESOLVE-LABEL: ocl_test_kernel
  %0 = call i32 @__builtin_IB_get_group_id(i32 0) #1
  %1 = call i32 @__builtin_IB_get_local_size(i32 0) #1
  %2 = mul i32 %1, %0
  %3 = call zeroext i16 @__builtin_IB_get_local_id_x() #1
  %4 = zext i16 %3 to i32
  %5 = add i32 %2, %4
  %6 = call i32 @__builtin_IB_get_global_offset(i32 0) #1
  %7 = add i32 %5, %6
; CHECK-RESOLVE: %off.str = getelementptr i32, i32 addrspace(1)* %ocl_test_results, i32 0
; CHECK-RESOLVE: %cast.str = bitcast i32 addrspace(1)* %off.str to [59 x i8]*
; CHECK-RESOLVE: %8 = bitcast [59 x i8] addrspace(2)* %cast.str to i8 addrspace(2)*
  %8 = bitcast [59 x i8]* @.str to i8*
  %arrayidx.i = getelementptr inbounds i8, i8* %8, i32 %7
  %9 = load i8, i8* %arrayidx.i, align 1, !tbaa !10
  %conv.i = sext i8 %9 to i32
  store i32 %conv.i, i32 addrspace(1)* %ocl_test_results, align 4, !tbaa !12
  ret void
; CHECK-RESOLVE: ret
}

declare i32 @__builtin_IB_get_global_offset(i32)
declare i16 @__builtin_IB_get_local_id_x()
declare i32 @__builtin_IB_get_local_size(i32)
declare i32 @__builtin_IB_get_group_id(i32)

attributes #0 = { alwaysinline nounwind }
attributes #1 = { nounwind }

!opencl.kernels = !{!0}
!opencl.compiler.options = !{!7}
!opencl.enable.FP_CONTRACT = !{}
!igc.version = !{}
!igc.input.ir = !{}
!igc.input.lang.info = !{}
!igc.functions = !{!40}
!igc.compiler.options = !{}

; CHECK-ANALYZE: !igc.inline.constants = !{[[INLINE_CONSTANTS:![0-9]+]]}
; CHECK-ANALYZE: !igc.inline.programscope.offsets = !{[[OFFSETS:![0-9]+]]}

!40 = !{void (i32 addrspace(1)*)* @ocl_test_kernel, !41}
!41 = !{!42, !43}
!42 = !{!"function_type", i32 0}
!43 = !{!"implicit_arg_desc"}

!0 = !{void (i32 addrspace(1)*)* @ocl_test_kernel, !1, !2, !3, !4, !5, !6}
!1 = !{!"kernel_arg_addr_space", i32 1}
!2 = !{!"kernel_arg_access_qual", !"none"}
!3 = !{!"kernel_arg_type", !"int*"}
!4 = !{!"kernel_arg_type_qual", !""}
!5 = !{!"kernel_arg_base_type", !"int*"}
!6 = !{!"kernel_arg_name", !"ocl_test_results"}
!7 = !{!"-cl-std=CL1.2", !"-cl-kernel-arg-info"}
!10 = !{!"omnipotent char", !11}
!11 = !{!"Simple C/C++ TBAA"}
!12 = !{!"int", !10}

; CHECK-ANALYZE: [[INLINE_CONSTANTS]] = !{[[BUFFER:![0-9]+]], i32 1}
; CHECK-ANALYZE: [[BUFFER]] = !{i8 65, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 0}
; CHECK-ANALYZE: [[OFFSETS]] = !{[59 x i8]* @.str, i32 0}
