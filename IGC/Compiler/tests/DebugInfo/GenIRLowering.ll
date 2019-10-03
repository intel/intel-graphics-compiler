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
; This test marked as XFAIL until the dependancy of GenIRLowering pass on CodeGencontext will be removed
; XFAIL: *
; RUN: opt -igc-gen-ir-lowering -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that GenIRLowering pass handles line debug info.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a64:64:64-f80:128:128-n8:16:32:64"
target triple = "igil_32_GEN9"

; Function Attrs: alwaysinline nounwind
define void @GenIRLowering_test(float addrspace(1)* %src1, float addrspace(1)* %src2, float addrspace(1)* %dst) #0 {
  %1 = getelementptr inbounds float addrspace(1)* %src1, i32 3, !dbg !2
  %2 = load float addrspace(1)* %1, align 4, !dbg !3
  %3 = getelementptr inbounds float addrspace(1)* %src2, i32 2, !dbg !4
  %4 = load float addrspace(1)* %3, align 4, !dbg !5
  %5 = fcmp oge float %2, %4
  %6 = select i1 %5, float %2, float %4
  %7 = fcmp uno float %2, 0.000000e+00
  %8 = select i1 %7, float %4, float %6
  %9 = fcmp uno float %4, 0.000000e+00
  %10 = select i1 %9, float %2, float %8, !dbg !6
  %11 = fcmp ole float %10, 5.000000e+00
  %12 = select i1 %11, float %10, float 5.000000e+00
  %13 = fcmp uno float %10, 0.000000e+00
  %14 = select i1 %13, float 5.000000e+00, float %12, !dbg !7
  store float %14, float addrspace(1)* %dst, align 4
  ret void

; CHECK: [[inst1:%[a-zA-Z0-9]+]] = ptrtoint float addrspace(1)* %src1 to i32, !dbg !2
; CHECK: [[inst2:%[a-zA-Z0-9]+]] = add i32 [[inst1]], 12, !dbg !2
; CHECK: [[inst3:%[a-zA-Z0-9]+]] = inttoptr i32 [[inst2]] to float addrspace(1)*, !dbg !2
; CHECK: [[inst4:%[a-zA-Z0-9]+]] = load float addrspace(1)* [[inst3]], align 4, !dbg !3
; CHECK: [[inst5:%[a-zA-Z0-9]+]] = ptrtoint float addrspace(1)* %src2 to i32, !dbg !4
; CHECK: [[inst6:%[a-zA-Z0-9]+]] = add i32 [[inst5]], 8, !dbg !4
; CHECK: [[inst7:%[a-zA-Z0-9]+]] = inttoptr i32 [[inst6]] to float addrspace(1)*, !dbg !4
; CHECK: [[inst8:%[a-zA-Z0-9]+]] = load float addrspace(1)* [[inst7]], align 4, !dbg !5
; CHECK: [[inst9:%[a-zA-Z0-9]+]] = call float @llvm.genx.GenISA.max.f32(float [[inst4]], float %8), !dbg !6
; CHECK: [[inst10:%[a-zA-Z0-9]+]] = call float @llvm.genx.GenISA.min.f32(float [[inst9]], float 5.000000e+00), !dbg !7

}

attributes #0 = { alwaysinline nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-frame-pointer-elim-non-leaf"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "unsafe-fp-math"="false" "use-soft-float"="false" }

;; This hack named metadata is needed to assure metadata order
!hack_order = !{!0, !1, !2, !3 ,!4, !5, !6, !7}

!igc.functions = !{!1}

!0 = metadata !{}
!1 = metadata !{void (float addrspace(1)*, float addrspace(1)*, float addrspace(1)*)* @GenIRLowering_test, metadata !0}
!2 = metadata !{i32 1, i32 0, metadata !0, null}
!3 = metadata !{i32 3, i32 0, metadata !0, null}
!4 = metadata !{i32 5, i32 0, metadata !0, null}
!5 = metadata !{i32 6, i32 0, metadata !0, null}
!6 = metadata !{i32 8, i32 0, metadata !0, null}
!7 = metadata !{i32 9, i32 0, metadata !0, null}
