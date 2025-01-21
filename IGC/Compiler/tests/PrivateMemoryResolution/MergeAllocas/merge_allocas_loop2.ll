;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt --igc-lower-byval-attribute --igc-merge-allocas --igc-private-mem-resolution -S %s --platformpvc
; ------------------------------------------------
; PrivateMemoryResolution
; ------------------------------------------------

; Check that merge allocas can process loop with alloca use inside in case that
; backedge points to block before loop header.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

%"class.sycl::_V1::vec.73" = type { <3 x double> }

; Function Attrs: noinline optnone
define internal spir_func i1 @_ZN12_GLOBAL__N_117check_elems_equalIdLi3EEEbRKN4sycl3_V13vecIT_XT0_EEES7_.28() #0 {
  %1 = alloca %"class.sycl::_V1::vec.73", i32 0, align 32
  br label %2

2:                                                ; preds = %7, %0
  %3 = load i32, i32 addrspace(4)* null, align 4
  %4 = icmp slt i32 %3, 0
  br i1 %4, label %5, label %8

5:                                                ; preds = %2
  %6 = call spir_func double null(%"class.sycl::_V1::vec.73"* %1)
  br label %7

7:                                                ; preds = %5
  br label %2

8:                                                ; preds = %2
  ret i1 false
}

define spir_kernel void @_ZTSN16accessor_utility34buffer_accessor_get_pointer_kernelIN25accessor_api_local_fp64__11kernel_nameIN4sycl3_V13vecIdLi3EEEEELi0ELNS4_6access4modeE1026ELNS8_6targetE2016ELNS8_11placeholderE0EEE() {
  %1 = call spir_func i1 @_ZN12_GLOBAL__N_117check_elems_equalIdLi3EEEbRKN4sycl3_V13vecIT_XT0_EEES7_.28()
  ret void
}

attributes #0 = { noinline optnone }
