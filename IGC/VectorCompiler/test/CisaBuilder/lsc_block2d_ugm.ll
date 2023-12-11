;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXModule -GenXCategoryWrapper -GenXCisaBuilderPass -GenXFinalizer \
; RUN: -march=genx64 -mtriple=spir64-unknown-unknown -finalizer-opts="-dumpcommonisa -isaasmToConsole" \
; RUN: -mcpu=XeHPC -o /dev/null < %s | FileCheck %s

declare <16 x i32> @llvm.vc.internal.lsc.load.block.2d.ugm.v16i32.v2i8(i1, i8, <2 x i8>, i8, i16, i16, i64, i32, i32, i32, i32, i32, i32, i32, <16 x i32>)
declare <32 x i16> @llvm.vc.internal.lsc.load.block.2d.ugm.v32i16.v2i8(i1, i8, <2 x i8>, i8, i16, i16, i64, i32, i32, i32, i32, i32, i32, i32, <32 x i16>)
declare <16 x i32> @llvm.vc.internal.lsc.load.block.2d.ugm.transposed.v16i32.v2i8(i1, i8, <2 x i8>, i8, i16, i16, i64, i32, i32, i32, i32, i32, i32, i32, <16 x i32>)
declare <64 x i8> @llvm.vc.internal.lsc.load.block.2d.ugm.vnni.v64i8.v2i8(i1, i8, <2 x i8>, i8, i16, i16, i64, i32, i32, i32, i32, i32, i32, i32, <64 x i8>)

declare void @llvm.vc.internal.lsc.prefetch.block.2d.ugm.v2i8(i1, i8, <2 x i8>, i8, i16, i16, i64, i32, i32, i32, i32, i32, i32, i32)

declare void @llvm.vc.internal.lsc.store.block.2d.ugm.v2i8.v16i32(i1, i8, <2 x i8>, i8, i16, i16, i64, i32, i32, i32, i32, i32, i32, i32, <16 x i32>)

; CHECK-LABEL: .kernel "test"

; CHECK: .decl [[BASE:V[0-9]+]] v_type=G type=uq num_elts=1 alias=<[[IBASE:V[0-9]+]], 0>
; CHECK: .decl [[WIDTH:V[0-9]+]] v_type=G type=ud num_elts=1 alias=<[[IWIDTH:V[0-9]+]], 0>
; CHECK: .decl [[HEIGHT:V[0-9]+]] v_type=G type=ud num_elts=1 alias=<[[IHEIGHT:V[0-9]+]], 0>
; CHECK: .decl [[PITCH:V[0-9]+]] v_type=G type=ud num_elts=1 alias=<[[IPITCH:V[0-9]+]], 0>
; CHECK: .decl [[X:V[0-9]+]] v_type=G type=d num_elts=1 alias=<[[IX:V[0-9]+]], 0>
; CHECK: .decl [[Y:V[0-9]+]] v_type=G type=d num_elts=1 alias=<[[IY:V[0-9]+]], 0>
; CHECK: .input [[IBASE]] offset=64 size=8
; CHECK: .input [[IWIDTH]] offset=72 size=4
; CHECK: .input [[IHEIGHT]] offset=76 size=4
; CHECK: .input [[IPITCH]] offset=80 size=4
; CHECK: .input [[IX]] offset=84 size=4
; CHECK: .input [[IY]] offset=88 size=4

define dllexport spir_kernel void @test(i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y) {
  ; CHECK: lsc_load_block2d.ugm.uc.ca (M1, 1)  [[LOAD:V[0-9]+]]:d32.8x2nn  flat[[[BASE]],[[WIDTH]],[[HEIGHT]],[[PITCH]],[[X]],[[Y]]]
  %load = call <16 x i32> @llvm.vc.internal.lsc.load.block.2d.ugm.v16i32.v2i8(i1 true, i8 3, <2 x i8> <i8 1, i8 2>, i8 1, i16 8, i16 2, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y, i32 0, i32 0, <16 x i32> undef)
  ; CHECK: lsc_load_block2d.ugm.ca.uc (M1, 1)  [[LOAD2:V[0-9]+]]:d16.2x8x2nn  flat[[[BASE]],[[WIDTH]],[[HEIGHT]],[[PITCH]],[[X]],[[Y]]]
  %load.a2 = call <32 x i16> @llvm.vc.internal.lsc.load.block.2d.ugm.v32i16.v2i8(i1 true, i8 2, <2 x i8> <i8 2, i8 1>, i8 2, i16 8, i16 2, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y, i32 0, i32 0, <32 x i16> undef)
  ; CHECK: lsc_load_block2d.ugm.st.uc (M1, 1)  [[LOADT:V[0-9]+]]:d32.2x8tn  flat[[[BASE]],[[WIDTH]],[[HEIGHT]],[[PITCH]],[[X]],[[Y]]]
  %load.t = call <16 x i32> @llvm.vc.internal.lsc.load.block.2d.ugm.transposed.v16i32.v2i8(i1 true, i8 3, <2 x i8> <i8 5, i8 1>, i8 1, i16 2, i16 8, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y, i32 0, i32 0, <16 x i32> undef)
  ; CHECK: lsc_load_block2d.ugm.st.ca (M1, 1)  [[LOADV:V[0-9]+]]:d8.4x16nt  flat[[[BASE]],[[WIDTH]],[[HEIGHT]],[[PITCH]],[[X]],[[Y]]]
  %load.v = call <64 x i8> @llvm.vc.internal.lsc.load.block.2d.ugm.vnni.v64i8.v2i8(i1 true, i8 1, <2 x i8> <i8 5, i8 2>, i8 1, i16 4, i16 16, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y, i32 0, i32 0, <64 x i8> undef)

  ; CHECK: lsc_load_block2d.ugm.uc.ca (M1, 1)  %null:d64.8x2nn  flat[[[BASE]],[[WIDTH]],[[HEIGHT]],[[PITCH]],[[X]],[[Y]]]
  call void @llvm.vc.internal.lsc.prefetch.block.2d.ugm.v2i8(i1 true, i8 4, <2 x i8> <i8 1, i8 2>, i8 1, i16 8, i16 2, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y, i32 0, i32 0)

  ; CHECK: lsc_store_block2d.ugm.wt.wb (M1, 1)  flat[[[BASE]],[[WIDTH]],[[HEIGHT]],[[PITCH]],[[X]],[[Y]]]  [[LOAD:V[0-9]+]]:d32.8x2nn
  call void @llvm.vc.internal.lsc.store.block.2d.ugm.v2i8.v16i32(i1 true, i8 3, <2 x i8> <i8 4, i8 3>, i8 1, i16 8, i16 2, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y, i32 0, i32 0, <16 x i32> %load)
  ret void
}

attributes #1 = { noinline nounwind "CMGenxMain" "VC.Stack.Amount"="0" "target-cpu"="XeHPC" }

!spirv.Source = !{!0}
!opencl.spir.version = !{!1}
!opencl.ocl.version = !{!0}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!2}
!spirv.Generator = !{!3}
!genx.kernels = !{!4}
!genx.kernel.internal = !{!8}

!0 = !{i32 0, i32 0}
!1 = !{i32 1, i32 2}
!2 = !{}
!3 = !{i16 6, i16 14}
!4 = !{void (i64, i32, i32, i32, i32, i32)* @test, !"test", !5, i32 0, !6, !0, !7, i32 0}
!5 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!6 = !{i32 64, i32 72, i32 76, i32 80, i32 84, i32 88}
!7 = !{!"svmptr_t"}
!8 = !{void (i64, i32, i32, i32, i32, i32)* @test, null, null, null, null}
