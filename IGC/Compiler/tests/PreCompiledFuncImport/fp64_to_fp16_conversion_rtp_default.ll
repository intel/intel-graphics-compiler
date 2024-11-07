;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt -regkey TestIGCPreCompiledFunctions=1 -regkey ForceEmuKind=2         --platformdg2 --igc-precompiled-import -S < %s | FileCheck %s --check-prefix=CHECK --check-prefix=POS-CASE-CHECK --check-prefix=POS-CASE-RTE-CHECK
; RUN: igc_opt -regkey TestIGCPreCompiledFunctions=1 -regkey ForceEmuKind=8         --platformdg2 --igc-precompiled-import -S < %s | FileCheck %s --check-prefix=CHECK --check-prefix=POS-CASE-CHECK --check-prefix=POS-CASE-RTE-CHECK
; RUN: igc_opt -regkey TestIGCPreCompiledFunctions=1 -regkey ForceEmuKind=128       --platformdg2 --igc-precompiled-import -S < %s | FileCheck %s --check-prefix=CHECK --check-prefix=NEG-CASE-CHECK --check-prefix=POS-CASE-RTE-CHECK
; RUN: igc_opt -regkey TestIGCPreCompiledFunctions=1 -regkey ForceEmuKind=0xfffff75 --platformdg2 --igc-precompiled-import -S < %s | FileCheck %s --check-prefix=CHECK --check-prefix=NEG-CASE-CHECK --check-prefix=NEG-CASE-RTE-CHECK
; ------------------------------------------------
; PreCompiledFuncImport
; ------------------------------------------------

; Check that appropriate call instruction to emulated code are inserted
; when we use PreCompiledFuncImport pass.

define void @main(double %in) {
entry:
; CHECK: entry:

  %res_default = fptrunc double %in to half
; POS-CASE-CHECK-NEXT: [[RES_DEFAULT:%res_default]] = call {{.*}} half @__precompiled_convert_f64_to_f16_rtp(double %in)
; NEG-CASE-CHECK-NEXT: [[RES_DEFAULT:%res_default]] = fptrunc double %in to half
  call void @use(half %res_default)
; CHECK-NEXT: call void @use(half [[RES_DEFAULT]])

  %res_rte = call half @llvm.genx.GenISA.ftof.rte(double %in)
; POS-CASE-RTE-CHECK-NEXT: [[RES_RTE:%res_rte]] = call {{.*}} half @__precompiled_convert_f64_to_f16_rte(double %in)
; NEG-CASE-RTE-CHECK-NEXT: [[RES_RTE:%res_rte]] = call half @llvm.genx.GenISA.ftof.rte(double %in)
  call void @use(half %res_rte)
; CHECK-NEXT: call void @use(half [[RES_RTE]])

  %res_rtz = call half @llvm.genx.GenISA.ftof.rtz(double %in)
; POS-CASE-CHECK-NEXT: [[RES_RTZ:%res_rtz]] = call {{.*}} half @__precompiled_convert_f64_to_f16_rtz(double %in)
; NEG-CASE-CHECK-NEXT: [[RES_RTZ:%res_rtz]] = call half @llvm.genx.GenISA.ftof.rtz(double %in)
  call void @use(half %res_rtz)
; CHECK-NEXT: call void @use(half [[RES_RTZ]])

  %res_rtn = call half @llvm.genx.GenISA.ftof.rtn(double %in)
; POS-CASE-CHECK-NEXT: [[RES_RTN:%res_rtn]] = call {{.*}} half @__precompiled_convert_f64_to_f16_rtn(double %in)
; NEG-CASE-CHECK-NEXT: [[RES_RTN:%res_rtn]] = call half @llvm.genx.GenISA.ftof.rtn(double %in)
  call void @use(half %res_rtn)
; CHECK-NEXT: call void @use(half [[RES_RTN]])

  %res_rtp = call half @llvm.genx.GenISA.ftof.rtp(double %in)
; POS-CASE-CHECK-NEXT: [[RES_RTP:%res_rtp]] = call {{.*}} half @__precompiled_convert_f64_to_f16_rtp(double %in)
; NEG-CASE-CHECK-NEXT: [[RES_RTP:%res_rtp]] = call half @llvm.genx.GenISA.ftof.rtp(double %in)
  call void @use(half %res_rtp)
; CHECK-NEXT: call void @use(half [[RES_RTP]])

  ret void
}

declare half @llvm.genx.GenISA.ftof.rte(double)
declare half @llvm.genx.GenISA.ftof.rtz(double)
declare half @llvm.genx.GenISA.ftof.rtp(double)
declare half @llvm.genx.GenISA.ftof.rtn(double)
declare void @use(half)

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FloatRoundingMode", i32 1}