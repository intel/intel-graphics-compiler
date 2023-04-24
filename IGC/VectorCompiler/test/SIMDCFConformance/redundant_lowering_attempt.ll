;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXEarlySimdCFConformance -march=genx64 -mcpu=XeHPC -S < %s | FileCheck %s

; COM: SIMD CF Conformance performs interference check for EM.
; COM: It can lower EM producers such as shufflevector if
; COM: interference is detected. Second attempt of lowering such
; COM: instruction would lead to compilation error since
; COM: it is not an EM producer anymore. This test checks that
; COM: the algorithm works correctly and failure doesn't occur.
;
; COM: This problem is specific to interference checker's lowering only.

define dllexport spir_kernel void @partial_phi_em_lowering(i32 %0) {
entry:
  %val = bitcast i32 %0 to <32 x i1>
  br label %loop
loop:
  %EM = phi <32 x i1> [ <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, %entry ], [ %newEM_iter, %loop_test ]
  %RM = phi <32 x i1> [ zeroinitializer, %entry ], [ %newRM_iter, %loop_test ]
; COM: EM in this shufflevector will be lowered due to EM interference of its users
; CHECK-LABEL: loop
; CHECK: shufflevector <32 x i1> %getEM
  %partial_EM = shufflevector <32 x i1> %EM, <32 x i1> undef, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %goto_struct_if = tail call { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1> %EM, <32 x i1> zeroinitializer, <32 x i1> %val)
  %newEM_if = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct_if, 0
  %newRM_if = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct_if, 1
  %branchCond_if = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct_if, 2
  br i1 %branchCond_if, label %simd_if_end, label %simd_if_then
simd_if_then:
; COM: conformant EM use except of interference with %newEM_if
; CHECK-LABEL: simd_if_then
; CHECK: %interference_in_simd_if = select <16 x i1> %partial_EM
  %interference_in_simd_if = select <16 x i1> %partial_EM, <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <16 x i1> zeroinitializer
  br label %simd_if_end
simd_if_end:
  %join_struct_if = tail call { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1> %newEM_if, <32 x i1> %newRM_if)
  %newEM_after_if = extractvalue { <32 x i1>, i1 } %join_struct_if, 0
  br label %loop_test
loop_test:
; COM: conformant EM use except of interference with %newEM_after_if
; CHECK-LABEL: loop_test
; CHECK: %interference_in_loop_test = select <16 x i1> %partial_EM
  %interference_in_loop_test = select <16 x i1> %partial_EM, <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <16 x i1> zeroinitializer
  %goto_struct_iter = tail call { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1> %newEM_after_if, <32 x i1> %RM, <32 x i1> %val)
  %newEM_iter = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct_iter, 0
  %newRM_iter = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct_iter, 1
  %branchCond_iter = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct_iter, 2
  br i1 %branchCond_iter, label %exit, label %loop
exit:
  %join_struct = tail call { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1> %newEM_iter, <32 x i1> %newRM_iter)
  ret void
}

declare { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1>, <32 x i1>, <32 x i1>) #2
declare { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1>, <32 x i1>)
