; UNSUPPORTED: system-windows
; REQUIRES: llvm-spirv, regkeys, dg2-supported

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_bfloat16_conversion -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; CHECK-LLVM: call spir_func i16 @_Z27__spirv_ConvertFToBF16INTELf(float
; CHECK-LLVM: call spir_func float @_Z27__spirv_ConvertBF16ToFINTELs(i16
; CHECK-LLVM: call spir_func <8 x i16> @_Z27__spirv_ConvertFToBF16INTELDv8_f(<8 x float>
; CHECK-LLVM: call spir_func <8 x float> @_Z27__spirv_ConvertBF16ToFINTELDv8_s(<8 x i16>
; CHECK-LLVM: call spir_func i16 @_Z27__spirv_ConvertFToBF16INTELf(float 1.000000e+00)
; CHECK-LLVM: call spir_func float @_Z27__spirv_ConvertBF16ToFINTELs(i16 67)

define spir_func void @_Z2opffv8(float %a, <8 x float> %in) {
  %1 = tail call spir_func zeroext i16 @_Z27__spirv_ConvertFToBF16INTELf(float %a)
  %2 = tail call spir_func float @_Z27__spirv_ConvertBF16ToFINTELs(i16 zeroext %1)
  %3 = tail call spir_func <8 x i16> @_Z27__spirv_ConvertFToBF16INTELDv8_f(<8 x float> %in)
  %4 = tail call spir_func <8 x float> @_Z27__spirv_ConvertBF16ToFINTELDv8_s(<8 x i16> %3)
  %5 = tail call spir_func zeroext i16 @_Z27__spirv_ConvertFToBF16INTELf(float 1.000000e+00)
  %6 = tail call spir_func float @_Z27__spirv_ConvertBF16ToFINTELs(i16 67)
  ret void
}

declare spir_func zeroext i16 @_Z27__spirv_ConvertFToBF16INTELf(float)

declare spir_func float @_Z27__spirv_ConvertBF16ToFINTELs(i16 zeroext)

declare spir_func <8 x i16> @_Z27__spirv_ConvertFToBF16INTELDv8_f(<8 x float>)

declare spir_func <8 x float> @_Z27__spirv_ConvertBF16ToFINTELDv8_s(<8 x i16>)

!opencl.spir.version = !{!0}
!spirv.Source = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, i32 2}
!1 = !{i32 4, i32 100000}
!2 = !{!"clang version 13.0.0"}
