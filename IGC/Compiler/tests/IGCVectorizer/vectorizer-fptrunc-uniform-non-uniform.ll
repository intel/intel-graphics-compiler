;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --opaque-pointers -platformbmg -rev-id B -has-emulated-64-bit-insts --igc-vectorizer -dce -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 < %s 2>&1 | FileCheck %s

; CHECK-LABEL: .kernel "fptrunc_uniform_and_non_uniform"
; CHECK-DAG: .decl vectorized_cast v_type=G type=hf num_elts=8 align=word
; CHECK-DAG: .decl [[NU:vectorized_cast[0-9]+]] v_type=G type=hf num_elts=128 align=wordx32

; CHECK:      mov (M1_NM, 1) vectorized_cast(0,0)<1> vector(0,0)<0;1,0>
; CHECK-NEXT: mov (M1_NM, 1) vectorized_cast(0,1)<1> vector(0,1)<0;1,0>
; CHECK-NEXT: mov (M1_NM, 1) vectorized_cast(0,2)<1> vector(0,2)<0;1,0>
; CHECK-NEXT: mov (M1_NM, 1) vectorized_cast(0,3)<1> vector(0,3)<0;1,0>
; CHECK-NEXT: mov (M1_NM, 1) vectorized_cast(0,4)<1> vector(0,4)<0;1,0>
; CHECK-NEXT: mov (M1_NM, 1) vectorized_cast(0,5)<1> vector(0,5)<0;1,0>
; CHECK-NEXT: mov (M1_NM, 1) vectorized_cast(0,6)<1> vector(0,6)<0;1,0>
; CHECK-NEXT: mov (M1_NM, 1) vectorized_cast(0,7)<1> vector(0,7)<0;1,0>
; CHECK:      mov (M1_NM, 1) ubc_0v(0,7)<1> vectorized_cast(0,7)<0;1,0>

; CHECK:      mov (M1, 16) [[NU]](0,0)<1> [[SRC:vector[0-9]+]](0,0)<1;1,0>
; CHECK-NEXT: mov (M1, 16) [[NU]](0,16)<1> [[SRC]](1,0)<1;1,0>
; CHECK-NEXT: mov (M1, 16) [[NU]](1,0)<1> [[SRC]](2,0)<1;1,0>
; CHECK-NEXT: mov (M1, 16) [[NU]](1,16)<1> [[SRC]](3,0)<1;1,0>
; CHECK-NEXT: mov (M1, 16) [[NU]](2,0)<1> [[SRC]](4,0)<1;1,0>
; CHECK-NEXT: mov (M1, 16) [[NU]](2,16)<1> [[SRC]](5,0)<1;1,0>
; CHECK-NEXT: mov (M1, 16) [[NU]](3,0)<1> [[SRC]](6,0)<1;1,0>
; CHECK-NEXT: mov (M1, 16) [[NU]](3,16)<1> [[SRC]](7,0)<1;1,0>

