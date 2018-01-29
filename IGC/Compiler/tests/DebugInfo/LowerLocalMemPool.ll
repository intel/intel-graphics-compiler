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
; RUN: opt -igc-lower-local-mem -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that LowerLocalMemPool pass handles line debug info.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a64:64:64-f80:128:128-n8:16:32:64"
target triple = "igil_32_GEN8"

; CHECK: [[LocalMemPool:@[a-zA-Z0-9.]+]] = internal addrspace(3) global [10 x i8] zeroinitializer, align 4

define void @test(float* %dst) #0 {
  %ptr = call float* @__builtin_IB_AllocLocalMemPool(i32 10, i32 4), !dbg !1
  %res = load float* %ptr, align 4, !dbg !2
  store float %res, float* %dst, align 4, !dbg !3
  ret void

; CHECK-NOT: = call float* @__builtin_IB_AllocLocalMemPool
; CHECK: [[mempoolcast:%[a-zA-Z0-9]+]] = bitcast [10 x i8] addrspace(3)* [[LocalMemPool]] to float*, !dbg !1
; CHECK: [[res:%[a-zA-Z0-9]+]] = load float* [[mempoolcast]], align 4, !dbg !2
}

declare float* @__builtin_IB_AllocLocalMemPool(i32, i32)

attributes #0 = { nounwind }

;; This hack named metadata is needed to assure metadata order
!hack_order = !{!0, !1, !2, !3}

!0 = metadata !{}
!1 = metadata !{i32 5, i32 0, metadata !0, null}
!2 = metadata !{i32 6, i32 0, metadata !0, null}
!3 = metadata !{i32 7, i32 0, metadata !0, null}
