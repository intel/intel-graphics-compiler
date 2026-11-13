;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt %s --opaque-pointers -platformbmg -igc-emit-visa -simd-mode 16 \
; RUN:         -regkey DumpVISAASMToConsole,AddVISADumpDeclarationsToEnd=1 \
; RUN: | FileCheck %s --check-prefix=CHECK-16
; RUN: igc_opt %s --opaque-pointers -platformbmg -igc-emit-visa -simd-mode 32 \
; RUN:         -regkey DumpVISAASMToConsole,AddVISADumpDeclarationsToEnd=1 \
; RUN: | FileCheck %s --check-prefix=CHECK-32

; SubgroupBitcastShuffle reinterprets the per-lane layout of its operand, where
; component c of lane l is at (c * SIMD + l). A uniform operand (constant,
; immediate or uniform kernel argument) is not aliased by DeSSA and has a single
; copy, so it must be broadcast to all lanes before the reinterpret. Otherwise
; every lane gets the plain bitcast of the operand.

; Constant vector: the constant is built in a uniform variable, component c is
; broadcast to the c-th SIMD-wide chunk, which is then read through an i32 alias
; (lane 0 = 0x03030303, not 0x36251403).
; CHECK-16-LABEL: .kernel "const_vec"
; CHECK-16:      mov (M1_NM, 1) [[C:[A-Za-z0-9_]+]](0,0)<1> 0x3:b
; CHECK-16-NEXT: mov (M1_NM, 1) [[C]](0,1)<1> 0x14:b
; CHECK-16-NEXT: mov (M1_NM, 1) [[C]](0,2)<1> 0x25:b
; CHECK-16-NEXT: mov (M1_NM, 1) [[C]](0,3)<1> 0x36:b
; CHECK-16-NEXT: mov (M1_NM, 16) [[B:[A-Za-z0-9_]+]](0,0)<1> [[C]](0,0)<0;1,0>
; CHECK-16-NEXT: mov (M1_NM, 16) [[B]](0,16)<1> [[C]](0,1)<0;1,0>
; CHECK-16-NEXT: mov (M1_NM, 16) [[B]](0,32)<1> [[C]](0,2)<0;1,0>
; CHECK-16-NEXT: mov (M1_NM, 16) [[B]](0,48)<1> [[C]](0,3)<0;1,0>
; CHECK-16-NEXT: mov (M1, 16) [[R:[A-Za-z0-9_]+]](0,0)<1> [[BD:[A-Za-z0-9_]+]](0,0)<1;1,0>
; CHECK-16:      lsc_store.ugm (M1, 16) {{.+}} [[R]]:d32
; CHECK-16-DAG:  // .decl [[B]] v_type=G type=b num_elts=64 align=wordx32
; CHECK-16-DAG:  // .decl [[BD]] v_type=G type=d num_elts=16 align=wordx32 alias=<[[B]], 0>
;
; CHECK-32-LABEL: .kernel "const_vec"
; CHECK-32:      mov (M1_NM, 1) [[C:[A-Za-z0-9_]+]](0,0)<1> 0x3:b
; CHECK-32-NEXT: mov (M1_NM, 1) [[C]](0,1)<1> 0x14:b
; CHECK-32-NEXT: mov (M1_NM, 1) [[C]](0,2)<1> 0x25:b
; CHECK-32-NEXT: mov (M1_NM, 1) [[C]](0,3)<1> 0x36:b
; CHECK-32-NEXT: mov (M1_NM, 32) [[B:[A-Za-z0-9_]+]](0,0)<1> [[C]](0,0)<0;1,0>
; CHECK-32-NEXT: mov (M1_NM, 32) [[B]](0,32)<1> [[C]](0,1)<0;1,0>
; CHECK-32-NEXT: mov (M1_NM, 32) [[B]](1,0)<1> [[C]](0,2)<0;1,0>
; CHECK-32-NEXT: mov (M1_NM, 32) [[B]](1,32)<1> [[C]](0,3)<0;1,0>
; CHECK-32-NEXT: mov (M1, 32) [[R:[A-Za-z0-9_]+]](0,0)<1> [[BD:[A-Za-z0-9_]+]](0,0)<1;1,0>
; CHECK-32:      lsc_store.ugm (M1, 32) {{.+}} [[R]]:d32
; CHECK-32-DAG:  // .decl [[B]] v_type=G type=b num_elts=128 align=wordx32
; CHECK-32-DAG:  // .decl [[BD]] v_type=G type=d num_elts=32 align=wordx32 alias=<[[B]], 0>
define spir_kernel void @const_vec(ptr addrspace(1) %out, <4 x i8> %x, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX) {
entry:
  %id = zext i16 %localIdX to i64
  %r = call i32 @llvm.genx.GenISA.SubgroupBitcastShuffle.i32.v4i8(<4 x i8> <i8 3, i8 20, i8 37, i8 54>)
  %addr = getelementptr i32, ptr addrspace(1) %out, i64 %id
  store i32 %r, ptr addrspace(1) %addr, align 4
  ret void
}

