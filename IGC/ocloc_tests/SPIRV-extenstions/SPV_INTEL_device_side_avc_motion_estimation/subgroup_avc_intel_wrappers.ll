; Source:
;
; #pragma OPENCL EXTENSION cl_intel_device_side_avc_motion_estimation : enable
; void  test(__read_only image2d_t src,
;            __read_only image2d_t ref,
;            sampler_t sampler) {
;
;   intel_sub_group_avc_ime_payload_t ime_payload;
;   ime_payload = intel_sub_group_avc_ime_set_inter_base_multi_reference_penalty(
;     0, ime_payload);
;
;   intel_sub_group_avc_ime_result_t ime_result;
;   intel_sub_group_avc_ime_get_motion_vectors(ime_result);
;
;   intel_sub_group_avc_ref_payload_t ref_payload;
;   ref_payload = intel_sub_group_avc_ref_set_inter_shape_penalty(0, ref_payload);
;
;   intel_sub_group_avc_ref_result_t ref_result;
;   intel_sub_group_avc_ref_get_inter_distortions(ref_result);
;
;   intel_sub_group_avc_sic_payload_t sic_payload;
;   sic_payload = intel_sub_group_avc_sic_set_motion_vector_cost_function(
;     0, 0, 0, sic_payload);
;
;   intel_sub_group_avc_sic_result_t sic_result;
;   intel_sub_group_avc_sic_get_inter_distortions(sic_result);
; }

; UNSUPPORTED: system-windows
; REQUIRES: llvm-spirv, regkeys, dg2-supported, llvm-16-plus

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=1 %s -o %t.bc
; RUN: llvm-spirv %t.bc -opaque-pointers=1 --spirv-ext=+SPV_INTEL_device_side_avc_motion_estimation -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'EnableOpaquePointersBackend=1,ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

; The test checks that 'cl_intel_device_side_avc_motion_estimation' wrapper built-ins correctly
; translated to 'SPV_INTEL_device_side_avc_motion_estimation' extension instructions.

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64"

; CHECK-LLVM: spirv.AvcImePayloadINTEL
; CHECK-LLVM: spirv.AvcImeResultINTEL
; CHECK-LLVM: spirv.AvcRefPayloadINTEL
; CHECK-LLVM: spirv.AvcRefResultINTEL
; CHECK-LLVM: spirv.AvcSicPayloadINTEL
; CHECK-LLVM: spirv.AvcSicResultINTEL

; Function Attrs: convergent noinline nounwind optnone
define spir_func void @test() #0 {
entry:
  %ime_payload = alloca target("spirv.AvcImePayloadINTEL"), align 8
  %ime_result = alloca target("spirv.AvcImeResultINTEL"), align 8
  %ref_payload = alloca target("spirv.AvcRefPayloadINTEL"), align 8
  %ref_result = alloca target("spirv.AvcRefResultINTEL"), align 8
  %sic_payload = alloca target("spirv.AvcSicPayloadINTEL"), align 8
  %sic_result = alloca target("spirv.AvcSicResultINTEL"), align 8

