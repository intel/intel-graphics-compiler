;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-safe-opt -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

declare i16 @llvm.genx.GenISA.simdLaneId()
declare float @llvm.genx.GenISA.WaveShuffleIndex.f32(float, i32, i32)
declare float @llvm.genx.GenISA.QuadBroadcast.f32(float, i32)

; Test basic quad broadcast pattern for lane 0
; CHECK-LABEL: @test_quad_broadcast_lane0
define float @test_quad_broadcast_lane0(float %x) nounwind {
entry:
    %lane = call i16 @llvm.genx.GenISA.simdLaneId()
    %masked = and i16 %lane, -4            ; Mask to quad boundary (0xFFFC)
    %idx = zext i16 %masked to i32
    ; CHECK: call float @llvm.genx.GenISA.QuadBroadcast.f32(float %x, i32 0)
    %result = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %x, i32 %idx, i32 1)
    ret float %result
}

; Test basic quad broadcast pattern for lane 1
; CHECK-LABEL: @test_quad_broadcast_lane1
define float @test_quad_broadcast_lane1(float %x) nounwind {
entry:
    %lane = call i16 @llvm.genx.GenISA.simdLaneId()
    %lane1 = or i16 %lane, 1              ; Set bit for lane 1
    %masked = and i16 %lane1, -4          ; Mask to quad boundary
    %idx = zext i16 %masked to i32
    ; CHECK: call float @llvm.genx.GenISA.QuadBroadcast.f32(float %x, i32 1)
    %result = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %x, i32 %idx, i32 1)
    ret float %result
}

; Test basic quad broadcast pattern for lane 2
; CHECK-LABEL: @test_quad_broadcast_lane2
define float @test_quad_broadcast_lane2(float %x) nounwind {
entry:
    %lane = call i16 @llvm.genx.GenISA.simdLaneId()
    %lane2 = or i16 %lane, 2              ; Set bit for lane 2
    %masked = and i16 %lane2, -4          ; Mask to quad boundary
    %idx = zext i16 %masked to i32
    ; CHECK: call float @llvm.genx.GenISA.QuadBroadcast.f32(float %x, i32 2)
    %result = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %x, i32 %idx, i32 1)
    ret float %result
}

; Test basic quad broadcast pattern for lane 3
; CHECK-LABEL: @test_quad_broadcast_lane3
define float @test_quad_broadcast_lane3(float %x) nounwind {
entry:
    %lane = call i16 @llvm.genx.GenISA.simdLaneId()
    %lane3 = or i16 %lane, 3              ; Set bit for lane 3
    %masked = and i16 %lane3, -4          ; Mask to quad boundary
    %idx = zext i16 %masked to i32
    ; CHECK: call float @llvm.genx.GenISA.QuadBroadcast.f32(float %x, i32 3)
    %result = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %x, i32 %idx, i32 1)
    ret float %result
}

; Test that we don't transform when helper lanes = 0
; CHECK-LABEL: @test_no_transform_helper_lanes
define float @test_no_transform_helper_lanes(float %x) nounwind {
entry:
    %lane = call i16 @llvm.genx.GenISA.simdLaneId()
    %masked = and i16 %lane, -4
    %idx = zext i16 %masked to i32
    ; CHECK: call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %x, i32 %idx, i32 0)
    %result = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %x, i32 %idx, i32 0)
    ret float %result
}

; Test that we don't transform when using different AND mask
; CHECK-LABEL: @test_no_transform_different_mask
define float @test_no_transform_different_mask(float %x) nounwind {
entry:
    %lane = call i16 @llvm.genx.GenISA.simdLaneId()
    %masked = and i16 %lane, -8           ; Different mask, not -4 (0xFFFC)
    %idx = zext i16 %masked to i32
    ; CHECK: call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %x, i32 %idx, i32 1)
    %result = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %x, i32 %idx, i32 1)
    ret float %result
}

; Test that we don't transform when OR constant is too large
; CHECK-LABEL: @test_no_transform_large_lane
define float @test_no_transform_large_lane(float %x) nounwind {
entry:
    %lane = call i16 @llvm.genx.GenISA.simdLaneId()
    %lane4 = or i16 %lane, 4              ; Invalid quad lane (must be 0-3)
    %masked = and i16 %lane4, -4          ; Mask to quad boundary
    %idx = zext i16 %masked to i32
    ; CHECK: call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %x, i32 %idx, i32 1)
    %result = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %x, i32 %idx, i32 1)
    ret float %result
}

; Test that we don't transform when OR uses non-constant value
; CHECK-LABEL: @test_no_transform_variable_lane
define float @test_no_transform_variable_lane(float %x, i16 %lane_val) nounwind {
entry:
    %lane = call i16 @llvm.genx.GenISA.simdLaneId()
    %laneN = or i16 %lane, %lane_val      ; Variable lane index
    %masked = and i16 %laneN, -4          ; Mask to quad boundary
    %idx = zext i16 %masked to i32
    ; CHECK: call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %x, i32 %idx, i32 1)
    %result = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %x, i32 %idx, i32 1)
    ret float %result
}

; Test that we don't transform valid quad pattern when helper_lanes = 0
; CHECK-LABEL: @test_no_transform_valid_lane_wrong_helper
define float @test_no_transform_valid_lane_wrong_helper(float %x) nounwind {
entry:
    %lane = call i16 @llvm.genx.GenISA.simdLaneId()
    %lane1 = or i16 %lane, 1              ; Valid lane (1)
    %masked = and i16 %lane1, -4          ; Correct mask
    %idx = zext i16 %masked to i32
    ; CHECK: call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %x, i32 %idx, i32 0)
    %result = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %x, i32 %idx, i32 0)
    ret float %result
}