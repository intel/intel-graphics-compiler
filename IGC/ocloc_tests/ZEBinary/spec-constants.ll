;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Test that ocloc embeds specialization constants (IDs and values) into the
; ZEBinary output as .misc.specConstantsIds and .misc.specConstantsValues
; sections when -spec_const is used.

; UNSUPPORTED: sys32
; REQUIRES: llvm-spirv, regkeys, pvc-supported, oneapi-readelf

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; RUN: printf "0: 42\n1: 100\n2: 65535" > %t_spec_const.txt
; RUN: ocloc compile -spirv_input -file %t.spv -spec_const %t_spec_const.txt \
; RUN:   -options "-igc_opts 'ProgbinDumpFileName=%t.progbin'" -device pvc
; RUN: oneapi-readelf -S -W %t.progbin | FileCheck %s --check-prefix=CHECK-SECTIONS
; RUN: oneapi-readelf -x .misc.specConstantsIds %t.progbin | FileCheck %s --check-prefix=CHECK-IDS
; RUN: oneapi-readelf -x .misc.specConstantsValues %t.progbin | FileCheck %s --check-prefix=CHECK-VALUES

; Verify that both misc sections are present in the ZEBinary
; CHECK-SECTIONS: .misc.specConstantsIds
; CHECK-SECTIONS: .misc.specConstantsValues

; Verify spec constant IDs 0, 1, 2 (3 x uint32_t, little-endian)
; CHECK-IDS: 00000000 01000000 02000000

; Verify spec constant values 42, 100, 65535 (3 x uint64_t, little-endian)
; CHECK-VALUES: 2a000000 00000000 64000000 00000000
; CHECK-VALUES: ffff0000 00000000

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spir64-unknown-unknown"

declare i32 @_Z20__spirv_SpecConstantii(i32, i32)

define spir_kernel void @test_spec_const(i32 addrspace(1)* %out) #0 {
entry:
  %val0 = call i32 @_Z20__spirv_SpecConstantii(i32 0, i32 0)
  %val1 = call i32 @_Z20__spirv_SpecConstantii(i32 1, i32 0)
  %val2 = call i32 @_Z20__spirv_SpecConstantii(i32 2, i32 0)
  %sum01 = add i32 %val0, %val1
  %sum012 = add i32 %sum01, %val2
  store i32 %sum012, i32 addrspace(1)* %out, align 4
  ret void
}

attributes #0 = { convergent norecurse nounwind "frame-pointer"="all" }

!opencl.spir.version = !{!0}
!spirv.Source = !{!1}
!opencl.ocl.version = !{!0}

!0 = !{i32 1, i32 2}
!1 = !{i32 0, i32 0}
