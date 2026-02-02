;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; platformNvl
; RUN: igc_opt -platformbmg -igc-emit-visa %s -inputcs -simd-mode 32 -regkey DumpVISAASMToConsole=1,DebugSoftwareNeedsA0Reset=1 | FileCheck %s --check-prefix=CHECK-BMG
; RUN: igc_opt -platformNvl -igc-emit-visa %s -inputcs -simd-mode 32 -regkey DumpVISAASMToConsole=1,DebugSoftwareNeedsA0Reset=1 | FileCheck %s --check-prefix=CHECK-NVL

; ------------------------------------------------
; EmitVISAPass - Testing addr_add instruction generation
; ------------------------------------------------
target datalayout = "e-p:32:32:32-p1:64:64:64-p2:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-v64:32:32-v128:32:32-a0:0:32-n8:16:32-S32"
target triple = "dxil-ms-dx"

@ThreadGroupSize_X = constant i32 1
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 32

; CHECK-BMG: mov {{.*}} A0
; CHECK-BMG: addr_add {{.*}} A0
; CHECK-BMG: mov {{.*}} A1
; CHECK-BMG: addr_add {{.*}} A1
; CHECK-BMG-NOT: mov {{.*}} A2
; CHECK-BMG: addr_add {{.*}} A2

; CHECK-NVL: mov {{.*}} A0
; CHECK-NVL: addr_add {{.*}} A0
; CHECK-NVL-NOT: mov {{.*}} A1
; CHECK-NVL: addr_add {{.*}} A1
; CHECK-NVL-NOT: mov {{.*}} A2
; CHECK-NVL: addr_add {{.*}} A2

; Function Attrs: null_pointer_is_valid
define void @CSMain(i32 %runtime_value_0, i32 %runtime_value_1, i32 %runtime_value_2) #0 {
  %threadGroupIdZ = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 2) ; THREAD_GROUP_ID_Z

  %src = inttoptr i32 %runtime_value_0 to <4 x float> addrspace(2490368)*
  %dst = inttoptr i32 %runtime_value_2 to <4 x float> addrspace(2490369)*
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane32 = zext i16 %lane to i32
  %shl_lane32 = shl i32 %lane32, 2

  %dynamic_buffer_idx = and i32 %threadGroupIdZ, 15  ; Limit to valid range
  %buffer_idx_ptr = inttoptr i32 %dynamic_buffer_idx to i8 addrspace(2490368)*

  %1 = add i32 %runtime_value_1, %threadGroupIdZ
  %2 = and i32 %lane32, 15
  %3 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %1, i32 %2, i32 0)
  %4 = trunc i32 %3 to i16
  %5 = lshr i32 %3, 16
  %6 = trunc i32 %5 to i16
  %7 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %2, i32 %1, i32 0)
  %8 = trunc i32 %7 to i16
  %9 = lshr i32 %7, 16
  %10 = trunc i32 %9 to i16

  %shuffle_0 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %1, i32 0, i32 0)
  %add_0 = add i32 %shuffle_0, %threadGroupIdZ  ; Use unaligned address
  %shuffle_1 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %runtime_value_1, i32 %2, i32 0)
  %add_1 = add i32 %shuffle_1, %add_0  ; Use unaligned address

  %indirect_ptr = getelementptr i8, i8 addrspace(2490368)* %buffer_idx_ptr, i32 %add_1
  %indirect_ptr_f32 = bitcast i8 addrspace(2490368)* %indirect_ptr to <4 x float> addrspace(2490368)*
  %a = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2490368v4f32(<4 x float> addrspace(2490368)* %indirect_ptr_f32, i32 %add_0, i32 4, i1 false)

  %offset_low = zext i16 %10 to i32
  %offset_high = zext i16 %6 to i32
  %combined_offset = or i32 %offset_low, %offset_high
  %unaligned_final = add i32 %combined_offset, %threadGroupIdZ
  %final_add = add i32 %unaligned_final, %a

  %waveAllSrc0 = insertelement <4 x i32> undef, i32 %final_add, i64 0
  %waveAllSrc1 = insertelement <4 x i32> %waveAllSrc0, i32 %a, i64 1
  %waveAllSrc2 = insertelement <4 x i32> %waveAllSrc1, i32 %a, i64 2
  %waveAllSrc3 = insertelement <4 x i32> %waveAllSrc2, i32 %final_add, i64 3
  %waveAllJoint = call <4 x i32> @llvm.genx.GenISA.WaveAll.v4i32.i8.i32(<4 x i32> %waveAllSrc3, i8 0, i1 true, i32 0)

  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId() #1
declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32) #1
declare i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2490368v4f32(<4 x float> addrspace(2490368)*, i32, i32, i1) #2
declare <4 x i32> @llvm.genx.GenISA.WaveAll.v4i32.i8.i32(<4 x i32>, i8, i1, i32) #3
declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32) #4
declare void @llvm.genx.GenISA.storerawvector.indexed.p2490377v4f32.v1i32(<4 x float> addrspace(2490369)*, i32, <1 x i32>, i32, i1) #5

attributes #0 = { null_pointer_is_valid }
attributes #1 = { nounwind readnone }
attributes #2 = { argmemonly nounwind readonly }
attributes #3 = { convergent inaccessiblememonly nounwind }
attributes #4 = { convergent nounwind readnone }
attributes #5 = { argmemonly nounwind writeonly }

!igc.functions = !{!0}
!IGCMetadata = !{!3}

!0 = !{void (i32, i32, i32)* @CSMain, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = !{!"FuncMDMap[0]", void (i32, i32, i32)* @CSMain}
!6 = !{!"FuncMDValue[0]"}
