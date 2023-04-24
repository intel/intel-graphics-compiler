;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check buffer arguments are converted to SSO parameters.

; RUN: opt %use_old_pass_manager% -GenXPromoteStatefulToBindless -vc-use-bindless-buffers -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; CHECK: @llvm.vc.predef.var.bss = external global i32
declare <8 x i32> @llvm.genx.oword.ld.v8i32(i32, i32, i32)
declare <8 x i32> @llvm.genx.oword.ld.unaligned.v8i32(i32, i32, i32)
declare void @llvm.genx.oword.st.v8i32(i32, i32, <8 x i32>)

; CHECK-LABEL: @simple(
; CHECK-SAME: i32 [[SURF:[^, ]*]]
; CHECK:      call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SURF]])
; CHECK-NEXT: call <8 x i32> @llvm.genx.oword.ld.predef.surface.v8i32.p0i32(i32 0, i32* @llvm.vc.predef.var.bss, i32 0)
define dllexport spir_kernel void @simple(i32 %surf, i32 %samp) #0 {
  %ret = call <8 x i32> @llvm.genx.oword.ld.v8i32(i32 0, i32 %surf, i32 0)
  ret void
}

; CHECK-LABEL: @read_write(
; CHECK-SAME: i32 [[INBUF:%[^, ]*]], i32 [[OUTBUF:%[^, ]*]], i32 [[INOFF:%[^, ]*]], i32 [[OUTOFF:%[^, ]*]])
; CHECK:      call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[INBUF]])
; CHECK-NEXT: [[BINDLESS:%[^ ]*]] = call <8 x i32> @llvm.genx.oword.ld.unaligned.predef.surface.v8i32.p0i32(i32 0, i32* @llvm.vc.predef.var.bss, i32 [[INOFF]])
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[OUTBUF]])
; CHECK-NEXT: call void @llvm.genx.oword.st.predef.surface.p0i32.v8i32(i32* @llvm.vc.predef.var.bss, i32 [[OUTOFF]], <8 x i32> [[BINDLESS]])
define dllexport spir_kernel void @read_write(i32 %inbuf, i32 %outbuf, i32 %inoff, i32 %outoff) #0 {
  %data = call <8 x i32> @llvm.genx.oword.ld.unaligned.v8i32(i32 0, i32 %inbuf, i32 %inoff)
  call void @llvm.genx.oword.st.v8i32(i32 %outbuf, i32 %outoff, <8 x i32> %data)
  ret void
}

attributes #0 = { "CMGenxMain" }

!genx.kernels = !{!0, !5}
!genx.kernel.internal = !{!4, !9}
; CHECK: !genx.kernels = !{[[SIMPLE_NODE:![0-9]+]], [[RW_NODE:![0-9]+]]}
; CHECK-DAG: [[SIMPLE_NODE]] = !{void (i32, i32)* @simple, !"simple", [[SIMPLE_KINDS:![0-9]+]]
; CHECK-DAG: [[SIMPLE_KINDS]] = !{i32 0, i32 1}
; CHECK-DAG: [[RW_NODE]] = !{void (i32, i32, i32, i32)* @read_write, !"read_write", [[RW_KINDS:![0-9]]]
; CHECK-DAG: [[RW_KINDS]] = !{i32 0, i32 0, i32 0, i32 0}

!0 = !{void (i32, i32)* @simple, !"simple", !1, i32 0, i32 0, !2, !3, i32 0}
!1 = !{i32 2, i32 1}
!2 = !{i32 0, i32 0}
!3 = !{!"buffer_t read_write", !"sampler_t"}
!4 = !{void (i32, i32)* @simple, null, null, null, null}

!5 = !{void (i32, i32, i32, i32)* @read_write, !"read_write", !6, i32 0, i32 0, !7, !8, i32 0}
!6 = !{i32 2, i32 2, i32 0, i32 0}
!7 = !{i32 0, i32 0, i32 0, i32 0}
!8 = !{!"buffer_t", !"buffer_t", !"", !""}
!9 = !{void (i32, i32, i32, i32)* @read_write, null, null, null, null}
