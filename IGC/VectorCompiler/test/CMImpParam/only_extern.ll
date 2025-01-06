;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_legacy_typed %use_old_pass_manager% -CMImpParam -march=genx64 -mcpu=XeHP -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_legacy_opaque %use_old_pass_manager% -CMImpParam -march=genx64 -mcpu=XeHP -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

; RUN: %opt_new_pm_typed -passes=CMImpParam -march=genx64 -mcpu=XeHP -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_new_pm_opaque -passes=CMImpParam -march=genx64 -mcpu=XeHP -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; COM: Struct indices:
; COM: StructSize = 0,
; COM: StructVersion = 1,
; COM: NumWorkDim = 2,
; COM: SIMDWidth = 3,
; COM: LocalSizeX = 4,
; COM: LocalSizeY = 5,
; COM: LocalSizeZ = 6,
; COM: GlobalSizeX = 7,
; COM: GlobalSizeY = 8,
; COM: GlobalSizeZ = 9,
; COM: PrintfBufferPtr = 10,
; COM: GlobalOffsetX = 11,
; COM: GlobalOffsetY = 12,
; COM: GlobalOffsetZ = 13,
; COM: LocalIDTablePtr = 14,
; COM: GroupCountX = 15,
; COM: GroupCountY = 16,
; COM: GroupCountZ = 17,
;CHECK-DAG: %vc.implicit.args.buf.type = type { {{.*}} }
;CHECK-DAG: %vc.ia.local.id.type = type { i16, i16, i16 }

declare <3 x i16> @llvm.genx.local.id16.v3i16()
declare <3 x i32> @llvm.genx.local.size.v3i32()
declare i32 @llvm.genx.group.id.x()
declare i32 @llvm.genx.group.id.y()
declare i32 @llvm.genx.group.id.z()
declare <3 x i32> @llvm.genx.group.count.v3i32()
declare i64 @llvm.vc.internal.print.buffer()

