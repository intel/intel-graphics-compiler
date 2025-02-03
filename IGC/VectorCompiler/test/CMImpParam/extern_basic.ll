;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_legacy_typed %use_old_pass_manager% -CMImpParam -march=genx64 -mcpu=XeHP -S < %s \
; RUN:    | FileCheck %s --check-prefixes=XeHP-OCL,XeHP-OCL-TYPED-PTRS
; RUN: %opt_legacy_opaque %use_old_pass_manager% -CMImpParam -march=genx64 -mcpu=XeHP -S < %s \
; RUN:    | FileCheck %s --check-prefixes=XeHP-OCL,XeHP-OCL-OPAQUE-PTRS
; RUN: %opt_legacy_typed %use_old_pass_manager% -CMImpParam -cmimpparam-payload-in-memory=false \
; RUN:    -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s --check-prefixes=XeHPG-OCL,XeHPG-OCL-TYPED-PTRS
; RUN: %opt_legacy_opaque %use_old_pass_manager% -CMImpParam -cmimpparam-payload-in-memory=false \
; RUN:    -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s --check-prefixes=XeHPG-OCL,XeHPG-OCL-OPAQUE-PTRS

; RUN: %opt_new_pm_typed -passes=CMImpParam -march=genx64 -mcpu=XeHP -S < %s \
; RUN:    | FileCheck %s --check-prefixes=XeHP-OCL,XeHP-OCL-TYPED-PTRS
; RUN: %opt_new_pm_opaque -passes=CMImpParam -march=genx64 -mcpu=XeHP -S < %s \
; RUN:    | FileCheck %s --check-prefixes=XeHP-OCL,XeHP-OCL-OPAQUE-PTRS
; RUN: %opt_new_pm_typed -passes=CMImpParam -cmimpparam-payload-in-memory=false \
; RUN:    -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s --check-prefixes=XeHPG-OCL,XeHPG-OCL-TYPED-PTRS
; RUN: %opt_new_pm_opaque -passes=CMImpParam -cmimpparam-payload-in-memory=false \
; RUN:    -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s --check-prefixes=XeHPG-OCL,XeHPG-OCL-OPAQUE-PTRS

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
; XeHP-OCL-DAG: %vc.implicit.args.buf.type = type { {{.*}} }
; XeHPG-OCL-DAG:  %vc.implicit.args.buf.type = type { {{.*}} }

; XeHPG-OCL-DAG: @llvm.vc.predef.var.impl.args.buf = external global i64 #[[PREDEF_VAR_ATTR:[0-9]+]]

declare <3 x i16> @llvm.genx.local.id16.v3i16()
declare <3 x i32> @llvm.genx.local.size.v3i32()
declare i32 @llvm.genx.group.id.x()
declare i32 @llvm.genx.group.id.y()
declare i32 @llvm.genx.group.id.z()
declare <3 x i32> @llvm.genx.group.count.v3i32()
declare i64 @llvm.vc.internal.print.buffer()

; COM: Printf buffer is used only in extern function
define dllexport spir_kernel void @kernel() {
; XeHP-OCL: define dllexport spir_kernel void @kernel(
; XeHP-OCL-NOT: i64 %impl.arg.llvm.vc.internal.print.buffer
; XeHP-OCL-NOT: <3 x i32> %impl.arg.llvm.genx.local.size
; XeHP-OCL-NOT: <3 x i32> %impl.arg.llvm.genx.group.count
; XeHP-OCL-SAME: ) #[[KERNEL_ATTR:[0-9]+]]
; XeHPG-OCL: define dllexport spir_kernel void @kernel(
; XeHPG-OCL-NOT: i64 %impl.arg.llvm.vc.internal.print.buffer
; XeHPG-OCL-NOT: <3 x i32> %impl.arg.llvm.genx.local.size
; XeHPG-OCL-NOT: <3 x i32> %impl.arg.llvm.genx.group.count
; XeHPG-OCL-DAG: i64 %impl.arg.impl.args.buffer
; XeHPG-OCL-SAME: ) #[[KERNEL_ATTR:[0-9]+]]
; XeHPG-OCL-TYPED-PTRS-NEXT: call void @llvm.vc.internal.write.variable.region.p0i64.i64.i1(i64* @llvm.vc.predef.var.impl.args.buf, i64 %impl.arg.impl.args.buffer, i32 1, i32 0, i1 true)
; XeHPG-OCL-OPAQUE-PTRS-NEXT: call void @llvm.vc.internal.write.variable.region.p0.i64.i1(ptr @llvm.vc.predef.var.impl.args.buf, i64 %impl.arg.impl.args.buffer, i32 1, i32 0, i1 true)
  call spir_func void @with_printf()
  call spir_func void @with_local_size()
  call spir_func void @with_group_count()
  ret void
}

