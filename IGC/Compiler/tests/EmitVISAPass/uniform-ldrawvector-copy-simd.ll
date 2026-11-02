;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; FIXME: make this test work without shader type
; REQUIRES: regkeys, shader-types
; RUN: igc_opt -platformbmg -igc-emit-visa %s -inputcs -simd-mode 16 -regkey DumpVISAASMToConsole=1 | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; A uniform ldrawvector.indexed is emitted as a SIMT1 transposed LSC load into a
; GRF-aligned temp, then copied to the dword-aligned destination variable.
; Check the temp-to-destination copy of 16 dwords (one 64B GRF) is a single
; SIMD16 mov rather than two SIMD8 movs.

target datalayout = "e-p:32:32:32-p1:64:64:64-p2:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-v64:32:32-v128:32:32-a0:0:32-n8:16:32-S32"

%dx.types.u32 = type { i32 }
%dx.types.i8x16 = type { [16 x i8] }

@ThreadGroupSize_X = constant i32 64
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 1

; CHECK: .decl [[DST:V[0-9]+]] v_type=G type=f num_elts=16 align=dword
; CHECK: .decl [[TMP:V[0-9]+]] v_type=G type=f num_elts=16 align=wordx32
; CHECK: lsc_load.ugm (M1_NM, 1) [[TMP]]:d32x16t
; CHECK-NEXT: mov (M1_NM, 16) [[DST]](0,0)<1> [[TMP]](0,0)<1;1,0>
; CHECK-NOT: mov (M1_NM, 8) [[DST]]

define void @main(<8 x i32> %r0, i8* %privateBase, i32 %0, i32 %1) #0 {
entry:
  %u0 = inttoptr i32 %0 to <4 x float> addrspace(2490369)*
  %cb = inttoptr i32 %1 to <4 x float> addrspace(2555904)*
  %2 = call <16 x float> @llvm.genx.GenISA.ldrawvector.indexed.v16f32.p2555904v4f32(<4 x float> addrspace(2555904)* %cb, i32 0, i32 -2147483648, i1 false)
  %3 = extractelement <16 x float> %2, i32 0
  %4 = extractelement <16 x float> %2, i32 4
  %5 = extractelement <16 x float> %2, i32 9
  %6 = extractelement <16 x float> %2, i32 15
  %7 = call i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32 17)
  %8 = zext i16 %7 to i32
  %9 = uitofp i32 %8 to float
  %10 = fmul fast float %9, %3
  %11 = fadd fast float %10, %4
  %12 = fmul fast float %11, %5
  %13 = fadd fast float %12, %6
  %14 = bitcast float %13 to i32
  %15 = shl i32 %8, 2
  call void @llvm.genx.GenISA.storeraw.indexed.p2490369v4f32.i32(<4 x float> addrspace(2490369)* %u0, i32 %15, i32 %14, i32 4, i1 false)
  ret void
}

declare <16 x float> @llvm.genx.GenISA.ldrawvector.indexed.v16f32.p2555904v4f32(<4 x float> addrspace(2555904)*, i32, i32, i1) #1
declare void @llvm.genx.GenISA.storeraw.indexed.p2490369v4f32.i32(<4 x float> addrspace(2490369)*, i32, i32, i32, i1) #2
declare i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32) #3

attributes #0 = { null_pointer_is_valid }
attributes #1 = { argmemonly nounwind readonly willreturn }
attributes #2 = { argmemonly nounwind writeonly }
attributes #3 = { nounwind readnone willreturn }

!dx.version = !{!0}
!dx.valver = !{!0}
!dx.shaderModel = !{!1}
!dx.resources = !{!2}
!dx.entryPoints = !{!7}
!igc.functions = !{!10}
!IGCMetadata = !{!13}

!0 = !{i32 1, i32 0}
!1 = !{!"cs", i32 6, i32 0}
!2 = !{null, !3, !5, null}
!3 = !{!4}
!4 = !{i32 0, %dx.types.u32 addrspace(1)* undef, !"U0", i32 0, i32 1, i32 1, i32 11, i1 false, i1 false, i1 false, null}
!5 = !{!6}
!6 = !{i32 0, %dx.types.i8x16 addrspace(2)* undef, !"CB0", i32 0, i32 0, i32 1, i32 64, null}
!7 = distinct !{null, !"main", null, !2, !8}
!8 = !{i32 0, i64 256, i32 4, !9}
!9 = !{i32 64, i32 1, i32 1}
!10 = !{void (<8 x i32>, i8*, i32, i32)* @main, !11}
!11 = !{!12}
!12 = !{!"function_type", i32 0}
!13 = !{!"ModuleMD", !14}
!14 = !{!"isPrecise", i1 false}