define spir_kernel void @fptrunc_uniform_and_non_uniform(ptr addrspace(1) %p) #0 {
entry:
  %lid = call i16 @llvm.genx.GenISA.simdLaneId()
  %lid32 = zext i16 %lid to i32
  %lane = shl i32 %lid32, 3
  %up0 = getelementptr float, ptr addrspace(1) %p, i32 0
  %u0 = load float, ptr addrspace(1) %up0
  %ut0 = fptrunc float %u0 to half
  %up1 = getelementptr float, ptr addrspace(1) %p, i32 1
  %u1 = load float, ptr addrspace(1) %up1
  %ut1 = fptrunc float %u1 to half
  %up2 = getelementptr float, ptr addrspace(1) %p, i32 2
  %u2 = load float, ptr addrspace(1) %up2
  %ut2 = fptrunc float %u2 to half
  %up3 = getelementptr float, ptr addrspace(1) %p, i32 3
  %u3 = load float, ptr addrspace(1) %up3
  %ut3 = fptrunc float %u3 to half
  %up4 = getelementptr float, ptr addrspace(1) %p, i32 4
  %u4 = load float, ptr addrspace(1) %up4
  %ut4 = fptrunc float %u4 to half
  %up5 = getelementptr float, ptr addrspace(1) %p, i32 5
  %u5 = load float, ptr addrspace(1) %up5
  %ut5 = fptrunc float %u5 to half
  %up6 = getelementptr float, ptr addrspace(1) %p, i32 6
  %u6 = load float, ptr addrspace(1) %up6
  %ut6 = fptrunc float %u6 to half
  %up7 = getelementptr float, ptr addrspace(1) %p, i32 7
  %u7 = load float, ptr addrspace(1) %up7
  %ut7 = fptrunc float %u7 to half
  %uv0 = insertelement <8 x half> undef, half %ut0, i64 0
  %uv1 = insertelement <8 x half> %uv0, half %ut1, i64 1
  %uv2 = insertelement <8 x half> %uv1, half %ut2, i64 2
  %uv3 = insertelement <8 x half> %uv2, half %ut3, i64 3
  %uv4 = insertelement <8 x half> %uv3, half %ut4, i64 4
  %uv5 = insertelement <8 x half> %uv4, half %ut5, i64 5
  %uv6 = insertelement <8 x half> %uv5, half %ut6, i64 6
  %uv7 = insertelement <8 x half> %uv6, half %ut7, i64 7
  %ubc = bitcast <8 x half> %uv7 to <4 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v4i32(i64 0, i32 -1, i32 -1, i32 -1, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <4 x i32> %ubc)
  %ni0 = add i32 %lane, 0
  %np0 = getelementptr float, ptr addrspace(1) %p, i32 %ni0
  %n0 = load float, ptr addrspace(1) %np0
  %nt0 = fptrunc float %n0 to half
  %ni1 = add i32 %lane, 1
  %np1 = getelementptr float, ptr addrspace(1) %p, i32 %ni1
  %n1 = load float, ptr addrspace(1) %np1
  %nt1 = fptrunc float %n1 to half
  %ni2 = add i32 %lane, 2
  %np2 = getelementptr float, ptr addrspace(1) %p, i32 %ni2
  %n2 = load float, ptr addrspace(1) %np2
  %nt2 = fptrunc float %n2 to half
  %ni3 = add i32 %lane, 3
  %np3 = getelementptr float, ptr addrspace(1) %p, i32 %ni3
  %n3 = load float, ptr addrspace(1) %np3
  %nt3 = fptrunc float %n3 to half
  %ni4 = add i32 %lane, 4
  %np4 = getelementptr float, ptr addrspace(1) %p, i32 %ni4
  %n4 = load float, ptr addrspace(1) %np4
  %nt4 = fptrunc float %n4 to half
  %ni5 = add i32 %lane, 5
  %np5 = getelementptr float, ptr addrspace(1) %p, i32 %ni5
  %n5 = load float, ptr addrspace(1) %np5
  %nt5 = fptrunc float %n5 to half
  %ni6 = add i32 %lane, 6
  %np6 = getelementptr float, ptr addrspace(1) %p, i32 %ni6
  %n6 = load float, ptr addrspace(1) %np6
  %nt6 = fptrunc float %n6 to half
  %ni7 = add i32 %lane, 7
  %np7 = getelementptr float, ptr addrspace(1) %p, i32 %ni7
  %n7 = load float, ptr addrspace(1) %np7
  %nt7 = fptrunc float %n7 to half
  %nv0 = insertelement <8 x half> undef, half %nt0, i64 0
  %nv1 = insertelement <8 x half> %nv0, half %nt1, i64 1
  %nv2 = insertelement <8 x half> %nv1, half %nt2, i64 2
  %nv3 = insertelement <8 x half> %nv2, half %nt3, i64 3
  %nv4 = insertelement <8 x half> %nv3, half %nt4, i64 4
  %nv5 = insertelement <8 x half> %nv4, half %nt5, i64 5
  %nv6 = insertelement <8 x half> %nv5, half %nt6, i64 6
  %nv7 = insertelement <8 x half> %nv6, half %nt7, i64 7
  %nbc = bitcast <8 x half> %nv7 to <4 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v4i32(i64 0, i32 -1, i32 -1, i32 -1, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <4 x i32> %nbc)
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId()
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v4i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <4 x i32>)

attributes #0 = { convergent }

!igc.functions = !{!0}

!0 = !{ptr @fptrunc_uniform_and_non_uniform, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

!3 = !{!"requiredSubGroupSize", i32 16}
!4 = !{!"FuncMDValue[0]", !3, !8}
!5 = !{!"FuncMDMap[0]", ptr @fptrunc_uniform_and_non_uniform}
!6 = !{!"FuncMD", !5, !4}
!7 = !{!"ModuleMD", !6}
!IGCMetadata = !{!7}
!8 = !{!"resAllocMD", !9}
!9 = !{!"argAllocMDList", !13}
!10 = !{!"type", i32 0}
!11 = !{!"extensionType", i32 -1}
!12 = !{!"indexType", i32 -1}
!13 = !{!"argAllocMDListVec[0]", !10, !11, !12}
