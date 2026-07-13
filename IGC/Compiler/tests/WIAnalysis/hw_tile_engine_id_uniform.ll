;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers -S -igc-propagate-cmp-uniformity -print-wia-check --disable-output --regkey=PrintToConsole=1 < %s 2>&1 | FileCheck %s

@ThreadGroupSize_X = constant i32 64
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 1

; ============================================================================
; Test: GenISA_hw_tile_id and GenISA_hw_engine_id are read from SR0 (per-HW-thread
; state register), so WIAnalysis must classify them as uniform_thread rather than
; random. A value derived only from them (add) must remain uniform_thread. Prior to
; the fix these intrinsics defaulted to random, forcing a per-lane indirect gather.
; ============================================================================
define spir_kernel void @test_hw_ids(ptr addrspace(1) %out) {
entry:
  %tile = call i32 @llvm.genx.GenISA.hw.tile.id()
  %engine = call i32 @llvm.genx.GenISA.hw.engine.id()
  %sum = add i32 %tile, %engine
  store i32 %sum, ptr addrspace(1) %out, align 4
  ret void
}

; CHECK-LABEL: define spir_kernel void @test_hw_ids
; CHECK: uniform_thread{{.*}}%tile = call i32 @llvm.genx.GenISA.hw.tile.id()
; CHECK: uniform_thread{{.*}}%engine = call i32 @llvm.genx.GenISA.hw.engine.id()
; CHECK: uniform_thread{{.*}}%sum = add i32 %tile, %engine

; ============================================================================
; Test: uniform_thread must not over-propagate. A tile id (uniform_thread) joined
; with a per-lane random value (LocalID_X, SystemValue 17) must yield random, so an
; address partly derived from the tile id is not wrongly scalarized.
; ============================================================================
define spir_kernel void @test_hw_id_join_random(ptr addrspace(1) %out) {
entry:
  %tile = call i32 @llvm.genx.GenISA.hw.tile.id()
  %lid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %addr = add i32 %tile, %lid
  store i32 %addr, ptr addrspace(1) %out, align 4
  ret void
}

; CHECK-LABEL: define spir_kernel void @test_hw_id_join_random
; CHECK: uniform_thread{{.*}}%tile = call i32 @llvm.genx.GenISA.hw.tile.id()
; CHECK: random{{.*}}%lid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
; CHECK: random{{.*}}%addr = add i32 %tile, %lid

declare i32 @llvm.genx.GenISA.hw.tile.id() #0
declare i32 @llvm.genx.GenISA.hw.engine.id() #0
declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32) #1

attributes #0 = { nounwind readnone willreturn }
attributes #1 = { nounwind readnone }

!IGCMetadata = !{!0}
!igc.functions = !{!1, !14}

!0 = !{!"ModuleMD", !2}
!1 = !{ptr @test_hw_ids, !3}
!2 = !{!"FuncMD", !4, !5, !15, !16}
!3 = !{!6}
!4 = !{!"FuncMDMap[0]", ptr @test_hw_ids}
!5 = !{!"FuncMDValue[0]", !7, !8, !9, !10}
!6 = !{!"function_type", i32 0}
!7 = !{!"localOffsets"}
!8 = !{!"workGroupWalkOrder", !11, !12, !13}
!9 = !{!"funcArgs"}
!10 = !{!"functionType", !"KernelFunction"}
!11 = !{!"dim0", i32 0}
!12 = !{!"dim1", i32 1}
!13 = !{!"dim2", i32 2}
!14 = !{ptr @test_hw_id_join_random, !3}
!15 = !{!"FuncMDMap[1]", ptr @test_hw_id_join_random}
!16 = !{!"FuncMDValue[1]", !7, !8, !9, !10}
