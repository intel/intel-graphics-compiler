;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 \
; RUN: -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPC \
; RUN: -mtriple=spir64-unknown-unknown -S < %s | FileCheck --check-prefix=CHECK-ADD3 %s

; CHECK-LABEL: alu_kernel

; CHECK: [[CAST_A:%.*]] = bitcast <16 x i64> [[IN_A:%.*]] to <32 x i32>
; CHECK: [[CAST_A_LO:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[CAST_A]], i32 0, i32 16, i32 2, i16 0, i32 undef)
; CHECK: [[CAST_A_HI:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[CAST_A]], i32 0, i32 16, i32 2, i16 4, i32 undef)
; CHECK: [[CAST_B:%.*]] = bitcast <16 x i64> [[IN_B:%.*]] to <32 x i32>
; CHECK: [[CAST_B_LO:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[CAST_B]], i32 0, i32 16, i32 2, i16 0, i32 undef)
; CHECK: [[CAST_B_HI:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[CAST_B]], i32 0, i32 16, i32 2, i16 4, i32 undef)
; CHECK: [[ADDC_A_B_LO:%.*]] = call { <16 x i32>, <16 x i32> } @llvm.genx.addc.v16i32.v16i32(<16 x i32> [[CAST_A_LO]], <16 x i32> [[CAST_B_LO]])
; CHECK: [[ADDC_A_B_LO_X:%.*]] = extractvalue { <16 x i32>, <16 x i32> } [[ADDC_A_B_LO]], 1
; CHECK: [[ADDC_A_B_LO_C:%.*]] = extractvalue { <16 x i32>, <16 x i32> } [[ADDC_A_B_LO]], 0
; CHECK: [[ADDC_A_B_HI:%.*]] = call { <16 x i32>, <16 x i32> } @llvm.genx.addc.v16i32.v16i32(<16 x i32> [[CAST_A_HI]], <16 x i32> [[CAST_B_HI]])
; CHECK: [[ADDC_A_B_HI_X:%.*]] = extractvalue { <16 x i32>, <16 x i32> } [[ADDC_A_B_HI]], 1
; CHECK: [[ADDC_A_B_HI_C:%.*]] = extractvalue { <16 x i32>, <16 x i32> } [[ADDC_A_B_HI]], 0
; CHECK: [[ADDC_X_C_HI:%.*]] = call { <16 x i32>, <16 x i32> } @llvm.genx.addc.v16i32.v16i32(<16 x i32> [[ADDC_A_B_HI_X]], <16 x i32> [[ADDC_A_B_LO_C]])
; CHECK: [[ADDC_X_C_HI_X:%.*]] = extractvalue { <16 x i32>, <16 x i32> } [[ADDC_X_C_HI]], 1
; CHECK: [[ADDC_X_C_HI_C:%.*]] = extractvalue { <16 x i32>, <16 x i32> } [[ADDC_X_C_HI]], 0
; CHECK: [[PART_RESULT_X:%.*]] = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> [[ADDC_A_B_LO_X]], i32 0, i32 16, i32 2, i16 0, i32 undef, i1 true)
; CHECK: [[RESULT_X_:%.*]] = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> [[PART_RESULT_X]], <16 x i32> [[ADDC_X_C_HI_X]], i32 0, i32 16, i32 2, i16 4, i32 undef, i1 true)
; CHECK: [[RESULT_X:%.*]] = bitcast <32 x i32> [[RESULT_X_]] to <16 x i64>
; CHECK: [[RESULT_C_:%.*]] = or <16 x i32> [[ADDC_X_C_HI_C]], [[ADDC_A_B_HI_C]]
; CHECK: [[RESULT_C:%.*]] = zext <16 x i32> [[RESULT_C_]] to <16 x i64>
; CHECK: [[RESULT_:%.*]] = insertvalue { <16 x i64>, <16 x i64> } undef, <16 x i64> [[RESULT_X]], 1
; CHECK: [[RESULT:%.*]] = insertvalue { <16 x i64>, <16 x i64> } [[RESULT_]], <16 x i64> [[RESULT_C]], 0


