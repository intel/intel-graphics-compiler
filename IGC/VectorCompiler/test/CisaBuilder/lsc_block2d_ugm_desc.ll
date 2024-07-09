;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: llc %s -march=genx64 -mcpu=XeHPC -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s

declare <16 x i32> @llvm.vc.internal.lsc.load.2d.ugm.desc.v16i32.v2i8(i1, <2 x i8>, i8, i16, i16, <16 x i32>, i32, i32, <16 x i32>)
declare <32 x i16> @llvm.vc.internal.lsc.load.2d.ugm.desc.v32i16.v2i8(i1, <2 x i8>, i8, i16, i16, <16 x i32>, i32, i32, <32 x i16>)
declare <16 x i32> @llvm.vc.internal.lsc.load.2d.ugm.desc.transpose.v16i32.v2i8(i1, <2 x i8>, i8, i16, i16, <16 x i32>, i32, i32, <16 x i32>)
declare <64 x i8> @llvm.vc.internal.lsc.load.2d.ugm.desc.vnni.v64i8.v2i8(i1, <2 x i8>, i8, i16, i16, <16 x i32>, i32, i32, <64 x i8>)

declare void @llvm.vc.internal.lsc.prefetch.2d.ugm.desc.v2i8(i1, <2 x i8>, i8, i16, i16, <16 x i32>, i32, i32, i64)

declare void @llvm.vc.internal.lsc.store.2d.ugm.desc.v2i8.v16i32(i1, <2 x i8>, i8, i16, i16, <16 x i32>, i32, i32, <16 x i32>)

; CHECK-LABEL: .kernel "test"

; CHECK: .decl [[BASE:V[0-9]+]] v_type=G type=q num_elts=1
; CHECK: .decl [[WIDTH:V[0-9]+]] v_type=G type=d num_elts=1
; CHECK: .decl [[HEIGHT:V[0-9]+]] v_type=G type=d num_elts=1
; CHECK: .decl [[PITCH:V[0-9]+]] v_type=G type=d num_elts=1
; CHECK: .decl [[X:V[0-9]+]] v_type=G type=d num_elts=1
; CHECK: .decl [[Y:V[0-9]+]] v_type=G type=d num_elts=1
; CHECK: .decl [[DESCQ:V[0-9]+]] v_type=G type=q num_elts=8 alias=<[[DESC:V[0-9]+]], 0>
; CHECK: .decl [[DESCU:V[0-9]+]] v_type=G type=ud num_elts=16 alias=<[[DESC]], 0>

; CHECK: .input [[BASE]] offset=64 size=8
; CHECK: .input [[WIDTH]] offset=72 size=4
; CHECK: .input [[HEIGHT]] offset=76 size=4
; CHECK: .input [[PITCH]] offset=80 size=4
; CHECK: .input [[X]] offset=84 size=4
; CHECK: .input [[Y]] offset=88 size=4

