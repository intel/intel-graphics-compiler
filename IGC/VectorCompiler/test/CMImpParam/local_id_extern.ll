;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_legacy_typed %use_old_pass_manager% -CMImpParam -cmimpparam-payload-in-memory=false \
; RUN:     -march=genx64 -mcpu=XeHPG -S < %s | \
; RUN:  FileCheck --check-prefixes=XeHPG,XeHPG-TYPED-PTRS --enable-var-scope %s
; RUN: %opt_legacy_opaque %use_old_pass_manager% -CMImpParam -cmimpparam-payload-in-memory=false \
; RUN:     -march=genx64 -mcpu=XeHPG -S < %s | \
; RUN:  FileCheck --check-prefixes=XeHPG,XeHPG-OPAQUE-PTRS --enable-var-scope %s

; RUN: %opt_new_pm_typed -passes=CMImpParam -cmimpparam-payload-in-memory=false \
; RUN:     -march=genx64 -mcpu=XeHPG -S < %s | \
; RUN:  FileCheck --check-prefixes=XeHPG,XeHPG-TYPED-PTRS --enable-var-scope %s
; RUN: %opt_new_pm_opaque -passes=CMImpParam -cmimpparam-payload-in-memory=false \
; RUN:     -march=genx64 -mcpu=XeHPG -S < %s | \
; RUN:  FileCheck --check-prefixes=XeHPG,XeHPG-OPAQUE-PTRS --enable-var-scope %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; COM: Struct indices:
; COM: X = 0
; COM: Y = 1
; COM: Z = 2
; XeHPG-DAG: %vc.ia.local.id.type = type { i16, i16, i16 }

declare <3 x i16> @llvm.genx.local.id16.v3i16()

