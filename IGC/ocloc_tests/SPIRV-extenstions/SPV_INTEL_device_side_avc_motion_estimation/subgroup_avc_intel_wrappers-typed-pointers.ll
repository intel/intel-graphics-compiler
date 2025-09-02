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

; This test won't work with LLVM 16+ due to different representation of OpenCL builtin types (pointers to opaque structs vs TargetExtTy).
; It would work on LLVM 16 with typed pointers forced (in CMake), but not on a default build of IGC with LLVM 16.
; REQUIRES: llvm-spirv, regkeys, dg2-supported, llvm-15-or-older

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_device_side_avc_motion_estimation -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

; The test checks that 'cl_intel_device_side_avc_motion_estimation' wrapper built-ins correctly
; translated to 'SPV_INTEL_device_side_avc_motion_estimation' extension instructions.

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64"

; CHECK-LLVM: %spirv.AvcImePayloadINTEL = type opaque
; CHECK-LLVM: %spirv.AvcImeResultINTEL = type opaque
; CHECK-LLVM: %spirv.AvcRefPayloadINTEL = type opaque
; CHECK-LLVM: %spirv.AvcRefResultINTEL = type opaque
; CHECK-LLVM: %spirv.AvcSicPayloadINTEL = type opaque
; CHECK-LLVM: %spirv.AvcSicResultINTEL = type opaque
; CHECK-LLVM: %spirv.AvcMcePayloadINTEL = type opaque
; CHECK-LLVM: %spirv.AvcMceResultINTEL = type opaque

%opencl.intel_sub_group_avc_ime_payload_t = type opaque
%opencl.intel_sub_group_avc_ime_result_t = type opaque
%opencl.intel_sub_group_avc_ref_payload_t = type opaque
%opencl.intel_sub_group_avc_ref_result_t = type opaque
%opencl.intel_sub_group_avc_sic_payload_t = type opaque
%opencl.intel_sub_group_avc_sic_result_t = type opaque

; Function Attrs: convergent noinline nounwind optnone
define spir_func void @test() #0 {
entry:

  %ime_payload = alloca %opencl.intel_sub_group_avc_ime_payload_t*, align 8
  %ime_result = alloca %opencl.intel_sub_group_avc_ime_result_t*, align 8
  %ref_payload = alloca %opencl.intel_sub_group_avc_ref_payload_t*, align 8
  %ref_result = alloca %opencl.intel_sub_group_avc_ref_result_t*, align 8
  %sic_payload = alloca %opencl.intel_sub_group_avc_sic_payload_t*, align 8
  %sic_result = alloca %opencl.intel_sub_group_avc_sic_result_t*, align 8

; CHECK-LLVM: [[LOAD0:%.*]] = load %spirv.AvcImePayloadINTEL*, %spirv.AvcImePayloadINTEL** %
; CHECK-LLVM: [[LOAD1:%.*]] = load %spirv.AvcImeResultINTEL*, %spirv.AvcImeResultINTEL** %
; CHECK-LLVM: [[LOAD2:%.*]] = load %spirv.AvcRefPayloadINTEL*, %spirv.AvcRefPayloadINTEL** %
; CHECK-LLVM: [[LOAD3:%.*]] = load %spirv.AvcRefResultINTEL*, %spirv.AvcRefResultINTEL** %
; CHECK-LLVM: [[LOAD4:%.*]] = load %spirv.AvcSicPayloadINTEL*, %spirv.AvcSicPayloadINTEL** %
; CHECK-LLVM: [[LOAD5:%.*]] = load %spirv.AvcSicResultINTEL*, %spirv.AvcSicResultINTEL** %
  %0 = load %opencl.intel_sub_group_avc_ime_payload_t*, %opencl.intel_sub_group_avc_ime_payload_t** %ime_payload, align 8
  %1 = load %opencl.intel_sub_group_avc_ime_result_t*, %opencl.intel_sub_group_avc_ime_result_t** %ime_result, align 8
  %2 = load %opencl.intel_sub_group_avc_ref_payload_t*, %opencl.intel_sub_group_avc_ref_payload_t** %ref_payload, align 8
  %3 = load %opencl.intel_sub_group_avc_ref_result_t*, %opencl.intel_sub_group_avc_ref_result_t** %ref_result, align 8
  %4 = load %opencl.intel_sub_group_avc_sic_payload_t*, %opencl.intel_sub_group_avc_sic_payload_t** %sic_payload, align 8
  %5 = load %opencl.intel_sub_group_avc_sic_result_t*, %opencl.intel_sub_group_avc_sic_result_t** %sic_result, align 8

