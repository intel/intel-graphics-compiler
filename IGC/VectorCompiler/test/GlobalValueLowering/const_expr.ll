;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXGlobalValueLowering -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXGlobalValueLowering -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32"

@simple_global_array = internal global [8 x i32] [i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49], align 4

define dllexport void @simple_array(i64 %provided.offset) {
; CHECK-TYPED-PTRS: %simple_global_array.gaddr = call i64 @llvm.genx.gaddr.i64.p0a8i32([8 x i32]* @simple_global_array)
; CHECK-TYPED-PTRS: %simple_global_array.lowered = inttoptr i64 %simple_global_array.gaddr to [8 x i32]*
; CHECK-OPAQUE-PTRS: %simple_global_array.gaddr = call i64 @llvm.genx.gaddr.i64.p0(ptr @simple_global_array)
; CHECK-OPAQUE-PTRS: %simple_global_array.lowered = inttoptr i64 %simple_global_array.gaddr to ptr
; COM: bitcast didn't survive IRReader, it was transformed into GEP at the very beginning. Because no one wants you to test bitcast.
; CHECK-TYPED-PTRS-DAG: %[[BC_CASE_CONST:[^ ]+]] = getelementptr inbounds [8 x i32], [8 x i32]* %simple_global_array.lowered, i32 0, i32 0
; CHECK-TYPED-PTRS-DAG: %[[BC_GEP_CASE_CONST:[^ ]+]] = getelementptr inbounds [8 x i32], [8 x i32]* %simple_global_array.lowered, i32 0, i64 3
; CHECK-TYPED-PTRS-DAG: %[[P2I_BC_GEP_CASE_CONST_GEP:[^ ]+]] = getelementptr inbounds [8 x i32], [8 x i32]* %simple_global_array.lowered, i32 0, i64 5
; CHECK-TYPED-PTRS-DAG: %[[P2I_BC_GEP_CASE_CONST:[^ ]+]] = ptrtoint i32* %[[P2I_BC_GEP_CASE_CONST_GEP]] to i64
; CHECK-OPAQUE-PTRS-DAG: %[[BC_GEP_CASE_CONST:[^ ]+]] = getelementptr inbounds i32, ptr %simple_global_array.lowered, i64 3
; CHECK-OPAQUE-PTRS-DAG: %[[P2I_BC_GEP_CASE_CONST_GEP:[^ ]+]] = getelementptr inbounds i32, ptr %simple_global_array.lowered, i64 5
; CHECK-OPAQUE-PTRS-DAG: %[[P2I_BC_GEP_CASE_CONST:[^ ]+]] = ptrtoint ptr %[[P2I_BC_GEP_CASE_CONST_GEP]] to i64

  %bc.case = getelementptr inbounds i32, i32* bitcast ([8 x i32]* @simple_global_array to i32*), i64 %provided.offset
; CHECK-TYPED-PTRS: %bc.case = getelementptr inbounds i32, i32* %[[BC_CASE_CONST]], i64 %provided.offset
; CHECK-OPAQUE-PTRS: %bc.case = getelementptr inbounds i32, ptr %simple_global_array.lowered, i64 %provided.offset

  %gep.bc.case = ptrtoint i32* getelementptr inbounds (i32, i32* bitcast ([8 x i32]* @simple_global_array to i32*), i64 3) to i64
; CHECK-TYPED-PTRS: %gep.bc.case = ptrtoint i32* %[[BC_GEP_CASE_CONST]] to i64
; CHECK-OPAQUE-PTRS: %gep.bc.case = ptrtoint ptr %[[BC_GEP_CASE_CONST]] to i64

  %p2i.gep.bc.case = add i64 ptrtoint (i32* getelementptr inbounds (i32, i32* bitcast ([8 x i32]* @simple_global_array to i32*), i64 5) to i64), 1
; CHECK: %p2i.gep.bc.case = add i64 %[[P2I_BC_GEP_CASE_CONST]], 1
  ret void
}
