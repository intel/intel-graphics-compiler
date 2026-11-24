;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-long-vectors-opt -S < %s | FileCheck %s

; Variable-index extracts that can't be split into element loads read from a private-memory copy of the vector. The
; copy is stored once, right after the vector is defined, and the index is clamped into the vector.

; Extracts from the same vector share one copy. An argument is copied at the entry.
; CHECK-LABEL: @arg_shared_copy(
; CHECK-NEXT:    [[SLOT:%.*]] = alloca <64 x float>
; CHECK-NEXT:    store <64 x float> %v, ptr [[SLOT]]
; CHECK-NEXT:    [[II:%.*]] = freeze i32 %i
; CHECK-NEXT:    [[I:%.*]] = and i32 [[II]], 63
; CHECK-NEXT:    [[P0:%.*]] = getelementptr <64 x float>, ptr [[SLOT]], i32 0, i32 [[I]]
; CHECK-NEXT:    [[E0:%.*]] = load float, ptr [[P0]], align 4
; CHECK-NEXT:    [[JJ:%.*]] = freeze i32 %j
; CHECK-NEXT:    [[J:%.*]] = and i32 [[JJ]], 63
; CHECK-NEXT:    [[P1:%.*]] = getelementptr <64 x float>, ptr [[SLOT]], i32 0, i32 [[J]]
; CHECK-NEXT:    [[E1:%.*]] = load float, ptr [[P1]], align 4
; CHECK-NEXT:    %s = fadd float [[E0]], [[E1]]
define float @arg_shared_copy(<64 x float> %v, i32 %i, i32 %j) {
  %e0 = extractelement <64 x float> %v, i32 %i
  %e1 = extractelement <64 x float> %v, i32 %j
  %s = fadd float %e0, %e1
  ret float %s
}

; A constant vector is copied at the entry too.
; CHECK-LABEL: @constant_vector(
; CHECK:       entry:
; CHECK-NEXT:    [[SLOT:%.*]] = alloca <64 x i32>
; CHECK-NEXT:    store <64 x i32> <i32 1, {{.*}}>, ptr [[SLOT]]
; CHECK-NEXT:    br label %next
; CHECK:       next:
; CHECK-NEXT:    [[JJ:%.*]] = freeze i32 %j
; CHECK-NEXT:    [[J:%.*]] = and i32 [[JJ]], 63
; CHECK-NEXT:    [[P:%.*]] = getelementptr <64 x i32>, ptr [[SLOT]], i32 0, i32 [[J]]
; CHECK-NEXT:    [[E:%.*]] = load i32, ptr [[P]], align 4
; CHECK-NEXT:    ret i32 [[E]]
define i32 @constant_vector(i32 %j) {
entry:
  br label %next
next:
  %e = extractelement <64 x i32> <i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39, i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55, i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63, i32 64>, i32 %j
  ret i32 %e
}

; An insert chain with constant indices only stays in registers. A variable-index extract from it reads from a copy
; stored right after the last insert.
; CHECK-LABEL: @constant_chain(
; CHECK-NEXT:    [[SLOT:%.*]] = alloca <64 x float>
; CHECK-NEXT:    %v1 = insertelement <64 x float> %v, float %a, i32 1
; CHECK-NEXT:    %v2 = insertelement <64 x float> %v1, float %b, i32 3
; CHECK-NEXT:    store <64 x float> %v2, ptr [[SLOT]]
; CHECK-NEXT:    [[JJ:%.*]] = freeze i32 %j
; CHECK-NEXT:    [[J:%.*]] = and i32 [[JJ]], 63
; CHECK-NEXT:    [[P:%.*]] = getelementptr <64 x float>, ptr [[SLOT]], i32 0, i32 [[J]]
; CHECK-NEXT:    [[E:%.*]] = load float, ptr [[P]], align 4
; CHECK-NEXT:    ret float [[E]]
define float @constant_chain(<64 x float> %v, float %a, float %b, i32 %j) {
  %v1 = insertelement <64 x float> %v, float %a, i32 1
  %v2 = insertelement <64 x float> %v1, float %b, i32 3
  %e = extractelement <64 x float> %v2, i32 %j
  ret float %e
}

; A store of an insert chain with constant indices only isn't split either.
; CHECK-LABEL: @constant_chain_store(
; CHECK-NEXT:    %v1 = insertelement <64 x float> %v, float %a, i32 1
; CHECK-NEXT:    store <64 x float> %v1, ptr addrspace(1) %out, align 4
; CHECK-NEXT:    ret void
define void @constant_chain_store(<64 x float> %v, float %a, ptr addrspace(1) %out) {
  %v1 = insertelement <64 x float> %v, float %a, i32 1
  store <64 x float> %v1, ptr addrspace(1) %out, align 4
  ret void
}

; Constant-index extracts read the element directly from registers, so no copy is made.
; CHECK-LABEL: @constant_index(
; CHECK-NEXT:    %e = extractelement <64 x float> %v, i32 5
; CHECK-NEXT:    ret float %e
define float @constant_index(<64 x float> %v) {
  %e = extractelement <64 x float> %v, i32 5
  ret float %e
}

; An out-of-range constant index from a load that can't be split is left alone too, rather than read past the copy.
; CHECK-LABEL: @constant_index_out_of_range(
; CHECK-NEXT:    %v = load volatile <64 x float>, ptr addrspace(1) %p, align 4
; CHECK-NEXT:    %e = extractelement <64 x float> %v, i32 64
; CHECK-NEXT:    ret float %e
define float @constant_index_out_of_range(ptr addrspace(1) %p) {
  %v = load volatile <64 x float>, ptr addrspace(1) %p, align 4
  %e = extractelement <64 x float> %v, i32 64
  ret float %e
}

; The element count is not a power of two, so the index is clamped with a select.
; CHECK-LABEL: @clamp_non_pow2(
; CHECK:         [[J:%.*]] = freeze i32 %j
; CHECK:         [[C:%.*]] = icmp ule i32 [[J]], 47
; CHECK-NEXT:    [[JJ:%.*]] = select i1 [[C]], i32 [[J]], i32 47
; CHECK-NEXT:    [[P:%.*]] = getelementptr <48 x float>, ptr {{%.*}}, i32 0, i32 [[JJ]]
define float @clamp_non_pow2(<48 x float> %v, i32 %j) {
  %e = extractelement <48 x float> %v, i32 %j
  ret float %e
}

; A load from read-only memory can't be changed by stores, so when the index is computed after the load, the element
; is loaded at the extract.
; CHECK-LABEL: @read_only_late_index(
; CHECK-NOT:     alloca
; CHECK:         %j = add i32 %i, 1
; CHECK-NEXT:    [[P:%.*]] = getelementptr <64 x float>, ptr addrspace(2) %p, i32 0, i32 %j
; CHECK-NEXT:    [[E:%.*]] = load float, ptr addrspace(2) [[P]], align 4
; CHECK-NEXT:    ret float [[E]]
define float @read_only_late_index(ptr addrspace(2) %p, i32 %i) {
  %v = load <64 x float>, ptr addrspace(2) %p, align 4
  %j = add i32 %i, 1
  %e = extractelement <64 x float> %v, i32 %j
  ret float %e
}

; Global memory may be written, but nothing writes it between the load and the extract, so the element is loaded at
; the extract.
; CHECK-LABEL: @writable_late_index(
; CHECK-NEXT:    %j = load i32, ptr addrspace(1) %q, align 4
; CHECK-NEXT:    [[P:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %p, i32 0, i32 %j
; CHECK-NEXT:    [[E:%.*]] = load float, ptr addrspace(1) [[P]], align 4
; CHECK-NEXT:    ret float [[E]]
define float @writable_late_index(ptr addrspace(1) %p, ptr addrspace(1) %q) {
  %v = load <64 x float>, ptr addrspace(1) %p, align 4
  %j = load i32, ptr addrspace(1) %q, align 4
  %e = extractelement <64 x float> %v, i32 %j
  ret float %e
}

; A store between the load and the extract may write the loaded memory. An index loaded from memory can't be computed
; again at the load, so the extract reads from a copy of the loaded vector instead.
; CHECK-LABEL: @written_late_index(
; CHECK-NEXT:    [[SLOT:%.*]] = alloca <64 x float>
; CHECK-NEXT:    %v = load <64 x float>, ptr addrspace(1) %p, align 4
; CHECK-NEXT:    store <64 x float> %v, ptr [[SLOT]]
; CHECK-NEXT:    store float 0.000000e+00, ptr addrspace(1) %p, align 4
; CHECK-NEXT:    %j = load i32, ptr addrspace(1) %q, align 4
; CHECK-NEXT:    [[JJ:%.*]] = freeze i32 %j
; CHECK-NEXT:    [[J:%.*]] = and i32 [[JJ]], 63
; CHECK-NEXT:    [[P:%.*]] = getelementptr <64 x float>, ptr [[SLOT]], i32 0, i32 [[J]]
; CHECK-NEXT:    [[E:%.*]] = load float, ptr [[P]], align 4
; CHECK-NEXT:    ret float [[E]]
define float @written_late_index(ptr addrspace(1) %p, ptr addrspace(1) %q) {
  %v = load <64 x float>, ptr addrspace(1) %p, align 4
  store float 0.0, ptr addrspace(1) %p, align 4
  %j = load i32, ptr addrspace(1) %q, align 4
  %e = extractelement <64 x float> %v, i32 %j
  ret float %e
}

; A cheap index computed after a writable load is computed again at the load, so the load is split as usual. The
; original computation is removed once unused.
; CHECK-LABEL: @hoisted_index_chain(
; CHECK-NEXT:    [[M:%.*]] = mul i32 %i, 3
; CHECK-NEXT:    [[J:%.*]] = add i32 [[M]], %k
; CHECK-NEXT:    [[P:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %p, i32 0, i32 [[J]]
; CHECK-NEXT:    [[E:%.*]] = load float, ptr addrspace(1) [[P]], align 4
; CHECK-NEXT:    store float 0.000000e+00, ptr addrspace(1) %p, align 4
; CHECK-NEXT:    ret float [[E]]
define float @hoisted_index_chain(ptr addrspace(1) %p, i32 %i, i32 %k) {
  %v = load <64 x float>, ptr addrspace(1) %p, align 4
  store float 0.0, ptr addrspace(1) %p, align 4
  %m = mul i32 %i, 3
  %j = add i32 %m, %k
  %e = extractelement <64 x float> %v, i32 %j
  ret float %e
}

; An instruction shared by several operands of the index is cloned once, before all its users.
; CHECK-LABEL: @hoisted_index_shared(
; CHECK-NEXT:    [[M:%.*]] = mul i32 %i, 3
; CHECK-NEXT:    [[A:%.*]] = add i32 [[M]], 1
; CHECK-NEXT:    [[B:%.*]] = add i32 [[M]], 2
; CHECK-NEXT:    [[J:%.*]] = add i32 [[A]], [[B]]
; CHECK-NEXT:    [[P:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %p, i32 0, i32 [[J]]
; CHECK-NEXT:    [[E:%.*]] = load float, ptr addrspace(1) [[P]], align 4
; CHECK-NEXT:    ret float [[E]]
define float @hoisted_index_shared(ptr addrspace(1) %p, i32 %i) {
  %v = load <64 x float>, ptr addrspace(1) %p, align 4
  %m = mul i32 %i, 3
  %a = add i32 %m, 1
  %b = add i32 %m, 2
  %j = add i32 %a, %b
  %e = extractelement <64 x float> %v, i32 %j
  ret float %e
}

; A division may trap, so it isn't computed again at the load. Nothing writes the loaded memory before the extract,
; so the element is loaded there.
; CHECK-LABEL: @trapping_index(
; CHECK-NEXT:    %j = udiv i32 %a, %b
; CHECK-NEXT:    [[P:%.*]] = getelementptr <64 x float>, ptr addrspace(1) %p, i32 0, i32 %j
; CHECK-NEXT:    [[E:%.*]] = load float, ptr addrspace(1) [[P]], align 4
define float @trapping_index(ptr addrspace(1) %p, i32 %a, i32 %b) {
  %v = load <64 x float>, ptr addrspace(1) %p, align 4
  %j = udiv i32 %a, %b
  %e = extractelement <64 x float> %v, i32 %j
  ret float %e
}
