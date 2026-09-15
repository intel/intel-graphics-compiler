;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; FIXME: make this test work without shader type
; REQUIRES: regkeys, shader-types
; RUN: igc_opt -platformbmg -igc-emit-visa %s -inputcs -simd-mode 32 -regkey DumpVISAASMToConsole=1 | FileCheck %s

; ------------------------------------------------
; EmitVISAPass - a WaveBroadcast whose only consumer shares its basic block is
; still folded into a <0;1,0> read of the source
; ------------------------------------------------
target datalayout = "e-p:32:32:32-p1:64:64:64-p2:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-v64:32:32-v128:32:32-a0:0:32-n8:16:32-S32"
target triple = "dxil-ms-dx"

@ThreadGroupSize_X = constant i32 32
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 1

; CHECK-NOT: mov (M1_NM, 1) bcast
; CHECK: add (M1, 32) idx(0,0)<1> val(0,0)<0;1,0> lane32(0,0)<1;1,0>
; CHECK-NOT: mov (M1_NM, 1) bcast

; Function Attrs: null_pointer_is_valid
define void @CSMain(i32 %runtime_value_0, i32 %runtime_value_1) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane32 = zext i16 %lane to i32

  ; Non uniform source; the xor itself produces no <0;1,0> region.
  %val = xor i32 %lane32, 7

  %bcast = call i32 @llvm.genx.GenISA.WaveBroadcast.i32(i32 %val, i32 0, i32 0)

  ; Sole consumer, in the same basic block as the intrinsic.
  %idx = add i32 %bcast, %lane32

  ; Sink. XOR, so the reduction emits no add that could match the CHECK above.
  call void @llvm.genx.GenISA.threadgroupbarrier()
  %waveAll = call i32 @llvm.genx.GenISA.WaveAll.i32.i8.i32(i32 %idx, i8 7, i1 true, i32 0)
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId() #1
declare i32 @llvm.genx.GenISA.WaveBroadcast.i32(i32, i32, i32) #2
declare i32 @llvm.genx.GenISA.WaveAll.i32.i8.i32(i32, i8, i1, i32) #3
declare void @llvm.genx.GenISA.threadgroupbarrier() #4

attributes #0 = { null_pointer_is_valid }
attributes #1 = { nounwind readnone }
attributes #2 = { convergent nounwind readnone }
attributes #3 = { convergent inaccessiblememonly nounwind }
attributes #4 = { convergent nounwind }

!igc.functions = !{!0}
!IGCMetadata = !{!3}

!0 = !{void (i32, i32)* @CSMain, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = !{!"FuncMDMap[0]", void (i32, i32)* @CSMain}
!6 = !{!"FuncMDValue[0]"}
