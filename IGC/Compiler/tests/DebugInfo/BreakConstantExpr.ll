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
; RUN: igc_opt -igc-break-const-expr -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that BreakConstantExpr pass handles line debug info.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target triple = "igil_32_GEN8"

@x = addrspace(2) constant float 8.000000e+00, align 4

define void @test(i32 addrspace(1)* %dst) #0 {
entry:
  call void @llvm.dbg.declare(metadata !{float addrspace(2)* getelementptr inbounds (float, float addrspace(2)* @x, i32 5)}, metadata !3), !dbg !2
  store i32 ptrtoint (float addrspace(2)* getelementptr inbounds (float, float addrspace(2)* @x, i32 6) to i32), i32 addrspace(1)* %dst, align 4, !dbg !1
  ret void, !dbg !2

; CHECK: [[gepInst:%[a-zA-Z0-9]+]] = getelementptr inbounds float, float addrspace(2)* @x, i32 6, !dbg !1
; CHECK: [[ptrtointInst:%[a-zA-Z0-9]+]] = ptrtoint float addrspace(2)* [[gepInst]] to i32, !dbg !1
; CHECK: store i32 [[ptrtointInst]], i32 addrspace(1)* %dst, align 4, !dbg !1
; CHECK: ret void, !dbg !2
}

declare void @llvm.dbg.declare(metadata, metadata) #1

attributes #0 = { alwaysinline nounwind }
attributes #1 = { nounwind readnone }

;; This hack named metadata is needed to assure metadata order
!hack_order = !{!0, !1, !2, !3}

!0 = !{}
!1 = !{i32 3, i32 0, !0, null}
!2 = !{i32 4, i32 0, !0, null}
!3 = !{i32 786688, !0, !"dst", !0, i32 3, !0, i32 0, i32 0}
