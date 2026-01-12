;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXStructSplitter -vc-struct-splitting=1 -march=genx64 -mcpu=XeLPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXStructSplitter -vc-struct-splitting=1 -march=genx64 -mcpu=XeLPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

declare void @llvm.lifetime.start.p0i8(i64 immarg, i8* nocapture)
declare void @llvm.lifetime.end.p0i8(i64 immarg, i8* nocapture)

%F = type {i32, float}

; Function Attrs: noinline norecurse nounwind optnone uwtable
define dllexport spir_kernel void @main() #1 {
entry:
; CHECK-DAG: %[[I_A:[^ ]+]] = alloca i32, align 4
; CHECK-DAG: %[[F_A:[^ ]+]] = alloca float, align 4
; CHECK:     %[[F1_A:[^ ]+]] = alloca %F, align 4
; CHECK:     %[[F2_A:[^ ]+]] = alloca %F, align 4
; CHECK:     %[[F3_A:[^ ]+]] = alloca %F, align 4
  %f1 = alloca %F, align 4
  %f2 = alloca %F, align 4
  %f3 = alloca %F, align 4

; CHECK-TYPED-PTRS:     %[[BC1:[^ ]+]] = bitcast %F* %f1 to i8*
; CHECK-TYPED-PTRS:     %[[BC2:[^ ]+]] = bitcast %F* %f2 to i8*
; CHECK-TYPED-PTRS-DAG: %[[BC_I_S:[^ ]+]] = bitcast i32* %[[I_A]] to i8*
; CHECK-TYPED-PTRS-DAG: call void @llvm.lifetime.start.p0i8(i64 4, i8* %[[BC_I_S]])
; CHECK-TYPED-PTRS-DAG: %[[BC_F_S:[^ ]+]] = bitcast float* %[[F_A]] to i8*
; CHECK-TYPED-PTRS-DAG: call void @llvm.lifetime.start.p0i8(i64 4, i8* %[[BC_F_S]])
; CHECK-OPAQUE-PTRS:     %[[BC1:[^ ]+]] = bitcast ptr %f1 to ptr
; CHECK-OPAQUE-PTRS:     %[[BC2:[^ ]+]] = bitcast ptr %f2 to ptr
; CHECK-OPAQUE-PTRS-DAG: call void @llvm.lifetime.start.p0(i64 4, ptr %[[I_A]])
; CHECK-OPAQUE-PTRS-DAG: call void @llvm.lifetime.start.p0(i64 4, ptr %[[F_A]])
  %bc1 = bitcast %F* %f1 to i8*
  %bc2 = bitcast %F* %f2 to i8*
  call void @llvm.lifetime.start.p0i8(i64 8, i8* %bc1)
  call void @llvm.lifetime.start.p0i8(i64 8, i8* %bc2)

; CHECK-TYPED-PTRS: %[[I_USAGE:[^ ]+]] = ptrtoint i32* %[[I_A]] to i64
; CHECK-OPAQUE-PTRS: %[[I_USAGE:[^ ]+]] = ptrtoint ptr %[[I_A]] to i64
  %i = getelementptr %F, %F* %f1, i32 0, i32 0
  %i_usage = ptrtoint i32* %i to i64            ; 'i' will be replaced to instruction: alloca i32.

; CHECK-TYPED-PTRS-NEXT: %[[F_GEP:[^ ]+]] =  getelementptr %F, %F* %[[F2_A]], i32 0, i32 1
; CHECK-TYPED-PTRS-NEXT: %[[F_USAGE:[^ ]+]] = ptrtoint float* %[[F_GEP]] to i64
; CHECK-TYPED-PTRS-NEXT: %[[FPTR:[^ ]+]] = ptrtoint i8* %[[BC2]] to i64
; CHECK-OPAQUE-PTRS-NEXT: %[[F_GEP:[^ ]+]] =  getelementptr %F, ptr %[[F2_A]], i32 0, i32 1
; CHECK-OPAQUE-PTRS-NEXT: %[[F_USAGE:[^ ]+]] = ptrtoint ptr %[[F_GEP]] to i64
; CHECK-OPAQUE-PTRS-NEXT: %[[FPTR:[^ ]+]] = ptrtoint ptr %[[BC2]] to i64
  %f = getelementptr %F, %F* %f2, i32 0, i32 1
  %f_usage = ptrtoint float* %f to i64          ; 'f' wont be replaced as bc2 uses not only in lifetime intrinsics.
  %fptr = ptrtoint i8* %bc2 to i64

; CHECK-TYPED-PTRS:      %[[I1_GEP:[^ ]+]] =  getelementptr %F, %F* %[[F3_A]], i32 0, i32 0
; CHECK-TYPED-PTRS-NEXT: %[[I1_USAGE:[^ ]+]] = ptrtoint i32* %[[I1_GEP]] to i64
; CHECK-OPAQUE-PTRS:      %[[I1_GEP:[^ ]+]] =  getelementptr %F, ptr %[[F3_A]], i32 0, i32 0
; CHECK-OPAQUE-PTRS-NEXT: %[[I1_USAGE:[^ ]+]] = ptrtoint ptr %[[I1_GEP]] to i64
  %addr = addrspacecast %F* %f3 to i32 addrspace(1)*
  %i1 = getelementptr %F, %F* %f3, i32 0, i32 0
  %i1_usage = ptrtoint i32* %i1 to i64            ; %i1 wont be replaced because of addrspacecast.

; CHECK-TYPED-PTRS-DAG: %[[BC_I_E:[^ ]+]] = bitcast i32* %[[I_A]] to i8*
; CHECK-TYPED-PTRS-DAG: call void @llvm.lifetime.end.p0i8(i64 4, i8* %[[BC_I_E]])
; CHECK-TYPED-PTRS-DAG: %[[BC_F_E:[^ ]+]] = bitcast float* %[[F_A]] to i8*
; CHECK-TYPED-PTRS-DAG: call void @llvm.lifetime.end.p0i8(i64 4, i8* %[[BC_F_E]])
; CHECK-OPAQUE-PTRS-DAG: call void @llvm.lifetime.end.p0(i64 4, ptr %[[I_A]])
; CHECK-OPAQUE-PTRS-DAG: call void @llvm.lifetime.end.p0(i64 4, ptr %[[F_A]])
  call void @llvm.lifetime.end.p0i8(i64 8, i8* %bc1)
  call void @llvm.lifetime.end.p0i8(i64 8, i8* %bc2)

  ret void
}
