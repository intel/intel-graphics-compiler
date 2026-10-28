;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -debugify -igc-dp-to-fp-load-store -check-debugify -S < %s 2>&1 | FileCheck --check-prefixes=CHECK,CHECK-OPAQUE %s %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -debugify -igc-dp-to-fp-load-store -check-debugify -S < %s 2>&1 | FileCheck --check-prefixes=CHECK,CHECK-TYPED %s %}
; ------------------------------------------------
; HandleLoadStoreInstructions
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_load(double addrspace(8585216)* %src) {
; CHECK-LABEL: @test_load(
; CHECK-TYPED:    [[TMP1:%.*]] = bitcast double addrspace(8585216)* %src to <2 x float> addrspace(8585216)*
; CHECK-TYPED:    [[TMP2:%.*]] = load <2 x float>, <2 x float> addrspace(8585216)* [[TMP1]]
; CHECK-OPAQUE:    [[TMP2:%.*]] = load <2 x float>, ptr addrspace(8585216) %src
; CHECK:    [[TMP3:%.*]] = bitcast <2 x float> [[TMP2]] to double
; CHECK:    call void @use.f64(double [[TMP3]])
; CHECK:    ret void
;
  %1 = load double, double addrspace(8585216)* %src
  call void @use.f64(double %1)
  ret void
}

define spir_kernel void @test_load_vec(<3 x double> addrspace(8585216)* %src) {
; CHECK-LABEL: @test_load_vec(
; CHECK-TYPED:    [[TMP1:%.*]] = bitcast <3 x double> addrspace(8585216)* %src to <6 x float> addrspace(8585216)*
; CHECK-TYPED:    [[TMP2:%.*]] = load <6 x float>, <6 x float> addrspace(8585216)* [[TMP1]]
; CHECK-OPAQUE:    [[TMP2:%.*]] = load <6 x float>, ptr addrspace(8585216) %src
; CHECK:    [[TMP3:%.*]] = bitcast <6 x float> [[TMP2]] to <3 x double>
; CHECK:    call void @use.v3f64(<3 x double> [[TMP3]])
; CHECK:    ret void
;
  %1 = load <3 x double>, <3 x double> addrspace(8585216)* %src
  call void @use.v3f64(<3 x double> %1)
  ret void
}

declare void @use.f64(double)
declare void @use.v3f64(<3 x double>)


define spir_kernel void @test_store(double %src, double addrspace(1)* %dst) {
; CHECK-LABEL: @test_store(
; CHECK-TYPED:    [[TMP1:%.*]] = bitcast double addrspace(1)* %dst to <2 x float> addrspace(1)*
; CHECK:    [[TMP2:%.*]] = bitcast double %src to <2 x float>
; CHECK-TYPED:    store <2 x float> [[TMP2]], <2 x float> addrspace(1)* [[TMP1]]
; CHECK-OPAQUE:    store <2 x float> [[TMP2]], ptr addrspace(1) %dst
; CHECK:    ret void
;
  store double %src, double addrspace(1)* %dst
  ret void
}

define spir_kernel void @test_store_vec(<2 x double> %src, <2 x double> addrspace(65536)* %dst) {
; CHECK-LABEL: @test_store_vec(
; CHECK-TYPED:    [[TMP1:%.*]] = bitcast <2 x double> addrspace(65536)* %dst to <4 x float> addrspace(65536)*
; CHECK:    [[TMP2:%.*]] = bitcast <2 x double> %src to <4 x float>
; CHECK-TYPED:    store <4 x float> [[TMP2]], <4 x float> addrspace(65536)* [[TMP1]]
; CHECK-OPAQUE:    store <4 x float> [[TMP2]], ptr addrspace(65536) %dst
; CHECK:    ret void
;
  store <2 x double> %src, <2 x double> addrspace(65536)* %dst
  ret void
}
