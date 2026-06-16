;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
;
; RUN: igc_opt -platformpvc -debugify -igc-subgroup-reduction-pattern -check-debugify -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-PVC
; RUN: igc_opt -platformmtl -debugify -igc-subgroup-reduction-pattern -check-debugify -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-MTL

; Test if a pattern of repeated ShuffleIndex + op is recognized and replaced with WaveClusteredInterleave.

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

define double @wave_clustered_interleave_8_4_2_simd32() {
entry:
; CHECK-LABEL: entry:
;
; CHECK-PVC:     %0 = call double @get_f64()
; CHECK-PVC:     [[RESULT:%.*]] = call double @llvm.genx.GenISA.WaveClusteredInterleave.f64(double %0, i8 9, i32 16, i32 2, i32 0)
; CHECK-PVC:     ret double [[RESULT]]
;
; CHECK-MTL-NOT: call double @llvm.genx.GenISA.WaveClusteredInterleave.f64
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call double @get_f64()
  %xor8 = xor i16 %simdLaneId, 8
  %zext8 = zext i16 %xor8 to i32
  %simdShuffle20 = call double @llvm.genx.GenISA.WaveShuffleIndex.f64(double %0, i32 %zext8, i32 0)
  %1 = fadd double %0, %simdShuffle20
  %xor4 = xor i16 %simdLaneId, 4
  %zext4 = zext i16 %xor4 to i32
  %simdShuffle23 = call double @llvm.genx.GenISA.WaveShuffleIndex.f64(double %1, i32 %zext4, i32 0)
  %2 = fadd double %1, %simdShuffle23
  %xor2 = xor i16 %simdLaneId, 2
  %zext2 = zext i16 %xor2 to i32
  %simdShuffle26 = call double @llvm.genx.GenISA.WaveShuffleIndex.f64(double %2, i32 %zext2, i32 0)
  %3 = fadd double %2, %simdShuffle26
  ret double %3
}

define double @wave_clustered_interleave_8_4_simd32() {
entry:
; CHECK-LABEL: entry:
; CHECK-NOT:     call double @llvm.genx.GenISA.WaveClusteredInterleave.f64
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call double @get_f64()
  %xor8 = xor i16 %simdLaneId, 8
  %zext8 = zext i16 %xor8 to i32
  %simdShuffle20 = call double @llvm.genx.GenISA.WaveShuffleIndex.f64(double %0, i32 %zext8, i32 0)
  %1 = fadd double %0, %simdShuffle20
  %xor4 = xor i16 %simdLaneId, 4
  %zext4 = zext i16 %xor4 to i32
  %simdShuffle23 = call double @llvm.genx.GenISA.WaveShuffleIndex.f64(double %1, i32 %zext4, i32 0)
  %2 = fadd double %1, %simdShuffle23
  ret double %2
}

define double @wave_clustered_interleave_8_simd32() {
entry:
; CHECK-LABEL: entry:
; CHECK-NOT:     call double @llvm.genx.GenISA.WaveClusteredInterleave.f64
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call double @get_f64()
  %xor8 = xor i16 %simdLaneId, 8
  %zext8 = zext i16 %xor8 to i32
  %simdShuffle20 = call double @llvm.genx.GenISA.WaveShuffleIndex.f64(double %0, i32 %zext8, i32 0)
  %1 = fadd double %0, %simdShuffle20
  ret double %1
}

define double @wave_clustered_interleave_4_2_simd32() {
entry:
; CHECK-LABEL: entry:
; CHECK-NOT:     call double @llvm.genx.GenISA.WaveClusteredInterleave.f64
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call double @get_f64()
  %xor4 = xor i16 %simdLaneId, 4
  %zext4 = zext i16 %xor4 to i32
  %simdShuffle23 = call double @llvm.genx.GenISA.WaveShuffleIndex.f64(double %0, i32 %zext4, i32 0)
  %1 = fadd double %0, %simdShuffle23
  %xor2 = xor i16 %simdLaneId, 2
  %zext2 = zext i16 %xor2 to i32
  %simdShuffle26 = call double @llvm.genx.GenISA.WaveShuffleIndex.f64(double %1, i32 %zext2, i32 0)
  %2 = fadd double %1, %simdShuffle26
  ret double %2
}