define spir_func void @with_printf() {
; COM: The signature shouldn't change.
; CHECK-LABEL: define spir_func void @with_printf()

; COM: Next 3 instructions obtain implicit args buffer (IAB) pointer:
; CHECK: %[[PRINTF_R0:[^ ]+]] = call i32 @llvm.genx.r0.i32()
; CHECK: %[[PRINTF_IAB_PTR_INT:[^ ]+]] = and i32 %[[PRINTF_R0]], -64
; CHECK-TYPED-PTRS: %[[PRINTF_IAB_PTR:[^ ]+]] = inttoptr i32 %[[PRINTF_IAB_PTR_INT]] to %vc.implicit.args.buf.type addrspace(6)*
; CHECK-OPAQUE-PTRS: %[[PRINTF_IAB_PTR:[^ ]+]] = inttoptr i32 %[[PRINTF_IAB_PTR_INT]] to ptr addrspace(6)

; COM: Loading printf buffer ptr from IAB and storing it to implict arg global:
; CHECK-TYPED-PTRS: %[[PRINTF_PBP_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[PRINTF_IAB_PTR]], i32 0, i32 10
; CHECK-TYPED-PTRS: %[[PRINTF_PBP:[^ ]+]] = load i64, i64 addrspace(6)* %[[PRINTF_PBP_PTR]]
; CHECK-TYPED-PTRS: store i64 %[[PRINTF_PBP]], i64* @__imparg_llvm.vc.internal.print.buffer
; CHECK-OPAQUE-PTRS: %[[PRINTF_PBP_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[PRINTF_IAB_PTR]], i32 0, i32 10
; CHECK-OPAQUE-PTRS: %[[PRINTF_PBP:[^ ]+]] = load i64, ptr addrspace(6) %[[PRINTF_PBP_PTR]]
; CHECK-OPAQUE-PTRS: store i64 %[[PRINTF_PBP]], ptr @__imparg_llvm.vc.internal.print.buffer

  %wp.print = call i64 @llvm.vc.internal.print.buffer()
; CHECK-TYPED-PTRS: %wp.print = load i64, i64* @__imparg_llvm.vc.internal.print.buffer
; CHECK-OPAQUE-PTRS: %wp.print = load i64, ptr @__imparg_llvm.vc.internal.print.buffer
  ret void
}

define spir_func void @with_local_size() {
; COM: The signature shouldn't change.
; CHECK-LABEL: define spir_func void @with_local_size()
; CHECK: %[[LOCSZ_R0:[^ ]+]] = call i32 @llvm.genx.r0.i32()
; CHECK: %[[LOCSZ_IAB_PTR_INT:[^ ]+]] = and i32 %[[LOCSZ_R0]], -64
; CHECK-TYPED-PTRS: %[[LOCSZ_IAB_PTR:[^ ]+]] = inttoptr i32 %[[LOCSZ_IAB_PTR_INT]] to %vc.implicit.args.buf.type addrspace(6)*
; CHECK-TYPED-PTRS-DAG: %[[LOCSZ_LSZ_X_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[LOCSZ_IAB_PTR]], i32 0, i32 4
; CHECK-TYPED-PTRS-DAG: %[[LOCSZ_LSZ_X:[^ ]+]] = load i32, i32 addrspace(6)* %[[LOCSZ_LSZ_X_PTR]]
; CHECK-TYPED-PTRS-DAG: %[[LOCSZ_LSZ_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[LOCSZ_IAB_PTR]], i32 0, i32 5
; CHECK-TYPED-PTRS-DAG: %[[LOCSZ_LSZ_Y:[^ ]+]] = load i32, i32 addrspace(6)* %[[LOCSZ_LSZ_Y_PTR]]
; CHECK-TYPED-PTRS-DAG: %[[LOCSZ_LSZ_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[LOCSZ_IAB_PTR]], i32 0, i32 6
; CHECK-TYPED-PTRS-DAG: %[[LOCSZ_LSZ_Z:[^ ]+]] = load i32, i32 addrspace(6)* %[[LOCSZ_LSZ_Z_PTR]]
; CHECK-OPAQUE-PTRS: %[[LOCSZ_IAB_PTR:[^ ]+]] = inttoptr i32 %[[LOCSZ_IAB_PTR_INT]] to ptr addrspace(6)
; CHECK-OPAQUE-PTRS-DAG: %[[LOCSZ_LSZ_X_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[LOCSZ_IAB_PTR]], i32 0, i32 4
; CHECK-OPAQUE-PTRS-DAG: %[[LOCSZ_LSZ_X:[^ ]+]] = load i32, ptr addrspace(6) %[[LOCSZ_LSZ_X_PTR]]
; CHECK-OPAQUE-PTRS-DAG: %[[LOCSZ_LSZ_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[LOCSZ_IAB_PTR]], i32 0, i32 5
; CHECK-OPAQUE-PTRS-DAG: %[[LOCSZ_LSZ_Y:[^ ]+]] = load i32, ptr addrspace(6) %[[LOCSZ_LSZ_Y_PTR]]
; CHECK-OPAQUE-PTRS-DAG: %[[LOCSZ_LSZ_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[LOCSZ_IAB_PTR]], i32 0, i32 6
; CHECK-OPAQUE-PTRS-DAG: %[[LOCSZ_LSZ_Z:[^ ]+]] = load i32, ptr addrspace(6) %[[LOCSZ_LSZ_Z_PTR]]
; CHECK: %[[LOCSZ_LSZ_PART_0:[^ ]+]] = insertelement <3 x i32> undef, i32 %[[LOCSZ_LSZ_X]], i64 0
; CHECK: %[[LOCSZ_LSZ_PART_1:[^ ]+]] = insertelement <3 x i32> %[[LOCSZ_LSZ_PART_0]], i32 %[[LOCSZ_LSZ_Y]], i64 1
; CHECK: %[[LOCSZ_LSZ:[^ ]+]] = insertelement <3 x i32> %[[LOCSZ_LSZ_PART_1]], i32 %[[LOCSZ_LSZ_Z]], i64 2
; CHECK-TYPED-PTRS: store <3 x i32> %[[LOCSZ_LSZ]], <3 x i32>* @__imparg_llvm.genx.local.size
; CHECK-OPAQUE-PTRS: store <3 x i32> %[[LOCSZ_LSZ]], ptr @__imparg_llvm.genx.local.size

  %wls.local.size = call <3 x i32> @llvm.genx.local.size.v3i32()
; CHECK-TYPED-PTRS: %wls.local.size = load <3 x i32>, <3 x i32>* @__imparg_llvm.genx.local.size
; CHECK-OPAQUE-PTRS: %wls.local.size = load <3 x i32>, ptr @__imparg_llvm.genx.local.size
  ret void
}

define spir_func void @with_group_count() {
; COM: The signature shouldn't change.
; CHECK-LABEL: define spir_func void @with_group_count()
; CHECK: %[[GRPCNT_R0:[^ ]+]] = call i32 @llvm.genx.r0.i32()
; CHECK: %[[GRPCNT_IAB_PTR_INT:[^ ]+]] = and i32 %[[GRPCNT_R0]], -64
; CHECK-TYPED-PTRS: %[[GRPCNT_IAB_PTR:[^ ]+]] = inttoptr i32 %[[GRPCNT_IAB_PTR_INT]] to %vc.implicit.args.buf.type addrspace(6)*
; CHECK-TYPED-PTRS-DAG: %[[GRPCNT_CNT_X_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[GRPCNT_IAB_PTR]], i32 0, i32 15
; CHECK-TYPED-PTRS-DAG: %[[GRPCNT_CNT_X:[^ ]+]] = load i32, i32 addrspace(6)* %[[GRPCNT_CNT_X_PTR]]
; CHECK-TYPED-PTRS-DAG: %[[GRPCNT_CNT_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[GRPCNT_IAB_PTR]], i32 0, i32 16
; CHECK-TYPED-PTRS-DAG: %[[GRPCNT_CNT_Y:[^ ]+]] = load i32, i32 addrspace(6)* %[[GRPCNT_CNT_Y_PTR]]
; CHECK-TYPED-PTRS-DAG: %[[GRPCNT_CNT_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[GRPCNT_IAB_PTR]], i32 0, i32 17
; CHECK-TYPED-PTRS-DAG: %[[GRPCNT_CNT_Z:[^ ]+]] = load i32, i32 addrspace(6)* %[[GRPCNT_CNT_Z_PTR]]
; CHECK-OPAQUE-PTRS: %[[GRPCNT_IAB_PTR:[^ ]+]] = inttoptr i32 %[[GRPCNT_IAB_PTR_INT]] to ptr addrspace(6)
; CHECK-OPAQUE-PTRS-DAG: %[[GRPCNT_CNT_X_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[GRPCNT_IAB_PTR]], i32 0, i32 15
; CHECK-OPAQUE-PTRS-DAG: %[[GRPCNT_CNT_X:[^ ]+]] = load i32, ptr addrspace(6) %[[GRPCNT_CNT_X_PTR]]
; CHECK-OPAQUE-PTRS-DAG: %[[GRPCNT_CNT_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[GRPCNT_IAB_PTR]], i32 0, i32 16
; CHECK-OPAQUE-PTRS-DAG: %[[GRPCNT_CNT_Y:[^ ]+]] = load i32, ptr addrspace(6) %[[GRPCNT_CNT_Y_PTR]]
; CHECK-OPAQUE-PTRS-DAG: %[[GRPCNT_CNT_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[GRPCNT_IAB_PTR]], i32 0, i32 17
; CHECK-OPAQUE-PTRS-DAG: %[[GRPCNT_CNT_Z:[^ ]+]] = load i32, ptr addrspace(6) %[[GRPCNT_CNT_Z_PTR]]
; CHECK: %[[GRPCNT_CNT_PART_0:[^ ]+]] = insertelement <3 x i32> undef, i32 %[[GRPCNT_CNT_X]], i64 0
; CHECK: %[[GRPCNT_CNT_PART_1:[^ ]+]] = insertelement <3 x i32> %[[GRPCNT_CNT_PART_0]], i32 %[[GRPCNT_CNT_Y]], i64 1
; CHECK: %[[GRPCNT_CNT:[^ ]+]] = insertelement <3 x i32> %[[GRPCNT_CNT_PART_1]], i32 %[[GRPCNT_CNT_Z]], i64 2
; CHECK-TYPED-PTRS: store <3 x i32> %[[GRPCNT_CNT]], <3 x i32>* @__imparg_llvm.genx.group.count
; CHECK-OPAQUE-PTRS: store <3 x i32> %[[GRPCNT_CNT]], ptr @__imparg_llvm.genx.group.count

  %wgc.group.count = call <3 x i32> @llvm.genx.group.count.v3i32()
; CHECK-TYPED-PTRS: %wgc.group.count = load <3 x i32>, <3 x i32>* @__imparg_llvm.genx.group.count
; CHECK-OPAQUE-PTRS: %wgc.group.count = load <3 x i32>, ptr @__imparg_llvm.genx.group.count
  ret void
}

define spir_func void @with_all() {
; COM: The signature shouldn't change.
; CHECK-LABEL: define spir_func void @with_all()

; COM: Next 3 instructions obtain implicit args buffer (IAB) pointer:
; CHECK: %[[WITHALL_R0:[^ ]+]] = call i32 @llvm.genx.r0.i32()
; CHECK: %[[WITHALL_IAB_PTR_INT:[^ ]+]] = and i32 %[[WITHALL_R0]], -64
; CHECK-TYPED-PTRS: %[[WITHALL_IAB_PTR:[^ ]+]] = inttoptr i32 %[[WITHALL_IAB_PTR_INT]] to %vc.implicit.args.buf.type addrspace(6)*
; CHECK-OPAQUE-PTRS: %[[WITHALL_IAB_PTR:[^ ]+]] = inttoptr i32 %[[WITHALL_IAB_PTR_INT]] to ptr addrspace(6)

; COM: handling printf buffer pointer:
; CHECK-TYPED-PTRS-DAG: %[[WITHALL_PBP_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[WITHALL_IAB_PTR]], i32 0, i32 10
; CHECK-TYPED-PTRS-DAG: %[[WITHALL_PBP:[^ ]+]] = load i64, i64 addrspace(6)* %[[WITHALL_PBP_PTR]]
; CHECK-TYPED-PTRS-DAG: store i64 %[[WITHALL_PBP]], i64* @__imparg_llvm.vc.internal.print.buffer
; CHECK-OPAQUE-PTRS-DAG: %[[WITHALL_PBP_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[WITHALL_IAB_PTR]], i32 0, i32 10
; CHECK-OPAQUE-PTRS-DAG: %[[WITHALL_PBP:[^ ]+]] = load i64, ptr addrspace(6) %[[WITHALL_PBP_PTR]]
; CHECK-OPAQUE-PTRS-DAG: store i64 %[[WITHALL_PBP]], ptr @__imparg_llvm.vc.internal.print.buffer

; COM: handling local size:
; CHECK-TYPED-PTRS-DAG: %[[WITHALL_LSZ_X_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[WITHALL_IAB_PTR]], i32 0, i32 4
; CHECK-TYPED-PTRS-DAG: %[[WITHALL_LSZ_X:[^ ]+]] = load i32, i32 addrspace(6)* %[[WITHALL_LSZ_X_PTR]]
; CHECK-TYPED-PTRS-DAG: %[[WITHALL_LSZ_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[WITHALL_IAB_PTR]], i32 0, i32 5
; CHECK-TYPED-PTRS-DAG: %[[WITHALL_LSZ_Y:[^ ]+]] = load i32, i32 addrspace(6)* %[[WITHALL_LSZ_Y_PTR]]
; CHECK-TYPED-PTRS-DAG: %[[WITHALL_LSZ_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[WITHALL_IAB_PTR]], i32 0, i32 6
; CHECK-TYPED-PTRS-DAG: %[[WITHALL_LSZ_Z:[^ ]+]] = load i32, i32 addrspace(6)* %[[WITHALL_LSZ_Z_PTR]]
; CHECK-OPAQUE-PTRS-DAG: %[[WITHALL_LSZ_X_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[WITHALL_IAB_PTR]], i32 0, i32 4
; CHECK-OPAQUE-PTRS-DAG: %[[WITHALL_LSZ_X:[^ ]+]] = load i32, ptr addrspace(6) %[[WITHALL_LSZ_X_PTR]]
; CHECK-OPAQUE-PTRS-DAG: %[[WITHALL_LSZ_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[WITHALL_IAB_PTR]], i32 0, i32 5
; CHECK-OPAQUE-PTRS-DAG: %[[WITHALL_LSZ_Y:[^ ]+]] = load i32, ptr addrspace(6) %[[WITHALL_LSZ_Y_PTR]]
; CHECK-OPAQUE-PTRS-DAG: %[[WITHALL_LSZ_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[WITHALL_IAB_PTR]], i32 0, i32 6
; CHECK-OPAQUE-PTRS-DAG: %[[WITHALL_LSZ_Z:[^ ]+]] = load i32, ptr addrspace(6) %[[WITHALL_LSZ_Z_PTR]]
; CHECK-DAG: %[[WITHALL_LSZ_PART_0:[^ ]+]] = insertelement <3 x i32> undef, i32 %[[WITHALL_LSZ_X]], i64 0
; CHECK-DAG: %[[WITHALL_LSZ_PART_1:[^ ]+]] = insertelement <3 x i32> %[[WITHALL_LSZ_PART_0]], i32 %[[WITHALL_LSZ_Y]], i64 1
; CHECK-DAG: %[[WITHALL_LSZ:[^ ]+]] = insertelement <3 x i32> %[[WITHALL_LSZ_PART_1]], i32 %[[WITHALL_LSZ_Z]], i64 2
; CHECK-TYPED-PTRS-DAG: store <3 x i32> %[[WITHALL_LSZ]], <3 x i32>* @__imparg_llvm.genx.local.size
; CHECK-OPAQUE-PTRS-DAG: store <3 x i32> %[[WITHALL_LSZ]], ptr @__imparg_llvm.genx.local.size

; COM: handling group count:
; CHECK-TYPED-PTRS-DAG: %[[WITHALL_CNT_X_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[WITHALL_IAB_PTR]], i32 0, i32 15
; CHECK-TYPED-PTRS-DAG: %[[WITHALL_CNT_X:[^ ]+]] = load i32, i32 addrspace(6)* %[[WITHALL_CNT_X_PTR]]
; CHECK-TYPED-PTRS-DAG: %[[WITHALL_CNT_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[WITHALL_IAB_PTR]], i32 0, i32 16
; CHECK-TYPED-PTRS-DAG: %[[WITHALL_CNT_Y:[^ ]+]] = load i32, i32 addrspace(6)* %[[WITHALL_CNT_Y_PTR]]
; CHECK-TYPED-PTRS-DAG: %[[WITHALL_CNT_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[WITHALL_IAB_PTR]], i32 0, i32 17
; CHECK-TYPED-PTRS-DAG: %[[WITHALL_CNT_Z:[^ ]+]] = load i32, i32 addrspace(6)* %[[WITHALL_CNT_Z_PTR]]
; CHECK-OPAQUE-PTRS-DAG: %[[WITHALL_CNT_X_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[WITHALL_IAB_PTR]], i32 0, i32 15
; CHECK-OPAQUE-PTRS-DAG: %[[WITHALL_CNT_X:[^ ]+]] = load i32, ptr addrspace(6) %[[WITHALL_CNT_X_PTR]]
; CHECK-OPAQUE-PTRS-DAG: %[[WITHALL_CNT_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[WITHALL_IAB_PTR]], i32 0, i32 16
; CHECK-OPAQUE-PTRS-DAG: %[[WITHALL_CNT_Y:[^ ]+]] = load i32, ptr addrspace(6) %[[WITHALL_CNT_Y_PTR]]
; CHECK-OPAQUE-PTRS-DAG: %[[WITHALL_CNT_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[WITHALL_IAB_PTR]], i32 0, i32 17
; CHECK-OPAQUE-PTRS-DAG: %[[WITHALL_CNT_Z:[^ ]+]] = load i32, ptr addrspace(6) %[[WITHALL_CNT_Z_PTR]]
; CHECK-DAG: %[[WITHALL_CNT_PART_0:[^ ]+]] = insertelement <3 x i32> undef, i32 %[[WITHALL_CNT_X]], i64 0
; CHECK-DAG: %[[WITHALL_CNT_PART_1:[^ ]+]] = insertelement <3 x i32> %[[WITHALL_CNT_PART_0]], i32 %[[WITHALL_CNT_Y]], i64 1
; CHECK-DAG: %[[WITHALL_CNT:[^ ]+]] = insertelement <3 x i32> %[[WITHALL_CNT_PART_1]], i32 %[[WITHALL_CNT_Z]], i64 2
; CHECK-TYPED-PTRS-DAG: store <3 x i32> %[[WITHALL_CNT]], <3 x i32>* @__imparg_llvm.genx.group.count
; CHECK-OPAQUE-PTRS-DAG: store <3 x i32> %[[WITHALL_CNT]], ptr @__imparg_llvm.genx.group.count

  %wa.print = call i64 @llvm.vc.internal.print.buffer()
  %wa.local.size = call <3 x i32> @llvm.genx.local.size.v3i32()
  %wa.group.count = call <3 x i32> @llvm.genx.group.count.v3i32()
; CHECK-TYPED-PTRS: %wa.print = load i64, i64* @__imparg_llvm.vc.internal.print.buffer
; CHECK-TYPED-PTRS: %wa.local.size = load <3 x i32>, <3 x i32>* @__imparg_llvm.genx.local.size
; CHECK-TYPED-PTRS: %wa.group.count = load <3 x i32>, <3 x i32>* @__imparg_llvm.genx.group.count
; CHECK-OPAQUE-PTRS: %wa.print = load i64, ptr @__imparg_llvm.vc.internal.print.buffer
; CHECK-OPAQUE-PTRS: %wa.local.size = load <3 x i32>, ptr @__imparg_llvm.genx.local.size
; CHECK-OPAQUE-PTRS: %wa.group.count = load <3 x i32>, ptr @__imparg_llvm.genx.group.count
  ret void
}

define spir_func void @with_local_id() {
; COM: The signature shouldn't change.
; CHECK-LABEL: define spir_func void @with_local_id()

; COM: Next 3 instructions obtain implicit args buffer (IAB) pointer:
; CHECK: %[[LOCID_R0:[^ ]+]] = call i32 @llvm.genx.r0.i32()
; CHECK: %[[LOCID_IAB_PTR_INT:[^ ]+]] = and i32 %[[LOCID_R0]], -64
; CHECK-TYPED-PTRS: %[[LOCID_IAB_PTR:[^ ]+]] = inttoptr i32 %[[LOCID_IAB_PTR_INT]] to %vc.implicit.args.buf.type addrspace(6)*
; CHECK-OPAQUE-PTRS: %[[LOCID_IAB_PTR:[^ ]+]] = inttoptr i32 %[[LOCID_IAB_PTR_INT]] to ptr addrspace(6)

; COM: Loading local ID table ptr from IAB:
; CHECK-TYPED-PTRS: %[[LOCID_TABLE_PTR_GEP:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[LOCID_IAB_PTR]], i32 0, i32 14
; CHECK-TYPED-PTRS: %[[LOCID_TABLE_PTR_INT:[^ ]+]] = load i64, i64 addrspace(6)* %[[LOCID_TABLE_PTR_GEP]]
; CHECK-TYPED-PTRS: %[[LOCID_TABLE_PTR:[^ ]+]] = inttoptr i64 %[[LOCID_TABLE_PTR_INT]] to %vc.ia.local.id.type addrspace(1)*
; CHECK-OPAQUE-PTRS: %[[LOCID_TABLE_PTR_GEP:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[LOCID_IAB_PTR]], i32 0, i32 14
; CHECK-OPAQUE-PTRS: %[[LOCID_TABLE_PTR_INT:[^ ]+]] = load i64, ptr addrspace(6) %[[LOCID_TABLE_PTR_GEP]]
; CHECK-OPAQUE-PTRS: %[[LOCID_TABLE_PTR:[^ ]+]] = inttoptr i64 %[[LOCID_TABLE_PTR_INT]] to ptr addrspace(1)

; COM: Selecting local ID structure from the table base on Group Thread ID.
; CHECK: %[[LOCID_GTID_R0:[^ ]+]] = call <4 x i32> @llvm.genx.r0.v4i32()
; CHECK: %[[LOCID_GTID_BC:[^ ]+]] = bitcast <4 x i32> %[[LOCID_GTID_R0]] to <16 x i8>
; CHECK: %[[LOCID_GTID:[^ ]+]] = extractelement <16 x i8> %[[LOCID_GTID_BC]], i64 8
; CHECK-TYPED-PTRS: %[[LOCID_BASE_PTR:[^ ]+]] = getelementptr %vc.ia.local.id.type, %vc.ia.local.id.type addrspace(1)* %[[LOCID_TABLE_PTR]], i8 %[[LOCID_GTID]]
; CHECK-OPAQUE-PTRS: %[[LOCID_BASE_PTR:[^ ]+]] = getelementptr %vc.ia.local.id.type, ptr addrspace(1) %[[LOCID_TABLE_PTR]], i8 %[[LOCID_GTID]]

; COM: Loading local IDs from the structure by elements.
; CHECK-TYPED-PTRS-DAG: %[[LOCID_X_PTR:[^ ]+]] = getelementptr inbounds %vc.ia.local.id.type, %vc.ia.local.id.type addrspace(1)* %[[LOCID_BASE_PTR]], i32 0, i32 0
; CHECK-TYPED-PTRS-DAG: %[[LOCID_X:[^ ]+]] = load i16, i16 addrspace(1)* %[[LOCID_X_PTR]]
; CHECK-TYPED-PTRS-DAG: %[[LOCID_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.ia.local.id.type, %vc.ia.local.id.type addrspace(1)* %[[LOCID_BASE_PTR]], i32 0, i32 1
; CHECK-TYPED-PTRS-DAG: %[[LOCID_Y:[^ ]+]] = load i16, i16 addrspace(1)* %[[LOCID_Y_PTR]]
; CHECK-TYPED-PTRS-DAG: %[[LOCID_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.ia.local.id.type, %vc.ia.local.id.type addrspace(1)* %[[LOCID_BASE_PTR]], i32 0, i32 2
; CHECK-TYPED-PTRS-DAG: %[[LOCID_Z:[^ ]+]] = load i16, i16 addrspace(1)* %[[LOCID_Z_PTR]]
; CHECK-OPAQUE-PTRS-DAG: %[[LOCID_X_PTR:[^ ]+]] = getelementptr inbounds %vc.ia.local.id.type, ptr addrspace(1) %[[LOCID_BASE_PTR]], i32 0, i32 0
; CHECK-OPAQUE-PTRS-DAG: %[[LOCID_X:[^ ]+]] = load i16, ptr addrspace(1) %[[LOCID_X_PTR]]
; CHECK-OPAQUE-PTRS-DAG: %[[LOCID_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.ia.local.id.type, ptr addrspace(1) %[[LOCID_BASE_PTR]], i32 0, i32 1
; CHECK-OPAQUE-PTRS-DAG: %[[LOCID_Y:[^ ]+]] = load i16, ptr addrspace(1) %[[LOCID_Y_PTR]]
; CHECK-OPAQUE-PTRS-DAG: %[[LOCID_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.ia.local.id.type, ptr addrspace(1) %[[LOCID_BASE_PTR]], i32 0, i32 2
; CHECK-OPAQUE-PTRS-DAG: %[[LOCID_Z:[^ ]+]] = load i16, ptr addrspace(1) %[[LOCID_Z_PTR]]
; CHECK-DAG: %[[LOCID_PART_0:[^ ]+]] = insertelement <3 x i16> undef, i16 %[[LOCID_X]], i64 0
; CHECK-DAG: %[[LOCID_PART_1:[^ ]+]] = insertelement <3 x i16> %[[LOCID_PART_0]], i16 %[[LOCID_Y]], i64 1
; CHECK-DAG: %[[LOCID_WHOLE:[^ ]+]] = insertelement <3 x i16> %[[LOCID_PART_1]], i16 %[[LOCID_Z]], i64 2
; CHECK-TYPED-PTRS: store <3 x i16> %[[LOCID_WHOLE]], <3 x i16>* @__imparg_llvm.genx.local.id16
; CHECK-OPAQUE-PTRS: store <3 x i16> %[[LOCID_WHOLE]], ptr @__imparg_llvm.genx.local.id16

  %wli.loc.id = call <3 x i16> @llvm.genx.local.id16.v3i16()
; CHECK-TYPED-PTRS: %wli.loc.id = load <3 x i16>, <3 x i16>* @__imparg_llvm.genx.local.id16
; CHECK-OPAQUE-PTRS: %wli.loc.id = load <3 x i16>, ptr @__imparg_llvm.genx.local.id16
  ret void
}

!genx.kernels = !{}
