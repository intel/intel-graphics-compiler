;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXStructSplitter -vc-struct-splitting=1 -march=genx64 -mcpu=XeLPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXStructSplitter -vc-struct-splitting=1 -march=genx64 -mcpu=XeLPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

; CHECK-LABEL: main
; CHECK-DAG:    %[[CI:[^ ]+]] = alloca i32, align 4
; CHECK-DAG:    %[[CF:[^ ]+]] = alloca %S2, align 4
; CHECK-TYPED-PTRS:        %[[F:[^ ]+]] = getelementptr %S2, %S2* %[[CF]], i32 0, i32 1, i32 1, i32 1
; CHECK-TYPED-PTRS:        %[[U_F:[^ ]+]] = ptrtoint float* %[[F]] to i64
; CHECK-TYPED-PTRS:        %[[U_I:[^ ]+]] = ptrtoint i32* %[[CI]] to i64
; CHECK-OPAQUE-PTRS:        %[[F:[^ ]+]] = getelementptr %S2, ptr %[[CF]], i32 0, i32 1, i32 1, i32 1
; CHECK-OPAQUE-PTRS:        %[[U_F:[^ ]+]] = ptrtoint ptr %[[F]] to i64
; CHECK-OPAQUE-PTRS:        %[[U_I:[^ ]+]] = ptrtoint ptr %[[CI]] to i64


%S = type { float, float }
%S1 = type { float, %S, float}
%S2 = type { float, %S1, float}
%C = type { %S2, i32 }


; Function Attrs: noinline norecurse nounwind optnone uwtable
define dllexport spir_kernel void @main() #1 {
entry:
  %c = alloca %C, align 4
  %f = getelementptr inbounds %C, %C* %c, i32 0, i32 0, i32 1, i32 1, i32 1
  %user_of_f = ptrtoint float* %f to i64

  %i = getelementptr inbounds %C, %C* %c, i32 0, i32 1
  %user_of_i = ptrtoint i32* %i to i64

  ret void
}
