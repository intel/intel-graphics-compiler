;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_legacy_typed %use_old_pass_manager% -CMImpParam -march=genx64 -mcpu=XeLP -S < %s \
; RUN:    | FileCheck %s --check-prefixes=XeLP-OCL,XeLP-OCL-TYPED-PTRS
; RUN: %opt_legacy_opaque %use_old_pass_manager% -CMImpParam -march=genx64 -mcpu=XeLP -S < %s \
; RUN:    | FileCheck %s --check-prefixes=XeLP-OCL,XeLP-OCL-OPAQUE-PTRS

; RUN: %opt_new_pm_typed -passes=CMImpParam -march=genx64 -mcpu=XeLP -S < %s \
; RUN:    | FileCheck %s --check-prefixes=XeLP-OCL,XeLP-OCL-TYPED-PTRS
; RUN: %opt_new_pm_opaque -passes=CMImpParam -march=genx64 -mcpu=XeLP -S < %s \
; RUN:    | FileCheck %s --check-prefixes=XeLP-OCL,XeLP-OCL-OPAQUE-PTRS

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
;XeLP-OCL-DAG: %vc.implicit.args.buf.type = type { {{.*}} }

declare <3 x i16> @llvm.genx.local.id16.v3i16()
declare <3 x i32> @llvm.genx.local.size.v3i32()
declare i32 @llvm.genx.group.id.x()
declare i32 @llvm.genx.group.id.y()
declare i32 @llvm.genx.group.id.z()
declare <3 x i32> @llvm.genx.group.count.v3i32()
declare i64 @llvm.vc.internal.print.buffer()

; COM: Printf buffer is used only in extern function
define dllexport spir_kernel void @kernel() {
; XeLP-OCL: define dllexport spir_kernel void @kernel(
; XeLP-OCL-NOT: i64 %impl.arg.llvm.vc.internal.print.buffer
; XeLP-OCL-NOT: <3 x i32> %impl.arg.llvm.genx.local.size
; XeLP-OCL-NOT: <3 x i32> %impl.arg.llvm.genx.group.count
; XeLP-OCL-SAME: ) #[[KERNEL_ATTR:[0-9]+]]
  call spir_func void @with_printf()
  call spir_func void @with_local_size()
  call spir_func void @with_group_count()
  ret void
}

