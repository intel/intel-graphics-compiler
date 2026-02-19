;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
;
; RUN: igc_opt --typed-pointers -igc-opencl-printf-resolution -S  < %s | FileCheck %s
; ------------------------------------------------
; OpenCLPrintfResolution
; ------------------------------------------------
;
; Was reduced from ocl test kernel:
; __kernel void test_printf(int src)
; {
;      float4 f = (float4)(1.0f, 2.0f, 3.0f, 4.0f);
;      uchar4 uc = (uchar4)(0xFA, 0xFB, 0xFC, 0xFD);
;      int i = 42;
;      float f1 = 4.2f;
;
;      printf("i, f = %d, %f\n", i, f1);
;      printf("f4 = %2.2v4hlf\n", f);
;      printf("uc = %#v4hhx\n", uc);
;      printf("%s\n", "this is a test string\n");
; }
;
; ------------------------------------------------

; Debugify fails, re-check

@.str = internal unnamed_addr addrspace(2) constant [15 x i8] c"i, f = %d, %f\0A\00", align 1
@.str.1 = internal unnamed_addr addrspace(2) constant [16 x i8] c"f4 = %2.2v4hlf\0A\00", align 1
@.str.2 = internal unnamed_addr addrspace(2) constant [14 x i8] c"uc = %#v4hhx\0A\00", align 1
@.str.3 = internal unnamed_addr addrspace(2) constant [4 x i8] c"%s\0A\00", align 1
@.str.4 = internal unnamed_addr addrspace(2) constant [23 x i8] c"this is a test string\0A\00", align 1