; CHECK-ADD3: [[CAST_A:%.*]] = bitcast <16 x i64> [[IN_A:%.*]] to <32 x i32>
; CHECK-ADD3: [[CAST_A_LO:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[CAST_A]], i32 0, i32 16, i32 2, i16 0, i32 undef)
; CHECK-ADD3: [[CAST_A_HI:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[CAST_A]], i32 0, i32 16, i32 2, i16 4, i32 undef)
; CHECK-ADD3: [[CAST_B:%.*]] = bitcast <16 x i64> [[IN_B:%.*]] to <32 x i32>
; CHECK-ADD3: [[CAST_B_LO:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[CAST_B]], i32 0, i32 16, i32 2, i16 0, i32 undef)
; CHECK-ADD3: [[CAST_B_HI:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[CAST_B]], i32 0, i32 16, i32 2, i16 4, i32 undef)
; CHECK-ADD3: [[ADDC_A_B_LO:%.*]] = call { <16 x i32>, <16 x i32> } @llvm.genx.addc.v16i32.v16i32(<16 x i32> [[CAST_A_LO]], <16 x i32> [[CAST_B_LO]])
; CHECK-ADD3: [[ADDC_A_B_LO_X:%.*]] = extractvalue { <16 x i32>, <16 x i32> } [[ADDC_A_B_LO]], 1
; CHECK-ADD3: [[ADDC_A_B_LO_C:%.*]] = extractvalue { <16 x i32>, <16 x i32> } [[ADDC_A_B_LO]], 0
; CHECK-ADD3: [[ADD3C_A_B_C_LO:%.*]] = call { <16 x i1>, <16 x i32> } @llvm.genx.add3c.v16i1.v16i32(<16 x i32> [[CAST_A_HI]], <16 x i32> [[CAST_B_HI]], <16 x i32> [[ADDC_A_B_LO_C]])
; CHECK-ADD3: [[ADD3C_A_B_C_LO_X:%.*]] = extractvalue { <16 x i1>, <16 x i32> } [[ADD3C_A_B_C_LO]], 1
; CHECK-ADD3: [[PART_RESULT_X:%.*]] = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> [[ADDC_A_B_LO_X]], i32 0, i32 16, i32 2, i16 0, i32 undef, i1 true)
; CHECK-ADD3: [[RESULT_X_:%.*]] = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> [[PART_RESULT_X]], <16 x i32> [[ADD3C_A_B_C_LO_X]], i32 0, i32 16, i32 2, i16 4, i32 undef, i1 true)
; CHECK-ADD3: [[RESULT_X:%.*]] = bitcast <32 x i32> [[RESULT_X_]] to <16 x i64>
; CHECK-ADD3: [[ADD3C_A_B_C_LO_C:%.*]] = extractvalue { <16 x i1>, <16 x i32> } [[ADD3C_A_B_C_LO]], 0
; CHECK-ADD3: [[RESULT_C_:%.*]] = select <16 x i1> [[ADD3C_A_B_C_LO_C]], <16 x i64> <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>, <16 x i64> zeroinitializer
; CHECK-ADD3: [[RESULT_:%.*]] = insertvalue { <16 x i64>, <16 x i64> } undef, <16 x i64> [[RESULT_X]], 1
; CHECK-ADD3: [[RESULT:%.*]] = insertvalue { <16 x i64>, <16 x i64> } [[RESULT_]], <16 x i64> [[RESULT_C_]], 0

; Function Attrs: nofree nosync nounwind readnone
declare { <16 x i64>, <16 x i64> } @llvm.genx.addc.v16i64.v16i64(<16 x i64>, <16 x i64>) #0

; Function Attrs: nofree nosync nounwind readnone
declare !genx_intrinsic_id !17 i32 @llvm.genx.group.id.x() #0

