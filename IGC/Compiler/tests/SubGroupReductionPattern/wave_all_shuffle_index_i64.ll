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
; RUN: igc_opt -platformdg2 -debugify -igc-subgroup-reduction-pattern -check-debugify -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-DG2

; Test if a pattern of repeated ShuffleIndex + op is recognized and replaced with WaveAll.

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

define i64 @wave_all_add_i64() {
entry:
; CHECK-LABEL: entry:
;
; CHECK-PVC:     %0 = call i64 @get_i64()
; CHECK-PVC:     [[RESULT:%.*]] = call i64 @llvm.genx.GenISA.WaveAll.i64(i64 %0, i8 0, i1 true, i32 0)
; CHECK-PVC:     ret i64 [[RESULT]]
;
; CHECK-DG2-NOT: @llvm.genx.GenISA.WaveAll
; CHECK-DG2:     ret i64 %add9
  %0 = call i64 @get_i64()
  %simdLaneId16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %xor16 = xor i16 %simdLaneId16, 16
  %zext16 = zext i16 %xor16 to i32
  %1 = bitcast i64 %0 to <2 x i32>
  %.scalar = extractelement <2 x i32> %1, i64 0
  %.scalar51 = extractelement <2 x i32> %1, i64 1
  %simdShuffle = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %.scalar, i32 %zext16, i32 0)
  %simdShuffle18 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %.scalar51, i32 %zext16, i32 0)
  %.assembled.vect = insertelement <2 x i32> undef, i32 %simdShuffle, i64 0
  %.assembled.vect60 = insertelement <2 x i32> %.assembled.vect, i32 %simdShuffle18, i64 1
  %2 = bitcast <2 x i32> %.assembled.vect60 to i64
  %add = add nsw i64 %0, %2
  %3 = bitcast i64 %add to <2 x i32>
  %.scalar52 = extractelement <2 x i32> %3, i64 0
  %.scalar53 = extractelement <2 x i32> %3, i64 1
  %simdShuffleXor = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %.scalar52, i32 8)
  %simdShuffleXor69 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %.scalar53, i32 8)
  %.assembled.vect61 = insertelement <2 x i32> undef, i32 %simdShuffleXor, i64 0
  %.assembled.vect62 = insertelement <2 x i32> %.assembled.vect61, i32 %simdShuffleXor69, i64 1
  %4 = bitcast <2 x i32> %.assembled.vect62 to i64
  %add3 = add nsw i64 %add, %4
  %5 = bitcast i64 %add3 to <2 x i32>
  %.scalar54 = extractelement <2 x i32> %5, i64 0
  %.scalar55 = extractelement <2 x i32> %5, i64 1
  %simdShuffleXor70 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %.scalar54, i32 4)
  %simdShuffleXor71 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %.scalar55, i32 4)
  %.assembled.vect63 = insertelement <2 x i32> undef, i32 %simdShuffleXor70, i64 0
  %.assembled.vect64 = insertelement <2 x i32> %.assembled.vect63, i32 %simdShuffleXor71, i64 1
  %6 = bitcast <2 x i32> %.assembled.vect64 to i64
  %add5 = add nsw i64 %add3, %6
  %7 = bitcast i64 %add5 to <2 x i32>
  %.scalar56 = extractelement <2 x i32> %7, i64 0
  %.scalar57 = extractelement <2 x i32> %7, i64 1
  %simdShuffleXor72 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %.scalar56, i32 2)
  %simdShuffleXor73 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %.scalar57, i32 2)
  %.assembled.vect65 = insertelement <2 x i32> undef, i32 %simdShuffleXor72, i64 0
  %.assembled.vect66 = insertelement <2 x i32> %.assembled.vect65, i32 %simdShuffleXor73, i64 1
  %8 = bitcast <2 x i32> %.assembled.vect66 to i64
  %add7 = add nsw i64 %add5, %8
  %9 = bitcast i64 %add7 to <2 x i32>
  %.scalar58 = extractelement <2 x i32> %9, i64 0
  %.scalar59 = extractelement <2 x i32> %9, i64 1
  %simdShuffleXor74 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %.scalar58, i32 1)
  %simdShuffleXor75 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %.scalar59, i32 1)
  %.assembled.vect67 = insertelement <2 x i32> undef, i32 %simdShuffleXor74, i64 0
  %.assembled.vect68 = insertelement <2 x i32> %.assembled.vect67, i32 %simdShuffleXor75, i64 1
  %10 = bitcast <2 x i32> %.assembled.vect68 to i64
  %add9 = add nsw i64 %add7, %10
  ret i64 %add9
}

