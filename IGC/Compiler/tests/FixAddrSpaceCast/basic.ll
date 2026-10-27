;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers --igc-addrspacecast-fix -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers --igc-addrspacecast-fix -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED %}
; ------------------------------------------------
; FixAddrSpaceCast
; ------------------------------------------------

define void @test_fixaddr_fix1(i64* %src) {
; CHECK-LABEL: @test_fixaddr_fix1(
; CHECK-TYPED:    [[DOTFIX1_BITCAST:%.*]] = bitcast i64* %src to i64*
; CHECK-OPAQUE:    [[DOTFIX1_BITCAST:%.*]] = bitcast ptr %src to ptr{{$}}
; CHECK-TYPED:    [[DOTFIX1_ADDRSPACECAST:%.*]] = addrspacecast i64* [[DOTFIX1_BITCAST]] to i64 addrspace(2)*
; CHECK-OPAQUE:    [[DOTFIX1_ADDRSPACECAST:%.*]] = addrspacecast ptr [[DOTFIX1_BITCAST]] to ptr addrspace(2)
; CHECK-TYPED:    store i64 13, i64 addrspace(2)* [[DOTFIX1_ADDRSPACECAST]]
; CHECK-OPAQUE:    store i64 13, ptr addrspace(2) [[DOTFIX1_ADDRSPACECAST]]
; CHECK:    ret void
;
  %1 = ptrtoint i64* %src to i64
  %2 = inttoptr i64 %1 to i64*
  %3 = ptrtoint i64* %2 to i64
  %4 = inttoptr i64 %3 to i64 addrspace(2)*
  store i64 13, i64 addrspace(2)* %4
  ret void
}

define void @test_fixaddr_fix2(i64 addrspace(0)* %src1, i64 addrspace(0)* %src2, i1 %cond) {
; CHECK-LABEL: @test_fixaddr_fix2(
; CHECK-TYPED:    [[SRC1_FIX2_ADDRSPACECAST:%.*]] = addrspacecast i64* %src1 to i64 addrspace(4)*
; CHECK-OPAQUE:    [[SRC1_FIX2_ADDRSPACECAST:%.*]] = addrspacecast ptr %src1 to ptr addrspace(4)
; CHECK-TYPED:    [[SRC2_FIX2_ADDRSPACECAST:%.*]] = addrspacecast i64* %src2 to i64 addrspace(4)*
; CHECK-OPAQUE:    [[SRC2_FIX2_ADDRSPACECAST:%.*]] = addrspacecast ptr %src2 to ptr addrspace(4)
; CHECK-TYPED:    [[TMP1:%.*]] = select i1 [[COND:%.*]], i64 addrspace(4)* [[SRC1_FIX2_ADDRSPACECAST]], i64 addrspace(4)* [[SRC2_FIX2_ADDRSPACECAST]]
; CHECK-OPAQUE:    [[TMP1:%.*]] = select i1 [[COND:%.*]], ptr addrspace(4) [[SRC1_FIX2_ADDRSPACECAST]], ptr addrspace(4) [[SRC2_FIX2_ADDRSPACECAST]]
; CHECK-TYPED:    store i64 13, i64 addrspace(4)* [[TMP1]]
; CHECK-OPAQUE:    store i64 13, ptr addrspace(4) [[TMP1]]
; CHECK:    ret void
;
  %1 = select i1 %cond, i64 addrspace(0)* %src1, i64 addrspace(0)* %src2
  %2 = addrspacecast i64 addrspace(0)* %1 to i64 addrspace(4)*
  store i64 13, i64 addrspace(4)* %2
  ret void
}