define double @wave_clustered_interleave_4_simd32() {
entry:
; CHECK-LABEL: entry:
; CHECK-NOT:     call double @llvm.genx.GenISA.WaveClusteredInterleave.f64
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call double @get_f64()
  %xor4 = xor i16 %simdLaneId, 4
  %zext4 = zext i16 %xor4 to i32
  %simdShuffle23 = call double @llvm.genx.GenISA.WaveShuffleIndex.f64(double %0, i32 %zext4, i32 0)
  %1 = fadd double %0, %simdShuffle23
  ret double %1
}

define double @wave_clustered_interleave_2_simd32() {
entry:
; CHECK-LABEL: entry:
; CHECK-NOT:     call double @llvm.genx.GenISA.WaveClusteredInterleave.f64
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call double @get_f64()
  %xor2 = xor i16 %simdLaneId, 2
  %zext2 = zext i16 %xor2 to i32
  %simdShuffle23 = call double @llvm.genx.GenISA.WaveShuffleIndex.f64(double %0, i32 %zext2, i32 0)
  %1 = fadd double %0, %simdShuffle23
  ret double %1
}

define double @wave_clustered_interleave_4_2_simd16() {
entry:
; CHECK-LABEL: entry:
; CHECK-NOT:     call double @llvm.genx.GenISA.WaveClusteredInterleave.f64
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call double @get_f64()
  %xor4 = xor i16 %simdLaneId, 4
  %zext4 = zext i16 %xor4 to i32
  %simdShuffle23 = call double @llvm.genx.GenISA.WaveShuffleIndex.f64(double %0, i32 %zext4, i32 0)
  %1 = fadd double %0, %simdShuffle23
  %xor2 = xor i16 %simdLaneId, 2
  %zext2 = zext i16 %xor2 to i32
  %simdShuffle26 = call double @llvm.genx.GenISA.WaveShuffleIndex.f64(double %1, i32 %zext2, i32 0)
  %2 = fadd double %1, %simdShuffle26
  ret double %2
}

define double @wave_clustered_interleave_4_simd16() {
entry:
; CHECK-LABEL: entry:
; CHECK-NOT:     call double @llvm.genx.GenISA.WaveClusteredInterleave.f64
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call double @get_f64()
  %xor4 = xor i16 %simdLaneId, 4
  %zext4 = zext i16 %xor4 to i32
  %simdShuffle23 = call double @llvm.genx.GenISA.WaveShuffleIndex.f64(double %0, i32 %zext4, i32 0)
  %1 = fadd double %0, %simdShuffle23
  ret double %1
}

define double @wave_clustered_interleave_2_simd16() {
entry:
; CHECK-LABEL: entry:
; CHECK-NOT:     call double @llvm.genx.GenISA.WaveClusteredInterleave.f64
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call double @get_f64()
  %xor2 = xor i16 %simdLaneId, 2
  %zext2 = zext i16 %xor2 to i32
  %simdShuffle23 = call double @llvm.genx.GenISA.WaveShuffleIndex.f64(double %0, i32 %zext2, i32 0)
  %1 = fadd double %0, %simdShuffle23
  ret double %1
}

define double @wave_clustered_interleave_2_simd8() {
entry:
; CHECK-LABEL: entry:
; CHECK-NOT:     call double @llvm.genx.GenISA.WaveClusteredInterleave.f64
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call double @get_f64()
  %xor2 = xor i16 %simdLaneId, 2
  %zext2 = zext i16 %xor2 to i32
  %simdShuffle23 = call double @llvm.genx.GenISA.WaveShuffleIndex.f64(double %0, i32 %zext2, i32 0)
  %1 = fadd double %0, %simdShuffle23
  ret double %1
}

