;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: ;;;;;;;;;; RUNNERS ;;;;;;;;;;

; RUN: %opt %use_old_pass_manager% -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeLP \
; RUN: -GenXModule -GenXCategoryWrapper -GenXCisaBuilderPass -GenXFinalizer \
; RUN: -finalizer-opts="-dumpcommonisa -isaasmToConsole" < %s | FileCheck %s

; COM: ;;;;;;;;;; CHECKERS ;;;;;;;;;;

; Gen9 VISA check
; CHECK: mov (M1_NM, 1) %impl_arg_buf_ptr({{.*}})<{{[0-9]}}> V{{[0-9].*}}(
; CHECK: mov (M1_NM, 8) V{{[0-9].*}}({{.*}})<{{[0-9]}}> %local_id_buf_ptr(

; COM: ;;;;;;;;;; KERNEL ;;;;;;;;;;

@llvm.vc.predef.var.impl.args.buf = external global i64 #0
@llvm.vc.predef.var.loc.id.buf = external global i64 #0

define dllexport spir_kernel void @kernel(<3 x i16> %impl.arg.llvm.genx.local.id16, i64 %impl.arg.private.base, i64 %impl.arg.impl.args.buffer) #1 {
  call void @llvm.vc.internal.write.variable.region.p0i64.i64.i1(i64* nonnull @llvm.vc.predef.var.impl.args.buf, i64 %impl.arg.impl.args.buffer, i32 1, i32 0, i1 true)
  %read = call <8 x i64> @llvm.vc.internal.read.variable.region.v8i64.p0i64(i64* nonnull @llvm.vc.predef.var.loc.id.buf, i32 0, i32 1, i32 1, i32 0)
  tail call void @llvm.genx.oword.st.v8i64(i32 0, i32 0, <8 x i64> %read) #4
  ret void
}

declare !internal_intrinsic_id !4 <8 x i64> @llvm.vc.internal.read.variable.region.v8i64.p0i64(i64*, i32, i32, i32, i32) #2
declare !internal_intrinsic_id !5 void @llvm.vc.internal.write.variable.region.p0i64.i64.i1(i64*, i64, i32, i32, i1) #3
declare void @llvm.genx.oword.st.v8i64(i32, i32, <8 x i64>) #3

attributes #0 = { "VCPredefinedVariable" }
attributes #1 = { "RequiresImplArgsBuffer" "target-cpu"="Gen9" }
attributes #2 = { nounwind readonly "target-cpu"="Gen9" }
attributes #3 = { nounwind writeonly "target-cpu"="Gen9" }
attributes #4 = { nounwind }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!3}

!0 = !{void (<3 x i16>, i64, i64)* @kernel, !"kernel", !1, i32 0, !1, !2, !2, i32 0, i32 0}
!1 = !{i32 24, i32 96, i32 120}
!2 = !{}
!3 = !{void (<3 x i16>, i64, i64)* @kernel, null, null, !2, null}
!4 = !{i32 11225}
!5 = !{i32 11229}