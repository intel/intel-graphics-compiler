;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --typed-pointers --igc-extension-funcs-resolution -S < %s | FileCheck %s
; ------------------------------------------------
; ExtensionFuncsResolution
; ------------------------------------------------

define spir_kernel void @test_vme_block_type(i32 %vmeMbBlockType) {
; CHECK-LABEL: @test_vme_block_type(
; CHECK:    call void @use.i32(i32 %vmeMbBlockType)
; CHECK:    ret void
;
  %1 = call spir_func i32 @__builtin_IB_vme_mb_block_type()
  call void @use.i32(i32 %1)
  ret void
}

define spir_kernel void @test_vme_subpix(i32 %vmeSubpixelMode) {
; CHECK-LABEL: @test_vme_subpix(
; CHECK:    call void @use.i32(i32 %vmeSubpixelMode)
; CHECK:    ret void
;
  %1 = call spir_func i32 @__builtin_IB_vme_subpixel_mode()
  call void @use.i32(i32 %1)
  ret void
}

define spir_kernel void @test_vme_sad(i32 %vmeSadAdjustMode) {
; CHECK-LABEL: @test_vme_sad(
; CHECK:    call void @use.i32(i32 %vmeSadAdjustMode)
; CHECK:    ret void
;
  %1 = call spir_func i32 @__builtin_IB_vme_sad_adjust_mode()
  call void @use.i32(i32 %1)
  ret void
}

define spir_kernel void @test_vme_search(i32 %vmeSearchPathType) {
; CHECK-LABEL: @test_vme_search(
; CHECK:    call void @use.i32(i32 %vmeSearchPathType)
; CHECK:    ret void
;
  %1 = call spir_func i32 @__builtin_IB_vme_search_path_type()
  call void @use.i32(i32 %1)
  ret void
}

%struct.mce_payload_t = type opaque

define spir_kernel void @test_vme_helper(%struct.mce_payload_t* %src1) {
; CHECK-LABEL: @test_vme_helper(
; CHECK:    [[TMP1:%.*]] = bitcast %struct.mce_payload_t* [[SRC1:%.*]] to <4 x i32>*
; CHECK:    [[TMP2:%.*]] = load <4 x i32>, <4 x i32>* [[TMP1]]
; CHECK:    call void @use.4i32(<4 x i32> [[TMP2]])
; CHECK:    ret void
;
  %1 = call spir_func <4 x i32> @__builtin_IB_vme_helper_get_handle_avc_mce_payload_t(%struct.mce_payload_t* %src1)
  call void @use.4i32(<4 x i32> %1)
  ret void
}

define spir_kernel void @test_vme_helper_as(<4 x i32> %src1) {
; CHECK-LABEL: @test_vme_helper_as(
; CHECK:    [[TMP1:%.*]] = alloca <4 x i32>
; CHECK:    store <4 x i32> [[SRC1:%.*]], <4 x i32>* [[TMP1]]
; CHECK:    [[TMP2:%.*]] = bitcast <4 x i32>* [[TMP1]] to %struct.mce_payload_t*
; CHECK:    call void @use.pstr(%struct.mce_payload_t* [[TMP2]])
; CHECK:    ret void
;
  %1 = call spir_func %struct.mce_payload_t* @__builtin_IB_vme_helper_get_as_avc_mce_payload_t(<4 x i32> %src1)
  call void @use.pstr(%struct.mce_payload_t* %1)
  ret void
}


declare void @use.i32(i32)
declare void @use.4i32(<4 x i32>)
declare void @use.pstr(%struct.mce_payload_t*)

declare spir_func i32 @__builtin_IB_vme_mb_block_type()
declare spir_func i32 @__builtin_IB_vme_subpixel_mode()
declare spir_func i32 @__builtin_IB_vme_sad_adjust_mode()
declare spir_func i32 @__builtin_IB_vme_search_path_type()
declare spir_func <4 x i32> @__builtin_IB_vme_helper_get_handle_avc_mce_payload_t(%struct.mce_payload_t*)
declare spir_func %struct.mce_payload_t* @__builtin_IB_vme_helper_get_as_avc_mce_payload_t(<4 x i32>)

!igc.functions = !{!0, !5, !9, !13, !17, !20}

!0 = !{void (i32)* @test_vme_block_type, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc",  !4}
!4 = !{i32 33}


!5 = !{void (i32)* @test_vme_subpix, !6}
!6 = !{!2, !7}
!7 = !{!"implicit_arg_desc",  !8}
!8 = !{i32 34}

!9 = !{void (i32)* @test_vme_sad, !10}
!10 = !{!2, !11}
!11 = !{!"implicit_arg_desc",  !12}
!12 = !{i32 35}

!13 = !{void (i32)* @test_vme_search, !14}
!14 = !{!2, !15}
!15 = !{!"implicit_arg_desc",  !16}
!16 = !{i32 36}

!17 = !{void (%struct.mce_payload_t*)* @test_vme_helper, !18}
!18 = !{!2, !19}
!19 = !{!"implicit_arg_desc"}

!20 = !{void (<4 x i32>)* @test_vme_helper_as, !21}
!21 = !{!2, !22}
!22 = !{!"implicit_arg_desc"}
