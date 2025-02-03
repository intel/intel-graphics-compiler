;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXVerify -genx-verify-terminate=no \
; RUN: -genx-verify-all-fatal=1 -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=XeHPG -genx-verify-stage=post-ir-adaptors -S < %s 2>&1 | FileCheck \
; RUN: --check-prefixes=CHECK %s
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXVerify -genx-verify-terminate=no \
; RUN: -genx-verify-all-fatal=1 -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=XeHPG -genx-verify-stage=post-ir-adaptors -S < %s 2>&1 | FileCheck \
; RUN: --check-prefixes=CHECK %s

; RUN: %opt_new_pm_typed -passes=GenXVerify -genx-verify-terminate=no \
; RUN: -genx-verify-all-fatal=1 -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=XeHPG -genx-verify-stage=post-ir-adaptors -S < %s 2>&1 | FileCheck \
; RUN: --check-prefixes=CHECK %s
; RUN: %opt_new_pm_opaque -passes=GenXVerify -genx-verify-terminate=no \
; RUN: -genx-verify-all-fatal=1 -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=XeHPG -genx-verify-stage=post-ir-adaptors -S < %s 2>&1 | FileCheck \
; RUN: --check-prefixes=CHECK %s

target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; CHECK: warning: {{.+}} <@SLMGV = addrspace(3) {{.+}}>: a volatile variable must reside in private address space
@SLMGV = addrspace(3) global <2 x i8> undef #0

; CHECK: warning: {{.+}} @GV {{.+}} a volatile variable may only be used in genx.vload/genx.vstore intrinsics and volatile loads/stores instructions
@GV = global <16 x i32> zeroinitializer, align 64 #1
@ALIAS = global <16 x i32> addrspace(1)* addrspacecast (<16 x i32>* @GV to <16 x i32> addrspace(1)*)

@INVALID = global <16 x float> zeroinitializer, align 64 #2
@VALID = global <16 x float> zeroinitializer, align 64 #3

define internal spir_func void @foo() {
  ; CHECK: warning: {{.+}} %cst = {{.+}} a volatile variable may only be used in genx.vload/genx.vstore intrinsics and volatile loads/stores instructions
  %cst = addrspacecast <16 x float>* @INVALID to <16 x float> addrspace(4)*

  ; CHECK-NOT: warning
  %ld.ic = tail call <16 x float> @llvm.genx.vload.v16f32.p0v16f32(<16 x float>* @VALID)
  call void @llvm.genx.vstore.v16f32.p0v16f32(<16 x float> %ld.ic, <16 x float>* @VALID)
  %ld.inst = load volatile <16 x float>, <16 x float>* @VALID
  store volatile <16 x float> %ld.inst, <16 x float>* @VALID
  ret void
}

declare <16 x float> @llvm.genx.vload.v16f32.p0v16f32(<16 x float>*)
declare void @llvm.genx.vstore.v16f32.p0v16f32(<16 x float>, <16 x float>*)

attributes #0 = { "VCByteOffset"="0" "VCGlobalVariable" "VCVolatile" "genx_byte_offset"="0" "genx_volatile" }
attributes #1 = { "VCByteOffset"="256" "VCGlobalVariable" "VCVolatile" "genx_byte_offset"="256" "genx_volatile" }
attributes #2 = { "VCByteOffset"="512" "VCGlobalVariable" "VCVolatile" "genx_byte_offset"="512" "genx_volatile" }
attributes #3 = { "VCByteOffset"="1024" "VCGlobalVariable" "VCVolatile" "genx_byte_offset"="1024" "genx_volatile" }
