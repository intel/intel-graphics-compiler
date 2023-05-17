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
;XeHP-OCL-DAG: %vc.implicit.args.buf.type = type { {{.*}} }

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
  call spir_func void @with_printf()
  call spir_func void @with_local_size()
  call spir_func void @with_group_count()
  ret void
}

define spir_func void @with_printf() {
; COM: The signature shouldn't change.
; XeHP-OCL-LABEL: define spir_func void @with_printf()

; COM: Next 3 instructions obtain implicit args buffer (IAB) pointer:
; XeHP-OCL: %[[PRINTF_R0:[^ ]+]] = call i32 @llvm.genx.r0.i32()
; XeHP-OCL: %[[PRINTF_IAB_PTR_INT:[^ ]+]] = and i32 %[[PRINTF_R0]], -64
; XeHP-OCL: %[[PRINTF_IAB_PTR:[^ ]+]] = inttoptr i32 %[[PRINTF_IAB_PTR_INT]] to %vc.implicit.args.buf.type addrspace(6)*

; COM: Loading printf buffer ptr from IAB and storing it to implict arg global:
; XeHP-OCL: %[[PRINTF_PBP_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[PRINTF_IAB_PTR]], i32 0, i32 10
; XeHP-OCL: %[[PRINTF_PBP:[^ ]+]] = load i64, i64 addrspace(6)* %[[PRINTF_PBP_PTR]]
; XeHP-OCL: store i64 %[[PRINTF_PBP]], i64* @__imparg_llvm.vc.internal.print.buffer

  call spir_func void @with_printf_impl()
  ret void
}

define internal spir_func void @with_printf_impl() {
  %wpi.print = call i64 @llvm.vc.internal.print.buffer()
  ret void
}

define spir_func void @with_local_size() {
; COM: The signature shouldn't change.
; XeHP-OCL-LABEL: define spir_func void @with_local_size()
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

  call spir_func void @with_local_size_impl()
  ret void
}

define internal spir_func void @with_local_size_impl() {
  %wlsi.local.size = call <3 x i32> @llvm.genx.local.size.v3i32()
  ret void
}

define spir_func void @with_group_count() {
; COM: The signature shouldn't change.
; XeHP-OCL-LABEL: define spir_func void @with_group_count()
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

  call spir_func void @with_group_count_impl()
  ret void
}

define internal spir_func void @with_group_count_impl() {
  %wgci.group.count = call <3 x i32> @llvm.genx.group.count.v3i32()
  ret void
}

define spir_func void @extern_calls_extern() {
  call spir_func void @with_printf()
  call spir_func void @with_local_size()
  call spir_func void @with_group_count()
  ret void
}
; COM: this function shouldn't change at all.
; XeHP-OCL: define spir_func void @extern_calls_extern()
; XeHP-OCL-NEXT: call spir_func void @with_printf()
; XeHP-OCL-NEXT: call spir_func void @with_local_size()
; XeHP-OCL-NEXT: call spir_func void @with_group_count()
; XeHP-OCL-NEXT: ret void

define spir_func void @extern_calls_extern_and_printf() {
  call spir_func void @with_printf_impl()
  call spir_func void @with_local_size()
  call spir_func void @with_group_count()
  ret void
}
; COM: only printf buffer pointer should be loaded
; XeHP-OCL: define spir_func void @extern_calls_extern_and_printf()
; XeHP-OCL-NEXT: %[[ECEAP_R0:[^ ]+]] = call i32 @llvm.genx.r0.i32()
; XeHP-OCL-NEXT: %[[ECEAP_IAB_PTR_INT:[^ ]+]] = and i32 %[[ECEAP_R0]], -64
; XeHP-OCL-NEXT: %[[ECEAP_IAB_PTR:[^ ]+]] = inttoptr i32 %[[ECEAP_IAB_PTR_INT]] to %vc.implicit.args.buf.type addrspace(6)*

; XeHP-OCL-NEXT: %[[ECEAP_PBP_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[ECEAP_IAB_PTR]], i32 0, i32 10
; XeHP-OCL-NEXT: %[[ECEAP_PBP:[^ ]+]] = load i64, i64 addrspace(6)* %[[ECEAP_PBP_PTR]]
; XeHP-OCL-NEXT: store i64 %[[ECEAP_PBP]], i64* @__imparg_llvm.vc.internal.print.buffer

; XeHP-OCL-NEXT: call spir_func void @with_printf_impl()
; XeHP-OCL-NEXT: call spir_func void @with_local_size()
; XeHP-OCL-NEXT: call spir_func void @with_group_count()
; XeHP-OCL-NEXT: ret void

; XeHP-OCL: attributes #[[KERNEL_ATTR]] = {
; XeHP-OCL-SAME: "RequiresImplArgsBuffer"

!genx.kernels = !{!0}

!0 = !{void ()* @kernel, !"kernel", !1, i32 0, !1, !1, !1, i32 0, i32 0}
!1 = !{}