define spir_kernel void @test_printf(i32 %src, <8 x i32> %r0, <8 x i32> %payloadHeader, i8 addrspace(2)* %constBase, i8* %privateBase, i8 addrspace(1)* %printfBuffer) {
; CHECK-LABEL: @test_printf(
; CHECK:    [[F:%[A-z0-9]*]] = alloca <4 x float>, align 16
; CHECK:    [[UC:%[A-z0-9]*]] = alloca <4 x i8>, align 4
; CHECK:    [[I:%[A-z0-9]*]] = alloca i32, align 4
; CHECK:    [[F1:%[A-z0-9]*]] = alloca float, align 4
; CHECK:    store <4 x float> <float 1.000000e+00, float 2.000000e+00, float 3.000000e+00, float 4.000000e+00>, <4 x float>* [[F]], align 16
; CHECK:    store <4 x i8> <i8 -6, i8 -5, i8 -4, i8 -3>, <4 x i8>* [[UC]], align 4
; CHECK:    store i32 42, i32* [[I]], align 4
; CHECK:    store float 0x4010CCCCC0000000, float* [[F1]], align 4
;
; Print i, f1
;
; CHECK:    [[TMP1:%[A-z0-9]*]] = load i32, i32* [[I]], align 4
; CHECK:    [[TMP2:%[A-z0-9]*]] = load float, float* [[F1]], align 4
; CHECK:    [[CONV:%[A-z0-9]*]] = call spir_func double @__builtin_spirv_OpFConvert_f64_f32(float [[TMP2]])
; CHECK:    [[TMP3:%[A-z0-9]*]] = getelementptr inbounds [15 x i8], [15 x i8] addrspace(2)* @.str, i64 0, i64 0
; CHECK:    [[PTRBC:%[A-z0-9]*]] = bitcast i8 addrspace(1)* [[PRINTFBUFFER:%[A-z0-9]*]] to i32 addrspace(1)*
; CHECK:    [[WRITE_OFFSET:%[A-z0-9]*]] = call i32 @__builtin_IB_atomic_add_global_i32(i32 addrspace(1)* [[PTRBC]], i32 28)
; CHECK:    [[END_OFFSET:%[A-z0-9]*]] = add i32 [[WRITE_OFFSET]], 28
; CHECK:    [[WRITE_OFFSET1:%[A-z0-9]*]] = zext i32 [[WRITE_OFFSET]] to i64
; CHECK:    [[BUFFER_PTR:%[A-z0-9]*]] = ptrtoint i8 addrspace(1)* [[PRINTFBUFFER]] to i64
; CHECK:    [[WRITE_OFFSET2:%[A-z0-9]*]] = add i64 [[BUFFER_PTR]], [[WRITE_OFFSET1]]
; CHECK:    [[TMP4:%[A-z0-9]*]] = icmp ule i32 [[END_OFFSET]], 4194304
; CHECK:    br i1 [[TMP4]], label [[WRITE_OFFSET_TRUE:%[A-z0-9]*]], label [[WRITE_OFFSET_FALSE:%[A-z0-9]*]]
; CHECK:  write_offset_true:
; CHECK:    [[WRITE_OFFSET_PTR:%[A-z0-9]*]] = inttoptr i64 [[WRITE_OFFSET2]] to i64 addrspace(1)*
; CHECK:    [[TMP5:%[A-z0-9]*]] = ptrtoint [15 x i8] addrspace(2)* @.str to i64
; CHECK:    store i64 [[TMP5]], i64 addrspace(1)* [[WRITE_OFFSET_PTR]], align 4
; CHECK:    [[WRITE_OFFSET3:%[A-z0-9]*]] = add i64 [[WRITE_OFFSET2]], 8
; CHECK:    [[WRITE_OFFSET_PTR4:%[A-z0-9]*]] = inttoptr i64 [[WRITE_OFFSET3]] to i32 addrspace(1)*
; CHECK:    store i32 3, i32 addrspace(1)* [[WRITE_OFFSET_PTR4]], align 4
; CHECK:    [[WRITE_OFFSET5:%[A-z0-9]*]] = add i64 [[WRITE_OFFSET3]], 4
; CHECK:    [[WRITE_OFFSET_PTR6:%[A-z0-9]*]] = inttoptr i64 [[WRITE_OFFSET5]] to i32 addrspace(1)*
; CHECK:    store i32 [[TMP1]], i32 addrspace(1)* [[WRITE_OFFSET_PTR6]], align 4
; CHECK:    [[WRITE_OFFSET7:%[A-z0-9]*]] = add i64 [[WRITE_OFFSET5]], 4
; CHECK:    [[WRITE_OFFSET_PTR8:%[A-z0-9]*]] = inttoptr i64 [[WRITE_OFFSET7]] to i32 addrspace(1)*
; CHECK:    store i32 8, i32 addrspace(1)* [[WRITE_OFFSET_PTR8]], align 4
; CHECK:    [[WRITE_OFFSET9:%[A-z0-9]*]] = add i64 [[WRITE_OFFSET7]], 4
; CHECK:    [[WRITE_OFFSET_PTR10:%[A-z0-9]*]] = inttoptr i64 [[WRITE_OFFSET9]] to double addrspace(1)*
; CHECK:    store double [[CONV]], double addrspace(1)* [[WRITE_OFFSET_PTR10]], align 4
; CHECK:    [[WRITE_OFFSET11:%[A-z0-9]*]] = add i64 [[WRITE_OFFSET9]], 8
; CHECK:    br label [[BBLOCKJOIN:%[A-z0-9]*]]
; CHECK:  write_offset_false:
; CHECK:    [[END_OFFSET12:%[A-z0-9]*]] = add i32 [[WRITE_OFFSET]], 4
; CHECK:    [[TMP6:%[A-z0-9]*]] = icmp ule i32 [[END_OFFSET12]], 4194304
; CHECK:    br i1 [[TMP6]], label [[WRITE_ERROR_STRING:%[A-z0-9]*]], label [[BBLOCKFALSEJOIN:%[A-z0-9]*]]
; CHECK:  write_error_string:
; CHECK:    [[WRITE_OFFSET_PTR13:%[A-z0-9]*]] = inttoptr i64 [[WRITE_OFFSET2]] to i32 addrspace(1)*
; CHECK:    store i32 -1, i32 addrspace(1)* [[WRITE_OFFSET_PTR13]], align 4
; CHECK:    br label [[BBLOCKFALSEJOIN]]
; CHECK:  bblockFalseJoin:
; CHECK:    br label [[BBLOCKJOIN]]
; CHECK:  bblockJoin:
; CHECK:    [[PRINTF_RET_VAL:%[A-z0-9]*]] = select i1 [[TMP4]], i32 0, i32 -1
;
; Print v4f
;
; CHECK:    [[TMP7:%[A-z0-9]*]] = load <4 x float>, <4 x float>* [[F]], align 16
; CHECK:    [[TMP8:%[A-z0-9]*]] = getelementptr inbounds [16 x i8], [16 x i8] addrspace(2)* @.str.1, i64 0, i64 0
; CHECK:    [[PTRBC14:%[A-z0-9]*]] = bitcast i8 addrspace(1)* [[PRINTFBUFFER]] to i32 addrspace(1)*
; CHECK:    [[WRITE_OFFSET15:%[A-z0-9]*]] = call i32 @__builtin_IB_atomic_add_global_i32(i32 addrspace(1)* [[PTRBC14]], i32 32)
; CHECK:    [[END_OFFSET16:%[A-z0-9]*]] = add i32 [[WRITE_OFFSET15]], 32
; CHECK:    [[WRITE_OFFSET17:%[A-z0-9]*]] = zext i32 [[WRITE_OFFSET15]] to i64
; CHECK:    [[BUFFER_PTR18:%[A-z0-9]*]] = ptrtoint i8 addrspace(1)* [[PRINTFBUFFER]] to i64
; CHECK:    [[WRITE_OFFSET19:%[A-z0-9]*]] = add i64 [[BUFFER_PTR18]], [[WRITE_OFFSET17]]
; CHECK:    [[TMP9:%[A-z0-9]*]] = icmp ule i32 [[END_OFFSET16]], 4194304
; CHECK:    br i1 [[TMP9]], label [[WRITE_OFFSET_TRUE21:%[A-z0-9]*]], label [[WRITE_OFFSET_FALSE22:%[A-z0-9]*]]
; CHECK:  write_offset_true21:
; CHECK:    [[WRITE_OFFSET_PTR23:%[A-z0-9]*]] = inttoptr i64 [[WRITE_OFFSET19]] to i64 addrspace(1)*
; CHECK:    [[TMP10:%[A-z0-9]*]] = ptrtoint [16 x i8] addrspace(2)* @.str.1 to i64
; CHECK:    store i64 [[TMP10]], i64 addrspace(1)* [[WRITE_OFFSET_PTR23]], align 4
; CHECK:    [[WRITE_OFFSET24:%[A-z0-9]*]] = add i64 [[WRITE_OFFSET19]], 8
; CHECK:    [[WRITE_OFFSET_PTR25:%[A-z0-9]*]] = inttoptr i64 [[WRITE_OFFSET24]] to i32 addrspace(1)*
; CHECK:    store i32 13, i32 addrspace(1)* [[WRITE_OFFSET_PTR25]], align 4
; CHECK:    [[WRITE_OFFSET26:%[A-z0-9]*]] = add i64 [[WRITE_OFFSET24]], 4
; CHECK:    [[WRITE_OFFSET_PTR27:%[A-z0-9]*]] = inttoptr i64 [[WRITE_OFFSET26]] to i32 addrspace(1)*
; CHECK:    store i32 4, i32 addrspace(1)* [[WRITE_OFFSET_PTR27]], align 4
; CHECK:    [[WRITE_OFFSET28:%[A-z0-9]*]] = add i64 [[WRITE_OFFSET26]], 4
; CHECK:    [[WRITE_OFFSET_PTR29:%[A-z0-9]*]] = inttoptr i64 [[WRITE_OFFSET28]] to <4 x float> addrspace(1)*
; CHECK:    store <4 x float> [[TMP7]], <4 x float> addrspace(1)* [[WRITE_OFFSET_PTR29]], align 4
; CHECK:    [[WRITE_OFFSET30:%[A-z0-9]*]] = add i64 [[WRITE_OFFSET28]], 16
; CHECK:    br label [[BBLOCKJOIN20:%[A-z0-9]*]]
; CHECK:  write_offset_false22:
; CHECK:    [[END_OFFSET31:%[A-z0-9]*]] = add i32 [[WRITE_OFFSET15]], 4
; CHECK:    [[TMP11:%[A-z0-9]*]] = icmp ule i32 [[END_OFFSET31]], 4194304
; CHECK:    br i1 [[TMP11]], label [[WRITE_ERROR_STRING32:%[A-z0-9]*]], label [[BBLOCKFALSEJOIN33:%[A-z0-9]*]]
; CHECK:  write_error_string32:
; CHECK:    [[WRITE_OFFSET_PTR34:%[A-z0-9]*]] = inttoptr i64 [[WRITE_OFFSET19]] to i32 addrspace(1)*
; CHECK:    store i32 -1, i32 addrspace(1)* [[WRITE_OFFSET_PTR34]], align 4
; CHECK:    br label [[BBLOCKFALSEJOIN33]]
; CHECK:  bblockFalseJoin33:
; CHECK:    br label [[BBLOCKJOIN20]]
; CHECK:  bblockJoin20:
; CHECK:    [[PRINTF_RET_VAL35:%[A-z0-9]*]] = select i1 [[TMP9]], i32 0, i32 -1
;
; Print v4i8
;
; CHECK:    [[TMP12:%[A-z0-9]*]] = load <4 x i8>, <4 x i8>* [[UC]], align 4
; CHECK:    [[TMP13:%[A-z0-9]*]] = getelementptr inbounds [14 x i8], [14 x i8] addrspace(2)* @.str.2, i64 0, i64 0
; CHECK:    [[PTRBC36:%[A-z0-9]*]] = bitcast i8 addrspace(1)* [[PRINTFBUFFER]] to i32 addrspace(1)*
; CHECK:    [[WRITE_OFFSET37:%[A-z0-9]*]] = call i32 @__builtin_IB_atomic_add_global_i32(i32 addrspace(1)* [[PTRBC36]], i32 32)
; CHECK:    [[END_OFFSET38:%[A-z0-9]*]] = add i32 [[WRITE_OFFSET37]], 32
; CHECK:    [[WRITE_OFFSET39:%[A-z0-9]*]] = zext i32 [[WRITE_OFFSET37]] to i64
; CHECK:    [[BUFFER_PTR40:%[A-z0-9]*]] = ptrtoint i8 addrspace(1)* [[PRINTFBUFFER]] to i64
; CHECK:    [[WRITE_OFFSET41:%[A-z0-9]*]] = add i64 [[BUFFER_PTR40]], [[WRITE_OFFSET39]]
; CHECK:    [[TMP14:%[A-z0-9]*]] = icmp ule i32 [[END_OFFSET38]], 4194304
; CHECK:    br i1 [[TMP14]], label [[WRITE_OFFSET_TRUE43:%[A-z0-9]*]], label [[WRITE_OFFSET_FALSE44:%[A-z0-9]*]]
; CHECK:  write_offset_true43:
; CHECK:    [[WRITE_OFFSET_PTR45:%[A-z0-9]*]] = inttoptr i64 [[WRITE_OFFSET41]] to i64 addrspace(1)*
; CHECK:    [[TMP15:%[A-z0-9]*]] = ptrtoint [14 x i8] addrspace(2)* @.str.2 to i64
; CHECK:    store i64 [[TMP15]], i64 addrspace(1)* [[WRITE_OFFSET_PTR45]], align 4
; CHECK:    [[WRITE_OFFSET46:%[A-z0-9]*]] = add i64 [[WRITE_OFFSET41]], 8
; CHECK:    [[WRITE_OFFSET_PTR47:%[A-z0-9]*]] = inttoptr i64 [[WRITE_OFFSET46]] to i32 addrspace(1)*
; CHECK:    store i32 9, i32 addrspace(1)* [[WRITE_OFFSET_PTR47]], align 4
; CHECK:    [[WRITE_OFFSET48:%[A-z0-9]*]] = add i64 [[WRITE_OFFSET46]], 4
; CHECK:    [[WRITE_OFFSET_PTR49:%[A-z0-9]*]] = inttoptr i64 [[WRITE_OFFSET48]] to i32 addrspace(1)*
; CHECK:    store i32 4, i32 addrspace(1)* [[WRITE_OFFSET_PTR49]], align 4
; CHECK:    [[WRITE_OFFSET50:%[A-z0-9]*]] = add i64 [[WRITE_OFFSET48]], 4
; CHECK:    [[WRITE_OFFSET_PTR51:%[A-z0-9]*]] = inttoptr i64 [[WRITE_OFFSET50]] to <4 x i8> addrspace(1)*
; CHECK:    store <4 x i8> [[TMP12]], <4 x i8> addrspace(1)* [[WRITE_OFFSET_PTR51]], align 4
; CHECK:    [[WRITE_OFFSET52:%[A-z0-9]*]] = add i64 [[WRITE_OFFSET50]], 16
; CHECK:    br label [[BBLOCKJOIN42:%[A-z0-9]*]]
; CHECK:  write_offset_false44:
; CHECK:    [[END_OFFSET53:%[A-z0-9]*]] = add i32 [[WRITE_OFFSET37]], 4
; CHECK:    [[TMP16:%[A-z0-9]*]] = icmp ule i32 [[END_OFFSET53]], 4194304
; CHECK:    br i1 [[TMP16]], label [[WRITE_ERROR_STRING54:%[A-z0-9]*]], label [[BBLOCKFALSEJOIN55:%[A-z0-9]*]]
; CHECK:  write_error_string54:
; CHECK:    [[WRITE_OFFSET_PTR56:%[A-z0-9]*]] = inttoptr i64 [[WRITE_OFFSET41]] to i32 addrspace(1)*
; CHECK:    store i32 -1, i32 addrspace(1)* [[WRITE_OFFSET_PTR56]], align 4
; CHECK:    br label [[BBLOCKFALSEJOIN55]]
; CHECK:  bblockFalseJoin55:
; CHECK:    br label [[BBLOCKJOIN42]]
; CHECK:  bblockJoin42:
; CHECK:    [[PRINTF_RET_VAL57:%[A-z0-9]*]] = select i1 [[TMP14]], i32 0, i32 -1
;
; Print string
;
; CHECK:    [[TMP17:%[A-z0-9]*]] = getelementptr inbounds [4 x i8], [4 x i8] addrspace(2)* @.str.3, i64 0, i64 0
; CHECK:    [[TMP18:%[A-z0-9]*]] = getelementptr inbounds [23 x i8], [23 x i8] addrspace(2)* @.str.4, i64 0, i64 0
; CHECK:    [[PTRBC58:%[A-z0-9]*]] = bitcast i8 addrspace(1)* [[PRINTFBUFFER]] to i32 addrspace(1)*
; CHECK:    [[WRITE_OFFSET59:%[A-z0-9]*]] = call i32 @__builtin_IB_atomic_add_global_i32(i32 addrspace(1)* [[PTRBC58]], i32 20)
; CHECK:    [[END_OFFSET60:%[A-z0-9]*]] = add i32 [[WRITE_OFFSET59]], 20
; CHECK:    [[WRITE_OFFSET61:%[A-z0-9]*]] = zext i32 [[WRITE_OFFSET59]] to i64
; CHECK:    [[BUFFER_PTR62:%[A-z0-9]*]] = ptrtoint i8 addrspace(1)* [[PRINTFBUFFER]] to i64
; CHECK:    [[WRITE_OFFSET63:%[A-z0-9]*]] = add i64 [[BUFFER_PTR62]], [[WRITE_OFFSET61]]
; CHECK:    [[TMP19:%[A-z0-9]*]] = icmp ule i32 [[END_OFFSET60]], 4194304
; CHECK:    br i1 [[TMP19]], label [[WRITE_OFFSET_TRUE65:%[A-z0-9]*]], label [[WRITE_OFFSET_FALSE66:%[A-z0-9]*]]
; CHECK:  write_offset_true65:
; CHECK:    [[WRITE_OFFSET_PTR67:%[A-z0-9]*]] = inttoptr i64 [[WRITE_OFFSET63]] to i64 addrspace(1)*
; CHECK:    [[TMP20:%[A-z0-9]*]] = ptrtoint [4 x i8] addrspace(2)* @.str.3 to i64
; CHECK:    store i64 [[TMP20]], i64 addrspace(1)* [[WRITE_OFFSET_PTR67]], align 4
; CHECK:    [[WRITE_OFFSET68:%[A-z0-9]*]] = add i64 [[WRITE_OFFSET63]], 8
; CHECK:    [[WRITE_OFFSET_PTR69:%[A-z0-9]*]] = inttoptr i64 [[WRITE_OFFSET68]] to i32 addrspace(1)*
; CHECK:    store i32 5, i32 addrspace(1)* [[WRITE_OFFSET_PTR69]], align 4
; CHECK:    [[WRITE_OFFSET70:%[A-z0-9]*]] = add i64 [[WRITE_OFFSET68]], 4
; CHECK:    [[WRITE_OFFSET_PTR71:%[A-z0-9]*]] = inttoptr i64 [[WRITE_OFFSET70]] to i64 addrspace(1)*
; CHECK:    [[TMP21:%[A-z0-9]*]] = ptrtoint [23 x i8] addrspace(2)* @.str.4 to i64
; CHECK:    store i64 [[TMP21]], i64 addrspace(1)* [[WRITE_OFFSET_PTR71]], align 4
; CHECK:    [[WRITE_OFFSET72:%[A-z0-9]*]] = add i64 [[WRITE_OFFSET70]], 8
; CHECK:    br label [[BBLOCKJOIN64:%[A-z0-9]*]]
; CHECK:  write_offset_false66:
; CHECK:    [[END_OFFSET73:%[A-z0-9]*]] = add i32 [[WRITE_OFFSET59]], 4
; CHECK:    [[TMP22:%[A-z0-9]*]] = icmp ule i32 [[END_OFFSET73]], 4194304
; CHECK:    br i1 [[TMP22]], label [[WRITE_ERROR_STRING74:%[A-z0-9]*]], label [[BBLOCKFALSEJOIN75:%[A-z0-9]*]]
; CHECK:  write_error_string74:
; CHECK:    [[WRITE_OFFSET_PTR76:%[A-z0-9]*]] = inttoptr i64 [[WRITE_OFFSET63]] to i32 addrspace(1)*
; CHECK:    store i32 -1, i32 addrspace(1)* [[WRITE_OFFSET_PTR76]], align 4
; CHECK:    br label [[BBLOCKFALSEJOIN75]]
; CHECK:  bblockFalseJoin75:
; CHECK:    br label [[BBLOCKJOIN64]]
; CHECK:  bblockJoin64:
; CHECK:    [[PRINTF_RET_VAL77:%[A-z0-9]*]] = select i1 [[TMP19]], i32 0, i32 -1
; CHECK:    ret void
;
  %f = alloca <4 x float>, align 16
  %uc = alloca <4 x i8>, align 4
  %i = alloca i32, align 4
  %f1 = alloca float, align 4
  store <4 x float> <float 1.000000e+00, float 2.000000e+00, float 3.000000e+00, float 4.000000e+00>, <4 x float>* %f, align 16
  store <4 x i8> <i8 -6, i8 -5, i8 -4, i8 -3>, <4 x i8>* %uc, align 4
  store i32 42, i32* %i, align 4
  store float 0x4010CCCCC0000000, float* %f1, align 4
  %1 = load i32, i32* %i, align 4
  %2 = load float, float* %f1, align 4
  %conv = call spir_func double @__builtin_spirv_OpFConvert_f64_f32(float %2)
  %3 = getelementptr inbounds [15 x i8], [15 x i8] addrspace(2)* @.str, i64 0, i64 0
  %call = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %3, i32 %1, double %conv)
  %4 = load <4 x float>, <4 x float>* %f, align 16
  %5 = getelementptr inbounds [16 x i8], [16 x i8] addrspace(2)* @.str.1, i64 0, i64 0
  %call2 = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %5, <4 x float> %4)
  %6 = load <4 x i8>, <4 x i8>* %uc, align 4
  %7 = getelementptr inbounds [14 x i8], [14 x i8] addrspace(2)* @.str.2, i64 0, i64 0
  %call3 = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %7, <4 x i8> %6)
  %8 = getelementptr inbounds [4 x i8], [4 x i8] addrspace(2)* @.str.3, i64 0, i64 0
  %9 = getelementptr inbounds [23 x i8], [23 x i8] addrspace(2)* @.str.4, i64 0, i64 0
  %call4 = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %8, i8 addrspace(2)* %9)
  ret void
}

declare spir_func double @__builtin_spirv_OpFConvert_f64_f32(float)

declare spir_func i32 @printf(i8 addrspace(2)*, ...)


!igc.functions = !{!0}
!IGCMetadata = !{!9}

!0 = !{void (i32, <8 x i32>, <8 x i32>, i8 addrspace(2)*, i8*, i8 addrspace(1)*)* @test_printf, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4, !5, !6, !7, !8}
!4 = !{i32 0}
!5 = !{i32 1}
!6 = !{i32 11}
!7 = !{i32 13}
!8 = !{i32 14}
!9 = !{!"ModuleMD", !10}
!10 = !{!"FuncMD", !11, !12}
!11 = distinct !{!"FuncMDMap[0]", void (i32, <8 x i32>, <8 x i32>, i8 addrspace(2)*, i8*, i8 addrspace(1)*)* @test_printf}
!12 = !{!"FuncMDValue[0]", !13, !14, !15}
!13 = !{!"localOffsets"}
!14 = !{!"funcArgs"}
!15 = !{!"functionType", !"KernelFunction"}