define dllexport spir_kernel void @test(i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y) {
; CHECK-DAG: mov (M1, 1) [[DESCQ]](0,0)<1> [[BASE]](0,0)<0;1,0>
; CHECK-DAG: mov (M1, 1) [[DESC]](0,2)<1> [[WIDTH]](0,0)<0;1,0>
; CHECK-DAG: mov (M1, 1) [[DESC]](0,3)<1> [[HEIGHT]](0,0)<0;1,0>
; CHECK-DAG: mov (M1, 1) [[DESC]](0,4)<1> [[PITCH]](0,0)<0;1,0>
; CHECK-DAG: mov (M1, 1) [[DESC]](0,5)<1> [[X]](0,0)<0;1,0>
; CHECK-DAG: mov (M1, 1) [[DESC]](0,6)<1> [[Y]](0,0)<0;1,0>
  %vbase = bitcast i64 %base to <2 x i32>
  %1 = shufflevector <2 x i32> %vbase, <2 x i32> undef, <16 x i32> <i32 0, i32 1, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2>
  %2 = insertelement <16 x i32> %1, i32 %width, i32 2
  %3 = insertelement <16 x i32> %2, i32 %height, i32 3
  %4 = insertelement <16 x i32> %3, i32 %pitch, i32 4
  %5 = insertelement <16 x i32> %4, i32 %x, i32 5
  %desc = insertelement <16 x i32> %5, i32 %y, i32 6

; CHECK-DAG: mov (M1, 1) [[DESC]](0,7)<1> 0x107:d
; CHECK: lsc_load_block2d.ugm.uc.ca (M1, 1) [[LOAD:V[0-9]+]]:d32.8x2nn flat[[[DESCU]] + (0,0)]
  %desc.1 = insertelement <16 x i32> %desc, i32 263, i32 7 ; 8x2
  %load = call <16 x i32> @llvm.vc.internal.lsc.load.2d.ugm.desc.v16i32.v2i8(i1 true, <2 x i8> <i8 1, i8 2>, i8 1, i16 8, i16 2, <16 x i32> %desc.1, i32 0, i32 0, <16 x i32> undef)

; CHECK: mov (M1, 1) [[DESC]](0,7)<1> 0x10107:d
; CHECK: lsc_load_block2d.ugm.ca.uc (M1, 1) V{{[0-9]+}}:d16.2x8x2nn flat[[[DESCU]] + (0,0)]
  %desc.2 = insertelement <16 x i32> %desc.1, i32 65799, i32 7 ; 2x8x2
  %load.a2 = call <32 x i16> @llvm.vc.internal.lsc.load.2d.ugm.desc.v32i16.v2i8(i1 true, <2 x i8> <i8 2, i8 1>, i8 2, i16 8, i16 2, <16 x i32> %desc.2, i32 0, i32 0, <32 x i16> undef)

; CHECK: mov (M1, 1) [[DESC]](0,7)<1> 0x701:d
; CHECK: lsc_load_block2d.ugm.st.uc (M1, 1) V{{[0-9]+}}:d32.2x8tn flat[[[DESCU]] + (0,0)]
  %desc.3 = insertelement <16 x i32> %desc.2, i32 1793, i32 7 ; 1x2x8
  %load.t = call <16 x i32> @llvm.vc.internal.lsc.load.2d.ugm.desc.transpose.v16i32.v2i8(i1 true, <2 x i8> <i8 5, i8 1>, i8 1, i16 2, i16 8, <16 x i32> %desc.3, i32 0, i32 0, <16 x i32> undef)

; CHECK: mov (M1, 1) [[DESC]](0,7)<1> 0xf03:d
; CHECK: lsc_load_block2d.ugm.st.ca (M1, 1) V{{[0-9]+}}:d8.4x16nt flat[[[DESCU]] + (0,0)]
  %desc.4 = insertelement <16 x i32> %desc.3, i32 3843, i32 7 ; 1x4x16
  %load.v = call <64 x i8> @llvm.vc.internal.lsc.load.2d.ugm.desc.vnni.v64i8.v2i8(i1 true, <2 x i8> <i8 5, i8 2>, i8 1, i16 4, i16 16, <16 x i32> %desc.4, i32 0, i32 0, <64 x i8> undef)

; CHECK-DAG: mov (M1, 1) [[DESC]](0,7)<1> 0x107:d
; CHECK: lsc_load_block2d.ugm.uc.ca (M1, 1) %null:d64.8x2nn flat[[[DESCU]] + (0,0)]
  %desc.5 = insertelement <16 x i32> %desc.4, i32 263, i32 7 ; 1x8x2
  call void @llvm.vc.internal.lsc.prefetch.2d.ugm.desc.v2i8(i1 true, <2 x i8> <i8 1, i8 2>, i8 1, i16 8, i16 2, <16 x i32> %desc.5, i32 0, i32 0, i64 undef)

; CHECK: lsc_store_block2d.ugm.wt.wb (M1, 1) flat[[[DESCU]] + (0,0)] [[LOAD]]:d32.8x2nn
  call void @llvm.vc.internal.lsc.store.2d.ugm.desc.v2i8.v16i32(i1 true, <2 x i8> <i8 4, i8 3>, i8 1, i16 8, i16 2, <16 x i32> %desc.5, i32 0, i32 0, <16 x i32> %load)
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
