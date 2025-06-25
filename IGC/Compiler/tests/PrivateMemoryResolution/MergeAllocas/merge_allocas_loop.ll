;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt --igc-lower-byval-attribute --igc-ocl-merge-allocas --igc-private-mem-resolution -S %s --platformpvc
; ------------------------------------------------
; PrivateMemoryResolution
; ------------------------------------------------

; Check that merge allocas can process loop with allocas inside without crashing.

%"class.sycl::_V1::vec.73" = type { <3 x double> }

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #0

; Function Attrs: argmemonly nofree nounwind willreturn
declare void @llvm.memcpy.p4i8.p4i8.i64(i8 addrspace(4)* noalias nocapture writeonly, i8 addrspace(4)* noalias nocapture readonly, i64, i1 immarg) #1

; Function Attrs: noinline optnone
define internal spir_func i1 @testFn(i1 %0) #2 {
  %2 = alloca %"class.sycl::_V1::vec.73", i32 0, align 32
  %3 = addrspacecast %"class.sycl::_V1::vec.73"* null to %"class.sycl::_V1::vec.73" addrspace(4)*
  br label %4

4:                                                ; preds = %10, %1
  %5 = icmp slt i32 undef, 0
  br i1 %0, label %6, label %11

6:                                                ; preds = %4
  %7 = addrspacecast %"class.sycl::_V1::vec.73" addrspace(4)* null to %"class.sycl::_V1::vec.73"*
  %8 = call spir_func double undef(%"class.sycl::_V1::vec.73"* byval(%"class.sycl::_V1::vec.73") null, i32 0)
  br label %9

9:                                                ; preds = %6
  br label %10

10:                                               ; preds = %9
  br label %4

11:                                               ; preds = %4
  ret i1 false
}

define spir_kernel void @main() {
  %1 = call spir_func i1 @testFn(i1 undef)
  ret void
}

attributes #0 = { nofree nosync nounwind readnone speculatable willreturn }
attributes #1 = { argmemonly nofree nounwind willreturn }
attributes #2 = { noinline optnone }