define spir_func void @with_printf() {
; COM: The signature shouldn't change.
; XeLP-OCL-LABEL: define spir_func void @with_printf()

; COM: Next 3 instructions obtain implicit args buffer (IAB) pointer:
; XeLP-OCL: %[[PRINTF_R0:[^ ]+]] = call i32 @llvm.genx.r0.i32()
; XeLP-OCL: %[[PRINTF_IAB_PTR_INT:[^ ]+]] = and i32 %[[PRINTF_R0]], -64
; XeLP-OCL-TYPED-PTRS: %[[PRINTF_IAB_PTR:[^ ]+]] = inttoptr i32 %[[PRINTF_IAB_PTR_INT]] to %vc.implicit.args.buf.type addrspace(6)*
; XeLP-OCL-OPAQUE-PTRS: %[[PRINTF_IAB_PTR:[^ ]+]] = inttoptr i32 %[[PRINTF_IAB_PTR_INT]] to ptr addrspace(6)

; COM: Loading printf buffer ptr from IAB and storing it to implict arg global:
; XeLP-OCL-TYPED-PTRS: %[[PRINTF_PBP_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[PRINTF_IAB_PTR]], i32 0, i32 10
; XeLP-OCL-TYPED-PTRS: %[[PRINTF_PBP:[^ ]+]] = load i64, i64 addrspace(6)* %[[PRINTF_PBP_PTR]]
; XeLP-OCL-TYPED-PTRS: store i64 %[[PRINTF_PBP]], i64* @__imparg_llvm.vc.internal.print.buffer
; XeLP-OCL-OPAQUE-PTRS: %[[PRINTF_PBP_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[PRINTF_IAB_PTR]], i32 0, i32 10
; XeLP-OCL-OPAQUE-PTRS: %[[PRINTF_PBP:[^ ]+]] = load i64, ptr addrspace(6) %[[PRINTF_PBP_PTR]]
; XeLP-OCL-OPAQUE-PTRS: store i64 %[[PRINTF_PBP]], ptr @__imparg_llvm.vc.internal.print.buffer

  call spir_func void @with_printf_impl()
  ret void
}

define internal spir_func void @with_printf_impl() {
  %wpi.print = call i64 @llvm.vc.internal.print.buffer()
  ret void
}

define spir_func void @with_local_size() {
; COM: The signature shouldn't change.
; XeLP-OCL-LABEL: define spir_func void @with_local_size()
; XeLP-OCL: %[[LOCSZ_R0:[^ ]+]] = call i32 @llvm.genx.r0.i32()
; XeLP-OCL: %[[LOCSZ_IAB_PTR_INT:[^ ]+]] = and i32 %[[LOCSZ_R0]], -64
; XeLP-OCL-TYPED-PTRS: %[[LOCSZ_IAB_PTR:[^ ]+]] = inttoptr i32 %[[LOCSZ_IAB_PTR_INT]] to %vc.implicit.args.buf.type addrspace(6)*
; XeLP-OCL-TYPED-PTRS-DAG: %[[LOCSZ_LSZ_X_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[LOCSZ_IAB_PTR]], i32 0, i32 4
; XeLP-OCL-TYPED-PTRS-DAG: %[[LOCSZ_LSZ_X:[^ ]+]] = load i32, i32 addrspace(6)* %[[LOCSZ_LSZ_X_PTR]]
; XeLP-OCL-TYPED-PTRS-DAG: %[[LOCSZ_LSZ_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[LOCSZ_IAB_PTR]], i32 0, i32 5
; XeLP-OCL-TYPED-PTRS-DAG: %[[LOCSZ_LSZ_Y:[^ ]+]] = load i32, i32 addrspace(6)* %[[LOCSZ_LSZ_Y_PTR]]
; XeLP-OCL-TYPED-PTRS-DAG: %[[LOCSZ_LSZ_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[LOCSZ_IAB_PTR]], i32 0, i32 6
; XeLP-OCL-TYPED-PTRS-DAG: %[[LOCSZ_LSZ_Z:[^ ]+]] = load i32, i32 addrspace(6)* %[[LOCSZ_LSZ_Z_PTR]]
; XeLP-OCL-OPAQUE-PTRS: %[[LOCSZ_IAB_PTR:[^ ]+]] = inttoptr i32 %[[LOCSZ_IAB_PTR_INT]] to ptr addrspace(6)
; XeLP-OCL-OPAQUE-PTRS-DAG: %[[LOCSZ_LSZ_X_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[LOCSZ_IAB_PTR]], i32 0, i32 4
; XeLP-OCL-OPAQUE-PTRS-DAG: %[[LOCSZ_LSZ_X:[^ ]+]] = load i32, ptr addrspace(6) %[[LOCSZ_LSZ_X_PTR]]
; XeLP-OCL-OPAQUE-PTRS-DAG: %[[LOCSZ_LSZ_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[LOCSZ_IAB_PTR]], i32 0, i32 5
; XeLP-OCL-OPAQUE-PTRS-DAG: %[[LOCSZ_LSZ_Y:[^ ]+]] = load i32, ptr addrspace(6) %[[LOCSZ_LSZ_Y_PTR]]
; XeLP-OCL-OPAQUE-PTRS-DAG: %[[LOCSZ_LSZ_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[LOCSZ_IAB_PTR]], i32 0, i32 6
; XeLP-OCL-OPAQUE-PTRS-DAG: %[[LOCSZ_LSZ_Z:[^ ]+]] = load i32, ptr addrspace(6) %[[LOCSZ_LSZ_Z_PTR]]
; XeLP-OCL: %[[LOCSZ_LSZ_PART_0:[^ ]+]] = insertelement <3 x i32> undef, i32 %[[LOCSZ_LSZ_X]], i64 0
; XeLP-OCL: %[[LOCSZ_LSZ_PART_1:[^ ]+]] = insertelement <3 x i32> %[[LOCSZ_LSZ_PART_0]], i32 %[[LOCSZ_LSZ_Y]], i64 1
; XeLP-OCL: %[[LOCSZ_LSZ:[^ ]+]] = insertelement <3 x i32> %[[LOCSZ_LSZ_PART_1]], i32 %[[LOCSZ_LSZ_Z]], i64 2
; XeLP-OCL-TYPED-PTRS: store <3 x i32> %[[LOCSZ_LSZ]], <3 x i32>* @__imparg_llvm.genx.local.size
; XeLP-OCL-OPAQUE-PTRS: store <3 x i32> %[[LOCSZ_LSZ]], ptr @__imparg_llvm.genx.local.size

  call spir_func void @with_local_size_impl()
  ret void
}

define internal spir_func void @with_local_size_impl() {
  %wlsi.local.size = call <3 x i32> @llvm.genx.local.size.v3i32()
  ret void
}

define spir_func void @with_group_count() {
; COM: The signature shouldn't change.
; XeLP-OCL-LABEL: define spir_func void @with_group_count()
; XeLP-OCL: %[[GRPCNT_R0:[^ ]+]] = call i32 @llvm.genx.r0.i32()
; XeLP-OCL: %[[GRPCNT_IAB_PTR_INT:[^ ]+]] = and i32 %[[GRPCNT_R0]], -64
; XeLP-OCL-TYPED-PTRS: %[[GRPCNT_IAB_PTR:[^ ]+]] = inttoptr i32 %[[GRPCNT_IAB_PTR_INT]] to %vc.implicit.args.buf.type addrspace(6)*
; XeLP-OCL-TYPED-PTRS-DAG: %[[GRPCNT_CNT_X_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[GRPCNT_IAB_PTR]], i32 0, i32 15
; XeLP-OCL-TYPED-PTRS-DAG: %[[GRPCNT_CNT_X:[^ ]+]] = load i32, i32 addrspace(6)* %[[GRPCNT_CNT_X_PTR]]
; XeLP-OCL-TYPED-PTRS-DAG: %[[GRPCNT_CNT_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[GRPCNT_IAB_PTR]], i32 0, i32 16
; XeLP-OCL-TYPED-PTRS-DAG: %[[GRPCNT_CNT_Y:[^ ]+]] = load i32, i32 addrspace(6)* %[[GRPCNT_CNT_Y_PTR]]
; XeLP-OCL-TYPED-PTRS-DAG: %[[GRPCNT_CNT_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[GRPCNT_IAB_PTR]], i32 0, i32 17
; XeLP-OCL-TYPED-PTRS-DAG: %[[GRPCNT_CNT_Z:[^ ]+]] = load i32, i32 addrspace(6)* %[[GRPCNT_CNT_Z_PTR]]
; XeLP-OCL-OPAQUE-PTRS: %[[GRPCNT_IAB_PTR:[^ ]+]] = inttoptr i32 %[[GRPCNT_IAB_PTR_INT]] to ptr addrspace(6)
; XeLP-OCL-OPAQUE-PTRS-DAG: %[[GRPCNT_CNT_X_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[GRPCNT_IAB_PTR]], i32 0, i32 15
; XeLP-OCL-OPAQUE-PTRS-DAG: %[[GRPCNT_CNT_X:[^ ]+]] = load i32, ptr addrspace(6) %[[GRPCNT_CNT_X_PTR]]
; XeLP-OCL-OPAQUE-PTRS-DAG: %[[GRPCNT_CNT_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[GRPCNT_IAB_PTR]], i32 0, i32 16
; XeLP-OCL-OPAQUE-PTRS-DAG: %[[GRPCNT_CNT_Y:[^ ]+]] = load i32, ptr addrspace(6) %[[GRPCNT_CNT_Y_PTR]]
; XeLP-OCL-OPAQUE-PTRS-DAG: %[[GRPCNT_CNT_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[GRPCNT_IAB_PTR]], i32 0, i32 17
; XeLP-OCL-OPAQUE-PTRS-DAG: %[[GRPCNT_CNT_Z:[^ ]+]] = load i32, ptr addrspace(6) %[[GRPCNT_CNT_Z_PTR]]
; XeLP-OCL: %[[GRPCNT_CNT_PART_0:[^ ]+]] = insertelement <3 x i32> undef, i32 %[[GRPCNT_CNT_X]], i64 0
; XeLP-OCL: %[[GRPCNT_CNT_PART_1:[^ ]+]] = insertelement <3 x i32> %[[GRPCNT_CNT_PART_0]], i32 %[[GRPCNT_CNT_Y]], i64 1
; XeLP-OCL: %[[GRPCNT_CNT:[^ ]+]] = insertelement <3 x i32> %[[GRPCNT_CNT_PART_1]], i32 %[[GRPCNT_CNT_Z]], i64 2
; XeLP-OCL-TYPED-PTRS: store <3 x i32> %[[GRPCNT_CNT]], <3 x i32>* @__imparg_llvm.genx.group.count
; XeLP-OCL-OPAQUE-PTRS: store <3 x i32> %[[GRPCNT_CNT]], ptr @__imparg_llvm.genx.group.count

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
; XeLP-OCL: define spir_func void @extern_calls_extern()
; XeLP-OCL-NEXT: call spir_func void @with_printf()
; XeLP-OCL-NEXT: call spir_func void @with_local_size()
; XeLP-OCL-NEXT: call spir_func void @with_group_count()
; XeLP-OCL-NEXT: ret void

define spir_func void @extern_calls_extern_and_printf() {
  call spir_func void @with_printf_impl()
  call spir_func void @with_local_size()
  call spir_func void @with_group_count()
  ret void
}
; COM: only printf buffer pointer should be loaded
; XeLP-OCL: define spir_func void @extern_calls_extern_and_printf()
; XeLP-OCL-NEXT: %[[ECEAP_R0:[^ ]+]] = call i32 @llvm.genx.r0.i32()
; XeLP-OCL-NEXT: %[[ECEAP_IAB_PTR_INT:[^ ]+]] = and i32 %[[ECEAP_R0]], -64
; XeLP-OCL-TYPED-PTRS-NEXT: %[[ECEAP_IAB_PTR:[^ ]+]] = inttoptr i32 %[[ECEAP_IAB_PTR_INT]] to %vc.implicit.args.buf.type addrspace(6)*
; XeLP-OCL-TYPED-PTRS-NEXT: %[[ECEAP_PBP_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, %vc.implicit.args.buf.type addrspace(6)* %[[ECEAP_IAB_PTR]], i32 0, i32 10
; XeLP-OCL-TYPED-PTRS-NEXT: %[[ECEAP_PBP:[^ ]+]] = load i64, i64 addrspace(6)* %[[ECEAP_PBP_PTR]]
; XeLP-OCL-TYPED-PTRS-NEXT: store i64 %[[ECEAP_PBP]], i64* @__imparg_llvm.vc.internal.print.buffer
; XeLP-OCL-OPAQUE-PTRS-NEXT: %[[ECEAP_IAB_PTR:[^ ]+]] = inttoptr i32 %[[ECEAP_IAB_PTR_INT]] to ptr addrspace(6)
; XeLP-OCL-OPAQUE-PTRS-NEXT: %[[ECEAP_PBP_PTR:[^ ]+]] = getelementptr inbounds %vc.implicit.args.buf.type, ptr addrspace(6) %[[ECEAP_IAB_PTR]], i32 0, i32 10
; XeLP-OCL-OPAQUE-PTRS-NEXT: %[[ECEAP_PBP:[^ ]+]] = load i64, ptr addrspace(6) %[[ECEAP_PBP_PTR]]
; XeLP-OCL-OPAQUE-PTRS-NEXT: store i64 %[[ECEAP_PBP]], ptr @__imparg_llvm.vc.internal.print.buffer

; XeLP-OCL-NEXT: call spir_func void @with_printf_impl()
; XeLP-OCL-NEXT: call spir_func void @with_local_size()
; XeLP-OCL-NEXT: call spir_func void @with_group_count()
; XeLP-OCL-NEXT: ret void

; XeLP-OCL: attributes #[[KERNEL_ATTR]] = {
; XeLP-OCL-SAME: "RequiresImplArgsBuffer"

!genx.kernels = !{!0}

!0 = !{void ()* @kernel, !"kernel", !1, i32 0, !1, !1, !1, i32 0, i32 0}
!1 = !{}
