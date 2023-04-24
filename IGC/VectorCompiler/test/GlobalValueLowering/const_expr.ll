;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXGlobalValueLowering -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

@simple_global_array = internal global [8 x i32] [i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49], align 4

define dllexport void @simple_array(i64 %provided.offset) {
; CHECK: %simple_global_array.gaddr = call i64 @llvm.genx.gaddr.i64.p0a8i32([8 x i32]* @simple_global_array)
; CHECK: %simple_global_array.lowered = inttoptr i64 %simple_global_array.gaddr to [8 x i32]*
; COM: bitcast didn't survive IRReader, it was transformed into GEP at the very beginning. Because no one wants you to test bitcast.
; CHECK-DAG: %[[BC_CASE_CONST:[^ ]+]] = getelementptr inbounds [8 x i32], [8 x i32]* %simple_global_array.lowered, i32 0, i32 0
; CHECK-DAG: %[[BC_GEP_CASE_CONST:[^ ]+]] = getelementptr inbounds [8 x i32], [8 x i32]* %simple_global_array.lowered, i32 0, i64 3
; CHECK-DAG: %[[P2I_BC_GEP_CASE_CONST_GEP:[^ ]+]] = getelementptr inbounds [8 x i32], [8 x i32]* %simple_global_array.lowered, i32 0, i64 5
; CHECK-DAG: %[[P2I_BC_GEP_CASE_CONST:[^ ]+]] = ptrtoint i32* %[[P2I_BC_GEP_CASE_CONST_GEP]] to i64

  %bc.case = getelementptr inbounds i32, i32* bitcast ([8 x i32]* @simple_global_array to i32*), i64 %provided.offset
; CHECK: %bc.case = getelementptr inbounds i32, i32* %[[BC_CASE_CONST]], i64 %provided.offset

  %gep.bc.case = ptrtoint i32* getelementptr inbounds (i32, i32* bitcast ([8 x i32]* @simple_global_array to i32*), i64 3) to i64
; CHECK: %gep.bc.case = ptrtoint i32* %[[BC_GEP_CASE_CONST]] to i64

  %p2i.gep.bc.case = add i64 ptrtoint (i32* getelementptr inbounds (i32, i32* bitcast ([8 x i32]* @simple_global_array to i32*), i64 5) to i64), 1
; CHECK: %p2i.gep.bc.case = add i64 %[[P2I_BC_GEP_CASE_CONST]], 1
  ret void
}
