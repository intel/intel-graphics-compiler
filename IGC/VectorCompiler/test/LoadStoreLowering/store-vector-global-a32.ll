;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_legacy_typed %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck %s
; RUN: %opt_legacy_opaque %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck %s
; RUN: %opt_legacy_typed %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck --check-prefix=CHECK-LSC %s
; RUN: %opt_legacy_opaque %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck --check-prefix=CHECK-LSC %s
;
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS
; CHECK-LSC-NOT: WARNING
; CHECK-LSC: CheckModuleDebugify: PASS

; COM: Basic test on store lowering pass
; COM: simplest store to addrspace(6)

target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Address space 6 (global 32-bit ptr) operations are lowered into bti(255) intrinsics

define void @replace_store_i8_block(<16 x i8> addrspace(6)* %pi8, <16 x i8> %val) {
; CHECK: call void @llvm.genx.oword.st.v16i8(i32 255, i32 %{{[a-zA-Z0-9.]+}}, <16 x i8> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v2i64(<1 x i1> <i1 true>, i8 2, i8 4, i8 2, <2 x i8> zeroinitializer, i32 255, i32 %{{[^,]+}}, i16 1, i32 0, <2 x i64> %{{[^)]+}})
  store <16 x i8> %val, <16 x i8> addrspace(6)* %pi8
  ret void
}

define void @replace_store_i16_block(<16 x i16> addrspace(6)* %pi16, <16 x i16> %val) {
; CHECK: call void @llvm.genx.oword.st.v16i16(i32 255, i32 %{{[a-zA-Z0-9.]+}}, <16 x i16> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v4i64(<1 x i1> <i1 true>, i8 2, i8 4, i8 4, <2 x i8> zeroinitializer, i32 255, i32 %{{[^,]+}}, i16 1, i32 0, <4 x i64> %{{[^)]+}})
  store <16 x i16> %val, <16 x i16> addrspace(6)* %pi16
  ret void
}

define void @replace_store_i32_block(<16 x i32> addrspace(6)* %pi32, <16 x i32> %val) {
; CHECK: call void @llvm.genx.oword.st.v16i32(i32 255, i32 %{{[a-zA-Z0-9.]+}}, <16 x i32> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v8i64(<1 x i1> <i1 true>, i8 2, i8 4, i8 5, <2 x i8> zeroinitializer, i32 255, i32 %{{[^,]+}}, i16 1, i32 0, <8 x i64> %{{[^)]+}})
  store <16 x i32> %val, <16 x i32> addrspace(6)* %pi32
  ret void
}

