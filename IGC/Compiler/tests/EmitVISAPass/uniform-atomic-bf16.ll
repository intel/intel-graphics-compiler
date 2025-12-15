;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys

; RUN: igc_opt --opaque-pointers -platformCri -igc-emit-visa -regkey DumpVISAASMToConsole < %s | FileCheck %s
; UNSUPPORTED: sys32

; This test verifies that the value for the atomic instruction is correctly converted to the D16_U32 type.
; Specifically, it checks that the bfloat16 value is first bitcast to an unsigned word (uw), and then the uw
; is moved to a ud (unsigned dword) typed register. The test ensures that the bfloat16 value is not directly
; converted to the ud type, confirming the correct intermediate conversion steps in the code generation.

; CHECK-DAG: lsc_atomic_bfadd.ugm (M1_NM, 1)  %null:d16u32  flat[{{.*}}]:a64  [[D16_U32_VAL:[A-z0-9]*]]
; CHECK-DAG: mov (M1_NM, 1) [[D16_U32_VAL]](0,0)<1> [[UW_VAL:[A-z0-9]*]](0,0)<0;1,0>
; CHECK-DAG: .decl [[UW_VAL]] v_type=G type=uw num_elts=1 align=wordx64 alias=<[[BF_VAL:[A-z0-9]*]], 0>
; CHECK-DAG: .decl [[BF_VAL]] v_type=G type=bf num_elts=1 align=wordx64

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_bf16_atomic(bfloat addrspace(1)* %sum, <8 x i32> %r0, <8 x i32> %payloadHeader, i32 %bufferOffset) {
entry:
  %sum_i16 = bitcast bfloat addrspace(1)* %sum to i16 addrspace(1)*
  %0 = call i16 @llvm.genx.GenISA.intatomicrawA64.i16.p1i16.p1i16(i16 addrspace(1)* %sum_i16, i16 addrspace(1)* %sum_i16, i16 16256, i32 44)
  ret void
}

declare i16 @llvm.genx.GenISA.intatomicrawA64.i16.p1i16.p1i16(i16 addrspace(1)*, i16 addrspace(1)*, i16, i32)

!IGCMetadata = !{!0}
!igc.functions = !{!21}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (i64 addrspace(1)*, <8 x i32>, <8 x i32>, i32)* @test_bf16_atomic}
!3 = !{!"FuncMDValue[0]", !4, !17}
!4 = !{!"resAllocMD", !5}
!5 = !{!"argAllocMDList", !6, !10, !11, !14, !15, !16}
!6 = !{!"argAllocMDListVec[0]", !7, !8, !9}
!7 = !{!"type", i32 0}
!8 = !{!"extensionType", i32 -1}
!9 = !{!"indexType", i32 -1}
!10 = !{!"argAllocMDListVec[1]", !7, !8, !9}
!11 = !{!"argAllocMDListVec[2]", !12, !8, !13}
!12 = !{!"type", i32 1}
!13 = !{!"indexType", i32 0}
!14 = !{!"argAllocMDListVec[3]", !7, !8, !9}
!15 = !{!"argAllocMDListVec[4]", !7, !8, !9}
!16 = !{!"argAllocMDListVec[5]", !7, !8, !9}
!17 = !{!"m_OpenCLArgTypeQualifiers", !18, !19, !20}
!18 = !{!"m_OpenCLArgTypeQualifiersVec[0]", !""}
!19 = !{!"m_OpenCLArgTypeQualifiersVec[1]", !""}
!20 = !{!"m_OpenCLArgTypeQualifiersVec[2]", !""}
!21 = !{void (i64 addrspace(1)*, <8 x i32>, <8 x i32>, i32)* @test_bf16_atomic, !22}
!22 = !{!23, !24}
!23 = !{!"function_type", i32 0}
!24 = !{!"implicit_arg_desc", !25, !26, !27}
!25 = !{i32 0}
!26 = !{i32 1}
!27 = !{i32 15, !28}
!28 = !{!"explicit_arg_num", i32 2}