define i64 @mixed_extract_insert_indices() {
entry:
; CHECK-LABEL: entry:
; CHECK-NOT:     @llvm.genx.GenISA.WaveClustered
; CHECK:         ret i64 %add1
  %0 = call i64 @get_i64()
  %simdLaneId16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %xor1 = xor i16 %simdLaneId16, 1
  %zext1 = zext i16 %xor1 to i32
  %1 = bitcast i64 %0 to <2 x i32>
  %.scalar = extractelement <2 x i32> %1, i64 0
  %.scalar51 = extractelement <2 x i32> %1, i64 1
  %simdShuffle = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %.scalar, i32 %zext1, i32 0)
  %simdShuffle18 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %.scalar51, i32 %zext1, i32 0)

; COM: Elements inserted in reverse order == invalid pattern.
  %.assembled.vect = insertelement <2 x i32> undef, i32 %simdShuffle, i64 1
  %.assembled.vect60 = insertelement <2 x i32> %.assembled.vect, i32 %simdShuffle18, i64 0

  %2 = bitcast <2 x i32> %.assembled.vect60 to i64
  %add1 = add nsw i64 %0, %2
  ret i64 %add1
}

define i64 @mixed_shuffle_lanes() {
entry:
; CHECK-LABEL: entry:
; CHECK-NOT:     @llvm.genx.GenISA.WaveClustered
; CHECK:         ret i64 %add9
  %0 = call i64 @get_i64()
  %1 = bitcast i64 %0 to <2 x i32>
  %.scalar56 = extractelement <2 x i32> %1, i64 0
  %.scalar57 = extractelement <2 x i32> %1, i64 1

; COM: Different shuffle indices == invalid pattern.
  %simdShuffleXor72 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %.scalar56, i32 2)
  %simdShuffleXor73 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %.scalar57, i32 1)

  %.assembled.vect65 = insertelement <2 x i32> undef, i32 %simdShuffleXor72, i64 0
  %.assembled.vect66 = insertelement <2 x i32> %.assembled.vect65, i32 %simdShuffleXor73, i64 1
  %2 = bitcast <2 x i32> %.assembled.vect66 to i64
  %add7 = add nsw i64 %0, %2
  %3 = bitcast i64 %add7 to <2 x i32>
  %.scalar58 = extractelement <2 x i32> %3, i64 0
  %.scalar59 = extractelement <2 x i32> %3, i64 1

; COM: Different shuffle indices == invalid pattern.
  %simdShuffleXor74 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %.scalar58, i32 1)
  %simdShuffleXor75 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %.scalar59, i32 2)

  %.assembled.vect67 = insertelement <2 x i32> undef, i32 %simdShuffleXor74, i64 0
  %.assembled.vect68 = insertelement <2 x i32> %.assembled.vect67, i32 %simdShuffleXor75, i64 1
  %4 = bitcast <2 x i32> %.assembled.vect68 to i64
  %add9 = add nsw i64 %add7, %4
  ret i64 %add9
}

declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32)
declare i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32, i32)
declare i16 @llvm.genx.GenISA.simdLaneId()

declare i64 @get_i64()

!igc.functions = !{!0, !1, !2}

!0 = !{i64 ()* @wave_all_add_i64, !100}
!1 = !{i64 ()* @mixed_extract_insert_indices, !100}
!2 = !{i64 ()* @mixed_shuffle_lanes, !100}
!100 = !{!101, !102}
!101 = !{!"function_type", i32 0}
!102 = !{!"sub_group_size", i32 32}