define void @replace_store_i64_block(<16 x i64> addrspace(6)* %pi64, <16 x i64> %val) {
; CHECK: call void @llvm.genx.oword.st.v16i64(i32 255, i32 %{{[a-zA-Z0-9.]+}}, <16 x i64> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v16i64(<1 x i1> <i1 true>, i8 2, i8 4, i8 6, <2 x i8> zeroinitializer, i32 255, i32 %{{[^,]+}}, i16 1, i32 0, <16 x i64> %val)
  store <16 x i64> %val, <16 x i64> addrspace(6)* %pi64
  ret void
}

define void @replace_store_i8_block_unaligned(<16 x i8> addrspace(6)* %pi8, <16 x i8> %val) {
; CHECK: call void @llvm.genx.scatter.scaled.v16i1.v16i32.v16i32
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v16i1.v2i8.v16i32.v16i32
  store <16 x i8> %val, <16 x i8> addrspace(6)* %pi8, align 1
  ret void
}

define void @replace_store_i16_block_unaligned(<16 x i16> addrspace(6)* %pi16, <16 x i16> %val) {
; CHECK: call void @llvm.genx.scatter.scaled.v16i1.v16i32.v16i32
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v16i1.v2i8.v16i32.v16i32
  store <16 x i16> %val, <16 x i16> addrspace(6)* %pi16, align 1
  ret void
}

define void @replace_store_i32_block_unaligned(<16 x i32> addrspace(6)* %pi32, <16 x i32> %val) {
; CHECK: call void @llvm.genx.scatter.scaled.v16i1.v16i32.v16i32
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v16i1.v2i8.v16i32.v16i32
  store <16 x i32> %val, <16 x i32> addrspace(6)* %pi32, align 1
  ret void
}

define void @replace_store_i64_block_unaligned(<16 x i64> addrspace(6)* %pi64, <16 x i64> %val) {
; CHECK: call void @llvm.genx.scatter.scaled.v32i1.v32i32.v32i32
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v16i1.v2i8.v16i32.v16i64
  store <16 x i64> %val, <16 x i64> addrspace(6)* %pi64, align 1
  ret void
}

define void @replace_store_i8_block_dwalign(<16 x i8> addrspace(6)* %pi8, <16 x i8> %val) {
; CHECK: call void @llvm.genx.scatter.scaled.v16i1.v16i32.v16i32
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v4i32
  store <16 x i8> %val, <16 x i8> addrspace(6)* %pi8, align 4
  ret void
}

define void @replace_store_i16_block_dwalign(<16 x i16> addrspace(6)* %pi16, <16 x i16> %val) {
; CHECK: call void @llvm.genx.scatter.scaled.v16i1.v16i32.v16i32
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v8i32
  store <16 x i16> %val, <16 x i16> addrspace(6)* %pi16, align 4
  ret void
}

define void @replace_store_i32_block_dwalign(<16 x i32> addrspace(6)* %pi32, <16 x i32> %val) {
; CHECK: call void @llvm.genx.scatter.scaled.v16i1.v16i32.v16i32
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v16i32
  store <16 x i32> %val, <16 x i32> addrspace(6)* %pi32, align 4
  ret void
}

define void @replace_store_i64_block_dwalign(<16 x i64> addrspace(6)* %pi64, <16 x i64> %val) {
; CHECK: call void @llvm.genx.scatter.scaled.v32i1.v32i32.v32i32
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v32i32
  store <16 x i64> %val, <16 x i64> addrspace(6)* %pi64, align 4
  ret void
}

define void @replace_store_i8_block_owalign(<16 x i8> addrspace(6)* %pi8, <16 x i8> %val) {
; CHECK: call void @llvm.genx.oword.st.v16i8(i32 255, i32 %{{[a-zA-Z0-9.]+}}, <16 x i8> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v2i64
  store <16 x i8> %val, <16 x i8> addrspace(6)* %pi8, align 16
  ret void
}

define void @replace_store_i16_block_owalign(<16 x i16> addrspace(6)* %pi16, <16 x i16> %val) {
; CHECK: call void @llvm.genx.oword.st.v16i16(i32 255, i32 %{{[a-zA-Z0-9.]+}}, <16 x i16> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v4i64
  store <16 x i16> %val, <16 x i16> addrspace(6)* %pi16, align 16
  ret void
}

define void @replace_store_i32_block_owalign(<16 x i32> addrspace(6)* %pi32, <16 x i32> %val) {
; CHECK: call void @llvm.genx.oword.st.v16i32(i32 255, i32 %{{[a-zA-Z0-9.]+}}, <16 x i32> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v8i64
  store <16 x i32> %val, <16 x i32> addrspace(6)* %pi32, align 16
  ret void
}

define void @replace_store_i64_block_owalign(<16 x i64> addrspace(6)* %pi64, <16 x i64> %val) {
; CHECK: call void @llvm.genx.oword.st.v16i64(i32 255, i32 %{{[a-zA-Z0-9.]+}}, <16 x i64> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v16i64
  store <16 x i64> %val, <16 x i64> addrspace(6)* %pi64, align 16
  ret void
}

define void @replace_store_i8_block_1023bytes(<1023 x i8> addrspace(6)* %p, <1023 x i8> %val) {
; CHECK: [[ADDR0:%[^ ]+]] = lshr i32 %[[BASE:[^,]+]], 4
; CHECK: call void @llvm.genx.oword.st.v128i8(i32 255, i32 [[ADDR0]], <128 x i8>
; CHECK: [[ADDR128:%[0-9a-zA-Z.]+]] = add i32 [[ADDR0]], 8
; CHECK: call void @llvm.genx.oword.st.v128i8(i32 255, i32 [[ADDR128]], <128 x i8>
; CHECK: [[ADDR256:%[0-9a-zA-Z.]+]] = add i32 [[ADDR0]], 16
; CHECK: call void @llvm.genx.oword.st.v128i8(i32 255, i32 [[ADDR256]], <128 x i8>
; CHECK: [[ADDR384:%[0-9a-zA-Z.]+]] = add i32 [[ADDR0]], 24
; CHECK: call void @llvm.genx.oword.st.v128i8(i32 255, i32 [[ADDR384]], <128 x i8>
; CHECK: [[ADDR512:%[0-9a-zA-Z.]+]] = add i32 [[ADDR0]], 32
; CHECK: call void @llvm.genx.oword.st.v128i8(i32 255, i32 [[ADDR512]], <128 x i8>
; CHECK: [[ADDR640:%[0-9a-zA-Z.]+]] = add i32 [[ADDR0]], 40
; CHECK: call void @llvm.genx.oword.st.v128i8(i32 255, i32 [[ADDR640]], <128 x i8>
; CHECK: [[ADDR768:%[0-9a-zA-Z.]+]] = add i32 [[ADDR0]], 48
; CHECK: call void @llvm.genx.oword.st.v128i8(i32 255, i32 [[ADDR768]], <128 x i8>
; CHECK: [[ADDR896:%[0-9a-zA-Z.]+]] = add i32 [[ADDR0]], 56
; CHECK: call void @llvm.genx.oword.st.v64i8(i32 255, i32 [[ADDR896]], <64 x i8>
; CHECK: [[ADDR960:%[0-9a-zA-Z.]+]] = add i32 [[ADDR0]], 60
; CHECK: call void @llvm.genx.oword.st.v32i8(i32 255, i32 [[ADDR960]], <32 x i8>
; CHECK: [[ADDR992:%[0-9a-zA-Z.]+]] = add i32 [[ADDR0]], 62
; CHECK: call void @llvm.genx.oword.st.v16i8(i32 255, i32 [[ADDR992]], <16 x i8>
; CHECK: call void @llvm.genx.scatter.scaled.v15i1.v15i32.v15i32
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v64i64(<1 x i1> <i1 true>, i8 2, i8 4, i8 8, <2 x i8> zeroinitializer, i32 255, i32 [[ADDR:%[a-zA-Z0-9.]+]], i16 1, i32 0, <64 x i64> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: [[ADDR512:%[^ ]+]] = add i32 [[ADDR]], 512
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v32i64(<1 x i1> <i1 true>, i8 2, i8 4, i8 7, <2 x i8> zeroinitializer, i32 255, i32 [[ADDR512]], i16 1, i32 0, <32 x i64> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: [[ADDR768:%[^ ]+]] = add i32 [[ADDR]], 768
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v16i64(<1 x i1> <i1 true>, i8 2, i8 4, i8 6, <2 x i8> zeroinitializer, i32 255, i32 [[ADDR768]], i16 1, i32 0, <16 x i64> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: [[ADDR896:%[^ ]+]] = add i32 [[ADDR]], 896
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v8i64(<1 x i1> <i1 true>, i8 2, i8 4, i8 5, <2 x i8> zeroinitializer, i32 255, i32 [[ADDR896]], i16 1, i32 0, <8 x i64> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: [[ADDR960:%[^ ]+]] = add i32 [[ADDR]], 960
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v4i64(<1 x i1> <i1 true>, i8 2, i8 4, i8 4, <2 x i8> zeroinitializer, i32 255, i32 [[ADDR960]], i16 1, i32 0, <4 x i64> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: [[ADDR992:%[^ ]+]] = add i32 [[ADDR]], 992
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v3i64(<1 x i1> <i1 true>, i8 2, i8 4, i8 3, <2 x i8> zeroinitializer, i32 255, i32 [[ADDR992]], i16 1, i32 0, <3 x i64> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v7i1.v2i8.v7i32.v7i32(<7 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 2, i8 5, i8 1, <2 x i8> zeroinitializer, i32 255, <7 x i32> %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <7 x i32> %{{[a-zA-Z0-9.]+}})
  store <1023 x i8> %val, <1023 x i8> addrspace(6)* %p
  ret void
}

define void @replace_store_i8_block_1023bytes_dwalign(<1023 x i8> addrspace(6)* %p, <1023 x i8> %val) {
; CHECK: call void @llvm.genx.scatter.scaled.v1023i1.v1023i32.v1023i32
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v64i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 8, <2 x i8> zeroinitializer, i32 255, i32 [[ADDRDW:%[a-zA-Z0-9.]+]], i16 1, i32 0, <64 x i32> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: [[ADDRDW256:%[^ ]+]] = add i32 [[ADDRDW]], 256
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v64i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 8, <2 x i8> zeroinitializer, i32 255, i32 [[ADDRDW256]], i16 1, i32 0, <64 x i32> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: [[ADDRDW512:%[^ ]+]] = add i32 [[ADDRDW]], 512
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v64i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 8, <2 x i8> zeroinitializer, i32 255, i32 [[ADDRDW512]], i16 1, i32 0, <64 x i32> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: [[ADDRDW768:%[^ ]+]] = add i32 [[ADDRDW]], 768
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v32i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 7, <2 x i8> zeroinitializer, i32 255, i32 [[ADDRDW768]], i16 1, i32 0, <32 x i32> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: [[ADDRDW896:%[^ ]+]] = add i32 [[ADDRDW]], 896
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v16i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 6, <2 x i8> zeroinitializer, i32 255, i32 [[ADDRDW896]], i16 1, i32 0, <16 x i32> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: [[ADDRDW960:%[^ ]+]] = add i32 [[ADDRDW]], 960
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v8i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 5, <2 x i8> zeroinitializer, i32 255, i32 [[ADDRDW960]], i16 1, i32 0, <8 x i32> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: [[ADDRDW992:%[^ ]+]] = add i32 [[ADDRDW]], 992
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v4i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 4, <2 x i8> zeroinitializer, i32 255, i32 [[ADDRDW992]], i16 1, i32 0, <4 x i32> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: [[ADDRDW1008:%[^ ]+]] = add i32 [[ADDRDW]], 1008
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v1i1.v2i8.i32.v3i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 3, <2 x i8> zeroinitializer, i32 255, i32 [[ADDRDW1008]], i16 1, i32 0, <3 x i32> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.bti.v3i1.v2i8.v3i32.v3i32(<3 x i1> <i1 true, i1 true, i1 true>, i8 2, i8 5, i8 1, <2 x i8> zeroinitializer, i32 255, <3 x i32> %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <3 x i32> %{{[a-zA-Z0-9.]+}})
  store <1023 x i8> %val, <1023 x i8> addrspace(6)* %p, align 4
  ret void
}
