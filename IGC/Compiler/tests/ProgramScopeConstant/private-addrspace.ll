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
; RUN: opt -igc-programscope-constant-analysis -verify -S < %s | FileCheck %s --check-prefix=ANALYZE
; RUN: opt -igc-programscope-constant-analysis -igc-programscope-constant-resolve -verify -S < %s | FileCheck %s --check-prefix=RESOLVE
target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a64:64:64-f80:128:128-n8:16:32:64"
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
; CHECK-RESOLVE: %off.str = getelementptr i32 addrspace(1)* %ocl_test_results, i32 0
; CHECK-RESOLVE: %cast.str = bitcast i32 addrspace(1)* %off.str to [59 x i8]*
; CHECK-RESOLVE: %8 = bitcast [59 x i8]* %cast.str to i8 addrspace(2)*
  %8 = bitcast [59 x i8]* @.str to i8 addrspace(2)*
  %arrayidx.i = getelementptr inbounds i8 addrspace(2)* %8, i32 %7
  %9 = load i8 addrspace(2)* %arrayidx.i, align 1, !tbaa !10
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

!40 = metadata !{void (i32 addrspace(1)*)* @ocl_test_kernel, metadata !41}
!41 = metadata !{metadata !42, metadata !43}
!42 = metadata !{metadata !"function_type", i32 0}
!43 = metadata !{metadata !"implicit_arg_desc"}

!0 = metadata !{void (i32 addrspace(1)*)* @ocl_test_kernel, metadata !1, metadata !2, metadata !3, metadata !4, metadata !5, metadata !6}
!1 = metadata !{metadata !"kernel_arg_addr_space", i32 1}
!2 = metadata !{metadata !"kernel_arg_access_qual", metadata !"none"}
!3 = metadata !{metadata !"kernel_arg_type", metadata !"int*"}
!4 = metadata !{metadata !"kernel_arg_type_qual", metadata !""}
!5 = metadata !{metadata !"kernel_arg_base_type", metadata !"int*"}
!6 = metadata !{metadata !"kernel_arg_name", metadata !"ocl_test_results"}
!7 = metadata !{metadata !"-cl-std=CL1.2", metadata !"-cl-kernel-arg-info"}
!10 = metadata !{metadata !"omnipotent char", metadata !11}
!11 = metadata !{metadata !"Simple C/C++ TBAA"}
!12 = metadata !{metadata !"int", metadata !10}

; CHECK-ANALYZE: [[INLINE_CONSTANTS]] = metadata !{metadata [[BUFFER:![0-9]+]], i32 1}
; CHECK-ANALYZE: [[BUFFER]] = metadata !{i8 65, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 66, i8 65, i8 0}
; CHECK-ANALYZE: [[OFFSETS]] = metadata !{[59 x i8]* @.str, i32 0}
