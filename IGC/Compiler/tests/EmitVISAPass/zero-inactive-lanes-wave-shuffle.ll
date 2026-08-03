;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; FIXME: make this test work without shader type
; REQUIRES: regkeys, shader-types
; RUN: igc_opt -platformbmg -igc-emit-visa %s -inputcs -simd-mode 32 -regkey DumpVISAASMToConsole=1,ZeroInactiveLanesForWaveShuffle=1 | FileCheck %s --check-prefix=CHECK-WA
; RUN: igc_opt -platformbmg -igc-emit-visa %s -inputcs -simd-mode 32 -regkey DumpVISAASMToConsole=1 | FileCheck %s --check-prefix=CHECK-OFF

; ------------------------------------------------
; EmitVISAPass - ZeroInactiveLanesForWaveShuffle
;
; A shader may WaveShuffleIndex from a lane that divergent control flow has
; deactivated. HLSL leaves that read undefined, and because the shuffle source is
; produced under the execution mask its inactive lanes still hold stale register
; contents. With the workaround enabled the source is materialized into a
; temporary that is zero filled NoMask first and then overwritten under the mask,
; so an inactive lane reads a deterministic zero and the indirect access is taken
; on that temporary instead of the raw source.
; ------------------------------------------------
target datalayout = "e-p:32:32:32-p1:64:64:64-p2:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-v64:32:32-v128:32:32-a0:0:32-n8:16:32-S32"
target triple = "dxil-ms-dx"

@ThreadGroupSize_X = constant i32 32
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 1

; With the workaround on, the shuffle source is zero filled NoMask, then copied
; under the execution mask, and the address is taken on the bounded temporary.
; CHECK-WA: mov (M1_NM, 32) ShuffleSrcZeroInactive
; CHECK-WA-SAME: 0x0:
; CHECK-WA: mov (M1, 32) ShuffleSrcZeroInactive
; CHECK-WA: addr_add
; CHECK-WA-SAME: &ShuffleSrcZeroInactive

; With the workaround off nothing is materialized and the raw source is used.
; CHECK-OFF-NOT: ShuffleSrcZeroInactive

; Function Attrs: null_pointer_is_valid
define void @CSMain(i32 %runtime_value_0, i32 %runtime_value_1) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane32 = zext i16 %lane to i32

  ; Lane varying condition, so the shuffle below runs with a partial exec mask.
  %cond = icmp ult i32 %lane32, %runtime_value_0
  br i1 %cond, label %reduce, label %exit

reduce:
  ; Non uniform shuffle source, produced inside the divergent region. It must be
  ; lane varying, otherwise the source is uniform and the workaround does not apply.
  %src = uitofp i32 %lane32 to float
  %scaled = fmul float %src, 2.000000e+00

  ; Butterfly style partner index. At SIMD32 the partner may be a lane that did
  ; not take this branch.
  %partner = xor i32 %lane32, 16
  %shuffled = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %scaled, i32 %partner, i32 0)
  %sum = fadd float %scaled, %shuffled
  br label %exit

exit:
  %result = phi float [ %sum, %reduce ], [ 0.000000e+00, %entry ]
  %resulti = bitcast float %result to i32
  call void @llvm.genx.GenISA.threadgroupbarrier()
  %waveAll = call i32 @llvm.genx.GenISA.WaveAll.i32.i8.i32(i32 %resulti, i8 0, i1 true, i32 0)
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId() #1
declare float @llvm.genx.GenISA.WaveShuffleIndex.f32(float, i32, i32) #2
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
