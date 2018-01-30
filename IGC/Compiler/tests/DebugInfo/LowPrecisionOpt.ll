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
; RUN: opt -igc-low-precision-opt -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that lowPrecisionOpt pass handles line debug info.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a64:64:64-f80:128:128-n8:16:32:64"
target triple = "igil_32_GEN9"

; Function Attrs: alwaysinline nounwind
define void @LowPrecisionOpt_test(float addrspace(1)* %src, float addrspace(1)* %dst) {
  %1 = load float addrspace(1)* %src, align 4, !dbg !4
  %2 = fptrunc float %1 to half, !dbg !5
  %3 = fpext half %2 to float, !dbg !6
  store float %3, float addrspace(1)* %dst, align 4, !dbg !7
  ret void
  
; CHECK: [[inst1:%[a-zA-Z0-9]+]] = load float addrspace(1)* %src, align 4, !dbg !4
; CHECK: [[inst2:%[a-zA-Z0-9]+]] = fptrunc float %1 to half, !dbg !5
; CHECK: store float [[inst1]], float addrspace(1)* %dst, align 4, !dbg !7
}
;; This hack named metadata is needed to assure metadata order
!hack_order = !{!0, !1, !2, !3 ,!4, !5, !6, !7}

!igc.functions = !{!3}

!0 = metadata !{}
!1 = metadata !{metadata !"function_type", i32 0}
!2 = metadata !{metadata !1}
!3 = metadata !{void (float addrspace(1)*, float addrspace(1)*)* @LowPrecisionOpt_test, metadata !2}
!4 = metadata !{i32 1, i32 0, metadata !0, null}
!5 = metadata !{i32 3, i32 0, metadata !0, null}
!6 = metadata !{i32 5, i32 0, metadata !0, null}
!7 = metadata !{i32 6, i32 0, metadata !0, null}
