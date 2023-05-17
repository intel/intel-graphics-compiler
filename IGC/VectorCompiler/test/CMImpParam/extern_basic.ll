;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmimpparam -cmimpparam-cmrt=false -march=genx64 -mcpu=XeHP -S < %s \
; RUN:    | FileCheck --check-prefix=XeHP-OCL %s
; RUN: %not_for_vc_diag% opt %use_old_pass_manager% -cmimpparam -cmimpparam-cmrt=true -march=genx64 -mcpu=XeHP -S \
; RUN:    < %s 2>&1 | FileCheck --check-prefix=XeHP-CM %s
; RUN: opt %use_old_pass_manager% -cmimpparam -cmimpparam-cmrt=false -cmimpparam-payload-in-memory=false \
; RUN:    -march=genx64 -mcpu=Gen9 -S < %s | FileCheck --check-prefix=Gen9-OCL %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; XeHP-CM: Implicit arguments buffer is required but it is not supported for CM RT

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
; Gen9-OCL-DAG:  %vc.implicit.args.buf.type = type { {{.*}} }

; Gen9-OCL-DAG: @llvm.vc.predef.var.impl.args.buf = external global i64 #[[PREDEF_VAR_ATTR:[0-9]+]]

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
; Gen9-OCL: define dllexport spir_kernel void @kernel(
; Gen9-OCL-NOT: i64 %impl.arg.llvm.vc.internal.print.buffer
; Gen9-OCL-NOT: <3 x i32> %impl.arg.llvm.genx.local.size
; Gen9-OCL-NOT: <3 x i32> %impl.arg.llvm.genx.group.count
; Gen9-OCL-DAG: i64 %impl.arg.impl.args.buffer
; Gen9-OCL-SAME: ) #[[KERNEL_ATTR:[0-9]+]]
; Gen9-OCL-NEXT: call void @llvm.vc.internal.write.variable.region.p0i64.i64.i1(i64* @llvm.vc.predef.var.impl.args.buf, i64 %impl.arg.impl.args.buffer, i32 1, i32 0, i1 true)
  call spir_func void @with_printf()
  call spir_func void @with_local_size()
  call spir_func void @with_group_count()
  ret void
}

