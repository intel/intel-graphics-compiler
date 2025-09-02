; Source:
;
; #pragma OPENCL EXTENSION cl_intel_device_side_avc_motion_estimation : enable
;
; void foo() {
;   intel_sub_group_avc_mce_payload_t payload_mce; // No literal initializer for mce types
;   intel_sub_group_avc_ime_payload_t payload_ime = CLK_AVC_IME_PAYLOAD_INITIALIZE_INTEL;
;   intel_sub_group_avc_ref_payload_t payload_ref = CLK_AVC_REF_PAYLOAD_INITIALIZE_INTEL;
;   intel_sub_group_avc_sic_payload_t payload_sic = CLK_AVC_SIC_PAYLOAD_INITIALIZE_INTEL;
;
;   intel_sub_group_avc_mce_result_t result_mce; // No literal initializer for mce types
;   intel_sub_group_avc_ime_result_t result_ime = CLK_AVC_IME_RESULT_INITIALIZE_INTEL;
;   intel_sub_group_avc_ref_result_t result_ref = CLK_AVC_REF_RESULT_INITIALIZE_INTEL;
;   intel_sub_group_avc_sic_result_t result_sic = CLK_AVC_SIC_RESULT_INITIALIZE_INTEL;
;
;   intel_sub_group_avc_ime_result_single_reference_streamout_t sstreamout = 0x0;
;   intel_sub_group_avc_ime_result_dual_reference_streamout_t dstreamout = 0x0;
;   intel_sub_group_avc_ime_single_reference_streamin_t sstreamin = 0x0;
;   intel_sub_group_avc_ime_dual_reference_streamin_t dstreamin = 0x0;
; }

; UNSUPPORTED: system-windows

; This test won't work with LLVM 16+ due to different representation of OpenCL builtin types (pointers to opaque structs vs TargetExtTy).
; It would work on LLVM 16 with typed pointers forced (in CMake), but not on a default build of IGC with LLVM 16.
; REQUIRES: llvm-spirv, regkeys, dg2-supported, llvm-15-or-older

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_device_side_avc_motion_estimation -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

; CHECK-LLVM: %spirv.AvcMcePayloadINTEL = type opaque
; CHECK-LLVM: %spirv.AvcImePayloadINTEL = type opaque
; CHECK-LLVM: %spirv.AvcRefPayloadINTEL = type opaque
; CHECK-LLVM: %spirv.AvcSicPayloadINTEL = type opaque
; CHECK-LLVM: %spirv.AvcMceResultINTEL = type opaque
; CHECK-LLVM: %spirv.AvcImeResultINTEL = type opaque
; CHECK-LLVM: %spirv.AvcRefResultINTEL = type opaque
; CHECK-LLVM: %spirv.AvcSicResultINTEL = type opaque
; CHECK-LLVM: %spirv.AvcImeResultSingleReferenceStreamoutINTEL = type opaque
; CHECK-LLVM: %spirv.AvcImeResultDualReferenceStreamoutINTEL = type opaque
; CHECK-LLVM: %spirv.AvcImeSingleReferenceStreaminINTEL = type opaque
; CHECK-LLVM: %spirv.AvcImeDualReferenceStreaminINTEL = type opaque

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir"

%opencl.intel_sub_group_avc_mce_payload_t = type opaque
%opencl.intel_sub_group_avc_ime_payload_t = type opaque
%opencl.intel_sub_group_avc_ref_payload_t = type opaque
%opencl.intel_sub_group_avc_sic_payload_t = type opaque
%opencl.intel_sub_group_avc_mce_result_t = type opaque
%opencl.intel_sub_group_avc_ime_result_t = type opaque
%opencl.intel_sub_group_avc_ref_result_t = type opaque
%opencl.intel_sub_group_avc_sic_result_t = type opaque
%opencl.intel_sub_group_avc_ime_result_single_reference_streamout_t = type opaque
%opencl.intel_sub_group_avc_ime_result_dual_reference_streamout_t = type opaque
%opencl.intel_sub_group_avc_ime_single_reference_streamin_t = type opaque
%opencl.intel_sub_group_avc_ime_dual_reference_streamin_t = type opaque

; Function Attrs: noinline nounwind optnone
define spir_func void @foo() #0 {
entry:
  %payload_mce = alloca %opencl.intel_sub_group_avc_mce_payload_t*, align 4
  %payload_ime = alloca %opencl.intel_sub_group_avc_ime_payload_t*, align 4
  %payload_ref = alloca %opencl.intel_sub_group_avc_ref_payload_t*, align 4
  %payload_sic = alloca %opencl.intel_sub_group_avc_sic_payload_t*, align 4
  %result_mce = alloca %opencl.intel_sub_group_avc_mce_result_t*, align 4
  %result_ime = alloca %opencl.intel_sub_group_avc_ime_result_t*, align 4
  %result_ref = alloca %opencl.intel_sub_group_avc_ref_result_t*, align 4
  %result_sic = alloca %opencl.intel_sub_group_avc_sic_result_t*, align 4
  %sstreamout = alloca %opencl.intel_sub_group_avc_ime_result_single_reference_streamout_t*, align 4
  %dstreamout = alloca %opencl.intel_sub_group_avc_ime_result_dual_reference_streamout_t*, align 4
  %sstreamin = alloca %opencl.intel_sub_group_avc_ime_single_reference_streamin_t*, align 4
  %dstreamin = alloca %opencl.intel_sub_group_avc_ime_dual_reference_streamin_t*, align 4
  store %opencl.intel_sub_group_avc_ime_payload_t* null, %opencl.intel_sub_group_avc_ime_payload_t** %payload_ime, align 4
  store %opencl.intel_sub_group_avc_ref_payload_t* null, %opencl.intel_sub_group_avc_ref_payload_t** %payload_ref, align 4
  store %opencl.intel_sub_group_avc_sic_payload_t* null, %opencl.intel_sub_group_avc_sic_payload_t** %payload_sic, align 4
  store %opencl.intel_sub_group_avc_ime_result_t* null, %opencl.intel_sub_group_avc_ime_result_t** %result_ime, align 4
  store %opencl.intel_sub_group_avc_ref_result_t* null, %opencl.intel_sub_group_avc_ref_result_t** %result_ref, align 4
  store %opencl.intel_sub_group_avc_sic_result_t* null, %opencl.intel_sub_group_avc_sic_result_t** %result_sic, align 4
  store %opencl.intel_sub_group_avc_ime_result_single_reference_streamout_t* null, %opencl.intel_sub_group_avc_ime_result_single_reference_streamout_t** %sstreamout, align 4
  store %opencl.intel_sub_group_avc_ime_result_dual_reference_streamout_t* null, %opencl.intel_sub_group_avc_ime_result_dual_reference_streamout_t** %dstreamout, align 4
  store %opencl.intel_sub_group_avc_ime_single_reference_streamin_t* null, %opencl.intel_sub_group_avc_ime_single_reference_streamin_t** %sstreamin, align 4
  store %opencl.intel_sub_group_avc_ime_dual_reference_streamin_t* null, %opencl.intel_sub_group_avc_ime_dual_reference_streamin_t** %dstreamin, align 4
  ret void
}

attributes #0 = { noinline nounwind optnone "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!opencl.enable.FP_CONTRACT = !{}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!2}
!opencl.compiler.options = !{!2}
!llvm.ident = !{!3}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, i32 2}
!2 = !{}
!3 = !{!"clang version 5.0.1 (cfe/trunk)"}
