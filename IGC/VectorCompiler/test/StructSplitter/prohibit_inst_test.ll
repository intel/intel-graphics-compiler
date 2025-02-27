;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXStructSplitter -vc-struct-splitting=1 -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXStructSplitter -vc-struct-splitting=1 -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

%F = type {i32, float}

; Function Attrs: noinline norecurse nounwind optnone uwtable
define dllexport spir_kernel void @main() #1 {
entry:
; CHECK-DAG:    %[[I_A:[^ ]+]] = alloca i32, align 4
; CHECK-DAG:    %[[F_A:[^ ]+]] = alloca float, align 4
; CHECK:        %[[F1_A:[^ ]+]] = alloca %F, align 4
; CHECK:        %[[F2_A:[^ ]+]] = alloca %F, align 4
  %f1 = alloca %F, align 4
  %f2 = alloca %F, align 4

; CHECK-TYPED-PTRS:        %[[I_USAGE:[^ ]+]] = ptrtoint i32* %[[I_A]] to i64
; CHECK-OPAQUE-PTRS:        %[[I_USAGE:[^ ]+]] = ptrtoint ptr %[[I_A]] to i64
  %i = getelementptr %F, %F* %f1, i32 0, i32 0
  %i_usage = ptrtoint i32* %i to i64            ; 'i' will be replaced to instruction: alloca i32.

; CHECK-TYPED-PTRS-NEXT:   %[[F_GEP:[^ ]+]] =  getelementptr %F, %F* %[[F2_A]], i32 0, i32 1
; CHECK-TYPED-PTRS-NEXT:   %[[F_USAGE:[^ ]+]] = ptrtoint float* %[[F_GEP]] to i64
; CHECK-TYPED-PTRS-NEXT:   %[[TMP:[^ ]+]] = ptrtoint %F* %[[F2_A]] to i64
; CHECK-OPAQUE-PTRS-NEXT:   %[[F_GEP:[^ ]+]] =  getelementptr %F, ptr %[[F2_A]], i32 0, i32 1
; CHECK-OPAQUE-PTRS-NEXT:   %[[F_USAGE:[^ ]+]] = ptrtoint ptr %[[F_GEP]] to i64
; CHECK-OPAQUE-PTRS-NEXT:   %[[TMP:[^ ]+]] = ptrtoint ptr %[[F2_A]] to i64
; CHECK-NEXT:   %[[SUB:[^ ]+]] = sub i64 4, %[[TMP]]
  %f = getelementptr %F, %F* %f2, i32 0, i32 1
  %f_usage = ptrtoint float* %f to i64          ; 'f' wont be replaced as f2 is forbidden to split.
  %tmp = ptrtoint %F* %f2 to i64
  %sub = sub i64 4, %tmp                        ; 'sub' is prohibited instr.

  ret void
}
