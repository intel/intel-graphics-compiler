;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; SPV_INTEL_bfloat16_conversion end to end. The same module is:
;   1) translated back from SPIR-V to the bfloat16 conversion builtins, and
;   2) compiled to vISA for a platform WITH native HW bfloat16 conversion (CRI)
;      and ones WITHOUT (METEORLAKE and TIGERLAKE, where the extension is exposed
;      as experimental - see SPIRVExtensions.td).
; On CRI the native conversion is emitted (via a bfloat-typed variable, type=bf).
; The platforms without such HW emulate the conversions in the BiF: no type=bf
; variable is used and the RNE rounding bias constant (0x7fff) is visible.

; REQUIRES: llvm-spirv, regkeys

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_bfloat16_conversion -o %t.spv

; Translation: the bfloat16 conversion builtins are restored from SPIR-V.
; RUN: %if cri-supported %{ ocloc compile -spirv_input -file %t.spv -device cri -options " -igc_opts 'ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM %}

; Native codegen (CRI): HW conversion declares a bfloat (type=bf) variable, no emulation constant.
; RUN: %if cri-supported %{ ocloc compile -spirv_input -file %t.spv -device cri -options " -igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-NATIVE %}

; Emulated codegen (METEORLAKE): no native type=bf variable, RNE rounding bias is present.
; RUN: %if mtl-supported %{ ocloc compile -spirv_input -file %t.spv -device mtl -options " -igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-EMU %}

; Emulated codegen (TIGERLAKE, a pre-XE_HPG platform): same software emulation.
; RUN: %if tgllp-supported %{ ocloc compile -spirv_input -file %t.spv -device tgllp -options " -igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-EMU %}

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; CHECK-LLVM: call spir_func i16 @_Z27__spirv_ConvertFToBF16INTELf(float
; CHECK-LLVM: call spir_func float @_Z27__spirv_ConvertBF16ToFINTELs(i16
; CHECK-LLVM: call spir_func <8 x i16> @_Z27__spirv_ConvertFToBF16INTELDv8_f(<8 x float>
; CHECK-LLVM: call spir_func <8 x float> @_Z27__spirv_ConvertBF16ToFINTELDv8_s(<8 x i16>

; CHECK-NATIVE: .kernel{{.*}}test_bf16_conversion
; CHECK-NATIVE: type=bf num_elts
; CHECK-NATIVE-NOT: 0x7fff

; CHECK-EMU: .kernel{{.*}}test_bf16_conversion
; CHECK-EMU-NOT: type=bf
; CHECK-EMU: 0x7fff

define spir_kernel void @test_bf16_conversion(float %a, i16 %b, <8 x float> %va, <8 x i16> %vb,
                                              i16 addrspace(1)* %out_bf, float addrspace(1)* %out_f,
                                              <8 x i16> addrspace(1)* %vout_bf, <8 x float> addrspace(1)* %vout_f) {
  %1 = call spir_func zeroext i16 @_Z27__spirv_ConvertFToBF16INTELf(float %a)
  store i16 %1, i16 addrspace(1)* %out_bf, align 2
  %2 = call spir_func float @_Z27__spirv_ConvertBF16ToFINTELs(i16 zeroext %b)
  store float %2, float addrspace(1)* %out_f, align 4
  %3 = call spir_func <8 x i16> @_Z27__spirv_ConvertFToBF16INTELDv8_f(<8 x float> %va)
  store <8 x i16> %3, <8 x i16> addrspace(1)* %vout_bf, align 16
  %4 = call spir_func <8 x float> @_Z27__spirv_ConvertBF16ToFINTELDv8_s(<8 x i16> %vb)
  store <8 x float> %4, <8 x float> addrspace(1)* %vout_f, align 32
  ret void
}

declare spir_func zeroext i16 @_Z27__spirv_ConvertFToBF16INTELf(float)
declare spir_func float @_Z27__spirv_ConvertBF16ToFINTELs(i16 zeroext)
declare spir_func <8 x i16> @_Z27__spirv_ConvertFToBF16INTELDv8_f(<8 x float>)
declare spir_func <8 x float> @_Z27__spirv_ConvertBF16ToFINTELDv8_s(<8 x i16>)