; CHECK-LLVM: [[CALL01:%.*]] = call spir_func %spirv.AvcMcePayloadINTEL* @_Z46__spirv_SubgroupAvcImeConvertToMcePayloadINTELP26__spirv_AvcImePayloadINTEL(%spirv.AvcImePayloadINTEL* [[LOAD0]])
; CHECK-LLVM: [[CALL02:%.*]] = call spir_func %spirv.AvcMcePayloadINTEL* @_Z60__spirv_SubgroupAvcMceSetInterBaseMultiReferencePenaltyINTELhP26__spirv_AvcMcePayloadINTEL(i8 0, %spirv.AvcMcePayloadINTEL* [[CALL01]])
; CHECK-LLVM: [[CALL0:%.*]] = call spir_func %spirv.AvcImePayloadINTEL* @_Z46__spirv_SubgroupAvcMceConvertToImePayloadINTELP26__spirv_AvcMcePayloadINTEL(%spirv.AvcMcePayloadINTEL* [[CALL02]])
  %call0 = call spir_func %opencl.intel_sub_group_avc_ime_payload_t* @_Z62intel_sub_group_avc_ime_set_inter_base_multi_reference_penaltyh37ocl_intel_sub_group_avc_ime_payload_t(i8 zeroext 0, %opencl.intel_sub_group_avc_ime_payload_t* %0) #2
; CHECK-LLVM: [[CALL11:%.*]] = call spir_func %spirv.AvcMceResultINTEL* @_Z45__spirv_SubgroupAvcImeConvertToMceResultINTELP25__spirv_AvcImeResultINTEL(%spirv.AvcImeResultINTEL* [[LOAD1]])
; CHECK-LLVM: [[CALL1:%.*]] = call spir_func i64 @_Z43__spirv_SubgroupAvcMceGetMotionVectorsINTELP25__spirv_AvcMceResultINTEL(%spirv.AvcMceResultINTEL* [[CALL11]])
  %call1 = call spir_func i64 @_Z42intel_sub_group_avc_ime_get_motion_vectors36ocl_intel_sub_group_avc_ime_result_t(%opencl.intel_sub_group_avc_ime_result_t* %1) #2
; CHECK-LLVM: [[CALL21:%.*]] = call spir_func %spirv.AvcMcePayloadINTEL* @_Z46__spirv_SubgroupAvcRefConvertToMcePayloadINTELP26__spirv_AvcRefPayloadINTEL(%spirv.AvcRefPayloadINTEL* [[LOAD2]])
; CHECK-LLVM: [[CALL22:%.*]] = call spir_func %spirv.AvcMcePayloadINTEL* @_Z47__spirv_SubgroupAvcMceSetInterShapePenaltyINTELmP26__spirv_AvcMcePayloadINTEL(i64 0, %spirv.AvcMcePayloadINTEL* [[CALL21]])
; CHECK-LLVM: [[CALL2:%.*]] = call spir_func %spirv.AvcRefPayloadINTEL* @_Z46__spirv_SubgroupAvcMceConvertToRefPayloadINTELP26__spirv_AvcMcePayloadINTEL(%spirv.AvcMcePayloadINTEL* [[CALL22]])
  %call2 = call spir_func %opencl.intel_sub_group_avc_ref_payload_t* @_Z47intel_sub_group_avc_ref_set_inter_shape_penaltym37ocl_intel_sub_group_avc_ref_payload_t(i64 0, %opencl.intel_sub_group_avc_ref_payload_t* %2) #2
; CHECK-LLVM: [[CALL31:%.*]] = call spir_func %spirv.AvcMceResultINTEL* @_Z45__spirv_SubgroupAvcRefConvertToMceResultINTELP25__spirv_AvcRefResultINTEL(%spirv.AvcRefResultINTEL* [[LOAD3]])
; CHECK-LLVM: [[CALL3:%.*]] = call spir_func i16 @_Z46__spirv_SubgroupAvcMceGetInterDistortionsINTELP25__spirv_AvcMceResultINTEL(%spirv.AvcMceResultINTEL* [[CALL31]])
  %call3 = call spir_func zeroext i16 @_Z45intel_sub_group_avc_ref_get_inter_distortions36ocl_intel_sub_group_avc_ref_result_t(%opencl.intel_sub_group_avc_ref_result_t* %3) #2
