;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --regkey DisableCodeScheduling=0 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:         --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 --igc-code-scheduling \
; RUN:         --regkey CodeSchedulingRPThreshold=-512 \
; RUN:         --regkey ForceOCLSIMDWidth=16 -S %s 2>&1 | FileCheck %s


; Checks that the register pressure is estimated correctly for the special cases related to
; addrspace cast and pointer/int casts

define void @test_lsc2dblockread(i64 %base_addr, i64 %offset, i32 %shift_val) {
; CHECK: Function test_lsc2dblockread
; CHECK: Original schedule: entry

entry:
; Calculate address components
; The first instruction adds a new value (the argument doesn't die)

; CHECK: {{\([0-9]+,[ ]*64[ ]*\) OG:[ A-Z]*}} [[ADDR_SHIFT:%.*]] = shl i32 [[SHIFT_VAL:%.*]], 6
  %addr_shift = shl i32 %shift_val, 6

; The second instruction should also estimate the register pressure increase
; Because we extend to a larger data type

; CHECK: {{\([0-9]+,[ ]*64[ ]*\) OG:[ A-Z]*}} [[ADDR_SHIFT_EXT:%.*]] = zext i32 [[ADDR_SHIFT]] to i64
  %addr_shift_ext = zext i32 %addr_shift to i64
; CHECK: {{\([0-9]+,[ ]*0[ ]*\) OG:[ A-Z]*}} [[BASE_PLUS_SHIFT:%.*]] = add i64 [[BASE_ADDR:%.*]], [[ADDR_SHIFT_EXT]]
  %base_plus_shift = add i64 %base_addr, %addr_shift_ext

; No changes as we continue calculating the address and perform various casts

; CHECK: {{\([0-9]+,[ ]*0[ ]*\) OG:[ A-Z]*}} [[FINAL_ADDR:%.*]] = add i64 [[BASE_PLUS_SHIFT]], [[OFFSET:%.*]]
  %final_addr = add i64 %base_plus_shift, %offset

; Convert to pointer and back through address spaces
; CHECK: {{\([0-9]+,[ ]*0[ ]*\) OG:[ A-Z]*}} [[PTR_AS1:%.*]] = inttoptr i64 [[FINAL_ADDR]] to ptr addrspace(1)
  %ptr_as1 = inttoptr i64 %final_addr to i8 addrspace(1)*
; CHECK: {{\([0-9]+,[ ]*0[ ]*\) OG:[ A-Z]*}} [[PTR_AS4:%.*]] = addrspacecast ptr addrspace(1) [[PTR_AS1]] to ptr addrspace(4)
  %ptr_as4 = addrspacecast i8 addrspace(1)* %ptr_as1 to i8 addrspace(4)*
; CHECK: {{\([0-9]+,[ ]*0[ ]*\) OG:[ A-Z]*}} [[ADDR_AS4:%.*]] = ptrtoint ptr addrspace(4) [[PTR_AS4]] to i64
  %addr_as4 = ptrtoint i8 addrspace(4)* %ptr_as4 to i64

; Convert to pointer pair
; CHECK: {{\([0-9]+,[ ]*0[ ]*\) OG:[ A-Z]*}} [[PTR_PAIR:%.*]] = call { i32, i32 } @llvm.genx.GenISA.ptr.to.pair.p4i8(ptr addrspace(4) [[PTR_AS4]])
  %ptr_pair = call { i32, i32 } @llvm.genx.GenISA.ptr.to.pair.p4i8(i8 addrspace(4)* %ptr_as4)

; Currently it's estimated as +64 -64: essentially correct, but
; may be not perfect for the instruction choosing heuristics
; CHECK: {{\([0-9]+,[ ]*64[ ]*\) OG:[ A-Z]*}} [[PAIR_LOW:%.*]] = extractvalue { i32, i32 } [[PTR_PAIR]], 0
  %pair_low = extractvalue { i32, i32 } %ptr_pair, 0
; CHECK: {{\([0-9]+,[ ]*-64[ ]*\) OG:[ A-Z]*}} [[PAIR_HIGH:%.*]] = extractvalue { i32, i32 } [[PTR_PAIR]], 1
  %pair_high = extractvalue { i32, i32 } %ptr_pair, 1

; Prepare the first parameter (base address as i64)
; CHECK: {{\([0-9]+,[ ]*0[ ]*\) OG:[ A-Z]*}} [[ALIGNED_LOW:%.*]] = and i32 [[PAIR_LOW]], -64
  %aligned_low = and i32 %pair_low, -64

; Estimated as +128 -128
; CHECK: {{\([0-9]+,[ ]*128[ ]*\) OG:[ A-Z]*}} [[VEC_LOW:%.*]] = insertelement <2 x i32> undef, i32 [[ALIGNED_LOW]], i32 0
  %vec_low = insertelement <2 x i32> undef, i32 %aligned_low, i32 0
; CHECK: {{\([0-9]+,[ ]*-128[ ]*\) OG:[ A-Z]*}} [[VEC_PAIR:%.*]] = insertelement <2 x i32> [[VEC_LOW]], i32 [[PAIR_HIGH]], i32 1
  %vec_pair = insertelement <2 x i32> %vec_low, i32 %pair_high, i32 1

; Bitcast is a no-op
; CHECK: {{\([0-9]+,[ ]*0[ ]*\) OG:[ A-Z]*}} [[BASE_ADDR_PARAM:%.*]] = bitcast <2 x i32> [[VEC_PAIR]] to i64
  %base_addr_param = bitcast <2 x i32> %vec_pair to i64

; Prepare the coordinate parameter
; Truncation decreases the register pressure as the return type becomes smaller
; CHECK: {{\([0-9]+,[ ]*-64[ ]*\) OG:[ A-Z]*}} [[ADDR_TRUNC:%.*]] = trunc i64 [[ADDR_AS4]] to i32
  %addr_trunc = trunc i64 %addr_as4 to i32
; CHECK: {{\([0-9]+,[ ]*0[ ]*\) OG:[ A-Z]*}} [[COORD_SHIFT:%.*]] = lshr i32 [[ADDR_TRUNC]], 1
  %coord_shift = lshr i32 %addr_trunc, 1
; CHECK: {{\([0-9]+,[ ]*0[ ]*\) OG:[ A-Z]*}} [[COORD_MASKED:%.*]] = and i32 [[COORD_SHIFT]], 31
  %coord_masked = and i32 %coord_shift, 31

; Execute the LSC2DBlockRead: 256 (i16 * v8 * SIMD16) - 128 (i64 x SIMD16) - 64 (i32 * SIMD16)
; CHECK: {{\([0-9]+,[ ]*64[ ]*\) OG:[ A-Z]*}} [[LOAD_RESULT:%.*]] = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 [[BASE_ADDR_PARAM]], i32 4095, i32 7, i32 4095, i32 [[COORD_MASKED]], i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  %load_result = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base_addr_param, i32 4095, i32 7, i32 4095, i32 %coord_masked, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)

; Execute the DPAS
; Returns the 2x larger type as the load, load dies
; CHECK: {{\([0-9]+,[ ]*256[ ]*\) OG:[ A-Z]*}} [[DPAS_RESULT:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> [[LOAD_RESULT]], <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  %dpas_result = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %load_result, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
; CHECK: {{\([0-9]+,[ ]*0[ ]*\) OG:[ A-Z]*}} ret void
  ret void
}


declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
  <8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #1

declare <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32) #1
declare { i32, i32 } @llvm.genx.GenISA.ptr.to.pair.p4i8(i8 addrspace(4)*) #3

attributes #0 = { convergent nounwind }
attributes #1 = { convergent nounwind readnone willreturn }
attributes #2 = { nofree nosync nounwind readnone speculatable willreturn }
attributes #3 = { nounwind readnone willreturn }