declare double @llvm.genx.GenISA.WaveShuffleIndex.f64(double, i32, i32)
declare i16 @llvm.genx.GenISA.simdLaneId()

declare double @get_f64()

!igc.functions = !{!0, !1, !2, !3, !4, !5, !6, !7, !8, !9}

!0 = !{double ()* @wave_clustered_interleave_8_4_2_simd32, !300}
!1 = !{double ()* @wave_clustered_interleave_8_4_simd32, !300}
!2 = !{double ()* @wave_clustered_interleave_8_simd32, !300}
!3 = !{double ()* @wave_clustered_interleave_4_2_simd32, !300}
!4 = !{double ()* @wave_clustered_interleave_4_simd32, !300}
!5 = !{double ()* @wave_clustered_interleave_2_simd32, !300}
!6 = !{double ()* @wave_clustered_interleave_4_2_simd16, !200}
!7 = !{double ()* @wave_clustered_interleave_4_simd16, !200}
!8 = !{double ()* @wave_clustered_interleave_2_simd16, !200}
!9 = !{double ()* @wave_clustered_interleave_2_simd8, !100}
!100 = !{!101}
!101 = !{!"function_type", i32 0}
!200 = !{!201}
!201 = !{!"function_type", i32 0}
!300 = !{!301}
!301 = !{!"function_type", i32 0}
!303 = !{!"requiredSubGroupSize", i32 32}
!304 = !{!"requiredSubGroupSize", i32 32}
!305 = !{!"requiredSubGroupSize", i32 32}
!306 = !{!"requiredSubGroupSize", i32 32}
!307 = !{!"requiredSubGroupSize", i32 32}
!308 = !{!"requiredSubGroupSize", i32 32}
!309 = !{!"requiredSubGroupSize", i32 16}
!310 = !{!"requiredSubGroupSize", i32 16}
!311 = !{!"requiredSubGroupSize", i32 16}
!312 = !{!"requiredSubGroupSize", i32 8}
!313 = !{!"FuncMDMap[0]", double ()* @wave_clustered_interleave_8_4_2_simd32}
!314 = !{!"FuncMDValue[0]", !303}
!315 = !{!"FuncMDMap[1]", double ()* @wave_clustered_interleave_8_4_simd32}
!316 = !{!"FuncMDValue[1]", !304}
!317 = !{!"FuncMDMap[2]", double ()* @wave_clustered_interleave_8_simd32}
!318 = !{!"FuncMDValue[2]", !305}
!319 = !{!"FuncMDMap[3]", double ()* @wave_clustered_interleave_4_2_simd32}
!320 = !{!"FuncMDValue[3]", !306}
!321 = !{!"FuncMDMap[4]", double ()* @wave_clustered_interleave_4_simd32}
!322 = !{!"FuncMDValue[4]", !307}
!323 = !{!"FuncMDMap[5]", double ()* @wave_clustered_interleave_2_simd32}
!324 = !{!"FuncMDValue[5]", !308}
!325 = !{!"FuncMDMap[6]", double ()* @wave_clustered_interleave_4_2_simd16}
!326 = !{!"FuncMDValue[6]", !309}
!327 = !{!"FuncMDMap[7]", double ()* @wave_clustered_interleave_4_simd16}
!328 = !{!"FuncMDValue[7]", !310}
!329 = !{!"FuncMDMap[8]", double ()* @wave_clustered_interleave_2_simd16}
!330 = !{!"FuncMDValue[8]", !311}
!331 = !{!"FuncMDMap[9]", double ()* @wave_clustered_interleave_2_simd8}
!332 = !{!"FuncMDValue[9]", !312}
!333 = !{!"FuncMD", !313, !314, !315, !316, !317, !318, !319, !320, !321, !322, !323, !324, !325, !326, !327, !328, !329, !330, !331, !332}
!334 = !{!"ModuleMD", !333}
!IGCMetadata = !{!334}