define spir_func void @with_printf() {
; COM: The signature shouldn't change.
; XeHP-OCL-LABEL: define spir_func void @with_printf()
; Gen9-OCL-LABEL:  define spir_func void @with_printf()

; COM: Next 3 instructions obtain implicit args buffer (IAB) pointer:
; XeHP-OCL: %[[PRINTF_R0:[^ ]+]] = call i32 @llvm.genx.r0.i32()
; XeHP-OCL: %[[PRINTF_IAB_PTR_INT:[^ ]+]] = and i32 %[[PRINTF_R0]], -64
; XeHP-OCL: %[[PRINTF_IAB_PTR:[^ ]+]] = inttoptr i32 %[[PRINTF_IAB_PTR_INT]] to %vc.implicit.args.buf.type addrspace(6)*

; COM: Next 2 instructions obtain implicit args buffer (IAB) pointer:
; Gen9-OCL: %[[PRINTF_IAB_PTR_INT:[^ ]+]] = call i64 @llvm.vc.internal.read.variable.region.i64.p0i64(i64* @llvm.vc.predef.var.impl.args.buf, i32 0, i32 1, i32 1, i32 0)
; Gen9-OCL: %[[PRINTF_IAB_PTR:[^ ]+]] = inttoptr i64 %[[PRINTF_IAB_PTR_INT]] to %vc.implicit.args.buf.type addrspace(1)*

; COM: Loading printf buffer ptr from IAB and storing it to implict arg global:
; XeHP-OCL: %[[PRINTF_PBP_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[PRINTF_IAB_PTR]], i32 0, i32 10
; XeHP-OCL: %[[PRINTF_PBP:[^ ]+]] = load i64, i64 addrspace(6)* %[[PRINTF_PBP_PTR]]
; XeHP-OCL: store i64 %[[PRINTF_PBP]], i64* @__imparg_llvm.vc.internal.print.buffer

; COM: Loading printf buffer ptr from IAB and storing it to implict arg global:
; Gen9-OCL: %[[PRINTF_PBP_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(1)* %[[PRINTF_IAB_PTR]], i32 0, i32 10
; Gen9-OCL: %[[PRINTF_PBP:[^ ]+]] = load i64, i64 addrspace(1)* %[[PRINTF_PBP_PTR]]
; Gen9-OCL: store i64 %[[PRINTF_PBP]], i64* @__imparg_llvm.vc.internal.print.buffer

  %wp.print = call i64 @llvm.vc.internal.print.buffer()
; XeHP-OCL: %wp.print = load i64, i64* @__imparg_llvm.vc.internal.print.buffer
; Gen9-OCL:  %wp.print = load i64, i64* @__imparg_llvm.vc.internal.print.buffer
  ret void
}

define spir_func void @with_local_size() {
; COM: The signature shouldn't change.
; XeHP-OCL-LABEL: define spir_func void @with_local_size()
; Gen9-OCL-LABEL:  define spir_func void @with_local_size()

; XeHP-OCL: %[[LOCSZ_R0:[^ ]+]] = call i32 @llvm.genx.r0.i32()
; XeHP-OCL: %[[LOCSZ_IAB_PTR_INT:[^ ]+]] = and i32 %[[LOCSZ_R0]], -64
; XeHP-OCL: %[[LOCSZ_IAB_PTR:[^ ]+]] = inttoptr i32 %[[LOCSZ_IAB_PTR_INT]] to %vc.implicit.args.buf.type addrspace(6)*
; XeHP-OCL-DAG: %[[LOCSZ_LSZ_X_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[LOCSZ_IAB_PTR]], i32 0, i32 4
; XeHP-OCL-DAG: %[[LOCSZ_LSZ_X:[^ ]+]] = load i32, i32 addrspace(6)* %[[LOCSZ_LSZ_X_PTR]]
; XeHP-OCL-DAG: %[[LOCSZ_LSZ_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[LOCSZ_IAB_PTR]], i32 0, i32 5
; XeHP-OCL-DAG: %[[LOCSZ_LSZ_Y:[^ ]+]] = load i32, i32 addrspace(6)* %[[LOCSZ_LSZ_Y_PTR]]
; XeHP-OCL-DAG: %[[LOCSZ_LSZ_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[LOCSZ_IAB_PTR]], i32 0, i32 6
; XeHP-OCL-DAG: %[[LOCSZ_LSZ_Z:[^ ]+]] = load i32, i32 addrspace(6)* %[[LOCSZ_LSZ_Z_PTR]]
; XeHP-OCL: %[[LOCSZ_LSZ_PART_0:[^ ]+]] = insertelement <3 x i32> undef, i32 %[[LOCSZ_LSZ_X]], i64 0
; XeHP-OCL: %[[LOCSZ_LSZ_PART_1:[^ ]+]] = insertelement <3 x i32> %[[LOCSZ_LSZ_PART_0]], i32 %[[LOCSZ_LSZ_Y]], i64 1
; XeHP-OCL: %[[LOCSZ_LSZ:[^ ]+]] = insertelement <3 x i32> %[[LOCSZ_LSZ_PART_1]], i32 %[[LOCSZ_LSZ_Z]], i64 2
; XeHP-OCL: store <3 x i32> %[[LOCSZ_LSZ]], <3 x i32>* @__imparg_llvm.genx.local.size

; Gen9-OCL: %[[LOCSZ_IAB_PTR_INT:[^ ]+]] = call i64 @llvm.vc.internal.read.variable.region.i64.p0i64(i64* @llvm.vc.predef.var.impl.args.buf, i32 0, i32 1, i32 1, i32 0)
; Gen9-OCL: %[[LOCSZ_IAB_PTR:[^ ]+]] = inttoptr i64 %[[PRINTF_IAB_PTR_INT]] to %vc.implicit.args.buf.type addrspace(1)*
; Gen9-OCL-DAG: %[[LOCSZ_LSZ_X_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(1)* %[[LOCSZ_IAB_PTR]], i32 0, i32 4
; Gen9-OCL-DAG: %[[LOCSZ_LSZ_X:[^ ]+]] = load i32, i32 addrspace(1)* %[[LOCSZ_LSZ_X_PTR]]
; Gen9-OCL-DAG: %[[LOCSZ_LSZ_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(1)* %[[LOCSZ_IAB_PTR]], i32 0, i32 5
; Gen9-OCL-DAG: %[[LOCSZ_LSZ_Y:[^ ]+]] = load i32, i32 addrspace(1)* %[[LOCSZ_LSZ_Y_PTR]]
; Gen9-OCL-DAG: %[[LOCSZ_LSZ_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(1)* %[[LOCSZ_IAB_PTR]], i32 0, i32 6
; Gen9-OCL-DAG: %[[LOCSZ_LSZ_Z:[^ ]+]] = load i32, i32 addrspace(1)* %[[LOCSZ_LSZ_Z_PTR]]
; Gen9-OCL: %[[LOCSZ_LSZ_PART_0:[^ ]+]] = insertelement <3 x i32> undef, i32 %[[LOCSZ_LSZ_X]], i64 0
; Gen9-OCL: %[[LOCSZ_LSZ_PART_1:[^ ]+]] = insertelement <3 x i32> %[[LOCSZ_LSZ_PART_0]], i32 %[[LOCSZ_LSZ_Y]], i64 1
; Gen9-OCL: %[[LOCSZ_LSZ:[^ ]+]] = insertelement <3 x i32> %[[LOCSZ_LSZ_PART_1]], i32 %[[LOCSZ_LSZ_Z]], i64 2
; Gen9-OCL: store <3 x i32> %[[LOCSZ_LSZ]], <3 x i32>* @__imparg_llvm.genx.local.size

  %wls.local.size = call <3 x i32> @llvm.genx.local.size.v3i32()
; XeHP-OCL: %wls.local.size = load <3 x i32>, <3 x i32>* @__imparg_llvm.genx.local.size
; Gen9-OCL:  %wls.local.size = load <3 x i32>, <3 x i32>* @__imparg_llvm.genx.local.size
  ret void
}

define spir_func void @with_group_count() {
; COM: The signature shouldn't change.
; XeHP-OCL-LABEL: define spir_func void @with_group_count()
; Gen9-OCL-LABEL:  define spir_func void @with_group_count()

; XeHP-OCL: %[[GRPCNT_R0:[^ ]+]] = call i32 @llvm.genx.r0.i32()
; XeHP-OCL: %[[GRPCNT_IAB_PTR_INT:[^ ]+]] = and i32 %[[GRPCNT_R0]], -64
; XeHP-OCL: %[[GRPCNT_IAB_PTR:[^ ]+]] = inttoptr i32 %[[GRPCNT_IAB_PTR_INT]] to %vc.implicit.args.buf.type addrspace(6)*
; XeHP-OCL-DAG: %[[GRPCNT_CNT_X_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[GRPCNT_IAB_PTR]], i32 0, i32 15
; XeHP-OCL-DAG: %[[GRPCNT_CNT_X:[^ ]+]] = load i32, i32 addrspace(6)* %[[GRPCNT_CNT_X_PTR]]
; XeHP-OCL-DAG: %[[GRPCNT_CNT_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[GRPCNT_IAB_PTR]], i32 0, i32 16
; XeHP-OCL-DAG: %[[GRPCNT_CNT_Y:[^ ]+]] = load i32, i32 addrspace(6)* %[[GRPCNT_CNT_Y_PTR]]
; XeHP-OCL-DAG: %[[GRPCNT_CNT_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[GRPCNT_IAB_PTR]], i32 0, i32 17
; XeHP-OCL-DAG: %[[GRPCNT_CNT_Z:[^ ]+]] = load i32, i32 addrspace(6)* %[[GRPCNT_CNT_Z_PTR]]
; XeHP-OCL: %[[GRPCNT_CNT_PART_0:[^ ]+]] = insertelement <3 x i32> undef, i32 %[[GRPCNT_CNT_X]], i64 0
; XeHP-OCL: %[[GRPCNT_CNT_PART_1:[^ ]+]] = insertelement <3 x i32> %[[GRPCNT_CNT_PART_0]], i32 %[[GRPCNT_CNT_Y]], i64 1
; XeHP-OCL: %[[GRPCNT_CNT:[^ ]+]] = insertelement <3 x i32> %[[GRPCNT_CNT_PART_1]], i32 %[[GRPCNT_CNT_Z]], i64 2
; XeHP-OCL: store <3 x i32> %[[GRPCNT_CNT]], <3 x i32>* @__imparg_llvm.genx.group.count

; Gen9-OCL: %[[GRPCNT_IAB_PTR_INT:[^ ]+]] = call i64 @llvm.vc.internal.read.variable.region.i64.p0i64(i64* @llvm.vc.predef.var.impl.args.buf, i32 0, i32 1, i32 1, i32 0)
; Gen9-OCL: %[[GRPCNT_IAB_PTR:[^ ]+]] = inttoptr i64 %[[PRINTF_IAB_PTR_INT]] to %vc.implicit.args.buf.type addrspace(1)*
; Gen9-OCL-DAG: %[[GRPCNT_CNT_X_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(1)* %[[GRPCNT_IAB_PTR]], i32 0, i32 15
; Gen9-OCL-DAG: %[[GRPCNT_CNT_X:[^ ]+]] = load i32, i32 addrspace(1)* %[[GRPCNT_CNT_X_PTR]]
; Gen9-OCL-DAG: %[[GRPCNT_CNT_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(1)* %[[GRPCNT_IAB_PTR]], i32 0, i32 16
; Gen9-OCL-DAG: %[[GRPCNT_CNT_Y:[^ ]+]] = load i32, i32 addrspace(1)* %[[GRPCNT_CNT_Y_PTR]]
; Gen9-OCL-DAG: %[[GRPCNT_CNT_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(1)* %[[GRPCNT_IAB_PTR]], i32 0, i32 17
; Gen9-OCL-DAG: %[[GRPCNT_CNT_Z:[^ ]+]] = load i32, i32 addrspace(1)* %[[GRPCNT_CNT_Z_PTR]]
; Gen9-OCL: %[[GRPCNT_CNT_PART_0:[^ ]+]] = insertelement <3 x i32> undef, i32 %[[GRPCNT_CNT_X]], i64 0
; Gen9-OCL: %[[GRPCNT_CNT_PART_1:[^ ]+]] = insertelement <3 x i32> %[[GRPCNT_CNT_PART_0]], i32 %[[GRPCNT_CNT_Y]], i64 1
; Gen9-OCL: %[[GRPCNT_CNT:[^ ]+]] = insertelement <3 x i32> %[[GRPCNT_CNT_PART_1]], i32 %[[GRPCNT_CNT_Z]], i64 2
; Gen9-OCL: store <3 x i32> %[[GRPCNT_CNT]], <3 x i32>* @__imparg_llvm.genx.group.count

  %wgc.group.count = call <3 x i32> @llvm.genx.group.count.v3i32()
; XeHP-OCL: %wgc.group.count = load <3 x i32>, <3 x i32>* @__imparg_llvm.genx.group.count
; Gen9-OCL:  %wgc.group.count = load <3 x i32>, <3 x i32>* @__imparg_llvm.genx.group.count
  ret void
}

; XeHP-OCL: attributes #[[KERNEL_ATTR]] = {
; XeHP-OCL-SAME: "RequiresImplArgsBuffer"
; Gen9-OCL: attributes #[[PREDEF_VAR_ATTR]] = {
; Gen9-OCL-SAME: "VCPredefinedVariable"
; Gen9-OCL: attributes #[[KERNEL_ATTR]] = {
; Gen9-OCL-SAME: "RequiresImplArgsBuffer"

!genx.kernels = !{!0}

!0 = !{void ()* @kernel, !"kernel", !1, i32 0, i32 0, !1, !1, i32 0, i32 0}
!1 = !{}
