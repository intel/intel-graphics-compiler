;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UseNativeBF16Conversion selects the native float<->bfloat16 conversion in the BiF.
; ARL-S and ARL-U reuse the MTL graphics die and must use the emulation, like MTL.
; Only ARL-H has the native conversion.

; RUN: igc_opt -platformarl -device-id=0x7D67 -igc-bif-flag-control-resolution -S < %s | FileCheck %s --check-prefix=EMU
; RUN: igc_opt -platformarl -device-id=0x7D41 -igc-bif-flag-control-resolution -S < %s | FileCheck %s --check-prefix=EMU
; RUN: igc_opt -platformarl -device-id=0x7D51 -igc-bif-flag-control-resolution -S < %s | FileCheck %s --check-prefix=NATIVE

; RUN: igc_opt -platformmtl -igc-bif-flag-control-resolution -S < %s | FileCheck %s --check-prefix=EMU
; RUN: igc_opt -platformtgllp -igc-bif-flag-control-resolution -S < %s | FileCheck %s --check-prefix=EMU
; RUN: igc_opt -platformdg2 -igc-bif-flag-control-resolution -S < %s | FileCheck %s --check-prefix=NATIVE

; EMU: @__bif_flag_UseNativeBF16Conversion = addrspace(2) constant i8 0
; NATIVE: @__bif_flag_UseNativeBF16Conversion = addrspace(2) constant i8 1

@__bif_flag_UseNativeBF16Conversion = external addrspace(2) constant i8
