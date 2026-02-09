;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; RUN: %llc_typed_ptrs  %s -march=genx64 -mcpu=Xe3PLPG -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null | FileCheck %s --check-prefixes=CHECK,CHECK-XE3PLPG
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=Xe3PLPG -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null | FileCheck %s --check-prefixes=CHECK,CHECK-XE3PLPG



; CHECK: Build option:

; CHECK-XE3PLPG-DAG: -enable320and448Vrt



target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(argmem: readwrite)
define dllexport spir_kernel void @test(i32 addrspace(1)* nocapture readonly %in, i32 addrspace(1)* nocapture writeonly %out, i32 %arg, i64 %impl.arg.private.base, i64 %impl.arg.indirect.data.buffer, i64 %impl.arg.scratch.buffer) local_unnamed_addr #0 {
entry:
  %val = load i32, i32 addrspace(1)* %in, align 4
  %add = add i32 %val, %arg
  store i32 %add, i32 addrspace(1)* %out, align 4
  ret void
}

attributes #0 = { mustprogress nofree noinline norecurse nosync nounwind willreturn memory(argmem: readwrite) "CMGenxMain" }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!5}

!0 = !{void (i32 addrspace(1)*, i32 addrspace(1)*, i32, i64, i64, i64)* @test, !"test", !1, i32 0, !2, !3, !4, i32 0}
!1 = !{i32 0, i32 0, i32 0, i32 96, i32 144, i32 152}
!2 = !{i32 200, i32 208, i32 216, i32 192, i32 128, i32 136}
!3 = !{i32 0, i32 0, i32 0}
!4 = !{!"svmptr_t", !"svmptr_t", !""}
!5 = !{void (i32 addrspace(1)*, i32 addrspace(1)*, i32, i64, i64, i64)* @test, !6, !7, !8, !9, i32 0}
!6 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!7 = !{i32 0, i32 1, i32 2, i32 3, i32 4, i32 5}
!8 = !{}
!9 = !{i32 255, i32 255, i32 -1, i32 255, i32 255, i32 255}