; Constant scalar 0x0807060504030201: an immediate (aliasing it used to assert).
; The i64 is broadcast as lo/hi dwords, which is the per-lane qword layout.
; CHECK-16-LABEL: .kernel "const_scalar"
; CHECK-16:      mov (M1_NM, 16) [[B:[A-Za-z0-9_]+]](0,0)<2> 0x4030201:ud
; CHECK-16-NEXT: mov (M1_NM, 16) [[B]](0,1)<2> 0x8070605:ud
; CHECK-16-NEXT: mov (M1, 16) [[R:[A-Za-z0-9_]+]](0,0)<1> [[BD:[A-Za-z0-9_]+]](0,0)<1;1,0>
; CHECK-16-NEXT: mov (M1, 16) [[R]](1,0)<1> [[BD]](1,0)<1;1,0>
; CHECK-16:      lsc_store.ugm (M1, 16) {{.+}} [[R]]:d32x2
;
; CHECK-32-LABEL: .kernel "const_scalar"
; CHECK-32:      mov (M1_NM, 32) [[B:[A-Za-z0-9_]+]](0,0)<2> 0x4030201:ud
; CHECK-32-NEXT: mov (M1_NM, 32) [[B]](0,1)<2> 0x8070605:ud
; CHECK-32-NEXT: mov (M1, 32) [[R:[A-Za-z0-9_]+]](0,0)<1> [[BD:[A-Za-z0-9_]+]](0,0)<1;1,0>
; CHECK-32-NEXT: mov (M1, 32) [[R]](2,0)<1> [[BD]](2,0)<1;1,0>
; CHECK-32:      lsc_store.ugm (M1, 32) {{.+}} [[R]]:d32x2
define spir_kernel void @const_scalar(ptr addrspace(1) %out, <4 x i8> %x, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX) {
entry:
  %id = zext i16 %localIdX to i64
  %r = call <2 x i32> @llvm.genx.GenISA.SubgroupBitcastShuffle.v2i32.i64(i64 578437695752307201)
  %addr = getelementptr <2 x i32>, ptr addrspace(1) %out, i64 %id
  store <2 x i32> %r, ptr addrspace(1) %addr, align 8
  ret void
}