define dllexport spir_kernel void @kernel() {
; XeHPG-LABEL: define dllexport spir_kernel void @kernel(
; XeHPG-NOT: i64 %impl.arg.llvm.vc.internal.print.buffer
; XeHPG-NOT: <3 x i32> %impl.arg.llvm.genx.local.size
; XeHPG-NOT: <3 x i32> %impl.arg.llvm.genx.group.count
; XeHPG-DAG: <3 x i16> %impl.arg.llvm.genx.local.id16
; XeHPG-DAG: i64 %impl.arg.impl.args.buffer
; XeHPG-SAME: )
; COM: Implict args buffer variable initialization in kernel prologue. It won't
; COM: be used but it must be initialized with local ID buffer variable.
; XeHPG-TYPED-PTRS: call void @llvm.vc.internal.write.variable.region.p0i64.i64.i1(i64* @llvm.vc.predef.var.impl.args.buf, i64 %impl.arg.impl.args.buffer, i32 1, i32 0, i1 true)
; XeHPG-OPAQUE-PTRS: call void @llvm.vc.internal.write.variable.region.p0.i64.i1(ptr @llvm.vc.predef.var.impl.args.buf, i64 %impl.arg.impl.args.buffer, i32 1, i32 0, i1 true)

; COM: Local ID buffer variable initialization. The buffer is allocated on kernel stack frame.
; XeHPG: %[[LOCID_BUF:[^ ]+]] = alloca %vc.ia.local.id.type
; XeHPG-DAG: %[[LOCID_X:[^ ]+]] = extractelement <3 x i16> %impl.arg.llvm.genx.local.id16, i64 0
; XeHPG-TYPED-PTRS-DAG: %[[LOCID_X_PTR:[^ ]+]] = getelementptr %vc.ia.local.id.type, %vc.ia.local.id.type* %[[LOCID_BUF]], i32 0, i32 0
; XeHPG-TYPED-PTRS-DAG: store i16 %[[LOCID_X]], i16* %[[LOCID_X_PTR]]
; XeHPG-OPAQUE-PTRS-DAG: %[[LOCID_X_PTR:[^ ]+]] = getelementptr %vc.ia.local.id.type, ptr %[[LOCID_BUF]], i32 0, i32 0
; XeHPG-OPAQUE-PTRS-DAG: store i16 %[[LOCID_X]], ptr %[[LOCID_X_PTR]]
; XeHPG-DAG: %[[LOCID_Y:[^ ]+]] = extractelement <3 x i16> %impl.arg.llvm.genx.local.id16, i64 1
; XeHPG-TYPED-PTRS-DAG: %[[LOCID_Y_PTR:[^ ]+]] = getelementptr %vc.ia.local.id.type, %vc.ia.local.id.type* %[[LOCID_BUF]], i32 0, i32 1
; XeHPG-TYPED-PTRS-DAG: store i16 %[[LOCID_Y]], i16* %[[LOCID_Y_PTR]]
; XeHPG-OPAQUE-PTRS-DAG: %[[LOCID_Y_PTR:[^ ]+]] = getelementptr %vc.ia.local.id.type, ptr %[[LOCID_BUF]], i32 0, i32 1
; XeHPG-OPAQUE-PTRS-DAG: store i16 %[[LOCID_Y]], ptr %[[LOCID_Y_PTR]]
; XeHPG-DAG: %[[LOCID_Z:[^ ]+]] = extractelement <3 x i16> %impl.arg.llvm.genx.local.id16, i64 2
; XeHPG-TYPED-PTRS-DAG: %[[LOCID_Z_PTR:[^ ]+]] = getelementptr %vc.ia.local.id.type, %vc.ia.local.id.type* %[[LOCID_BUF]], i32 0, i32 2
; XeHPG-TYPED-PTRS-DAG: store i16 %[[LOCID_Z]], i16* %[[LOCID_Z_PTR]]
; XeHPG-TYPED-PTRS-DAG: %[[LOCID_BUF_INT:[^ ]+]] = ptrtoint %vc.ia.local.id.type* %[[LOCID_BUF]] to i64
; XeHPG-TYPED-PTRS-DAG: call void @llvm.vc.internal.write.variable.region.p0i64.i64.i1(i64* @llvm.vc.predef.var.loc.id.buf, i64 %[[LOCID_BUF_INT]], i32 1, i32 0, i1 true)
; XeHPG-OPAQUE-PTRS-DAG: %[[LOCID_Z_PTR:[^ ]+]] = getelementptr %vc.ia.local.id.type, ptr %[[LOCID_BUF]], i32 0, i32 2
; XeHPG-OPAQUE-PTRS-DAG: store i16 %[[LOCID_Z]], ptr %[[LOCID_Z_PTR]]
; XeHPG-OPAQUE-PTRS-DAG: %[[LOCID_BUF_INT:[^ ]+]] = ptrtoint ptr %[[LOCID_BUF]] to i64
; XeHPG-OPAQUE-PTRS-DAG: call void @llvm.vc.internal.write.variable.region.p0.i64.i1(ptr @llvm.vc.predef.var.loc.id.buf, i64 %[[LOCID_BUF_INT]], i32 1, i32 0, i1 true)

  call spir_func void @with_local_id()
  ret void
}

