;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unkonwn-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck %s
; RUN: opt %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unkonwn-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck --check-prefix=CHECK-LSC %s

; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS
; CHECK-LSC-NOT: WARNING
; CHECK-LSC: CheckModuleDebugify: PASS

; COM: Basic test on atomic lowering pass
; COM: load/store atomic, atomicrmw, cmpxchg from addrspace(3), i32

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

; Address space 1 (global) operations are lowered into dword/slm intrinsics

define i32 @load_i32(i32 addrspace(3)* %ptr) {
  ; CHECK: [[LOAD_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK: [[LOAD_VADDR:%[^ ]+]] = bitcast i32 [[LOAD_ADDR]] to <1 x i32>
  ; CHECK: [[LOAD_VDATA:%[^ ]+]] = call <1 x i32> @llvm.genx.dword.atomic.or.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i32 254, <1 x i32> [[LOAD_VADDR]], <1 x i32> zeroinitializer, <1 x i32> undef)
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: %res = bitcast <1 x i32> [[LOAD_VDATA]] to i32
  ; CHECK-LSC: [[LOAD_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK-LSC: [[LOAD_VADDR:%[^ ]+]] = bitcast i32 [[LOAD_ADDR]] to <1 x i32>
  ; CHECK-LSC: [[LOAD_VDATA:%[^ ]+]] = call <1 x i32> @llvm.genx.lsc.xatomic.slm.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i8 10, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> [[LOAD_VADDR]], <1 x i32> undef, <1 x i32> undef, i32 0, <1 x i32> undef)
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 2)
  ; CHECK-LSC: %res = bitcast <1 x i32> [[LOAD_VDATA]] to i32
  %res = load atomic i32, i32 addrspace(3)* %ptr syncscope("device") acquire, align 4
  ret i32 %res
}

define void @store_i32(i32 addrspace(3)* %ptr, i32 %arg) {
  ; CHECK: [[STORE_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK: [[STORE_VDATA:%[^ ]+]] = bitcast i32 %arg to <1 x i32>
  ; CHECK: [[STORE_VADDR:%[^ ]+]] = bitcast i32 [[STORE_ADDR]] to <1 x i32>
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: call <1 x i32> @llvm.genx.dword.atomic.xchg.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i32 254, <1 x i32> [[STORE_VADDR]], <1 x i32> [[STORE_VDATA]], <1 x i32> undef)
  ; CHECK-LSC: [[STORE_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK-LSC: [[STORE_VADDR:%[^ ]+]] = bitcast i32 [[STORE_ADDR]] to <1 x i32>
  ; CHECK-LSC: [[STORE_VDATA:%[^ ]+]] = bitcast i32 %arg to <1 x i32>
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 2)
  ; CHECK-LSC: call <1 x i32> @llvm.genx.lsc.xatomic.slm.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i8 11, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> [[STORE_VADDR]], <1 x i32> [[STORE_VDATA]], <1 x i32> undef, i32 0, <1 x i32> undef)
  store atomic i32 %arg, i32 addrspace(3)* %ptr syncscope("device") release, align 4
  ret void
}

define i32 @xchg_i32(i32 addrspace(3)* %ptr, i32 %arg) {
  ; CHECK: [[XCHG_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK: [[XCHG_VDATA:%[^ ]+]] = bitcast i32 %arg to <1 x i32>
  ; CHECK: [[XCHG_VADDR:%[^ ]+]] = bitcast i32 [[XCHG_ADDR]] to <1 x i32>
  ; CHECK: [[XCHG_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.dword.atomic.xchg.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i32 254, <1 x i32> [[XCHG_VADDR]], <1 x i32> [[XCHG_VDATA]], <1 x i32> undef)
  ; CHECK: %res = bitcast <1 x i32> [[XCHG_VRES]] to i32
  ; CHECK-LSC: [[XCHG_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK-LSC: [[XCHG_VADDR:%[^ ]+]] = bitcast i32 [[XCHG_ADDR]] to <1 x i32>
  ; CHECK-LSC: [[XCHG_VDATA:%[^ ]+]] = bitcast i32 %arg to <1 x i32>
  ; CHECK-LSC: [[XCHG_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.lsc.xatomic.slm.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i8 11, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> [[XCHG_VADDR]], <1 x i32> [[XCHG_VDATA]], <1 x i32> undef, i32 0, <1 x i32> undef)
  ; CHECK-LSC: %res = bitcast <1 x i32> [[XCHG_VRES]] to i32
  %res = atomicrmw xchg i32 addrspace(3)* %ptr, i32 %arg syncscope("all_devices") monotonic
  ret i32 %res
}

define i32 @add_i32(i32 addrspace(3)* %ptr, i32 %arg) {
  ; CHECK: [[ADD_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK: [[ADD_VDATA:%[^ ]+]] = bitcast i32 %arg to <1 x i32>
  ; CHECK: [[ADD_VADDR:%[^ ]+]] = bitcast i32 [[ADD_ADDR]] to <1 x i32>
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: [[ADD_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.dword.atomic.add.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i32 254, <1 x i32> [[ADD_VADDR]], <1 x i32> [[ADD_VDATA]], <1 x i32> undef)
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: %res = bitcast <1 x i32> [[ADD_VRES]] to i32
  ; CHECK-LSC: [[ADD_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK-LSC: [[ADD_VADDR:%[^ ]+]] = bitcast i32 [[ADD_ADDR]] to <1 x i32>
  ; CHECK-LSC: [[ADD_VDATA:%[^ ]+]] = bitcast i32 %arg to <1 x i32>
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 0)
  ; CHECK-LSC: [[ADD_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.lsc.xatomic.slm.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i8 12, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> [[ADD_VADDR]], <1 x i32> [[ADD_VDATA]], <1 x i32> undef, i32 0, <1 x i32> undef)
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 0)
  ; CHECK-LSC: %res = bitcast <1 x i32> [[ADD_VRES]] to i32
  %res = atomicrmw add i32 addrspace(3)* %ptr, i32 %arg syncscope("subgroup") acq_rel
  ret i32 %res
}

define i32 @inc_i32(i32 addrspace(3)* %ptr) {
  ; CHECK: [[INC_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK: [[INC_VADDR:%[^ ]+]] = bitcast i32 [[INC_ADDR]] to <1 x i32>
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: [[INC_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.dword.atomic.inc.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i32 254, <1 x i32> [[INC_VADDR]], <1 x i32> undef)
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: %res = bitcast <1 x i32> [[INC_VRES]] to i32
  ; CHECK-LSC: [[INC_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK-LSC: [[INC_VADDR:%[^ ]+]] = bitcast i32 [[INC_ADDR]] to <1 x i32>
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 0)
  ; CHECK-LSC: [[INC_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.lsc.xatomic.slm.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i8 8, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> [[INC_VADDR]], <1 x i32> undef, <1 x i32> undef, i32 0, <1 x i32> undef)
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 0)
  ; CHECK-LSC: %res = bitcast <1 x i32> [[INC_VRES]] to i32
  %res = atomicrmw add i32 addrspace(3)* %ptr, i32 1 syncscope("subgroup") seq_cst
  ret i32 %res
}

define i32 @sub_i32(i32 addrspace(3)* %ptr, i32 %arg) {
  ; CHECK: [[SUB_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK: [[SUB_VDATA:%[^ ]+]] = bitcast i32 %arg to <1 x i32>
  ; CHECK: [[SUB_VADDR:%[^ ]+]] = bitcast i32 [[SUB_ADDR]] to <1 x i32>
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: [[SUB_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.dword.atomic.sub.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i32 254, <1 x i32> [[SUB_VADDR]], <1 x i32> [[SUB_VDATA]], <1 x i32> undef)
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: %res = bitcast <1 x i32> [[SUB_VRES]] to i32
  ; CHECK-LSC: [[SUB_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK-LSC: [[SUB_VADDR:%[^ ]+]] = bitcast i32 [[SUB_ADDR]] to <1 x i32>
  ; CHECK-LSC: [[SUB_VDATA:%[^ ]+]] = bitcast i32 %arg to <1 x i32>
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 1)
  ; CHECK-LSC: [[SUB_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.lsc.xatomic.slm.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i8 13, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> [[SUB_VADDR]], <1 x i32> [[SUB_VDATA]], <1 x i32> undef, i32 0, <1 x i32> undef)
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 1)
  ; CHECK-LSC: %res = bitcast <1 x i32> [[SUB_VRES]] to i32
  %res = atomicrmw sub i32 addrspace(3)* %ptr, i32 %arg syncscope("workgroup") acq_rel
  ret i32 %res
}

define i32 @dec_i32(i32 addrspace(3)* %ptr) {
  ; CHECK: [[DEC_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK: [[DEC_VADDR:%[^ ]+]] = bitcast i32 [[DEC_ADDR]] to <1 x i32>
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: [[DEC_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.dword.atomic.dec.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i32 254, <1 x i32> [[DEC_VADDR]], <1 x i32> undef)
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: %res = bitcast <1 x i32> [[DEC_VRES]] to i32
  ; CHECK-LSC: [[DEC_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK-LSC: [[DEC_VADDR:%[^ ]+]] = bitcast i32 [[DEC_ADDR]] to <1 x i32>
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 1)
  ; CHECK-LSC: [[DEC_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.lsc.xatomic.slm.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i8 9, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> [[DEC_VADDR]], <1 x i32> undef, <1 x i32> undef, i32 0, <1 x i32> undef)
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 1)
  ; CHECK-LSC: %res = bitcast <1 x i32> [[DEC_VRES]] to i32
  %res = atomicrmw sub i32 addrspace(3)* %ptr, i32 1 syncscope("workgroup") seq_cst
  ret i32 %res
}

define i32 @and_i32(i32 addrspace(3)* %ptr, i32 %arg) {
  ; CHECK: [[AND_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK: [[AND_VDATA:%[^ ]+]] = bitcast i32 %arg to <1 x i32>
  ; CHECK: [[AND_VADDR:%[^ ]+]] = bitcast i32 [[AND_ADDR]] to <1 x i32>
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: [[AND_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.dword.atomic.and.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i32 254, <1 x i32> [[AND_VADDR]], <1 x i32> [[AND_VDATA]], <1 x i32> undef)
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: %res = bitcast <1 x i32> [[AND_VRES]] to i32
  ; CHECK-LSC: [[AND_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK-LSC: [[AND_VADDR:%[^ ]+]] = bitcast i32 [[AND_ADDR]] to <1 x i32>
  ; CHECK-LSC: [[AND_VDATA:%[^ ]+]] = bitcast i32 %arg to <1 x i32>
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 4)
  ; CHECK-LSC: [[AND_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.lsc.xatomic.slm.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i8 24, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> [[AND_VADDR]], <1 x i32> [[AND_VDATA]], <1 x i32> undef, i32 0, <1 x i32> undef)
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 4)
  ; CHECK-LSC: %res = bitcast <1 x i32> [[AND_VRES]] to i32
  %res = atomicrmw and i32 addrspace(3)* %ptr, i32 %arg syncscope("all_devices") acq_rel
  ret i32 %res
}

define i32 @or_i32(i32 addrspace(3)* %ptr, i32 %arg) {
  ; CHECK: [[OR_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK: [[OR_VDATA:%[^ ]+]] = bitcast i32 %arg to <1 x i32>
  ; CHECK: [[OR_VADDR:%[^ ]+]] = bitcast i32 [[OR_ADDR]] to <1 x i32>
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: [[OR_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.dword.atomic.or.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i32 254, <1 x i32> [[OR_VADDR]], <1 x i32> [[OR_VDATA]], <1 x i32> undef)
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: %res = bitcast <1 x i32> [[OR_VRES]] to i32
  ; CHECK-LSC: [[OR_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK-LSC: [[OR_VADDR:%[^ ]+]] = bitcast i32 [[OR_ADDR]] to <1 x i32>
  ; CHECK-LSC: [[OR_VDATA:%[^ ]+]] = bitcast i32 %arg to <1 x i32>
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 0)
  ; CHECK-LSC: [[OR_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.lsc.xatomic.slm.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i8 25, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> [[OR_VADDR]], <1 x i32> [[OR_VDATA]], <1 x i32> undef, i32 0, <1 x i32> undef)
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 0)
  ; CHECK-LSC: %res = bitcast <1 x i32> [[OR_VRES]] to i32
  %res = atomicrmw or i32 addrspace(3)* %ptr, i32 %arg syncscope("signlethread") acq_rel
  ret i32 %res
}

define i32 @xor_i32(i32 addrspace(3)* %ptr, i32 %arg) {
  ; CHECK: [[XOR_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK: [[XOR_VDATA:%[^ ]+]] = bitcast i32 %arg to <1 x i32>
  ; CHECK: [[XOR_VADDR:%[^ ]+]] = bitcast i32 [[XOR_ADDR]] to <1 x i32>
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: [[XOR_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.dword.atomic.xor.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i32 254, <1 x i32> [[XOR_VADDR]], <1 x i32> [[XOR_VDATA]], <1 x i32> undef)
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: %res = bitcast <1 x i32> [[XOR_VRES]] to i32
  ; CHECK-LSC: [[XOR_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK-LSC: [[XOR_VADDR:%[^ ]+]] = bitcast i32 [[XOR_ADDR]] to <1 x i32>
  ; CHECK-LSC: [[XOR_VDATA:%[^ ]+]] = bitcast i32 %arg to <1 x i32>
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 0)
  ; CHECK-LSC: [[XOR_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.lsc.xatomic.slm.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i8 26, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> [[XOR_VADDR]], <1 x i32> [[XOR_VDATA]], <1 x i32> undef, i32 0, <1 x i32> undef)
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 0)
  ; CHECK-LSC: %res = bitcast <1 x i32> [[XOR_VRES]] to i32
  %res = atomicrmw xor i32 addrspace(3)* %ptr, i32 %arg syncscope("signlethread") acq_rel
  ret i32 %res
}

define i32 @max_i32(i32 addrspace(3)* %ptr, i32 %arg) {
  ; CHECK: [[MAX_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK: [[MAX_VDATA:%[^ ]+]] = bitcast i32 %arg to <1 x i32>
  ; CHECK: [[MAX_VADDR:%[^ ]+]] = bitcast i32 [[MAX_ADDR]] to <1 x i32>
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: [[MAX_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.dword.atomic.imax.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i32 254, <1 x i32> [[MAX_VADDR]], <1 x i32> [[MAX_VDATA]], <1 x i32> undef)
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: %res = bitcast <1 x i32> [[MAX_VRES]] to i32
  ; CHECK-LSC: [[MAX_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK-LSC: [[MAX_VADDR:%[^ ]+]] = bitcast i32 [[MAX_ADDR]] to <1 x i32>
  ; CHECK-LSC: [[MAX_VDATA:%[^ ]+]] = bitcast i32 %arg to <1 x i32>
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 0)
  ; CHECK-LSC: [[MAX_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.lsc.xatomic.slm.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i8 15, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> [[MAX_VADDR]], <1 x i32> [[MAX_VDATA]], <1 x i32> undef, i32 0, <1 x i32> undef)
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 0)
  ; CHECK-LSC: %res = bitcast <1 x i32> [[MAX_VRES]] to i32
  %res = atomicrmw max i32 addrspace(3)* %ptr, i32 %arg syncscope("signlethread") acq_rel
  ret i32 %res
}

define i32 @min_i32(i32 addrspace(3)* %ptr, i32 %arg) {
  ; CHECK: [[MIN_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK: [[MIN_VDATA:%[^ ]+]] = bitcast i32 %arg to <1 x i32>
  ; CHECK: [[MIN_VADDR:%[^ ]+]] = bitcast i32 [[MIN_ADDR]] to <1 x i32>
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: [[MIN_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.dword.atomic.imin.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i32 254, <1 x i32> [[MIN_VADDR]], <1 x i32> [[MIN_VDATA]], <1 x i32> undef)
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: %res = bitcast <1 x i32> [[MIN_VRES]] to i32
  ; CHECK-LSC: [[MIN_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK-LSC: [[MIN_VADDR:%[^ ]+]] = bitcast i32 [[MIN_ADDR]] to <1 x i32>
  ; CHECK-LSC: [[MIN_VDATA:%[^ ]+]] = bitcast i32 %arg to <1 x i32>
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 0)
  ; CHECK-LSC: [[MIN_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.lsc.xatomic.slm.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i8 14, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> [[MIN_VADDR]], <1 x i32> [[MIN_VDATA]], <1 x i32> undef, i32 0, <1 x i32> undef)
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 0)
  ; CHECK-LSC: %res = bitcast <1 x i32> [[MIN_VRES]] to i32
  %res = atomicrmw min i32 addrspace(3)* %ptr, i32 %arg syncscope("signlethread") acq_rel
  ret i32 %res
}

define i32 @umax_i32(i32 addrspace(3)* %ptr, i32 %arg) {
  ; CHECK: [[UMAX_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK: [[UMAX_VDATA:%[^ ]+]] = bitcast i32 %arg to <1 x i32>
  ; CHECK: [[UMAX_VADDR:%[^ ]+]] = bitcast i32 [[UMAX_ADDR]] to <1 x i32>
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: [[UMAX_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.dword.atomic.max.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i32 254, <1 x i32> [[UMAX_VADDR]], <1 x i32> [[UMAX_VDATA]], <1 x i32> undef)
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: %res = bitcast <1 x i32> [[UMAX_VRES]] to i32
  ; CHECK-LSC: [[UMAX_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK-LSC: [[UMAX_VADDR:%[^ ]+]] = bitcast i32 [[UMAX_ADDR]] to <1 x i32>
  ; CHECK-LSC: [[UMAX_VDATA:%[^ ]+]] = bitcast i32 %arg to <1 x i32>
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 0)
  ; CHECK-LSC: [[UMAX_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.lsc.xatomic.slm.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i8 17, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> [[UMAX_VADDR]], <1 x i32> [[UMAX_VDATA]], <1 x i32> undef, i32 0, <1 x i32> undef)
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 0)
  ; CHECK-LSC: %res = bitcast <1 x i32> [[UMAX_VRES]] to i32
  %res = atomicrmw umax i32 addrspace(3)* %ptr, i32 %arg syncscope("signlethread") acq_rel
  ret i32 %res
}

define i32 @umin_i32(i32 addrspace(3)* %ptr, i32 %arg) {
  ; CHECK: [[UMIN_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK: [[UMIN_VDATA:%[^ ]+]] = bitcast i32 %arg to <1 x i32>
  ; CHECK: [[UMIN_VADDR:%[^ ]+]] = bitcast i32 [[UMIN_ADDR]] to <1 x i32>
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: [[UMIN_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.dword.atomic.min.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i32 254, <1 x i32> [[UMIN_VADDR]], <1 x i32> [[UMIN_VDATA]], <1 x i32> undef)
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: %res = bitcast <1 x i32> [[UMIN_VRES]] to i32
  ; CHECK-LSC: [[UMIN_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK-LSC: [[UMIN_VADDR:%[^ ]+]] = bitcast i32 [[UMIN_ADDR]] to <1 x i32>
  ; CHECK-LSC: [[UMIN_VDATA:%[^ ]+]] = bitcast i32 %arg to <1 x i32>
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 0)
  ; CHECK-LSC: [[UMIN_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.lsc.xatomic.slm.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i8 16, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> [[UMIN_VADDR]], <1 x i32> [[UMIN_VDATA]], <1 x i32> undef, i32 0, <1 x i32> undef)
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 0)
  ; CHECK-LSC: %res = bitcast <1 x i32> [[UMIN_VRES]] to i32
  %res = atomicrmw umin i32 addrspace(3)* %ptr, i32 %arg syncscope("signlethread") acq_rel
  ret i32 %res
}

define i32 @cmpxchg_i32(i32 addrspace(3)* %ptr, i32 %cmp, i32 %arg) {
  ; CHECK: [[CAS_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK: [[CAS_VARG:%[^ ]+]] = bitcast i32 %arg to <1 x i32>
  ; CHECK: [[CAS_VCMP:%[^ ]+]] = bitcast i32 %cmp to <1 x i32>
  ; CHECK: [[CAS_VADDR:%[^ ]+]] = bitcast i32 [[CAS_ADDR]] to <1 x i32>
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: [[CAS_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.dword.atomic.cmpxchg.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i32 254, <1 x i32> [[CAS_VADDR]], <1 x i32> [[CAS_VARG]], <1 x i32> [[CAS_VCMP]], <1 x i32> undef)
  ; CHECK: call void @llvm.genx.fence(i8 33)
  ; CHECK: [[CAS_VAL:%[^ ]+]] = bitcast <1 x i32> [[CAS_VRES]] to i32
  ; CHECK: [[CAS_CMP:%[^ ]+]] = icmp eq i32 [[CAS_VAL]], %cmp
  ; CHECK: [[CAS_INS:%[^ ]+]] = insertvalue { i32, i1 } undef, i32 [[CAS_VAL]], 0
  ; CHECK: %res = insertvalue { i32, i1 } [[CAS_INS]], i1 [[CAS_CMP]], 1
  ; CHECK-LSC: [[CAS_ADDR:%[^ ]+]] = ptrtoint i32 addrspace(3)* %ptr to i32
  ; CHECK-LSC: [[CAS_VADDR:%[^ ]+]] = bitcast i32 [[CAS_ADDR]] to <1 x i32>
  ; CHECK-LSC: [[CAS_VCMP:%[^ ]+]] = bitcast i32 %cmp to <1 x i32>
  ; CHECK-LSC: [[CAS_VARG:%[^ ]+]] = bitcast i32 %arg to <1 x i32>
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 0)
  ; CHECK-LSC: [[CAS_VRES:%[^ ]+]] = call <1 x i32> @llvm.genx.lsc.xatomic.slm.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i8 18, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <1 x i32> [[CAS_VADDR]], <1 x i32> [[CAS_VCMP]], <1 x i32> [[CAS_VARG]], i32 0, <1 x i32> undef)
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 0)
  ; CHECK-LSC: [[CAS_VAL:%[^ ]+]] = bitcast <1 x i32> [[CAS_VRES]] to i32
  ; CHECK-LSC: [[CAS_CMP:%[^ ]+]] = icmp eq i32 [[CAS_VAL]], %cmp
  ; CHECK-LSC: [[CAS_INS:%[^ ]+]] = insertvalue { i32, i1 } undef, i32 [[CAS_VAL]], 0
  ; CHECK-LSC: %res = insertvalue { i32, i1 } [[CAS_INS]], i1 [[CAS_CMP]], 1
  %res = cmpxchg i32 addrspace(3)* %ptr, i32 %cmp, i32 %arg syncscope("signlethread") acq_rel acquire
  %extr = extractvalue {i32, i1} %res, 0
  ret i32 %extr
}
