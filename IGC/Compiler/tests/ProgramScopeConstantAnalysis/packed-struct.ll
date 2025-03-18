;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-programscope-constant-analysis -S -igc-serialize-metadata < %s | FileCheck %s
; ------------------------------------------------
; ProgramScopeConstantAnalysisPass
; ------------------------------------------------
; This test checks that ProgramScopeConstantAnalysisPass
; aligns correctly the packed struct

%struct.packed_struct = type <{ i8, i8, i8, i64 }>

@__const.test.s1 = internal addrspace(2) constant %struct.packed_struct <{ i8 1, i8 2, i8 3, i64 44975 }>, align 1

define spir_kernel void @test(i64 addrspace(1)* %out) #0 {
entry:
  %out.addr = alloca i64 addrspace(1)*, align 8
  %s1 = alloca %struct.packed_struct, align 1
  %a = alloca i64, align 8
  store i64 addrspace(1)* %out, i64 addrspace(1)** %out.addr, align 8
  %0 = bitcast %struct.packed_struct* %s1 to i8*
  %1 = getelementptr inbounds %struct.packed_struct, %struct.packed_struct addrspace(2)* @__const.test.s1, i32 0, i32 0
  call void @llvm.memcpy.p0i8.p2i8.i64(i8* align 1 %0, i8 addrspace(2)* align 1 %1, i64 11, i1 false)
  %a1 = getelementptr inbounds %struct.packed_struct, %struct.packed_struct* %s1, i32 0, i32 3
  %2 = load i64, i64* %a1, align 1
  store i64 %2, i64* %a, align 8
  %3 = load i64, i64* %a, align 8
  %4 = load i64 addrspace(1)*, i64 addrspace(1)** %out.addr, align 8
  %arrayidx = getelementptr inbounds i64, i64 addrspace(1)* %4, i64 0
  ret void
}

declare void @llvm.memcpy.p0i8.p2i8.i64(i8* noalias nocapture writeonly, i8 addrspace(2)* noalias nocapture readonly, i64, i1 immarg) #1

!igc.functions = !{!1}

!1 = !{void (i64 addrspace(1)*)* @test, !2}
!2 = !{!3, !4}
!3 = !{!"function_type", i32 0}
!4 = !{!"implicit_arg_desc", !5, !6, !7}
!5 = !{i32 0}
!6 = !{i32 1}
!7 = !{i32 11}
; CHECK: !{!"BufferVec[0]", i8 1}
; CHECK: !{!"BufferVec[1]", i8 2}
; CHECK: !{!"BufferVec[2]", i8 3}
; CHECK: !{!"BufferVec[3]", i8 -81}
; CHECK: !{!"BufferVec[4]", i8 -81}
