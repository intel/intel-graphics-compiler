;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: not %opt_typed_ptrs %use_old_pass_manager% -GenXPropagateSurfaceState -march=genx64 -mcpu=Xe3PLPG -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown  -S < %s 2>&1 | FileCheck %s
; RUN: not %opt_opaque_ptrs %use_old_pass_manager% -GenXPropagateSurfaceState -march=genx64 -mcpu=Xe3PLPG -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown  -S < %s 2>&1 | FileCheck %s

; CHECK: GenXPropagateSurfaceState: BTI argument is not expected

define spir_kernel void @linear(i32 %ibuf, i32 %obuf, <3 x i16> %impl.arg.llvm.genx.local.id16, <3 x i32> %impl.arg.llvm.genx.local.size, i64 %impl.arg.private.base, i64 %impl.arg.indirect.data.buffer, i64 %impl.arg.scratch.buffer) #0 {
entry:
  %call1.i.i = call <256 x i8> @llvm.vc.internal.lsc.load.2d.tgm.bti.v256i8.v3i8(<3 x i8> zeroinitializer, i32 %ibuf, i32 0, i32 0, i32 0, i32 0)
  ret void
}

declare <256 x i8> @llvm.vc.internal.lsc.load.2d.tgm.bti.v256i8.v3i8(<3 x i8>, i32, i32, i32, i32, i32)

attributes #0 = { "CMGenxMain" }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!5}

!0 = !{void (i32, i32, <3 x i16>, <3 x i32>, i64, i64, i64)* @linear, !"linear", !1, i32 0, !2, !3, !4, i32 0}
!1 = !{i32 2, i32 2, i32 24, i32 8, i32 96, i32 144, i32 152}
!2 = !{i32 216, i32 224, i32 64, i32 192, i32 208, i32 128, i32 136}
!3 = !{i32 0, i32 0}
!4 = !{!"image2d_t read_write", !"image2d_t read_write"}
!5 = !{void (i32, i32, <3 x i16>, <3 x i32>, i64, i64, i64)* @linear, !6, !7, !8, !9, i32 0}
!6 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!7 = !{i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6}
!8 = !{}
!9 = !{i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255}