; Uniform kernel argument: not a constant, but uniform as well.
; CHECK-16-LABEL: .kernel "uniform_arg"
; CHECK-16:      mov (M1_NM, 16) [[B:[A-Za-z0-9_]+]](0,0)<1> x(0,0)<0;1,0>
; CHECK-16-NEXT: mov (M1_NM, 16) [[B]](0,16)<1> x(0,1)<0;1,0>
; CHECK-16-NEXT: mov (M1_NM, 16) [[B]](0,32)<1> x(0,2)<0;1,0>
; CHECK-16-NEXT: mov (M1_NM, 16) [[B]](0,48)<1> x(0,3)<0;1,0>
; CHECK-16-NEXT: mov (M1, 16) [[R:[A-Za-z0-9_]+]](0,0)<1> [[BD:[A-Za-z0-9_]+]](0,0)<1;1,0>
; CHECK-16:      lsc_store.ugm (M1, 16) {{.+}} [[R]]:d32
; CHECK-16-DAG:  // .decl [[BD]] v_type=G type=d num_elts=16 align=wordx32 alias=<[[B]], 0>
;
; CHECK-32-LABEL: .kernel "uniform_arg"
; CHECK-32:      mov (M1_NM, 32) [[B:[A-Za-z0-9_]+]](0,0)<1> x(0,0)<0;1,0>
; CHECK-32-NEXT: mov (M1_NM, 32) [[B]](0,32)<1> x(0,1)<0;1,0>
; CHECK-32-NEXT: mov (M1_NM, 32) [[B]](1,0)<1> x(0,2)<0;1,0>
; CHECK-32-NEXT: mov (M1_NM, 32) [[B]](1,32)<1> x(0,3)<0;1,0>
; CHECK-32-NEXT: mov (M1, 32) [[R:[A-Za-z0-9_]+]](0,0)<1> [[BD:[A-Za-z0-9_]+]](0,0)<1;1,0>
; CHECK-32:      lsc_store.ugm (M1, 32) {{.+}} [[R]]:d32
; CHECK-32-DAG:  // .decl [[BD]] v_type=G type=d num_elts=32 align=wordx32 alias=<[[B]], 0>
define spir_kernel void @uniform_arg(ptr addrspace(1) %out, <4 x i8> %x, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX) {
entry:
  %id = zext i16 %localIdX to i64
  %r = call i32 @llvm.genx.GenISA.SubgroupBitcastShuffle.i32.v4i8(<4 x i8> %x)
  %addr = getelementptr i32, ptr addrspace(1) %out, i64 %id
  store i32 %r, ptr addrspace(1) %addr, align 4
  ret void
}

declare i32 @llvm.genx.GenISA.SubgroupBitcastShuffle.i32.v4i8(<4 x i8>)
declare <2 x i32> @llvm.genx.GenISA.SubgroupBitcastShuffle.v2i32.i64(i64)

!igc.functions = !{!0, !1, !2}
!0 = !{ptr @const_vec, !3}
!1 = !{ptr @const_scalar, !3}
!2 = !{ptr @uniform_arg, !3}
!3 = !{!{!"function_type", i32 0}}

!IGCMetadata = !{!10}
!10 = !{!"ModuleMD", !11}
!11 = !{!"FuncMD", !12, !13, !20, !21, !22, !23}
!12 = !{!"FuncMDMap[0]", ptr @const_vec}
!13 = !{!"FuncMDValue[0]", !14, !17}
!14 = !{!"resAllocMD", !15}
!15 = !{!"argAllocMDList", !16, !16, !16, !16, !16}
!16 = !{!"argAllocMDListVec[0]", !{!"type", i32 0}, !{!"extensionType", i32 -1}, !{!"indexType", i32 -1}}
!17 = !{!"implicitArgInfoList", !18, !19, !24}
!18 = !{!"implicitArgInfoListVec[0]", !{!"argId", i32 0}}
!19 = !{!"implicitArgInfoListVec[1]", !{!"argId", i32 1}}
!24 = !{!"implicitArgInfoListVec[2]", !{!"argId", i32 8}}
!20 = !{!"FuncMDMap[1]", ptr @const_scalar}
!21 = !{!"FuncMDValue[1]", !14, !17}
!22 = !{!"FuncMDMap[2]", ptr @uniform_arg}
!23 = !{!"FuncMDValue[2]", !14, !17}