; CHECK-LLVM: [[LOAD0:%.*]] = load target("spirv.AvcImePayloadINTEL"), ptr
; CHECK-LLVM: [[LOAD1:%.*]] = load target("spirv.AvcImeResultINTEL"), ptr
; CHECK-LLVM: [[LOAD2:%.*]] = load target("spirv.AvcRefPayloadINTEL"), ptr
; CHECK-LLVM: [[LOAD3:%.*]] = load target("spirv.AvcRefResultINTEL"), ptr
; CHECK-LLVM: [[LOAD4:%.*]] = load target("spirv.AvcSicPayloadINTEL"), ptr
; CHECK-LLVM: [[LOAD5:%.*]] = load target("spirv.AvcSicResultINTEL"), ptr
  %0 = load target("spirv.AvcImePayloadINTEL"), target("spirv.AvcImePayloadINTEL")* %ime_payload, align 8
  %1 = load target("spirv.AvcImeResultINTEL"), target("spirv.AvcImeResultINTEL")* %ime_result, align 8
  %2 = load target("spirv.AvcRefPayloadINTEL"), target("spirv.AvcRefPayloadINTEL")* %ref_payload, align 8
  %3 = load target("spirv.AvcRefResultINTEL"), target("spirv.AvcRefResultINTEL")* %ref_result, align 8
  %4 = load target("spirv.AvcSicPayloadINTEL"), target("spirv.AvcSicPayloadINTEL")* %sic_payload, align 8
  %5 = load target("spirv.AvcSicResultINTEL"), target("spirv.AvcSicResultINTEL")* %sic_result, align 8

  %call0 = call spir_func target("spirv.AvcImePayloadINTEL") @_Z62intel_sub_group_avc_ime_set_inter_base_multi_reference_penaltyh37ocl_intel_sub_group_avc_ime_payload_t(i8 zeroext 0, target("spirv.AvcImePayloadINTEL") %0) #2
  %call1 = call spir_func i64 @_Z42intel_sub_group_avc_ime_get_motion_vectors36ocl_intel_sub_group_avc_ime_result_t(target("spirv.AvcImeResultINTEL") %1) #2
  %call2 = call spir_func target("spirv.AvcRefPayloadINTEL") @_Z47intel_sub_group_avc_ref_set_inter_shape_penaltym37ocl_intel_sub_group_avc_ref_payload_t(i64 0, target("spirv.AvcRefPayloadINTEL") %2) #2
  %call3 = call spir_func zeroext i16 @_Z45intel_sub_group_avc_ref_get_inter_distortions36ocl_intel_sub_group_avc_ref_result_t(target("spirv.AvcRefResultINTEL") %3) #2
  %call4 = call spir_func target("spirv.AvcSicPayloadINTEL") @_Z55intel_sub_group_avc_sic_set_motion_vector_cost_functionmDv2_jh37ocl_intel_sub_group_avc_sic_payload_t(i64 0, <2 x i32> zeroinitializer, i8 zeroext 0, target("spirv.AvcSicPayloadINTEL") %4) #2
  %call5 = call spir_func zeroext i16 @_Z45intel_sub_group_avc_sic_get_inter_distortions36ocl_intel_sub_group_avc_sic_result_t(target("spirv.AvcSicResultINTEL") %5) #2
  ret void
}

; Function Attrs: convergent
declare spir_func target("spirv.AvcImePayloadINTEL") @_Z62intel_sub_group_avc_ime_set_inter_base_multi_reference_penaltyh37ocl_intel_sub_group_avc_ime_payload_t(i8 zeroext, target("spirv.AvcImePayloadINTEL")) #1

; Function Attrs: convergent
declare spir_func i64 @_Z42intel_sub_group_avc_ime_get_motion_vectors36ocl_intel_sub_group_avc_ime_result_t(target("spirv.AvcImeResultINTEL")) #1

; Function Attrs: convergent
declare spir_func target("spirv.AvcRefPayloadINTEL") @_Z47intel_sub_group_avc_ref_set_inter_shape_penaltym37ocl_intel_sub_group_avc_ref_payload_t(i64, target("spirv.AvcRefPayloadINTEL")) #1

; Function Attrs: convergent
declare spir_func zeroext i16 @_Z45intel_sub_group_avc_ref_get_inter_distortions36ocl_intel_sub_group_avc_ref_result_t(target("spirv.AvcRefResultINTEL")) #1

; Function Attrs: convergent
declare spir_func target("spirv.AvcSicPayloadINTEL") @_Z55intel_sub_group_avc_sic_set_motion_vector_cost_functionmDv2_jh37ocl_intel_sub_group_avc_sic_payload_t(i64, <2 x i32>, i8 zeroext, target("spirv.AvcSicPayloadINTEL")) #1

; Function Attrs: convergent
declare spir_func zeroext i16 @_Z45intel_sub_group_avc_sic_get_inter_distortions36ocl_intel_sub_group_avc_sic_result_t(target("spirv.AvcSicResultINTEL")) #1

attributes #0 = { convergent noinline nounwind optnone "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { convergent "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { convergent }

!llvm.module.flags = !{!0}
!opencl.enable.FP_CONTRACT = !{}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!3}
!opencl.compiler.options = !{!2}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, i32 2}
!2 = !{}
!3 = !{!"cl_images"}
!4 = !{!"clang version 6.0.0"}
