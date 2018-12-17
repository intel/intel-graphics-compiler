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
; RUN: igc_opt -igc-scalarize -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that Scalarizer pass handles line debug info.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target triple = "igil_32_GEN8"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; General case check:
;; 1. that shuffle does not steal debug line info of its operands (add & load)
;; 2. that extract generated due to the load takes the load debug line info
;; 3. that inseart generated due to the store takes the shuuffle debug line info
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define void @test_general(<4 x float> addrspace(1)* %dst, <4 x float> %x, <4 x float> %y) #0 {
entry:
  %res = fadd <4 x float> %x, %y, !dbg !1
  %z = load <4 x float>, <4 x float> addrspace(1)* %dst, !dbg !2
  %res1 = shufflevector <4 x float> %res, <4 x float> %z, <4 x i32> <i32 0, i32 1, i32 4, i32 5>, !dbg !3
  store <4 x float> %res1, <4 x float> addrspace(1)* %dst, align 16, !dbg !4
  ret void

; CHECK: @test_general
; CHECK-NEXT: entry:
; CHECK-NEXT: [[y0:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %y, i32 0
; CHECK-NEXT: [[y1:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %y, i32 1
; CHECK-NEXT: [[y2:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %y, i32 2
; CHECK-NEXT: [[y3:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %y, i32 3
; CHECK-NEXT: [[x0:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %x, i32 0
; CHECK-NEXT: [[x1:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %x, i32 1
; CHECK-NEXT: [[x2:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %x, i32 2
; CHECK-NEXT: [[x3:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %x, i32 3

; CHECK-NEXT: [[res0:%[a-zA-Z0-9_.]+]] = fadd float [[x0]], [[y0]], !dbg !1
; CHECK-NEXT: [[res1:%[a-zA-Z0-9_.]+]] = fadd float [[x1]], [[y1]], !dbg !1
; CHECK-NEXT: [[res2:%[a-zA-Z0-9_.]+]] = fadd float [[x2]], [[y2]], !dbg !1
; CHECK-NEXT: [[res3:%[a-zA-Z0-9_.]+]] = fadd float [[x3]], [[y3]], !dbg !1

; CHECK-NEXT: [[z:%[a-zA-Z0-9_.]+]] = load <4 x float>, <4 x float> addrspace(1)* %dst, !dbg !2
; CHECK-NEXT: [[z0:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> [[z]], i32 0, !dbg !2
; CHECK-NEXT: [[z1:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> [[z]], i32 1, !dbg !2
; CHECK-NEXT: [[z2:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> [[z]], i32 2, !dbg !2
; CHECK-NEXT: [[z3:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> [[z]], i32 3, !dbg !2

; CHECK-NEXT: [[res10:%[a-zA-Z0-9_.]+]] = insertelement <4 x float> undef, float [[res0]], i32 0, !dbg !3
; CHECK-NEXT: [[res11:%[a-zA-Z0-9_.]+]] = insertelement <4 x float> [[res10]], float [[res1]], i32 1, !dbg !3
; CHECK-NEXT: [[res12:%[a-zA-Z0-9_.]+]] = insertelement <4 x float> [[res11]], float [[z0]], i32 2, !dbg !3
; CHECK-NEXT: [[res13:%[a-zA-Z0-9_.]+]] = insertelement <4 x float> [[res12]], float [[z1]], i32 3, !dbg !3

; CHECK-NEXT: store <4 x float> [[res13]], <4 x float> addrspace(1)* %dst, align 16, !dbg !4
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; BinaryOperator case check:
;; 1. that add instruction preserve its debug line info
;; 2. that insertelement generated from the add instruction (for the store
;;    operand value) also preserve the debug line info of the add instruction
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define void @test_BinaryOperator(<4 x float> addrspace(1)* %dst, <4 x float> %x, <4 x float> %y) #0 {
entry:
  %res = fadd <4 x float> %x, %y, !dbg !1
  store <4 x float> %res, <4 x float> addrspace(1)* %dst, align 16, !dbg !2
  ret void

; CHECK: @test_BinaryOperator
; CHECK-NEXT: entry:
; CHECK-NEXT: [[BinaryOperator_y0:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %y, i32 0
; CHECK-NEXT: [[BinaryOperator_y1:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %y, i32 1
; CHECK-NEXT: [[BinaryOperator_y2:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %y, i32 2
; CHECK-NEXT: [[BinaryOperator_y3:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %y, i32 3
; CHECK-NEXT: [[BinaryOperator_x0:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %x, i32 0
; CHECK-NEXT: [[BinaryOperator_x1:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %x, i32 1
; CHECK-NEXT: [[BinaryOperator_x2:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %x, i32 2
; CHECK-NEXT: [[BinaryOperator_x3:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %x, i32 3

; CHECK-NEXT: [[BinaryOperator_res0:%[a-zA-Z0-9_.]+]] = fadd float [[BinaryOperator_x0]], [[BinaryOperator_y0]], !dbg !1
; CHECK-NEXT: [[BinaryOperator_res1:%[a-zA-Z0-9_.]+]] = fadd float [[BinaryOperator_x1]], [[BinaryOperator_y1]], !dbg !1
; CHECK-NEXT: [[BinaryOperator_res2:%[a-zA-Z0-9_.]+]] = fadd float [[BinaryOperator_x2]], [[BinaryOperator_y2]], !dbg !1
; CHECK-NEXT: [[BinaryOperator_res3:%[a-zA-Z0-9_.]+]] = fadd float [[BinaryOperator_x3]], [[BinaryOperator_y3]], !dbg !1

; CHECK-NEXT: [[BinaryOperator_inst0:%[a-zA-Z0-9_.]+]] = insertelement <4 x float> undef, float [[BinaryOperator_res0]], i32 0, !dbg !1
; CHECK-NEXT: [[BinaryOperator_inst1:%[a-zA-Z0-9_.]+]] = insertelement <4 x float> [[BinaryOperator_inst0]], float [[BinaryOperator_res1]], i32 1, !dbg !1
; CHECK-NEXT: [[BinaryOperator_inst2:%[a-zA-Z0-9_.]+]] = insertelement <4 x float> [[BinaryOperator_inst1]], float [[BinaryOperator_res2]], i32 2, !dbg !1
; CHECK-NEXT: [[BinaryOperator_inst3:%[a-zA-Z0-9_.]+]] = insertelement <4 x float> [[BinaryOperator_inst2]], float [[BinaryOperator_res3]], i32 3, !dbg !1

; CHECK-NEXT: store <4 x float> [[BinaryOperator_inst3]], <4 x float> addrspace(1)* %dst, align 16, !dbg !2
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; CmpInst_CastInst case check:
;; 1. that cmp instruction preserve its debug line info
;; 2. that cast instruction preserve its debug line info
;; 3. that insertelement generated from the cast instruction (for the store
;;    operand value) also preserve the debug line info of the add instruction
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define void @test_CmpInst_CastInst(<4 x i32> addrspace(1)* %dst, <4 x float> %x, <4 x float> %y) #0 {
entry:
  %cond = fcmp oeq <4 x float> %x, %y, !dbg !1
  %res = zext <4 x i1> %cond to <4 x i32>, !dbg !2
  store <4 x i32> %res, <4 x i32> addrspace(1)* %dst, align 16, !dbg !3
  ret void

; CHECK: @test_CmpInst_CastInst
; CHECK-NEXT: entry:
; CHECK-NEXT: [[CmpInst_CastInst_y0:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %y, i32 0
; CHECK-NEXT: [[CmpInst_CastInst_y1:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %y, i32 1
; CHECK-NEXT: [[CmpInst_CastInst_y2:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %y, i32 2
; CHECK-NEXT: [[CmpInst_CastInst_y3:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %y, i32 3
; CHECK-NEXT: [[CmpInst_CastInst_x0:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %x, i32 0
; CHECK-NEXT: [[CmpInst_CastInst_x1:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %x, i32 1
; CHECK-NEXT: [[CmpInst_CastInst_x2:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %x, i32 2
; CHECK-NEXT: [[CmpInst_CastInst_x3:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %x, i32 3

; CHECK-NEXT: [[CmpInst_CastInst_cond0:%[a-zA-Z0-9_.]+]] = fcmp oeq float [[CmpInst_CastInst_x0]], [[CmpInst_CastInst_y0]], !dbg !1
; CHECK-NEXT: [[CmpInst_CastInst_cond1:%[a-zA-Z0-9_.]+]] = fcmp oeq float [[CmpInst_CastInst_x1]], [[CmpInst_CastInst_y1]], !dbg !1
; CHECK-NEXT: [[CmpInst_CastInst_cond2:%[a-zA-Z0-9_.]+]] = fcmp oeq float [[CmpInst_CastInst_x2]], [[CmpInst_CastInst_y2]], !dbg !1
; CHECK-NEXT: [[CmpInst_CastInst_cond3:%[a-zA-Z0-9_.]+]] = fcmp oeq float [[CmpInst_CastInst_x3]], [[CmpInst_CastInst_y3]], !dbg !1

; CHECK-NEXT: [[CmpInst_CastInst_res0:%[a-zA-Z0-9_.]+]] = zext i1 [[CmpInst_CastInst_cond0]] to i32, !dbg !2
; CHECK-NEXT: [[CmpInst_CastInst_res1:%[a-zA-Z0-9_.]+]] = zext i1 [[CmpInst_CastInst_cond1]] to i32, !dbg !2
; CHECK-NEXT: [[CmpInst_CastInst_res2:%[a-zA-Z0-9_.]+]] = zext i1 [[CmpInst_CastInst_cond2]] to i32, !dbg !2
; CHECK-NEXT: [[CmpInst_CastInst_res3:%[a-zA-Z0-9_.]+]] = zext i1 [[CmpInst_CastInst_cond3]] to i32, !dbg !2

; CHECK-NEXT: [[CmpInst_CastInst_inst0:%[a-zA-Z0-9_.]+]] = insertelement <4 x i32> undef, i32 [[CmpInst_CastInst_res0]], i32 0, !dbg !2
; CHECK-NEXT: [[CmpInst_CastInst_inst1:%[a-zA-Z0-9_.]+]] = insertelement <4 x i32> [[CmpInst_CastInst_inst0]], i32 [[CmpInst_CastInst_res1]], i32 1, !dbg !2
; CHECK-NEXT: [[CmpInst_CastInst_inst2:%[a-zA-Z0-9_.]+]] = insertelement <4 x i32> [[CmpInst_CastInst_inst1]], i32 [[CmpInst_CastInst_res2]], i32 2, !dbg !2
; CHECK-NEXT: [[CmpInst_CastInst_inst3:%[a-zA-Z0-9_.]+]] = insertelement <4 x i32> [[CmpInst_CastInst_inst2]], i32 [[CmpInst_CastInst_res3]], i32 3, !dbg !2

; CHECK-NEXT: store <4 x i32> [[CmpInst_CastInst_inst3]], <4 x i32> addrspace(1)* %dst, align 16, !dbg !3
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; SelectInst case check:
;; 1. that cmp instruction preserve its debug line info
;; 2. that select instruction preserve its debug line info
;; 3. that insertelement generated from the select instruction (for the store
;;    operand value) also preserve the debug line info of the add instruction
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define void @test_SelectInst(<4 x float> addrspace(1)* %dst, <4 x float> %x, <4 x float> %y) #0 {
entry:
  %cond = fcmp oeq <4 x float> %x, %y, !dbg !1
  %res = select <4 x i1> %cond, <4 x float> %x, <4 x float> %y, !dbg !2
  store <4 x float> %res, <4 x float> addrspace(1)* %dst, align 16, !dbg !3
  ret void

; CHECK: @test_SelectInst
; CHECK-NEXT: entry:
; CHECK-NEXT: [[SelectInst_y0:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %y, i32 0
; CHECK-NEXT: [[SelectInst_y1:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %y, i32 1
; CHECK-NEXT: [[SelectInst_y2:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %y, i32 2
; CHECK-NEXT: [[SelectInst_y3:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %y, i32 3
; CHECK-NEXT: [[SelectInst_x0:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %x, i32 0
; CHECK-NEXT: [[SelectInst_x1:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %x, i32 1
; CHECK-NEXT: [[SelectInst_x2:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %x, i32 2
; CHECK-NEXT: [[SelectInst_x3:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %x, i32 3

; CHECK-NEXT: [[SelectInst_cond0:%[a-zA-Z0-9_.]+]] = fcmp oeq float [[SelectInst_x0]], [[SelectInst_y0]], !dbg !1
; CHECK-NEXT: [[SelectInst_cond1:%[a-zA-Z0-9_.]+]] = fcmp oeq float [[SelectInst_x1]], [[SelectInst_y1]], !dbg !1
; CHECK-NEXT: [[SelectInst_cond2:%[a-zA-Z0-9_.]+]] = fcmp oeq float [[SelectInst_x2]], [[SelectInst_y2]], !dbg !1
; CHECK-NEXT: [[SelectInst_cond3:%[a-zA-Z0-9_.]+]] = fcmp oeq float [[SelectInst_x3]], [[SelectInst_y3]], !dbg !1

; CHECK-NEXT: [[SelectInst_res0:%[a-zA-Z0-9_.]+]] = select i1 [[SelectInst_cond0]], float [[SelectInst_x0]], float [[SelectInst_y0]], !dbg !2
; CHECK-NEXT: [[SelectInst_res1:%[a-zA-Z0-9_.]+]] = select i1 [[SelectInst_cond1]], float [[SelectInst_x1]], float [[SelectInst_y1]], !dbg !2
; CHECK-NEXT: [[SelectInst_res2:%[a-zA-Z0-9_.]+]] = select i1 [[SelectInst_cond2]], float [[SelectInst_x2]], float [[SelectInst_y2]], !dbg !2
; CHECK-NEXT: [[SelectInst_res3:%[a-zA-Z0-9_.]+]] = select i1 [[SelectInst_cond3]], float [[SelectInst_x3]], float [[SelectInst_y3]], !dbg !2

; CHECK-NEXT: [[SelectInst_inst0:%[a-zA-Z0-9_.]+]] = insertelement <4 x float> undef, float [[SelectInst_res0]], i32 0, !dbg !2
; CHECK-NEXT: [[SelectInst_inst1:%[a-zA-Z0-9_.]+]] = insertelement <4 x float> [[SelectInst_inst0]], float [[SelectInst_res1]], i32 1, !dbg !2
; CHECK-NEXT: [[SelectInst_inst2:%[a-zA-Z0-9_.]+]] = insertelement <4 x float> [[SelectInst_inst1]], float [[SelectInst_res2]], i32 2, !dbg !2
; CHECK-NEXT: [[SelectInst_inst3:%[a-zA-Z0-9_.]+]] = insertelement <4 x float> [[SelectInst_inst2]], float [[SelectInst_res3]], i32 3, !dbg !2

; CHECK-NEXT: store <4 x float> [[CmpInst_CastInst_inst3]], <4 x float> addrspace(1)* %dst, align 16, !dbg !3
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; ExtractElementInst case check:
;; 1. that extract instruction is not a real instruction in scalaraizer
;;    thus, it will not preserve line debug info, but will take line debug info
;;    from the source (operand vector) of this extract instruction
;;    (in this case the load instruction)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define void @test_ExtractElementInst(float addrspace(1)* %dst, <4 x float> addrspace(1)* %src) #0 {
entry:
  %x = load <4 x float>, <4 x float> addrspace(1)* %src, !dbg !1
  %res = extractelement <4 x float> %x, i32 1, !dbg !2
  store float %res, float addrspace(1)* %dst, align 4, !dbg !3
  ret void

; CHECK: @test_ExtractElementInst
; CHECK-NEXT: entry:

; CHECK-NEXT: [[ExtractElementInst_x:%[a-zA-Z0-9_.]+]] = load <4 x float>, <4 x float> addrspace(1)* %src, !dbg !1

; CHECK-NEXT: [[ExtractElementInst_x0:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> [[ExtractElementInst_x]], i32 0, !dbg !1
; CHECK-NEXT: [[ExtractElementInst_x1:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> [[ExtractElementInst_x]], i32 1, !dbg !1
; CHECK-NEXT: [[ExtractElementInst_x2:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> [[ExtractElementInst_x]], i32 2, !dbg !1
; CHECK-NEXT: [[ExtractElementInst_x3:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> [[ExtractElementInst_x]], i32 3, !dbg !1

; CHECK-NEXT: store float [[ExtractElementInst_x1]], float addrspace(1)* %dst, align 4, !dbg !3
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; InsertElementInst case check:
;; 1. that insert instruction preserve its debug line info
;; 2. It also tries to steal the extract instruction generated due to its vector
;;    operand, and it suceeds only because the extract is from argument,
;;    which has no debug line info
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
define void @test_InsertElementInst(<4 x float> addrspace(1)* %dst, <4 x float> %x, float %y) #0 {
entry:
  %res = insertelement <4 x float> %x, float %y, i32 2, !dbg !1
  store <4 x float> %res, <4 x float> addrspace(1)* %dst, align 16, !dbg !2
  ret void

; CHECK: @test_InsertElementInst
; CHECK-NEXT: entry:
; CHECK-NEXT: [[InsertElementInst_x0:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %x, i32 0, !dbg !1
; CHECK-NEXT: [[InsertElementInst_x1:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %x, i32 1, !dbg !1
; CHECK-NEXT: [[InsertElementInst_x2:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %x, i32 2
; CHECK-NEXT: [[InsertElementInst_x3:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %x, i32 3, !dbg !1

; CHECK-NEXT: [[InsertElementInst_res0:%[a-zA-Z0-9_.]+]] = insertelement <4 x float> undef, float [[InsertElementInst_x0]], i32 0, !dbg !1
; CHECK-NEXT: [[InsertElementInst_res1:%[a-zA-Z0-9_.]+]]  = insertelement <4 x float> [[InsertElementInst_res0]], float [[InsertElementInst_x1]], i32 1, !dbg !1
; CHECK-NEXT: [[InsertElementInst_res2:%[a-zA-Z0-9_.]+]]  = insertelement <4 x float> [[InsertElementInst_res1]], float %y, i32 2, !dbg !1
; CHECK-NEXT: [[InsertElementInst_res3:%[a-zA-Z0-9_.]+]]  = insertelement <4 x float> [[InsertElementInst_res2]], float [[InsertElementInst_x3]], i32 3, !dbg !1

; CHECK-NEXT: store <4 x float> [[InsertElementInst_res3]], <4 x float> addrspace(1)* %dst, align 16, !dbg !2
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; ShuffleVectorInst case check:
;; 1. that shufflevector instruction preserve its debug line info
;; 2. It also tries to steal the extract instruction generated due to its vector
;;    operand, and it suceeds only because the extract is from argument,
;;    which has no debug line info
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define void @test_ShuffleVectorInst(<4 x float> addrspace(1)* %dst, <4 x float> %x, <4 x float> %y) #0 {
entry:
  %res = shufflevector <4 x float> %x, <4 x float> %y, <4 x i32> <i32 0, i32 1, i32 4, i32 5>, !dbg !1
  store <4 x float> %res, <4 x float> addrspace(1)* %dst, align 16, !dbg !2
  ret void

; CHECK: @test_ShuffleVectorInst
; CHECK-NEXT: entry:
; CHECK-NEXT: [[ShuffleVectorInst_y0:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %y, i32 0, !dbg !1
; CHECK-NEXT: [[ShuffleVectorInst_y1:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %y, i32 1, !dbg !1
; CHECK-NEXT: [[ShuffleVectorInst_y2:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %y, i32 2
; CHECK-NEXT: [[ShuffleVectorInst_y3:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %y, i32 3
; CHECK-NEXT: [[ShuffleVectorInst_x0:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %x, i32 0, !dbg !1
; CHECK-NEXT: [[ShuffleVectorInst_x1:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %x, i32 1, !dbg !1
; CHECK-NEXT: [[ShuffleVectorInst_x2:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %x, i32 2
; CHECK-NEXT: [[ShuffleVectorInst_x3:%[a-zA-Z0-9_.]+]] = extractelement <4 x float> %x, i32 3

; CHECK-NEXT: [[ShuffleVectorInst_res0:%[a-zA-Z0-9_.]+]] = insertelement <4 x float> undef, float [[ShuffleVectorInst_x0]], i32 0, !dbg !1
; CHECK-NEXT: [[ShuffleVectorInst_res1:%[a-zA-Z0-9_.]+]] = insertelement <4 x float> [[ShuffleVectorInst_res0]], float [[ShuffleVectorInst_x1]], i32 1, !dbg !1
; CHECK-NEXT: [[ShuffleVectorInst_res2:%[a-zA-Z0-9_.]+]] = insertelement <4 x float> [[ShuffleVectorInst_res1]], float [[ShuffleVectorInst_y0]], i32 2, !dbg !1
; CHECK-NEXT: [[ShuffleVectorInst_res3:%[a-zA-Z0-9_.]+]] = insertelement <4 x float> [[ShuffleVectorInst_res2]], float [[ShuffleVectorInst_y1]], i32 3, !dbg !1

; CHECK-NEXT: store <4 x float> [[ShuffleVectorInst_res3]], <4 x float> addrspace(1)* %dst, align 16, !dbg !2
}


attributes #0 = { alwaysinline nounwind }

;; This hack named metadata is needed to assure metadata order
!hack_order = !{!0, !1, !2, !3, !4}


!0 = !{}
!1 = !{i32 3, i32 0, !0, null}
!2 = !{i32 4, i32 0, !0, null}
!3 = !{i32 5, i32 0, !0, null}
!4 = !{i32 6, i32 0, !0, null}
