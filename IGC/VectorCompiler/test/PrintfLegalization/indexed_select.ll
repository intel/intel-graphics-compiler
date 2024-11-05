;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXPrintfLegalization -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXPrintfLegalization -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

@str.true = internal unnamed_addr addrspace(2) constant [5 x i8] c"true\00", align 1 #0
@str.false = internal unnamed_addr addrspace(2) constant [6 x i8] c"false\00", align 1 #0

declare i32 @llvm.vc.internal.print.format.index.p2i8(i8 addrspace(2)*)

define dllexport void @simple(i1 %cond) {
  %gep.true = getelementptr inbounds [5 x i8], [5 x i8] addrspace(2)* @str.true, i64 0, i64 0
  %gep.false = getelementptr inbounds [6 x i8], [6 x i8] addrspace(2)* @str.false, i64 0, i64 0
; CHECK-TYPED-PTRS: %gep.true = getelementptr inbounds [5 x i8], [5 x i8] addrspace(2)* @str.true, i64 0, i64 0
; CHECK-TYPED-PTRS: %gep.false = getelementptr inbounds [6 x i8], [6 x i8] addrspace(2)* @str.false, i64 0, i64 0
; CHECK-TYPED-PTRS-DAG: %[[SIMPLE_GEP_TRUE_IDX:[^ ]+]] = call i32 @llvm.vc.internal.print.format.index.p2i8(i8 addrspace(2)* %gep.true)
; CHECK-TYPED-PTRS-DAG: %[[SIMPLE_GEP_FALSE_IDX:[^ ]+]] = call i32 @llvm.vc.internal.print.format.index.p2i8(i8 addrspace(2)* %gep.false)
; CHECK-OPAQUE-PTRS: %gep.true = getelementptr inbounds [5 x i8], ptr addrspace(2) @str.true, i64 0, i64 0
; CHECK-OPAQUE-PTRS: %gep.false = getelementptr inbounds [6 x i8], ptr addrspace(2) @str.false, i64 0, i64 0
; CHECK-OPAQUE-PTRS-DAG: %[[SIMPLE_GEP_TRUE_IDX:[^ ]+]] = call i32 @llvm.vc.internal.print.format.index.p2(ptr addrspace(2) %gep.true)
; CHECK-OPAQUE-PTRS-DAG: %[[SIMPLE_GEP_FALSE_IDX:[^ ]+]] = call i32 @llvm.vc.internal.print.format.index.p2(ptr addrspace(2) %gep.false)
  %select.gep = select i1 %cond, i8 addrspace(2)* %gep.true, i8 addrspace(2)* %gep.false
; CHECK: %[[SIMPLE_GEP_SELECT:[^ ]+]] = select i1 %cond, i32 %[[SIMPLE_GEP_TRUE_IDX]], i32 %[[SIMPLE_GEP_FALSE_IDX]]
  %idx.select.gep = call i32 @llvm.vc.internal.print.format.index.p2i8(i8 addrspace(2)* %select.gep)
  %user.gep = add i32 %idx.select.gep, 1
; CHECK: %user.gep = add i32 %[[SIMPLE_GEP_SELECT]], 1

; CHECK-TYPED-PTRS-DAG: %[[SIMPLE_MIXED_TRUE_IDX:[^ ]+]] = call i32 @llvm.vc.internal.print.format.index.p2i8(i8 addrspace(2)* %gep.true)
; CHECK-TYPED-PTRS-DAG: %[[SIMPLE_MIXED_FALSE_IDX:[^ ]+]] = call i32 @llvm.vc.internal.print.format.index.p2i8(i8 addrspace(2)* getelementptr inbounds ([6 x i8], [6 x i8] addrspace(2)* @str.false, i64 0, i64 0))
; CHECK-OPAQUE-PTRS-DAG: %[[SIMPLE_MIXED_TRUE_IDX:[^ ]+]] = call i32 @llvm.vc.internal.print.format.index.p2(ptr addrspace(2) %gep.true)
; CHECK-OPAQUE-PTRS-DAG: %[[SIMPLE_MIXED_FALSE_IDX:[^ ]+]] = call i32 @llvm.vc.internal.print.format.index.p2({{.*}}ptr addrspace(2) @str.false
  %select.mixed = select i1 %cond, i8 addrspace(2)* %gep.true, i8 addrspace(2)* getelementptr inbounds ([6 x i8], [6 x i8] addrspace(2)* @str.false, i64 0, i64 0)
; CHECK: %[[SIMPLE_MIXED_SELECT:[^ ]+]] = select i1 %cond, i32 %[[SIMPLE_MIXED_TRUE_IDX]], i32 %[[SIMPLE_MIXED_FALSE_IDX]]
  %idx.select.mixed = call i32 @llvm.vc.internal.print.format.index.p2i8(i8 addrspace(2)* %select.mixed)

; CHECK-TYPED-PTRS-DAG: %[[SIMPLE_DIRECT_TRUE_IDX:[^ ]+]] = call i32 @llvm.vc.internal.print.format.index.p2i8(i8 addrspace(2)* getelementptr inbounds ([5 x i8], [5 x i8] addrspace(2)* @str.true, i64 0, i64 0))
; CHECK-TYPED-PTRS-DAG: %[[SIMPLE_DIRECT_FALSE_IDX:[^ ]+]] = call i32 @llvm.vc.internal.print.format.index.p2i8(i8 addrspace(2)* getelementptr inbounds ([6 x i8], [6 x i8] addrspace(2)* @str.false, i64 0, i64 0))
; CHECK-OPAQUE-PTRS-DAG: %[[SIMPLE_DIRECT_TRUE_IDX:[^ ]+]] = call i32 @llvm.vc.internal.print.format.index.p2({{.*}}ptr addrspace(2) @str.true
; CHECK-OPAQUE-PTRS-DAG: %[[SIMPLE_DIRECT_FALSE_IDX:[^ ]+]] = call i32 @llvm.vc.internal.print.format.index.p2({{.*}}ptr addrspace(2) @str.false
  %select.direct = select i1 %cond, i8 addrspace(2)* getelementptr inbounds ([5 x i8], [5 x i8] addrspace(2)* @str.true, i64 0, i64 0), i8 addrspace(2)* getelementptr inbounds ([6 x i8], [6 x i8] addrspace(2)* @str.false, i64 0, i64 0)
; CHECK: %[[SIMPLE_DIRECT_SELECT:[^ ]+]] = select i1 %cond, i32 %[[SIMPLE_DIRECT_TRUE_IDX]], i32 %[[SIMPLE_DIRECT_FALSE_IDX]]
  %idx.select.direct = call i32 @llvm.vc.internal.print.format.index.p2i8(i8 addrspace(2)* %select.direct)
  ret void
}

define dllexport void @two_indices_one_select(i1 %cond) {
  %gep.true = getelementptr inbounds [5 x i8], [5 x i8] addrspace(2)* @str.true, i64 0, i64 0
  %gep.false = getelementptr inbounds [6 x i8], [6 x i8] addrspace(2)* @str.false, i64 0, i64 0
; CHECK-TYPED-PTRS: %gep.true = getelementptr inbounds [5 x i8], [5 x i8] addrspace(2)* @str.true, i64 0, i64 0
; CHECK-TYPED-PTRS: %gep.false = getelementptr inbounds [6 x i8], [6 x i8] addrspace(2)* @str.false, i64 0, i64 0
; CHECK-TYPED-PTRS-DAG: %[[IDX2SEL1_TRUE_IDX:[^ ]+]] = call i32 @llvm.vc.internal.print.format.index.p2i8(i8 addrspace(2)* %gep.true)
; CHECK-TYPED-PTRS-DAG: %[[IDX2SEL1_FALSE_IDX:[^ ]+]] = call i32 @llvm.vc.internal.print.format.index.p2i8(i8 addrspace(2)* %gep.false)
; CHECK-OPAQUE-PTRS: %gep.true = getelementptr inbounds [5 x i8], ptr addrspace(2) @str.true, i64 0, i64 0
; CHECK-OPAQUE-PTRS: %gep.false = getelementptr inbounds [6 x i8], ptr addrspace(2) @str.false, i64 0, i64 0
; CHECK-OPAQUE-PTRS-DAG: %[[IDX2SEL1_TRUE_IDX:[^ ]+]] = call i32 @llvm.vc.internal.print.format.index.p2(ptr addrspace(2) %gep.true)
; CHECK-OPAQUE-PTRS-DAG: %[[IDX2SEL1_FALSE_IDX:[^ ]+]] = call i32 @llvm.vc.internal.print.format.index.p2(ptr addrspace(2) %gep.false)
  %select.gep = select i1 %cond, i8 addrspace(2)* %gep.true, i8 addrspace(2)* %gep.false
; CHECK: %[[IDX2SEL1_SELECT:[^ ]+]] = select i1 %cond, i32 %[[IDX2SEL1_TRUE_IDX]], i32 %[[IDX2SEL1_FALSE_IDX]]
  %idx.select.gep.0 = call i32 @llvm.vc.internal.print.format.index.p2i8(i8 addrspace(2)* %select.gep)
  %idx.select.gep.1 = call i32 @llvm.vc.internal.print.format.index.p2i8(i8 addrspace(2)* %select.gep)
  %user.0 = add i32 %idx.select.gep.0, 1
; CHECK: %user.0 = add i32 %[[IDX2SEL1_SELECT]], 1
  %user.1 = add i32 %idx.select.gep.1, 2
; CHECK: %user.1 = add i32 %[[IDX2SEL1_SELECT]], 2
  ret void
}

; COM:          %select.top
; COM:          /         \
; COM:         v           v
; COM: %select.left <- %select.right
; COM:          \         /
; COM:           v       v
; COM:        %select.bottom
define dllexport void @entangled(i1 %cond) {
  %gep.true = getelementptr inbounds [5 x i8], [5 x i8] addrspace(2)* @str.true, i64 0, i64 0
  %gep.false = getelementptr inbounds [6 x i8], [6 x i8] addrspace(2)* @str.false, i64 0, i64 0
; CHECK-TYPED-PTRS: %gep.true = getelementptr inbounds [5 x i8], [5 x i8] addrspace(2)* @str.true, i64 0, i64 0
; CHECK-TYPED-PTRS: %gep.false = getelementptr inbounds [6 x i8], [6 x i8] addrspace(2)* @str.false, i64 0, i64 0
; CHECK-TYPED-PTRS-DAG: %[[ENTANGLED_TOP_TRUE_IDX:[^ ]+]] = call i32 @llvm.vc.internal.print.format.index.p2i8(i8 addrspace(2)* %gep.true)
; CHECK-TYPED-PTRS-DAG: %[[ENTANGLED_TOP_FALSE_IDX:[^ ]+]] = call i32 @llvm.vc.internal.print.format.index.p2i8(i8 addrspace(2)* %gep.false)
; CHECK-OPAQUE-PTRS: %gep.true = getelementptr inbounds [5 x i8], ptr addrspace(2) @str.true, i64 0, i64 0
; CHECK-OPAQUE-PTRS: %gep.false = getelementptr inbounds [6 x i8], ptr addrspace(2) @str.false, i64 0, i64 0
; CHECK-OPAQUE-PTRS-DAG: %[[ENTANGLED_TOP_TRUE_IDX:[^ ]+]] = call i32 @llvm.vc.internal.print.format.index.p2(ptr addrspace(2) %gep.true)
; CHECK-OPAQUE-PTRS-DAG: %[[ENTANGLED_TOP_FALSE_IDX:[^ ]+]] = call i32 @llvm.vc.internal.print.format.index.p2(ptr addrspace(2) %gep.false)
  %select.top = select i1 %cond, i8 addrspace(2)* %gep.true, i8 addrspace(2)* %gep.false
; CHECK: %[[ENTANGLED_TOP_SELECT:[^ ]+]] = select i1 %cond, i32 %[[ENTANGLED_TOP_TRUE_IDX]], i32 %[[ENTANGLED_TOP_FALSE_IDX]]
; CHECK-TYPED-PTRS: %[[ENTANGLED_RIGHT_FALSE_IDX:[^ ]+]] = call i32 @llvm.vc.internal.print.format.index.p2i8(i8 addrspace(2)* %gep.false)
; CHECK-OPAQUE-PTRS: %[[ENTANGLED_RIGHT_FALSE_IDX:[^ ]+]] = call i32 @llvm.vc.internal.print.format.index.p2(ptr addrspace(2) %gep.false)
  %select.right = select i1 %cond, i8 addrspace(2)* %select.top, i8 addrspace(2)* %gep.false
; CHECK: %[[ENTANGLED_RIGHT_SELECT:[^ ]+]] = select i1 %cond, i32 %[[ENTANGLED_TOP_SELECT]], i32 %[[ENTANGLED_RIGHT_FALSE_IDX]]
  %select.left = select i1 %cond, i8 addrspace(2)* %select.top, i8 addrspace(2)* %select.right
; CHECK: %[[ENTANGLED_LEFT_SELECT:[^ ]+]] = select i1 %cond, i32 %[[ENTANGLED_TOP_SELECT]], i32 %[[ENTANGLED_RIGHT_SELECT]]
  %select.bottom = select i1 %cond, i8 addrspace(2)* %select.left, i8 addrspace(2)* %select.right
; CHECK: %[[ENTANGLED_BOTTOM_SELECT:[^ ]+]] = select i1 %cond, i32 %[[ENTANGLED_LEFT_SELECT]], i32 %[[ENTANGLED_RIGHT_SELECT]]
  %idx = call i32 @llvm.vc.internal.print.format.index.p2i8(i8 addrspace(2)* %select.bottom)
  %user = add i32 %idx, 1
; CHECK: %user = add i32 %[[ENTANGLED_BOTTOM_SELECT]], 1
  ret void
}

attributes #0 = { "VCPrintfStringVariable" }
; CHECK: attributes #0 = { "VCPrintfStringVariable" }