; CHECK-LLVM: [[CALL41:%.*]] = call spir_func %spirv.AvcMcePayloadINTEL* @_Z46__spirv_SubgroupAvcSicConvertToMcePayloadINTELP26__spirv_AvcSicPayloadINTEL(%spirv.AvcSicPayloadINTEL* [[LOAD4]])
; CHECK-LLVM: [[CALL42:%.*]] = call spir_func %spirv.AvcMcePayloadINTEL* @_Z54__spirv_SubgroupAvcMceSetMotionVectorCostFunctionINTELmDv2_jhP26__spirv_AvcMcePayloadINTEL(i64 0, <2 x i32> zeroinitializer, i8 0, %spirv.AvcMcePayloadINTEL* [[CALL41]])
; CHECK-LLVM: [[CALL4:%.*]] = call spir_func %spirv.AvcSicPayloadINTEL* @_Z46__spirv_SubgroupAvcMceConvertToSicPayloadINTELP26__spirv_AvcMcePayloadINTEL(%spirv.AvcMcePayloadINTEL* [[CALL42]])
  %call4 = call spir_func %opencl.intel_sub_group_avc_sic_payload_t* @_Z55intel_sub_group_avc_sic_set_motion_vector_cost_functionmDv2_jh37ocl_intel_sub_group_avc_sic_payload_t(i64 0, <2 x i32> zeroinitializer, i8 zeroext 0, %opencl.intel_sub_group_avc_sic_payload_t* %4) #2
; CHECK-LLVM: [[CALL51:%.*]] = call spir_func %spirv.AvcMceResultINTEL* @_Z45__spirv_SubgroupAvcSicConvertToMceResultINTELP25__spirv_AvcSicResultINTEL(%spirv.AvcSicResultINTEL* [[LOAD5]])
; CHECK-LLVM: [[CALL5:%.*]] = call spir_func i16 @_Z46__spirv_SubgroupAvcMceGetInterDistortionsINTELP25__spirv_AvcMceResultINTEL(%spirv.AvcMceResultINTEL* [[CALL51]])
  %call5 = call spir_func zeroext i16 @_Z45intel_sub_group_avc_sic_get_inter_distortions36ocl_intel_sub_group_avc_sic_result_t(%opencl.intel_sub_group_avc_sic_result_t* %5) #2
  ret void
}

; Function Attrs: convergent
declare spir_func %opencl.intel_sub_group_avc_ime_payload_t* @_Z62intel_sub_group_avc_ime_set_inter_base_multi_reference_penaltyh37ocl_intel_sub_group_avc_ime_payload_t(i8 zeroext, %opencl.intel_sub_group_avc_ime_payload_t*) #1

; Function Attrs: convergent
declare spir_func i64 @_Z42intel_sub_group_avc_ime_get_motion_vectors36ocl_intel_sub_group_avc_ime_result_t(%opencl.intel_sub_group_avc_ime_result_t*) #1

; Function Attrs: convergent
declare spir_func %opencl.intel_sub_group_avc_ref_payload_t* @_Z47intel_sub_group_avc_ref_set_inter_shape_penaltym37ocl_intel_sub_group_avc_ref_payload_t(i64, %opencl.intel_sub_group_avc_ref_payload_t*) #1

; Function Attrs: convergent
declare spir_func zeroext i16 @_Z45intel_sub_group_avc_ref_get_inter_distortions36ocl_intel_sub_group_avc_ref_result_t(%opencl.intel_sub_group_avc_ref_result_t*) #1

; Function Attrs: convergent
declare spir_func %opencl.intel_sub_group_avc_sic_payload_t* @_Z55intel_sub_group_avc_sic_set_motion_vector_cost_functionmDv2_jh37ocl_intel_sub_group_avc_sic_payload_t(i64, <2 x i32>, i8 zeroext, %opencl.intel_sub_group_avc_sic_payload_t*) #1

; Function Attrs: convergent
declare spir_func zeroext i16 @_Z45intel_sub_group_avc_sic_get_inter_distortions36ocl_intel_sub_group_avc_sic_result_t(%opencl.intel_sub_group_avc_sic_result_t*) #1

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
