;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmimpparam -cmimpparam-cmrt=false -cmimpparam-payload-in-memory=false \
; RUN:     -march=genx64 -mcpu=Gen9 -S < %s | \
; RUN:  FileCheck --check-prefix=Gen9 --enable-var-scope %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; COM: Struct indices:
; COM: X = 0
; COM: Y = 1
; COM: Z = 2
;Gen9-DAG: %vc.ia.local.id.type = type { i16, i16, i16 }

declare <3 x i16> @llvm.genx.local.id16.v3i16()

define dllexport spir_kernel void @kernel() {
; Gen9-LABEL: define dllexport spir_kernel void @kernel(
; Gen9-NOT: i64 %impl.arg.llvm.vc.internal.print.buffer
; Gen9-NOT: <3 x i32> %impl.arg.llvm.genx.local.size
; Gen9-NOT: <3 x i32> %impl.arg.llvm.genx.group.count
; Gen9-DAG: <3 x i16> %impl.arg.llvm.genx.local.id16
; Gen9-DAG: i64 %impl.arg.impl.args.buffer
; Gen9-SAME: )
; COM: Implict args buffer variable initialization in kernel prologue. It won't
; COM: be used but it must be initialized with local ID buffer variable.
; Gen9: call void @llvm.vc.internal.write.variable.region.p0i64.i64.i1(i64* @llvm.vc.predef.var.impl.args.buf, i64 %impl.arg.impl.args.buffer, i32 1, i32 0, i1 true)

; COM: Local ID buffer variable initialization. The buffer is allocated on kernel stack frame.
; Gen9: %[[LOCID_BUF:[^ ]+]] = alloca %vc.ia.local.id.type
; Gen9-DAG: %[[LOCID_X:[^ ]+]] = extractelement <3 x i16> %impl.arg.llvm.genx.local.id16, i64 0
; Gen9-DAG: %[[LOCID_X_PTR:[^ ]+]] = getelementptr %vc.ia.local.id.type, %vc.ia.local.id.type* %[[LOCID_BUF]], i32 0, i32 0
; Gen9-DAG: store i16 %[[LOCID_X]], i16* %[[LOCID_X_PTR]]
; Gen9-DAG: %[[LOCID_Y:[^ ]+]] = extractelement <3 x i16> %impl.arg.llvm.genx.local.id16, i64 1
; Gen9-DAG: %[[LOCID_Y_PTR:[^ ]+]] = getelementptr %vc.ia.local.id.type, %vc.ia.local.id.type* %[[LOCID_BUF]], i32 0, i32 1
; Gen9-DAG: store i16 %[[LOCID_Y]], i16* %[[LOCID_Y_PTR]]
; Gen9-DAG: %[[LOCID_Z:[^ ]+]] = extractelement <3 x i16> %impl.arg.llvm.genx.local.id16, i64 2
; Gen9-DAG: %[[LOCID_Z_PTR:[^ ]+]] = getelementptr %vc.ia.local.id.type, %vc.ia.local.id.type* %[[LOCID_BUF]], i32 0, i32 2
; Gen9-DAG: store i16 %[[LOCID_Z]], i16* %[[LOCID_Z_PTR]]
; Gen9-DAG: %[[LOCID_BUF_INT:[^ ]+]] = ptrtoint %vc.ia.local.id.type* %[[LOCID_BUF]] to i64
; Gen9-DAG: call void @llvm.vc.internal.write.variable.region.p0i64.i64.i1(i64* @llvm.vc.predef.var.loc.id.buf, i64 %[[LOCID_BUF_INT]], i32 1, i32 0, i1 true)

  call spir_func void @with_local_id()
  ret void
}

define spir_func void @with_local_id() {
; COM: The signature shouldn't change.
; Gen9-LABEL: define spir_func void @with_local_id()

; COM: Obtaining local ID structure pointer.
; Gen9: %[[LOCID_BASE_PTR_INT:[^ ]+]] = call i64 @llvm.vc.internal.read.variable.region.i64.p0i64(i64* @llvm.vc.predef.var.loc.id.buf, i32 0, i32 1, i32 1, i32 0)
; Gen9: %[[LOCID_BASE_PTR:[^ ]+]] = inttoptr i64 %[[LOCID_BASE_PTR_INT]] to %vc.ia.local.id.type addrspace(1)*

; COM: Loading local IDs from the structure by elements.
; Gen9-DAG: %[[LOCID_X_PTR:[^ ]+]] = getelementptr inbounds %vc.ia.local.id.type, %vc.ia.local.id.type addrspace(1)* %[[LOCID_BASE_PTR]], i32 0, i32 0
; Gen9-DAG: %[[LOCID_X:[^ ]+]] = load i16, i16 addrspace(1)* %[[LOCID_X_PTR]]
; Gen9-DAG: %[[LOCID_Y_PTR:[^ ]+]] = getelementptr inbounds %vc.ia.local.id.type, %vc.ia.local.id.type addrspace(1)* %[[LOCID_BASE_PTR]], i32 0, i32 1
; Gen9-DAG: %[[LOCID_Y:[^ ]+]] = load i16, i16 addrspace(1)* %[[LOCID_Y_PTR]]
; Gen9-DAG: %[[LOCID_Z_PTR:[^ ]+]] = getelementptr inbounds %vc.ia.local.id.type, %vc.ia.local.id.type addrspace(1)* %[[LOCID_BASE_PTR]], i32 0, i32 2
; Gen9-DAG: %[[LOCID_Z:[^ ]+]] = load i16, i16 addrspace(1)* %[[LOCID_Z_PTR]]
; Gen9-DAG: %[[LOCID_PART_0:[^ ]+]] = insertelement <3 x i16> undef, i16 %[[LOCID_X]], i64 0
; Gen9-DAG: %[[LOCID_PART_1:[^ ]+]] = insertelement <3 x i16> %[[LOCID_PART_0]], i16 %[[LOCID_Y]], i64 1
; Gen9-DAG: %[[LOCID_WHOLE:[^ ]+]] = insertelement <3 x i16> %[[LOCID_PART_1]], i16 %[[LOCID_Z]], i64 2
; Gen9: store <3 x i16> %[[LOCID_WHOLE]], <3 x i16>* @__imparg_llvm.genx.local.id16

  %wli.loc.id = call <3 x i16> @llvm.genx.local.id16.v3i16()
; Gen9: %wli.loc.id = load <3 x i16>, <3 x i16>* @__imparg_llvm.genx.local.id16
  ret void
}

!genx.kernels = !{!1}

!0 = !{}
!1 = !{void ()* @kernel, !"kernel", !0, i32 0, i32 0, !0, !0, i32 0, i32 0}