define spir_func void @with_printf() {
; COM: The signature shouldn't change.
; XeHP-OCL-LABEL: define spir_func void @with_printf()
; XeHPG-OCL-LABEL:  define spir_func void @with_printf()

; COM: Next 3 instructions obtain implicit args buffer (IAB) pointer:
; XeHP-OCL: %[[PRINTF_R0:[^ ]+]] = call i32 @llvm.genx.r0.i32()
; XeHP-OCL: %[[PRINTF_IAB_PTR_INT:[^ ]+]] = and i32 %[[PRINTF_R0]], -64
; XeHP-OCL-TYPED-PTRS: %[[PRINTF_IAB_PTR:[^ ]+]] = inttoptr i32 %[[PRINTF_IAB_PTR_INT]] to %vc.implicit.args.buf.type addrspace(6)*
; XeHP-OCL-OPAQUE-PTRS: %[[PRINTF_IAB_PTR:[^ ]+]] = inttoptr i32 %[[PRINTF_IAB_PTR_INT]] to ptr addrspace(6)

; COM: Next 2 instructions obtain implicit args buffer (IAB) pointer:
; XeHPG-OCL-TYPED-PTRS: %[[PRINTF_IAB_PTR_INT:[^ ]+]] = call i64 @llvm.vc.internal.read.variable.region.i64.p0i64(i64* @llvm.vc.predef.var.impl.args.buf, i32 0, i32 1, i32 1, i32 0)
; XeHPG-OCL-TYPED-PTRS: %[[PRINTF_IAB_PTR:[^ ]+]] = inttoptr i64 %[[PRINTF_IAB_PTR_INT]] to %vc.implicit.args.buf.type addrspace(1)*
; XeHPG-OCL-OPAQUE-PTRS: %[[PRINTF_IAB_PTR_INT:[^ ]+]] = call i64 @llvm.vc.internal.read.variable.region.i64.p0(ptr @llvm.vc.predef.var.impl.args.buf, i32 0, i32 1, i32 1, i32 0)
; XeHPG-OCL-OPAQUE-PTRS: %[[PRINTF_IAB_PTR:[^ ]+]] = inttoptr i64 %[[PRINTF_IAB_PTR_INT]] to ptr addrspace(1)

; COM: Loading printf buffer ptr from IAB and storing it to implict arg global:
; XeHP-OCL-TYPED-PTRS: %[[PRINTF_PBP_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[PRINTF_IAB_PTR]], i32 0, i32 10
; XeHP-OCL-TYPED-PTRS: %[[PRINTF_PBP:[^ ]+]] = load i64, i64 addrspace(6)* %[[PRINTF_PBP_PTR]]
; XeHP-OCL-TYPED-PTRS: store i64 %[[PRINTF_PBP]], i64* @__imparg_llvm.vc.internal.print.buffer
; XeHP-OCL-OPAQUE-PTRS: %[[PRINTF_PBP_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[PRINTF_IAB_PTR]], i32 0, i32 10
; XeHP-OCL-OPAQUE-PTRS: %[[PRINTF_PBP:[^ ]+]] = load i64, ptr addrspace(6) %[[PRINTF_PBP_PTR]]
; XeHP-OCL-OPAQUE-PTRS: store i64 %[[PRINTF_PBP]], ptr @__imparg_llvm.vc.internal.print.buffer

; COM: Loading printf buffer ptr from IAB and storing it to implict arg global:
; XeHPG-OCL-TYPED-PTRS: %[[PRINTF_PBP_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(1)* %[[PRINTF_IAB_PTR]], i32 0, i32 10
; XeHPG-OCL-TYPED-PTRS: %[[PRINTF_PBP:[^ ]+]] = load i64, i64 addrspace(1)* %[[PRINTF_PBP_PTR]]
; XeHPG-OCL-TYPED-PTRS: store i64 %[[PRINTF_PBP]], i64* @__imparg_llvm.vc.internal.print.buffer
; XeHPG-OCL-OPAQUE-PTRS: %[[PRINTF_PBP_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(1) %[[PRINTF_IAB_PTR]], i32 0, i32 10
; XeHPG-OCL-OPAQUE-PTRS: %[[PRINTF_PBP:[^ ]+]] = load i64, ptr addrspace(1) %[[PRINTF_PBP_PTR]]
; XeHPG-OCL-OPAQUE-PTRS: store i64 %[[PRINTF_PBP]], ptr @__imparg_llvm.vc.internal.print.buffer

  %wp.print = call i64 @llvm.vc.internal.print.buffer()
; XeHP-OCL-TYPED-PTRS: %wp.print = load i64, i64* @__imparg_llvm.vc.internal.print.buffer
; XeHPG-OCL-TYPED-PTRS: %wp.print = load i64, i64* @__imparg_llvm.vc.internal.print.buffer
; XeHP-OCL-OPAQUE-PTRS: %wp.print = load i64, ptr @__imparg_llvm.vc.internal.print.buffer
; XeHPG-OCL-OPAQUE-PTRS: %wp.print = load i64, ptr @__imparg_llvm.vc.internal.print.buffer
  ret void
}

define spir_func void @with_local_size() {
; COM: The signature shouldn't change.
; XeHP-OCL-LABEL: define spir_func void @with_local_size()
; XeHPG-OCL-LABEL:  define spir_func void @with_local_size()

; XeHP-OCL: %[[LOCSZ_R0:[^ ]+]] = call i32 @llvm.genx.r0.i32()
; XeHP-OCL: %[[LOCSZ_IAB_PTR_INT:[^ ]+]] = and i32 %[[LOCSZ_R0]], -64
; XeHP-OCL-TYPED-PTRS: %[[LOCSZ_IAB_PTR:[^ ]+]] = inttoptr i32 %[[LOCSZ_IAB_PTR_INT]] to %vc.implicit.args.buf.type addrspace(6)*
; XeHP-OCL-TYPED-PTRS-DAG: %[[LOCSZ_LSZ_X_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[LOCSZ_IAB_PTR]], i32 0, i32 4
; XeHP-OCL-TYPED-PTRS-DAG: %[[LOCSZ_LSZ_X:[^ ]+]] = load i32, i32 addrspace(6)* %[[LOCSZ_LSZ_X_PTR]]
; XeHP-OCL-TYPED-PTRS-DAG: %[[LOCSZ_LSZ_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[LOCSZ_IAB_PTR]], i32 0, i32 5
; XeHP-OCL-TYPED-PTRS-DAG: %[[LOCSZ_LSZ_Y:[^ ]+]] = load i32, i32 addrspace(6)* %[[LOCSZ_LSZ_Y_PTR]]
; XeHP-OCL-TYPED-PTRS-DAG: %[[LOCSZ_LSZ_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[LOCSZ_IAB_PTR]], i32 0, i32 6
; XeHP-OCL-TYPED-PTRS-DAG: %[[LOCSZ_LSZ_Z:[^ ]+]] = load i32, i32 addrspace(6)* %[[LOCSZ_LSZ_Z_PTR]]
; XeHP-OCL-OPAQUE-PTRS: %[[LOCSZ_IAB_PTR:[^ ]+]] = inttoptr i32 %[[LOCSZ_IAB_PTR_INT]] to ptr addrspace(6)
; XeHP-OCL-OPAQUE-PTRS-DAG: %[[LOCSZ_LSZ_X_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[LOCSZ_IAB_PTR]], i32 0, i32 4
; XeHP-OCL-OPAQUE-PTRS-DAG: %[[LOCSZ_LSZ_X:[^ ]+]] = load i32, ptr addrspace(6) %[[LOCSZ_LSZ_X_PTR]]
; XeHP-OCL-OPAQUE-PTRS-DAG: %[[LOCSZ_LSZ_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[LOCSZ_IAB_PTR]], i32 0, i32 5
; XeHP-OCL-OPAQUE-PTRS-DAG: %[[LOCSZ_LSZ_Y:[^ ]+]] = load i32, ptr addrspace(6) %[[LOCSZ_LSZ_Y_PTR]]
; XeHP-OCL-OPAQUE-PTRS-DAG: %[[LOCSZ_LSZ_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[LOCSZ_IAB_PTR]], i32 0, i32 6
; XeHP-OCL-OPAQUE-PTRS-DAG: %[[LOCSZ_LSZ_Z:[^ ]+]] = load i32, ptr addrspace(6) %[[LOCSZ_LSZ_Z_PTR]]
; XeHP-OCL: %[[LOCSZ_LSZ_PART_0:[^ ]+]] = insertelement <3 x i32> undef, i32 %[[LOCSZ_LSZ_X]], i64 0
; XeHP-OCL: %[[LOCSZ_LSZ_PART_1:[^ ]+]] = insertelement <3 x i32> %[[LOCSZ_LSZ_PART_0]], i32 %[[LOCSZ_LSZ_Y]], i64 1
; XeHP-OCL: %[[LOCSZ_LSZ:[^ ]+]] = insertelement <3 x i32> %[[LOCSZ_LSZ_PART_1]], i32 %[[LOCSZ_LSZ_Z]], i64 2
; XeHP-OCL-TYPED-PTRS: store <3 x i32> %[[LOCSZ_LSZ]], <3 x i32>* @__imparg_llvm.genx.local.size
; XeHP-OCL-OPAQUE-PTRS: store <3 x i32> %[[LOCSZ_LSZ]], ptr @__imparg_llvm.genx.local.size

; XeHPG-OCL-TYPED-PTRS: %[[LOCSZ_IAB_PTR_INT:[^ ]+]] = call i64 @llvm.vc.internal.read.variable.region.i64.p0i64(i64* @llvm.vc.predef.var.impl.args.buf, i32 0, i32 1, i32 1, i32 0)
; XeHPG-OCL-TYPED-PTRS: %[[LOCSZ_IAB_PTR:[^ ]+]] = inttoptr i64 %[[PRINTF_IAB_PTR_INT]] to %vc.implicit.args.buf.type addrspace(1)*
; XeHPG-OCL-TYPED-PTRS-DAG: %[[LOCSZ_LSZ_X_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(1)* %[[LOCSZ_IAB_PTR]], i32 0, i32 4
; XeHPG-OCL-TYPED-PTRS-DAG: %[[LOCSZ_LSZ_X:[^ ]+]] = load i32, i32 addrspace(1)* %[[LOCSZ_LSZ_X_PTR]]
; XeHPG-OCL-TYPED-PTRS-DAG: %[[LOCSZ_LSZ_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(1)* %[[LOCSZ_IAB_PTR]], i32 0, i32 5
; XeHPG-OCL-TYPED-PTRS-DAG: %[[LOCSZ_LSZ_Y:[^ ]+]] = load i32, i32 addrspace(1)* %[[LOCSZ_LSZ_Y_PTR]]
; XeHPG-OCL-TYPED-PTRS-DAG: %[[LOCSZ_LSZ_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(1)* %[[LOCSZ_IAB_PTR]], i32 0, i32 6
; XeHPG-OCL-TYPED-PTRS-DAG: %[[LOCSZ_LSZ_Z:[^ ]+]] = load i32, i32 addrspace(1)* %[[LOCSZ_LSZ_Z_PTR]]
; XeHPG-OCL-OPAQUE-PTRS: %[[LOCSZ_IAB_PTR_INT:[^ ]+]] = call i64 @llvm.vc.internal.read.variable.region.i64.p0(ptr @llvm.vc.predef.var.impl.args.buf, i32 0, i32 1, i32 1, i32 0)
; XeHPG-OCL-OPAQUE-PTRS: %[[LOCSZ_IAB_PTR:[^ ]+]] = inttoptr i64 %[[PRINTF_IAB_PTR_INT]] to ptr addrspace(1)
; XeHPG-OCL-OPAQUE-PTRS-DAG: %[[LOCSZ_LSZ_X_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(1) %[[LOCSZ_IAB_PTR]], i32 0, i32 4
; XeHPG-OCL-OPAQUE-PTRS-DAG: %[[LOCSZ_LSZ_X:[^ ]+]] = load i32, ptr addrspace(1) %[[LOCSZ_LSZ_X_PTR]]
; XeHPG-OCL-OPAQUE-PTRS-DAG: %[[LOCSZ_LSZ_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(1) %[[LOCSZ_IAB_PTR]], i32 0, i32 5
; XeHPG-OCL-OPAQUE-PTRS-DAG: %[[LOCSZ_LSZ_Y:[^ ]+]] = load i32, ptr addrspace(1) %[[LOCSZ_LSZ_Y_PTR]]
; XeHPG-OCL-OPAQUE-PTRS-DAG: %[[LOCSZ_LSZ_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(1) %[[LOCSZ_IAB_PTR]], i32 0, i32 6
; XeHPG-OCL-OPAQUE-PTRS-DAG: %[[LOCSZ_LSZ_Z:[^ ]+]] = load i32, ptr addrspace(1) %[[LOCSZ_LSZ_Z_PTR]]
; XeHPG-OCL: %[[LOCSZ_LSZ_PART_0:[^ ]+]] = insertelement <3 x i32> undef, i32 %[[LOCSZ_LSZ_X]], i64 0
; XeHPG-OCL: %[[LOCSZ_LSZ_PART_1:[^ ]+]] = insertelement <3 x i32> %[[LOCSZ_LSZ_PART_0]], i32 %[[LOCSZ_LSZ_Y]], i64 1
; XeHPG-OCL: %[[LOCSZ_LSZ:[^ ]+]] = insertelement <3 x i32> %[[LOCSZ_LSZ_PART_1]], i32 %[[LOCSZ_LSZ_Z]], i64 2
; XeHPG-OCL-TYPED-PTRS: store <3 x i32> %[[LOCSZ_LSZ]], <3 x i32>* @__imparg_llvm.genx.local.size
; XeHPG-OCL-OPAQUE-PTRS: store <3 x i32> %[[LOCSZ_LSZ]], ptr @__imparg_llvm.genx.local.size

  %wls.local.size = call <3 x i32> @llvm.genx.local.size.v3i32()
; XeHP-OCL-TYPED-PTRS: %wls.local.size = load <3 x i32>, <3 x i32>* @__imparg_llvm.genx.local.size
; XeHPG-OCL-TYPED-PTRS: %wls.local.size = load <3 x i32>, <3 x i32>* @__imparg_llvm.genx.local.size
; XeHP-OCL-OPAQUE-PTRS: %wls.local.size = load <3 x i32>, ptr @__imparg_llvm.genx.local.size
; XeHPG-OCL-OPAQUE-PTRS: %wls.local.size = load <3 x i32>, ptr @__imparg_llvm.genx.local.size
  ret void
}

define spir_func void @with_group_count() {
; COM: The signature shouldn't change.
; XeHP-OCL-LABEL: define spir_func void @with_group_count()
; XeHPG-OCL-LABEL:  define spir_func void @with_group_count()

; XeHP-OCL: %[[GRPCNT_R0:[^ ]+]] = call i32 @llvm.genx.r0.i32()
; XeHP-OCL: %[[GRPCNT_IAB_PTR_INT:[^ ]+]] = and i32 %[[GRPCNT_R0]], -64
; XeHP-OCL-TYPED-PTRS: %[[GRPCNT_IAB_PTR:[^ ]+]] = inttoptr i32 %[[GRPCNT_IAB_PTR_INT]] to %vc.implicit.args.buf.type addrspace(6)*
; XeHP-OCL-TYPED-PTRS-DAG: %[[GRPCNT_CNT_X_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[GRPCNT_IAB_PTR]], i32 0, i32 15
; XeHP-OCL-TYPED-PTRS-DAG: %[[GRPCNT_CNT_X:[^ ]+]] = load i32, i32 addrspace(6)* %[[GRPCNT_CNT_X_PTR]]
; XeHP-OCL-TYPED-PTRS-DAG: %[[GRPCNT_CNT_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[GRPCNT_IAB_PTR]], i32 0, i32 16
; XeHP-OCL-TYPED-PTRS-DAG: %[[GRPCNT_CNT_Y:[^ ]+]] = load i32, i32 addrspace(6)* %[[GRPCNT_CNT_Y_PTR]]
; XeHP-OCL-TYPED-PTRS-DAG: %[[GRPCNT_CNT_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[GRPCNT_IAB_PTR]], i32 0, i32 17
; XeHP-OCL-TYPED-PTRS-DAG: %[[GRPCNT_CNT_Z:[^ ]+]] = load i32, i32 addrspace(6)* %[[GRPCNT_CNT_Z_PTR]]
; XeHP-OCL-OPAQUE-PTRS: %[[GRPCNT_IAB_PTR:[^ ]+]] = inttoptr i32 %[[GRPCNT_IAB_PTR_INT]] to ptr addrspace(6)
; XeHP-OCL-OPAQUE-PTRS-DAG: %[[GRPCNT_CNT_X_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[GRPCNT_IAB_PTR]], i32 0, i32 15
; XeHP-OCL-OPAQUE-PTRS-DAG: %[[GRPCNT_CNT_X:[^ ]+]] = load i32, ptr addrspace(6) %[[GRPCNT_CNT_X_PTR]]
; XeHP-OCL-OPAQUE-PTRS-DAG: %[[GRPCNT_CNT_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[GRPCNT_IAB_PTR]], i32 0, i32 16
; XeHP-OCL-OPAQUE-PTRS-DAG: %[[GRPCNT_CNT_Y:[^ ]+]] = load i32, ptr addrspace(6) %[[GRPCNT_CNT_Y_PTR]]
; XeHP-OCL-OPAQUE-PTRS-DAG: %[[GRPCNT_CNT_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[GRPCNT_IAB_PTR]], i32 0, i32 17
; XeHP-OCL-OPAQUE-PTRS-DAG: %[[GRPCNT_CNT_Z:[^ ]+]] = load i32, ptr addrspace(6) %[[GRPCNT_CNT_Z_PTR]]
; XeHP-OCL: %[[GRPCNT_CNT_PART_0:[^ ]+]] = insertelement <3 x i32> undef, i32 %[[GRPCNT_CNT_X]], i64 0
; XeHP-OCL: %[[GRPCNT_CNT_PART_1:[^ ]+]] = insertelement <3 x i32> %[[GRPCNT_CNT_PART_0]], i32 %[[GRPCNT_CNT_Y]], i64 1
; XeHP-OCL: %[[GRPCNT_CNT:[^ ]+]] = insertelement <3 x i32> %[[GRPCNT_CNT_PART_1]], i32 %[[GRPCNT_CNT_Z]], i64 2
; XeHP-OCL-TYPED-PTRS: store <3 x i32> %[[GRPCNT_CNT]], <3 x i32>* @__imparg_llvm.genx.group.count
; XeHP-OCL-OPAQUE-PTRS: store <3 x i32> %[[GRPCNT_CNT]], ptr @__imparg_llvm.genx.group.count

; XeHPG-OCL-TYPED-PTRS: %[[GRPCNT_IAB_PTR_INT:[^ ]+]] = call i64 @llvm.vc.internal.read.variable.region.i64.p0i64(i64* @llvm.vc.predef.var.impl.args.buf, i32 0, i32 1, i32 1, i32 0)
; XeHPG-OCL-TYPED-PTRS: %[[GRPCNT_IAB_PTR:[^ ]+]] = inttoptr i64 %[[PRINTF_IAB_PTR_INT]] to %vc.implicit.args.buf.type addrspace(1)*
; XeHPG-OCL-TYPED-PTRS-DAG: %[[GRPCNT_CNT_X_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(1)* %[[GRPCNT_IAB_PTR]], i32 0, i32 15
; XeHPG-OCL-TYPED-PTRS-DAG: %[[GRPCNT_CNT_X:[^ ]+]] = load i32, i32 addrspace(1)* %[[GRPCNT_CNT_X_PTR]]
; XeHPG-OCL-TYPED-PTRS-DAG: %[[GRPCNT_CNT_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(1)* %[[GRPCNT_IAB_PTR]], i32 0, i32 16
; XeHPG-OCL-TYPED-PTRS-DAG: %[[GRPCNT_CNT_Y:[^ ]+]] = load i32, i32 addrspace(1)* %[[GRPCNT_CNT_Y_PTR]]
; XeHPG-OCL-TYPED-PTRS-DAG: %[[GRPCNT_CNT_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(1)* %[[GRPCNT_IAB_PTR]], i32 0, i32 17
; XeHPG-OCL-TYPED-PTRS-DAG: %[[GRPCNT_CNT_Z:[^ ]+]] = load i32, i32 addrspace(1)* %[[GRPCNT_CNT_Z_PTR]]
; XeHPG-OCL-OPAQUE-PTRS: %[[GRPCNT_IAB_PTR_INT:[^ ]+]] = call i64 @llvm.vc.internal.read.variable.region.i64.p0(ptr @llvm.vc.predef.var.impl.args.buf, i32 0, i32 1, i32 1, i32 0)
; XeHPG-OCL-OPAQUE-PTRS: %[[GRPCNT_IAB_PTR:[^ ]+]] = inttoptr i64 %[[PRINTF_IAB_PTR_INT]] to ptr addrspace(1)
; XeHPG-OCL-OPAQUE-PTRS-DAG: %[[GRPCNT_CNT_X_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(1) %[[GRPCNT_IAB_PTR]], i32 0, i32 15
; XeHPG-OCL-OPAQUE-PTRS-DAG: %[[GRPCNT_CNT_X:[^ ]+]] = load i32, ptr addrspace(1) %[[GRPCNT_CNT_X_PTR]]
; XeHPG-OCL-OPAQUE-PTRS-DAG: %[[GRPCNT_CNT_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(1) %[[GRPCNT_IAB_PTR]], i32 0, i32 16
; XeHPG-OCL-OPAQUE-PTRS-DAG: %[[GRPCNT_CNT_Y:[^ ]+]] = load i32, ptr addrspace(1) %[[GRPCNT_CNT_Y_PTR]]
; XeHPG-OCL-OPAQUE-PTRS-DAG: %[[GRPCNT_CNT_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(1) %[[GRPCNT_IAB_PTR]], i32 0, i32 17
; XeHPG-OCL-OPAQUE-PTRS-DAG: %[[GRPCNT_CNT_Z:[^ ]+]] = load i32, ptr addrspace(1) %[[GRPCNT_CNT_Z_PTR]]
; XeHPG-OCL: %[[GRPCNT_CNT_PART_0:[^ ]+]] = insertelement <3 x i32> undef, i32 %[[GRPCNT_CNT_X]], i64 0
; XeHPG-OCL: %[[GRPCNT_CNT_PART_1:[^ ]+]] = insertelement <3 x i32> %[[GRPCNT_CNT_PART_0]], i32 %[[GRPCNT_CNT_Y]], i64 1
; XeHPG-OCL: %[[GRPCNT_CNT:[^ ]+]] = insertelement <3 x i32> %[[GRPCNT_CNT_PART_1]], i32 %[[GRPCNT_CNT_Z]], i64 2
; XeHPG-OCL-TYPED-PTRS: store <3 x i32> %[[GRPCNT_CNT]], <3 x i32>* @__imparg_llvm.genx.group.count
; XeHPG-OCL-OPAQUE-PTRS: store <3 x i32> %[[GRPCNT_CNT]], ptr @__imparg_llvm.genx.group.count

  %wgc.group.count = call <3 x i32> @llvm.genx.group.count.v3i32()
; XeHP-OCL-TYPED-PTRS: %wgc.group.count = load <3 x i32>, <3 x i32>* @__imparg_llvm.genx.group.count
; XeHPG-OCL-TYPED-PTRS:  %wgc.group.count = load <3 x i32>, <3 x i32>* @__imparg_llvm.genx.group.count
; XeHP-OCL-OPAQUE-PTRS: %wgc.group.count = load <3 x i32>, ptr @__imparg_llvm.genx.group.count
; XeHPG-OCL-OPAQUE-PTRS:  %wgc.group.count = load <3 x i32>, ptr @__imparg_llvm.genx.group.count
  ret void
}

; XeHP-OCL: attributes #[[KERNEL_ATTR]] = {
; XeHP-OCL-SAME: "RequiresImplArgsBuffer"
; XeHPG-OCL: attributes #[[PREDEF_VAR_ATTR]] = {
; XeHPG-OCL-SAME: "VCPredefinedVariable"
; XeHPG-OCL: attributes #[[KERNEL_ATTR]] = {
; XeHPG-OCL-SAME: "RequiresImplArgsBuffer"

!genx.kernels = !{!0}

!0 = !{void ()* @kernel, !"kernel", !1, i32 0, i32 0, !1, !1, i32 0, i32 0}
!1 = !{}
