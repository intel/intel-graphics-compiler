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
; RUN: igc_opt -igc-resolve-inline-locals -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that InlineLocalsResolution pass handles variable debug info.
;; Handlesd case: inline local buffer.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target triple = "igil_32_GEN9"

@test.localTmp = internal addrspace(3) global [2 x i32] zeroinitializer, align 4

; Function Attrs: alwaysinline nounwind
define void @test(i32 addrspace(1)* %dst, i32 %i) #0 {
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %dst, i32 0, !dbg !1
  %0 = load i32 addrspace(1)* %arrayidx, align 4, !dbg !1
  %arrayidx1 = getelementptr inbounds [2 x i32], [2 x i32] addrspace(3)* @test.localTmp, i32 0, i32 %i, !dbg !1
  store i32 %0, i32 addrspace(3)* %arrayidx1, align 4, !dbg !1
  %1 = getelementptr inbounds [2 x i32], [2 x i32] addrspace(3)* @test.localTmp, i32 0, i32 0, !dbg !2
  %2 = load i32 addrspace(3)* %1, align 4, !dbg !2
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %dst, i32 0, !dbg !2
  store i32 %2, i32 addrspace(1)* %arrayidx2, align 4, !dbg !2
  ret void, !dbg !3

; CHECK: call void @llvm.dbg.declare(metadata [[m19:![0-9]+]], metadata [[m20:![0-9]+]])
; CHECK: %arrayidx1 = getelementptr inbounds [2 x i32], [2 x i32] addrspace(3)* @test.localTmp, i32 0, i32 %i, !dbg !1
; CHECK: %1 = getelementptr inbounds [2 x i32], [2 x i32] addrspace(3)* @test.localTmp, i32 0, i32 0, !dbg !2

}

; Function Attrs: nounwind readnone
declare void @llvm.dbg.declare(metadata, metadata) #1

; Function Attrs: nounwind readnone
declare void @llvm.dbg.value(metadata, i64, metadata) #1

attributes #0 = { alwaysinline nounwind }
attributes #1 = { nounwind readnone }

;; This hack named metadata is needed to assure metadata order
!hack_order = !{!0, !1, !2, !3}

!llvm.dbg.cu = !{!4}
!igc.functions = !{!13}


!0 = !{}
!1 = !{i32 17, i32 0, !0, null}
!2 = !{i32 18, i32 0, !0, null}
!3 = !{i32 19, i32 0, !0, null}
!4 = !{i32 786449, !5, i32 12, !"clang version 3.4 ", i1 true, !"", i32 0, !0, !0, !6, !10, !0, !""}
!5 = !{!"filename.cl", !"dirname"}
!6 = !{!7}
!7 = !{i32 786478, !5, !8, !"test", !"test", !"", i32 14, !9, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, null, null, null, !0, i32 15}
!8 = !{i32 786473, !5}
!9 = !{i32 786453, i32 0, i32 0, !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, !0, i32 0, i32 0}
!10 = !{!11}
!11 = !{i32 786484, i32 0, !7, !"localTmp", !"localTmp", !"", !8, i32 16, !12, i32 1, i32 1, [2 x i32] addrspace(3)* @test.localTmp, null}
!12 = !{i32 786468, null, null, !"int", i32 0, i64 32, i64 32, i64 0, i32 0, i32 5}
!13 = !{void (i32 addrspace(1)*, i32)* @test, !14}
!14 = !{!15}
!15 = !{!"function_type", i32 0}

; CHECK: [[m19]] = !{[2 x i32] addrspace(3)* @test.localTmp}
; CHECK: [[m20]] = !{i32 786688, !7, !"localTmp", !8, i32 16, !12, i32 0, i32 0}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; Test is based on following source
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;__kernel void test(global uint *dst, int i)
;;{
;;    local uint localTmp[2];
;;    localTmp[i] = dst[0];
;;    dst[0] = localTmp[0];
;;}
