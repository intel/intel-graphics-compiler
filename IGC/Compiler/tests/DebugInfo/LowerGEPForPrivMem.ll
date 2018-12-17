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
; RUN: igc_opt -igc-priv-mem-to-reg -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that LowerGEPForPrivMem pass handles variable and line debug info.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target triple = "igil_32_GEN8"


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This case check alloca array of type 3D array of float (total array nested level 3)
;; It has 3 GEP usages,
;;   1st is pointer of load instruction (with variable indices)
;;   2nd is pointer of store instruction (with immediate constant indices)
;;   3rd has no usages (with mixed indices - at lea one variable)
;; We check that alloca, GEP, load and store instructions are handled as expected
;; for alloca we expect that the debug intrinsic will be fixed to point to the new alloca
;; for 1st GEP we expect to see code that calculate the new index
;; for 2nd GEP we do not expect to see code, as the new index is an immediate constant
;; for 3rd GEP we expect to see nothing as it has no usages.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define void @test(float addrspace(1)* %dst, float addrspace(1)* %src, i32 %i, i32 %j, i32 %k) #0 {
entry:
  %a = alloca [5 x [3 x float]], i32 2, align 4, !dbg !1
  call void @llvm.dbg.declare(metadata [5 x [3 x float]]* %a,  metadata !10), !dbg !2
  %arrayidx = getelementptr inbounds [5 x [3 x float]], [5 x [3 x float]]* %a, i32 %i, i32 %j, i32 %k, !dbg !3
  %res = load float, float* %arrayidx, align 4, !dbg !4
  %arrayidx2 = getelementptr inbounds [5 x [3 x float]], [5 x [3 x float]]* %a, i32 1, i32 2, i32 3, !dbg !5
  store float %res, float* %arrayidx2, align 4, !dbg !6
  %arrayidx3 = getelementptr inbounds [5 x [3 x float]], [5 x [3 x float]]* %a, i32 1, i32 2, i32 %k, !dbg !7
  ret void

; CHECK: [[alloca:%[a-zA-Z0-9]+]] = alloca <30 x float>, !dbg !1
; CHECK: call void @llvm.dbg.declare(metadata !{<30 x float>* [[alloca]]}, !10), !dbg !2
; CHECK: [[mul_size1:%[a-zA-Z0-9]+]] = mul i32 %i, 5, !dbg !3
; CHECK: [[add_index2:%[a-zA-Z0-9]+]] = add i32 [[mul_size1]], %j, !dbg !3
; CHECK: [[mul_size2:%[a-zA-Z0-9]+]] = mul i32 [[add_index2]], 3, !dbg !3
; CHECK: [[add_index3:%[a-zA-Z0-9]+]] = add i32 [[mul_size2]], %k, !dbg !3
; CHECK: [[load1:%[a-zA-Z0-9]+]] = load <30 x float>, <30 x float>* [[alloca]], !dbg !4
; CHECK: [[extract:%[a-zA-Z0-9]+]] = extractelement <30 x float> [[load1]], i32 [[add_index3]], !dbg !4
; CHECK: [[load2:%[a-zA-Z0-9]+]] = load <30 x float>, <30 x float>* [[alloca]], !dbg !6
; CHECK: [[insert:%[a-zA-Z0-9]+]] = insertelement <30 x float> [[load2]], float [[extract]], i32 24, !dbg !6
; CHECK: store <30 x float> [[insert]], <30 x float>* [[alloca]], !dbg !6
; CHECK-NEXT: ret void
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This case of alloca with bitcast usage is not supported by this optimization.
;; We expect the code to be the same (no change to the function).
;; Note: this test is not relevant to the debug info,
;;       i.e. may be moved to LowerGEPForPrivMem pass LIT tests suite.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define void @testBad1(float addrspace(1)* %dst) #0 {
entry:
  %a = alloca [10 x float], align 4, !dbg !8
  %arrayidx = bitcast [10 x float]* %a to float*, !dbg !8
  %res = load float, float* %arrayidx, align 4, !dbg !8
  store float %res, float addrspace(1)* %dst, align 4, !dbg !8
  ret void

; CHECK: @testBad1
; CHECK-NEXT: entry:
; CHECK-NEXT: %a = alloca [10 x float], align 4, !dbg !8
; CHECK-NEXT: %arrayidx = bitcast [10 x float]* %a to float*, !dbg !8
; CHECK-NEXT: %res = load float, float* %arrayidx, align 4, !dbg !8
; CHECK-NEXT: store float %res, float addrspace(1)* %dst, align 4, !dbg !8
; CHECK-NEXT: ret void
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This case of two alloca's - both are not supported by this optimization.
;;   1st of array type but the GEP instruction is used as value operand of the Store.
;;   2nd of float* type (not an array type).
;; We expect the code to be the same (no change to the function).
;; Note: this test is not relevant to the debug info,
;;       i.e. may be moved to LowerGEPForPrivMem pass LIT tests suite.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define void @testBad2() #0 {
entry:
  %a = alloca [10 x float], align 4, !dbg !9
  %b = alloca float*, align 4, !dbg !9
  %resPtr = getelementptr inbounds [10 x float], [10 x float]* %a, i32 0, i32 0, !dbg !9
  store float* %resPtr, float** %b, align 4, !dbg !9
  ret void

; CHECK: @testBad2
; CHECK-NEXT: entry:
; CHECK-NEXT: %a = alloca [10 x float], align 4, !dbg !9
; CHECK-NEXT: %b = alloca float*, align 4, !dbg !9
; CHECK-NEXT: %resPtr = getelementptr inbounds [10 x float], [10 x float]* %a, i32 0, i32 0, !dbg !9
; CHECK-NEXT: store float* %resPtr, float** %b, align 4, !dbg !9
; CHECK-NEXT: ret void
}

declare void @llvm.dbg.declare(metadata, metadata) #1

attributes #0 = { alwaysinline nounwind }
attributes #1 = { nounwind readnone }

;; This hack named metadata is needed to assure metadata order
!hack_order = !{!0, !1, !2, !3, !4, !5, !6, !7, !8, !9, !10, !11, !12, !13, !14, !15}

!igc.functions = !{!13, !14, !15}

!0 = !{}
!1 = !{i32 3, i32 0, !0, null}
!2 = !{i32 4, i32 0, !0, null}
!3 = !{i32 5, i32 0, !0, null}
!4 = !{i32 6, i32 0, !0, null}
!5 = !{i32 7, i32 0, !0, null}
!6 = !{i32 8, i32 0, !0, null}
!7 = !{i32 9, i32 0, !0, null}
!8 = !{i32 10, i32 0, !0, null}
!9 = !{i32 11, i32 0, !0, null}
!10 = !{i32 786688, !0, !"a", !0, i32 3, !0, i32 0, i32 0}
!11 = !{!"function_type", i32 0}
!12 = !{!11}
!13 = !{void (float addrspace(1)*, float addrspace(1)*, i32, i32, i32)* @test, !12}
!14 = !{void (float addrspace(1)*)* @testBad1, !12}
!15 = !{void ()* @testBad2, !12}