define spir_func void @with_local_id() {
; COM: The signature shouldn't change.
; XeHPG-LABEL: define spir_func void @with_local_id()

; COM: Obtaining local ID structure pointer.
; XeHPG-TYPED-PTRS: %[[LOCID_BASE_PTR_INT:[^ ]+]] = call i64 @llvm.vc.internal.read.variable.region.i64.p0i64(i64* @llvm.vc.predef.var.loc.id.buf, i32 0, i32 1, i32 1, i32 0)
; XeHPG-TYPED-PTRS: %[[LOCID_BASE_PTR:[^ ]+]] = inttoptr i64 %[[LOCID_BASE_PTR_INT]] to %vc.ia.local.id.type addrspace(1)*
; XeHPG-OPAQUE-PTRS: %[[LOCID_BASE_PTR_INT:[^ ]+]] = call i64 @llvm.vc.internal.read.variable.region.i64.p0(ptr @llvm.vc.predef.var.loc.id.buf, i32 0, i32 1, i32 1, i32 0)
; XeHPG-OPAQUE-PTRS: %[[LOCID_BASE_PTR:[^ ]+]] = inttoptr i64 %[[LOCID_BASE_PTR_INT]] to ptr addrspace(1)

; COM: Loading local IDs from the structure by elements.
; XeHPG-TYPED-PTRS-DAG: %[[LOCID_X_PTR:[^ ]+]] = getelementptr inbounds %vc.ia.local.id.type, %vc.ia.local.id.type addrspace(1)* %[[LOCID_BASE_PTR]], i32 0, i32 0
; XeHPG-TYPED-PTRS-DAG: %[[LOCID_X:[^ ]+]] = load i16, i16 addrspace(1)* %[[LOCID_X_PTR]]
; XeHPG-TYPED-PTRS-DAG: %[[LOCID_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.ia.local.id.type, %vc.ia.local.id.type addrspace(1)* %[[LOCID_BASE_PTR]], i32 0, i32 1
; XeHPG-TYPED-PTRS-DAG: %[[LOCID_Y:[^ ]+]] = load i16, i16 addrspace(1)* %[[LOCID_Y_PTR]]
; XeHPG-TYPED-PTRS-DAG: %[[LOCID_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.ia.local.id.type, %vc.ia.local.id.type addrspace(1)* %[[LOCID_BASE_PTR]], i32 0, i32 2
; XeHPG-TYPED-PTRS-DAG: %[[LOCID_Z:[^ ]+]] = load i16, i16 addrspace(1)* %[[LOCID_Z_PTR]]
; XeHPG-OPAQUE-PTRS-DAG: %[[LOCID_X_PTR:[^ ]+]] = getelementptr inbounds %vc.ia.local.id.type, ptr addrspace(1) %[[LOCID_BASE_PTR]], i32 0, i32 0
; XeHPG-OPAQUE-PTRS-DAG: %[[LOCID_X:[^ ]+]] = load i16, ptr addrspace(1) %[[LOCID_X_PTR]]
; XeHPG-OPAQUE-PTRS-DAG: %[[LOCID_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.ia.local.id.type, ptr addrspace(1) %[[LOCID_BASE_PTR]], i32 0, i32 1
; XeHPG-OPAQUE-PTRS-DAG: %[[LOCID_Y:[^ ]+]] = load i16, ptr addrspace(1) %[[LOCID_Y_PTR]]
; XeHPG-OPAQUE-PTRS-DAG: %[[LOCID_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.ia.local.id.type, ptr addrspace(1) %[[LOCID_BASE_PTR]], i32 0, i32 2
; XeHPG-OPAQUE-PTRS-DAG: %[[LOCID_Z:[^ ]+]] = load i16, ptr addrspace(1) %[[LOCID_Z_PTR]]
; XeHPG-DAG: %[[LOCID_PART_0:[^ ]+]] = insertelement <3 x i16> undef, i16 %[[LOCID_X]], i64 0
; XeHPG-DAG: %[[LOCID_PART_1:[^ ]+]] = insertelement <3 x i16> %[[LOCID_PART_0]], i16 %[[LOCID_Y]], i64 1
; XeHPG-DAG: %[[LOCID_WHOLE:[^ ]+]] = insertelement <3 x i16> %[[LOCID_PART_1]], i16 %[[LOCID_Z]], i64 2
; XeHPG-TYPED-PTRS: store <3 x i16> %[[LOCID_WHOLE]], <3 x i16>* @__imparg_llvm.genx.local.id16
; XeHPG-OPAQUE-PTRS: store <3 x i16> %[[LOCID_WHOLE]], ptr @__imparg_llvm.genx.local.id16

  %wli.loc.id = call <3 x i16> @llvm.genx.local.id16.v3i16()
; XeHPG-TYPED-PTRS: %wli.loc.id = load <3 x i16>, <3 x i16>* @__imparg_llvm.genx.local.id16
; XeHPG-OPAQUE-PTRS: %wli.loc.id = load <3 x i16>, ptr @__imparg_llvm.genx.local.id16
  ret void
}

!genx.kernels = !{!1}

!0 = !{}
!1 = !{void ()* @kernel, !"kernel", !0, i32 0, i32 0, !0, !0, i32 0, i32 0}
