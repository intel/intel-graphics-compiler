;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: split-file %s %t
; RUN: igc_opt --opaque-pointers --regkey PrintToConsole --CheckInstrTypes --enable-instrtypes-print -S < %t/const.ll 2>&1 | FileCheck %s --check-prefix=CONST
; RUN: igc_opt --opaque-pointers --regkey PrintToConsole --CheckInstrTypes --enable-instrtypes-print -S < %t/var.ll 2>&1 | FileCheck %s --check-prefix=VAR
; RUN: igc_opt --opaque-pointers --regkey PrintToConsole --CheckInstrTypes --enable-instrtypes-print -S < %t/broadcast.ll 2>&1 | FileCheck %s --check-prefix=BROADCAST
; ------------------------------------------------
; CheckInstrTypes: WaveShuffleIndex/WaveBroadcast set mayHaveIndirectOperands
; only for a non-constant lane; a constant lane is emitted as a direct region.
; ------------------------------------------------

; CONST: mayHaveIndirectOperands: 0
; CONST: numWaveIntrinsics: 3

; VAR: mayHaveIndirectOperands: 1
; VAR: numWaveIntrinsics: 2

; BROADCAST: mayHaveIndirectOperands: 1
; BROADCAST: numWaveIntrinsics: 2

;--- const.ll
define spir_kernel void @const_lane(i32 %a, i8 %b, ptr %dst) {
  %s0 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 0, i32 0)
  %s1 = call i8 @llvm.genx.GenISA.WaveShuffleIndex.i8(i8 %b, i32 5, i32 0)
  %s2 = call i32 @llvm.genx.GenISA.WaveBroadcast.i32(i32 %a, i32 3, i32 0)
  %e1 = zext i8 %s1 to i32
  %t0 = add i32 %s0, %e1
  %t1 = add i32 %t0, %s2
  store i32 %t1, ptr %dst, align 4
  ret void
}

declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32)
declare i8 @llvm.genx.GenISA.WaveShuffleIndex.i8(i8, i32, i32)
declare i32 @llvm.genx.GenISA.WaveBroadcast.i32(i32, i32, i32)

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD"}
!1 = !{ptr @const_lane, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}

;--- var.ll
define spir_kernel void @var_lane(i32 %a, i32 %lane, ptr %dst) {
  %s0 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 0, i32 0)
  %s1 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 %lane, i32 0)
  %t0 = add i32 %s0, %s1
  store i32 %t0, ptr %dst, align 4
  ret void
}

declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32)

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD"}
!1 = !{ptr @var_lane, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}

;--- broadcast.ll
define spir_kernel void @broadcast_lane(i32 %value, i32 %lane, ptr %dst) {
  %dynamic = call i32 @llvm.genx.GenISA.WaveBroadcast.i32(i32 %value, i32 %lane, i32 0)
  %constant = call i32 @llvm.genx.GenISA.WaveBroadcast.i32(i32 %value, i32 3, i32 0)
  %sum = add i32 %dynamic, %constant
  store i32 %sum, ptr %dst, align 4
  ret void
}

declare i32 @llvm.genx.GenISA.WaveBroadcast.i32(i32, i32, i32)

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD"}
!1 = !{ptr @broadcast_lane, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
