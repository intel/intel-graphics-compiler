;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers %s -S -o - --igc-legalize-function-signatures | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers %s -S -o - --igc-legalize-function-signatures | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED %}

; ------------------------------------------------
; LegalizeFunctionSignatures
; ------------------------------------------------

; The test checks if struct argument passed as generic pointer with byval attribute is correctly legalized

%structtype = type { i32, i32 }

define spir_kernel void @kernel() {
; CHECK-LABEL: @kernel(
; CHECK: [[AL:%.*]] = alloca %structtype
; CHECK-TYPED: [[TMP0:%.*]] = addrspacecast %structtype* [[AL]] to %structtype addrspace(4)*
; CHECK-OPAQUE: [[TMP0:%.*]] = addrspacecast ptr [[AL]] to ptr addrspace(4)
; CHECK-TYPED: [[GEP0:%.*]] = getelementptr inbounds %structtype, %structtype addrspace(4)* [[TMP0]], i32 0, i32 0
; CHECK-OPAQUE: [[GEP0:%.*]] = getelementptr inbounds %structtype, ptr addrspace(4) [[TMP0]], i32 0, i32 0
; CHECK-TYPED: [[L0:%.*]] = load i32, i32 addrspace(4)* [[GEP0]]
; CHECK-OPAQUE: [[L0:%.*]] = load i32, ptr addrspace(4) [[GEP0]]
; CHECK: [[I0:%.*]] = insertvalue %structtype undef, i32 [[L0]], 0
; CHECK-TYPED: [[GEP1:%.*]] = getelementptr inbounds %structtype, %structtype addrspace(4)* [[TMP0]], i32 0, i32 1
; CHECK-OPAQUE: [[GEP1:%.*]] = getelementptr inbounds %structtype, ptr addrspace(4) [[TMP0]], i32 0, i32 1
; CHECK-TYPED: [[L1:%.*]] = load i32, i32 addrspace(4)* [[GEP1]]
; CHECK-OPAQUE: [[L1:%.*]] = load i32, ptr addrspace(4) [[GEP1]]
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
; CHECK-TYPED: [[GEP0:%.*]] = getelementptr inbounds %structtype, %structtype* [[AL]], i32 0, i32 0
; CHECK-OPAQUE: [[GEP0:%.*]] = getelementptr inbounds %structtype, ptr [[AL]], i32 0, i32 0
; CHECK: [[E0:%.*]] = extractvalue %structtype %src, 0
; CHECK-TYPED: store i32 [[E0]], i32* [[GEP0]]
; CHECK-OPAQUE: store i32 [[E0]], ptr [[GEP0]]
; CHECK-TYPED: [[GEP1:%.*]] = getelementptr inbounds %structtype, %structtype* [[AL]], i32 0, i32 1
; CHECK-OPAQUE: [[GEP1:%.*]] = getelementptr inbounds %structtype, ptr [[AL]], i32 0, i32 1
; CHECK: [[E1:%.*]] = extractvalue %structtype %src, 1
; CHECK-TYPED: store i32 [[E1]], i32* [[GEP1]]
; CHECK-OPAQUE: store i32 [[E1]], ptr [[GEP1]]
;
; CHECK-TYPED: [[ASC:%.*]] = addrspacecast %structtype* [[AL]] to %structtype addrspace(4)*
; CHECK-OPAQUE: [[ASC:%.*]] = addrspacecast ptr [[AL]] to ptr addrspace(4)
; CHECK-TYPED: [[GEP2:%.*]] = getelementptr inbounds %structtype, %structtype addrspace(4)* [[ASC]], i64 0, i32 1
; CHECK-OPAQUE: [[GEP2:%.*]] = getelementptr inbounds %structtype, ptr addrspace(4) [[ASC]], i64 0, i32 1
; CHECK-TYPED: [[LOAD:%.*]] = load i32, i32 addrspace(4)* [[GEP2]]
; CHECK-OPAQUE: [[LOAD:%.*]] = load i32, ptr addrspace(4) [[GEP2]]
; CHECK: ret i32 [[LOAD]]
  %1 = getelementptr inbounds %structtype, %structtype addrspace(4)* %src, i64 0, i32 1
  %2 = load i32, i32 addrspace(4)* %1
  ret i32 %2
}

attributes #0 = { "visaStackCall" }
