;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: llc %s -march=genx64 -mcpu=Xe2 -vc-skip-ocl-runtime-info -finalizer-opts='-dumpvisa -dumpcommonisa' -o /dev/null
; RUN: cat test_typed.visaasm | FileCheck %s --check-prefix=CHECK

target triple = "genx64-unknown-unknown"

declare <16 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v16i32.v16i1.v16i32(<16 x i1>, i8, i8, i8, i32, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>)
declare <32 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v32i32.v16i1.v16i32(<16 x i1>, i8, i8, i8, i32, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, <32 x i32>)
declare <48 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v48i32.v16i1.v16i32(<16 x i1>, i8, i8, i8, i32, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, <48 x i32>)
declare <64 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v64i32.v16i1.v16i32(<16 x i1>, i8, i8, i8, i32, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, <64 x i32>)

declare void @llvm.vc.internal.lsc.prefetch.quad.tgm.v16i1.v16i32(<16 x i1>, i8, i8, i8, i32, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>)

declare void @llvm.vc.internal.lsc.store.quad.tgm.v16i1.v16i32.v16i32(<16 x i1>, i8, i8, i8, i32, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>)
declare void @llvm.vc.internal.lsc.store.quad.tgm.v16i1.v16i32.v32i32(<16 x i1>, i8, i8, i8, i32, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, <32 x i32>)
declare void @llvm.vc.internal.lsc.store.quad.tgm.v16i1.v16i32.v48i32(<16 x i1>, i8, i8, i8, i32, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, <48 x i32>)
declare void @llvm.vc.internal.lsc.store.quad.tgm.v16i1.v16i32.v64i32(<16 x i1>, i8, i8, i8, i32, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, <64 x i32>)

; CHECK: .decl [[PRED:P[0-9]+]] v_type=P num_elts=16

define spir_kernel void @test_typed(<16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, i16 %pred) local_unnamed_addr #0 {
  %mask = bitcast i16 %pred to <16 x i1>
; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  [[VX:V[0-9]+]]:d32.x  bti(0x7B){{.}}[[U:V[0-9]+]],[[V:V[0-9]+]],[[R:V[0-9]+]],[[LOD:V[0-9]+]]{{.}}:a32
  %x = call <16 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v16i32.v16i1.v16i32(<16 x i1> %mask, i8 0, i8 0, i8 1, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <16 x i32> undef)
; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  V{{[0-9]+}}:d32.y  bti(0x7B){{.}}[[U]],[[V]],[[R]]{{.}}:a32
  %y = call <16 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v16i32.v16i1.v16i32(<16 x i1> %mask, i8 0, i8 0, i8 2, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> undef, <16 x i32> undef)
; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  V{{[0-9]+}}:d32.z  bti(0x7B){{.}}[[U]],[[V]]{{.}}:a32
  %z = call <16 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v16i32.v16i1.v16i32(<16 x i1> %mask, i8 0, i8 0, i8 4, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> undef, <16 x i32> undef, <16 x i32> undef)
; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  V{{[0-9]+}}:d32.w  bti(0x7B){{.}}[[U]]{{.}}:a32
  %w = call <16 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v16i32.v16i1.v16i32(<16 x i1> %mask, i8 0, i8 0, i8 8, i32 123, <16 x i32> %u, <16 x i32> undef, <16 x i32> undef, <16 x i32> undef, <16 x i32> undef)

; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  [[VXY:V[0-9]+]]:d32.xy  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  %xy = call <32 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v32i32.v16i1.v16i32(<16 x i1> %mask, i8 0, i8 0, i8 3, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <32 x i32> undef)
; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  V{{[0-9]+}}:d32.xz  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  %xz = call <32 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v32i32.v16i1.v16i32(<16 x i1> %mask, i8 0, i8 0, i8 5, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <32 x i32> undef)
; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  V{{[0-9]+}}:d32.xw  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  %xw = call <32 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v32i32.v16i1.v16i32(<16 x i1> %mask, i8 0, i8 0, i8 9, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <32 x i32> undef)
; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  V{{[0-9]+}}:d32.yz  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  %yz = call <32 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v32i32.v16i1.v16i32(<16 x i1> %mask, i8 0, i8 0, i8 6, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <32 x i32> undef)
; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  V{{[0-9]+}}:d32.yw  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  %yw = call <32 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v32i32.v16i1.v16i32(<16 x i1> %mask, i8 0, i8 0, i8 10, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <32 x i32> undef)
; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  V{{[0-9]+}}:d32.zw  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  %zw = call <32 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v32i32.v16i1.v16i32(<16 x i1> %mask, i8 0, i8 0, i8 12, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <32 x i32> undef)

; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  [[VXYZ:V[0-9]+]]:d32.xyz  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  %xyz = call <48 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v48i32.v16i1.v16i32(<16 x i1> %mask, i8 0, i8 0, i8 7, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <48 x i32> undef)
; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  V{{[0-9]+}}:d32.xyw  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  %xyw = call <48 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v48i32.v16i1.v16i32(<16 x i1> %mask, i8 0, i8 0, i8 11, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <48 x i32> undef)
; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  V{{[0-9]+}}:d32.xzw  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  %xzw = call <48 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v48i32.v16i1.v16i32(<16 x i1> %mask, i8 0, i8 0, i8 13, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <48 x i32> undef)
; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  V{{[0-9]+}}:d32.yzw  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  %yzw = call <48 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v48i32.v16i1.v16i32(<16 x i1> %mask, i8 0, i8 0, i8 14, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <48 x i32> undef)

; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  [[VXYZW:V[0-9]+]]:d32.xyzw  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  %xyzw = call <64 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v64i32.v16i1.v16i32(<16 x i1> %mask, i8 0, i8 0, i8 15, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <64 x i32> undef)

; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca (M1, 16)  %null:d32.x  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.v16i1.v16i32(<16 x i1> %mask, i8 2, i8 2, i8 1, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)
; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca (M1, 16)  %null:d32.y  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.v16i1.v16i32(<16 x i1> %mask, i8 2, i8 2, i8 2, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)
; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca (M1, 16)  %null:d32.z  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.v16i1.v16i32(<16 x i1> %mask, i8 2, i8 2, i8 4, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)
; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca (M1, 16)  %null:d32.w  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.v16i1.v16i32(<16 x i1> %mask, i8 2, i8 2, i8 8, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)

; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca (M1, 16)  %null:d32.xy  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.v16i1.v16i32(<16 x i1> %mask, i8 2, i8 2, i8 3, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)
; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca (M1, 16)  %null:d32.xz  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.v16i1.v16i32(<16 x i1> %mask, i8 2, i8 2, i8 5, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)
; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca (M1, 16)  %null:d32.xw  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.v16i1.v16i32(<16 x i1> %mask, i8 2, i8 2, i8 9, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)
; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca (M1, 16)  %null:d32.yz  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.v16i1.v16i32(<16 x i1> %mask, i8 2, i8 2, i8 6, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)
; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca (M1, 16)  %null:d32.yw  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.v16i1.v16i32(<16 x i1> %mask, i8 2, i8 2, i8 10, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)
; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca (M1, 16)  %null:d32.zw  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.v16i1.v16i32(<16 x i1> %mask, i8 2, i8 2, i8 12, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)

; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca (M1, 16)  %null:d32.xyz  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.v16i1.v16i32(<16 x i1> %mask, i8 2, i8 2, i8 7, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)
; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca (M1, 16)  %null:d32.xyw  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.v16i1.v16i32(<16 x i1> %mask, i8 2, i8 2, i8 11, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)
; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca (M1, 16)  %null:d32.xzw  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.v16i1.v16i32(<16 x i1> %mask, i8 2, i8 2, i8 13, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)
; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca (M1, 16)  %null:d32.yzw  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.v16i1.v16i32(<16 x i1> %mask, i8 2, i8 2, i8 14, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)

; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca (M1, 16)  %null:d32.xyzw  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.v16i1.v16i32(<16 x i1> %mask, i8 2, i8 2, i8 15, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)

; CHECK: ([[PRED]]) lsc_store_quad.tgm (M1, 16)  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32  [[VX]]:d32.x
  call void @llvm.vc.internal.lsc.store.quad.tgm.v16i1.v16i32.v16i32(<16 x i1> %mask, i8 0, i8 0, i8 1, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <16 x i32> %x)
; CHECK: ([[PRED]]) lsc_store_quad.tgm (M1, 16)  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32  [[VXY]]:d32.xy
  call void @llvm.vc.internal.lsc.store.quad.tgm.v16i1.v16i32.v32i32(<16 x i1> %mask, i8 0, i8 0, i8 3, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <32 x i32> %xy)
; CHECK: ([[PRED]]) lsc_store_quad.tgm (M1, 16)  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32  [[VXYZ]]:d32.xyz
  call void @llvm.vc.internal.lsc.store.quad.tgm.v16i1.v16i32.v48i32(<16 x i1> %mask, i8 0, i8 0, i8 7, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <48 x i32> %xyz)
; CHECK: ([[PRED]]) lsc_store_quad.tgm (M1, 16)  bti(0x7B){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32  [[VXYZW]]:d32.xyzw
  call void @llvm.vc.internal.lsc.store.quad.tgm.v16i1.v16i32.v64i32(<16 x i1> %mask, i8 0, i8 0, i8 15, i32 123, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <64 x i32> %xyzw)

  ret void
}


attributes #0 = { noinline nounwind "CMGenxMain" }

!spirv.Source = !{!1}
!opencl.spir.version = !{!2}
!opencl.ocl.version = !{!1}
!opencl.used.extensions = !{!0}
!opencl.used.optional.core.features = !{!0}
!spirv.Generator = !{!3}
!genx.kernels = !{!4}
!genx.kernel.internal = !{!8}

!0 = !{}
!1 = !{i32 0}
!2 = !{i32 1, i32 1}
!3 = !{i16 6, i16 14}
!4 = !{void (<16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, i16)* @test_typed, !"test_typed", !5, i32 0, !6, !1, !7, i32 0}
!5 = !{i32 0, i32 0, i32 0, i32 0, i32 0}
!6 = !{i32 64, i32 128, i32 192, i32 256, i32 320}
!7 = !{!"image2d_t"}
!8 = !{void (<16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, i16)* @test_typed, null, null, null, null}
