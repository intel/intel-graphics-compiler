;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - --igc-legalize-function-signatures | FileCheck %s

; ------------------------------------------------
; LegalizeFunctionSignatures
; ------------------------------------------------

; The test checks if struct argument passed as generic pointer with byval attribute is correctly legalized

%structtype = type { i32, i32 }

define spir_kernel void @kernel() {
; CHECK-LABEL: @kernel(
; CHECK: [[AL:%.*]] = alloca %structtype
; CHECK: [[TMP0:%.*]] = addrspacecast %structtype* [[AL]] to %structtype addrspace(4)*
; CHECK: [[GEP0:%.*]] = getelementptr inbounds %structtype, %structtype addrspace(4)* [[TMP0]], i32 0, i32 0
; CHECK: [[L0:%.*]] = load i32, i32 addrspace(4)* [[GEP0]]
; CHECK: [[I0:%.*]] = insertvalue %structtype undef, i32 [[L0]], 0
; CHECK: [[GEP1:%.*]] = getelementptr inbounds %structtype, %structtype addrspace(4)* [[TMP0]], i32 0, i32 1
; CHECK: [[L1:%.*]] = load i32, i32 addrspace(4)* [[GEP1]]
; CHECK: [[I1:%.*]] = insertvalue %structtype [[I0]], i32 [[L1]], 1
; CHECK: call i32 @foo(%structtype [[I1]])
; CHECK: ret void

  %1 = alloca %structtype
  %2 = addrspacecast %structtype* %1 to %structtype addrspace(4)*
  call i32 @foo(%structtype addrspace(4)* byval(%structtype) %2)
  ret void
}

define spir_func i32 @foo(%structtype addrspace(4)* byval(%structtype) %src) #0 {
; CHECK-LABEL: define spir_func i32 @foo(%structtype %src)
; CHECK: [[AL:%.*]] = alloca %structtype
; CHECK: [[GEP0:%.*]] = getelementptr inbounds %structtype, %structtype* [[AL]], i32 0, i32 0
; CHECK: [[E0:%.*]] = extractvalue %structtype %src, 0
; CHECK: store i32 [[E0]], i32* [[GEP0]]
; CHECK: [[GEP1:%.*]] = getelementptr inbounds %structtype, %structtype* [[AL]], i32 0, i32 1
; CHECK: [[E1:%.*]] = extractvalue %structtype %src, 1
; CHECK: store i32 [[E1]], i32* [[GEP1]]
;
; CHECK: [[ASC:%.*]] = addrspacecast %structtype* [[AL]] to %structtype addrspace(4)*
; CHECK: [[GEP2:%.*]] = getelementptr inbounds %structtype, %structtype addrspace(4)* [[ASC]], i64 0, i32 1
; CHECK: [[LOAD:%.*]] = load i32, i32 addrspace(4)* [[GEP2]]
; CHECK: ret i32 [[LOAD]]
  %1 = getelementptr inbounds %structtype, %structtype addrspace(4)* %src, i64 0, i32 1
  %2 = load i32, i32 addrspace(4)* %1
  ret i32 %2
}

attributes #0 = { "visaStackCall" }