; Function Attrs: mustprogress nofree noinline nosync nounwind willreturn
define dllexport spir_kernel void @"alu_kernel<unsigned long long, unsigned long long, unsigned long long>"(i8 addrspace(1)* %ibuf1, i8 addrspace(1)* %ibuf2, i8 addrspace(1)* %ibuf3, i8 addrspace(1)* %obuf, i64 %param1, i64 %param2, i64 %param3, <3 x i16> %impl.arg.llvm.genx.local.id16, <3 x i32> %impl.arg.llvm.genx.local.size, i64 %impl.arg.private.base) local_unnamed_addr #1 {
entry:
  %0 = ptrtoint i8 addrspace(1)* %ibuf1 to i64
  %1 = ptrtoint i8 addrspace(1)* %ibuf2 to i64
  %2 = ptrtoint i8 addrspace(1)* %ibuf3 to i64
  %3 = ptrtoint i8 addrspace(1)* %obuf to i64
  %call.i.i.i = tail call i32 @llvm.genx.group.id.x() #2
  %vecext.i.i23 = extractelement <3 x i32> %impl.arg.llvm.genx.local.size, i64 0
  %mul = mul i32 %call.i.i.i, %vecext.i.i23
  %4 = extractelement <3 x i16> %impl.arg.llvm.genx.local.id16, i64 0
  %vecext.i.i14 = zext i16 %4 to i32
  %add = add i32 %mul, %vecext.i.i14
  %mul3 = shl i32 %add, 4
  %conv.i11 = sext i32 %mul3 to i64
  %mul.i = shl nsw i64 %conv.i11, 3
  %add.i = add i64 %mul.i, %0
  %5 = inttoptr i64 %add.i to <16 x i64> addrspace(1)*
  %6 = load <16 x i64>, <16 x i64> addrspace(1)* %5, align 16
  %add.i38 = add i64 %mul.i, %1
  %7 = inttoptr i64 %add.i38 to <16 x i64> addrspace(1)*
  %8 = load <16 x i64>, <16 x i64> addrspace(1)* %7, align 16
  %add.i30 = add i64 %mul.i, %2
  %9 = inttoptr i64 %add.i30 to <16 x i64> addrspace(1)*
  %call1.i = tail call { <16 x i64>, <16 x i64> } @llvm.genx.addc.v16i64.v16i64(<16 x i64> %6, <16 x i64> %8)
  %10 = extractvalue { <16 x i64>, <16 x i64> } %call1.i, 0
  %11 = extractvalue { <16 x i64>, <16 x i64> } %call1.i, 1
  %add.i.i = add i64 %mul.i, %3
  %12 = inttoptr i64 %add.i.i to <16 x i64> addrspace(1)*
  store <16 x i64> %11, <16 x i64> addrspace(1)* %12, align 16
  store <16 x i64> %10, <16 x i64> addrspace(1)* %9, align 16
  ret void
}

attributes #0 = { nofree nosync nounwind readnone }
attributes #1 = { mustprogress nofree noinline nosync nounwind willreturn "CMGenxMain" "oclrt"="1" }
attributes #2 = { nounwind }

!spirv.MemoryModel = !{!0}
!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!1}
!opencl.spir.version = !{!2, !3, !3, !3}
!opencl.ocl.version = !{!1, !3, !3, !3}
!opencl.used.extensions = !{!4}
!opencl.used.optional.core.features = !{!4}
!spirv.Generator = !{!5}
!genx.kernels = !{!6}
!llvm.ident = !{!11, !11, !11}
!llvm.module.flags = !{!12}
!genx.kernel.internal = !{!13}

!0 = !{i32 2, i32 2}
!1 = !{i32 0, i32 0}
!2 = !{i32 1, i32 2}
!3 = !{i32 2, i32 0}
!4 = !{}
!5 = !{i16 6, i16 14}
!6 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*, i64, i64, i64, <3 x i16>, <3 x i32>, i64)* @"alu_kernel<unsigned long long, unsigned long long, unsigned long long>", !"alu_kernel<unsigned long long, unsigned long long, unsigned long long>", !7, i32 0, !8, !9, !10, i32 0}
!7 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 24, i32 8, i32 96}
!8 = !{i32 88, i32 96, i32 104, i32 112, i32 120, i32 128, i32 136, i32 32, i32 64, i32 80}
!9 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!10 = !{!"svmptr_t", !"svmptr_t", !"svmptr_t", !"svmptr_t", !"", !"", !""}
!11 = !{!"Ubuntu clang version 14.0.6"}
!12 = !{i32 1, !"wchar_size", i32 4}
!13 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*, i64, i64, i64, <3 x i16>, <3 x i32>, i64)* @"alu_kernel<unsigned long long, unsigned long long, unsigned long long>", !14, !15, !4, !16}
!14 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!15 = !{i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9}
!16 = !{i32 255, i32 255, i32 255, i32 255, i32 -1, i32 -1, i32 -1, i32 255, i32 255, i32 255}
!17 = !{i32 10880}
