;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus, regkeys, bmg-supported
; RUN: llvm-as %OPAQUE_PTR_FLAG% < %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device bmg -options "-igc_opts 'VectorizerAllowBITCAST=1, EnableOpaquePointersBackend=1'" &> %t_output.ll
; RUN: FileCheck --input-file %t_output.ll %s

; CHECK: Build succeeded.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%union._iml_v2_sp_union_t = type { [1 x i32] }

@__sln_la_CoutTab = dso_local unnamed_addr addrspace(2) constant [210 x %union._iml_v2_sp_union_t] [%union._iml_v2_sp_union_t { [1 x i32] [i32 1065353216] }, %union._iml_v2_sp_union_t zeroinitializer, %union._iml_v2_sp_union_t zeroinitializer, %union._iml_v2_sp_union_t { [1 x i32] [i32 1065091072] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1015087104] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 900509991] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1064828928] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1023541248] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 925811956] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1064566784] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1027915776] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1210746152] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1064304640] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1032073216] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1227975484] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1064173568] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1033195520] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 882149603] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1063911424] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1035468800] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 928189163] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1063649280] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1037783040] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 927501741] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1063518208] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1038958592] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1218929540] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1063256064] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1040759808] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 904405630] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1063124992] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1041361920] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1242735772] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1062862848] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1042581504] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 922094799] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1062731776] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1043201024] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1224846673] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1062469632] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1044455424] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1225102663] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1062338560] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1045091328] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1231778780] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1062207488] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1045733376] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1240065111] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1061945344] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1047035904] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 920635797] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1061814272] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1047697408] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 904920689] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1061683200] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1048365056] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 912483742] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1061552128] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1048807936] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1242302891] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1061421056] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1049148416] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 912794238] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1061158912] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1049840384] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 889474359] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1061027840] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1050191872] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1235098934] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1060896768] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1050546944] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1235710771] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1060765696] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1050905600] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 912008988] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1060634624] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1051268352] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 912290698] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1060503552] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1051635200] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1257756248] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1060372480] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1052005888] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 906226119] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1060241408] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1052380928] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1242486991] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1060110336] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1052760064] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1246198531] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1059979264] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1053143552] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1244991846] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1059848192] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1053531392] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 894485718] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1059717120] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1053923840] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 897598623] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1059586048] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1054320896] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 907355277] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1059586048] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1054320896] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 907355277] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1059454976] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1054722816] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 881705073] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1059323904] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1055129600] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1245243563] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1059192832] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1055541248] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 890353599] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1059061760] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1055958016] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 908173938] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1059061760] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1055958016] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 908173938] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1058930688] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1056380160] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 883644938] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1058799616] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1056807680] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1242951497] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1058668544] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1057102592] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 884897284] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1058668544] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1057102592] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 884897284] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1058537472] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1057321920] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1257334826] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1058406400] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1057544128] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 865017195] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1058275328] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1057769344] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1252030750] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1058275328] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1057769344] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1252030750] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1058144256] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1057997568] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 903344518] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1058013184] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1058228992] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 897862967] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1058013184] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1058228992] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 897862967] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1057882112] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1058463680] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1247145016] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1057882112] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1058463680] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1247145016] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1057751040] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1058701632] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 883793293] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1057619968] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1058943040] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 851667963] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1057619968] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1058943040] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 851667963] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1057488896] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1059187968] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1294963260] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1057488896] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1059187968] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1294963260] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1057357824] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1059436544] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1247536579] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1057357824] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1059436544] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1247536579] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1057226752] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1059688832] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1251164988] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1057226752] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1059688832] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1251164988] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1057095680] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1059944960] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 876113044] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1057095680] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1059944960] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 876113044] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1056964608] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1060205056] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 901758606] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1060205056] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 901758606] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1207959616] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1174405120] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1008730112] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1291845632] }, %union._iml_v2_sp_union_t zeroinitializer, %union._iml_v2_sp_union_t { [1 x i32] [i32 1065353216] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1090519040] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1051372203] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1098907648] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1045220557] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1104500053] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 1041387009] }, %union._iml_v2_sp_union_t { [1 x i32] [i32 -1107294816] }], align 4, !igc_bif !0

; Function Attrs: convergent nounwind
define spir_kernel void @fa_fwd_kernel(ptr addrspace(1) %0, ptr addrspace(1) %1, ptr addrspace(1) %2, ptr addrspace(1) %3, ptr addrspace(1) %4, ptr addrspace(1) nocapture readonly %5, ptr addrspace(1) nocapture readonly %6, ptr addrspace(1) nocapture readonly align 1 %7, ptr addrspace(1) nocapture readonly align 1 %8, i32 %9, i32 %10, double %11, ptr addrspace(1) nocapture readnone align 1 %12, ptr addrspace(1) nocapture readnone align 1 %13, <8 x i32> %r0, <3 x i32> %globalOffset, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr %privateBase, i32 %bufferOffset, i32 %bufferOffset1, i32 %bufferOffset2, i32 %bufferOffset3, i32 %bufferOffset4, i32 %bufferOffset5, i32 %bufferOffset6, i32 %bufferOffset7, i32 %bufferOffset8, i32 %bufferOffset9, i32 %bufferOffset10, i32 %bindlessOffset, i32 %bindlessOffset11, i32 %bindlessOffset12, i32 %bindlessOffset13, i32 %bindlessOffset14, i32 %bindlessOffset15, i32 %bindlessOffset16, i32 %bindlessOffset17, i32 %bindlessOffset18, i32 %bindlessOffset19, i32 %bindlessOffset20) #0 {
  %15 = extractelement <8 x i32> %r0, i64 1
  %16 = extractelement <8 x i32> %r0, i64 6
  %17 = extractelement <8 x i32> %r0, i64 7
  %18 = zext i16 %localIdX to i32
  %19 = zext i32 %16 to i64
  %20 = zext i32 %17 to i64
  %21 = sdiv i32 %9, %10
  %22 = sdiv i32 %16, %21
  %sext = shl nuw i64 %20, 32
  %23 = ashr exact i64 %sext, 29
  %24 = getelementptr i8, ptr addrspace(1) %7, i64 %23
  %25 = load i64, ptr addrspace(1) %24, align 8
  %26 = getelementptr i8, ptr addrspace(1) %24, i64 8
  %27 = load i64, ptr addrspace(1) %26, align 8
  %28 = sub i64 %27, %25
  %29 = shl i32 %15, 7
  %30 = sext i32 %29 to i64
  %31 = icmp sgt i64 %28, %30
  br i1 %31, label %32, label %common.ret

common.ret:                                       ; preds = %32, %14, %_Z15__spirv_ocl_logf.exit100
  ret void

32:                                               ; preds = %14
  %33 = getelementptr i8, ptr addrspace(1) %8, i64 %23
  %34 = load i64, ptr addrspace(1) %33, align 8
  %35 = getelementptr i8, ptr addrspace(1) %33, i64 8
  %36 = load i64, ptr addrspace(1) %35, align 8
  %37 = sub i64 %36, %34
  %38 = icmp sgt i64 %37, %30
  br i1 %38, label %39, label %common.ret

39:                                               ; preds = %32
  %40 = fptrunc double %11 to float
  %41 = fmul contract float %40, 0x3FF7154760000000
  %42 = and i32 %18, 15
  %43 = lshr i32 %18, 1
  %44 = and i32 %43, 120
  %45 = or i32 %29, %44
  %46 = or i32 %45, 1
  %47 = or i32 %45, 2
  %48 = or i32 %45, 3
  %49 = or i32 %45, 4
  %50 = or i32 %45, 5
  %51 = or i32 %45, 6
  %52 = or i32 %45, 7
  %53 = sext i32 %9 to i64
  %54 = mul i64 %25, %53
  %55 = shl i64 %54, 6
  %56 = getelementptr [2 x i8], ptr addrspace(1) %0, i64 %55
  %57 = shl i32 %16, 6
  %58 = sext i32 %57 to i64
  %59 = getelementptr [2 x i8], ptr addrspace(1) %56, i64 %58
  %60 = sext i32 %10 to i64
  %61 = shl nsw i64 %60, 6
  %62 = mul i64 %61, %34
  %63 = getelementptr [2 x i8], ptr addrspace(1) %1, i64 %62
  %64 = shl i32 %22, 6
  %65 = sext i32 %64 to i64
  %66 = getelementptr [2 x i8], ptr addrspace(1) %63, i64 %65
  %67 = getelementptr [2 x i8], ptr addrspace(1) %2, i64 %62
  %68 = getelementptr [2 x i8], ptr addrspace(1) %67, i64 %65
  %69 = getelementptr [2 x i8], ptr addrspace(1) %3, i64 %55
  %70 = getelementptr [2 x i8], ptr addrspace(1) %69, i64 %58
  %71 = shl i32 %9, 7
  %72 = trunc i64 %28 to i32
  %73 = trunc i64 %37 to i32
  %74 = getelementptr [4 x i8], ptr addrspace(1) %4, i64 %54
  %sext6 = shl nuw i64 %19, 32
  %75 = ashr exact i64 %sext6, 30
  %76 = getelementptr i8, ptr addrspace(1) %74, i64 %75
  %77 = getelementptr [4 x i8], ptr addrspace(1) %5, i64 %25
  %78 = getelementptr [4 x i8], ptr addrspace(1) %6, i64 %34
  %79 = bitcast <8 x i32> %r0 to <32 x i8>
  %80 = extractelement <32 x i8> %79, i64 8
  %localThreadId56 = zext i8 %80 to i32
  %81 = shl nuw nsw i32 %localThreadId56, 3
  %82 = and i32 %81, 120
  %83 = or i32 %82, %29
  %84 = ptrtoint ptr addrspace(1) %59 to i64
  %85 = and i64 %84, -64
  %86 = trunc i64 %84 to i32
  %87 = and i32 %86, 63
  %88 = lshr i32 %87, 1
  %width.m1 = add nuw nsw i32 %87, 127
  %height.m1 = add i32 %72, -1
  %pitch.m1 = add i32 %71, -1
  %89 = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %85, i32 %width.m1, i32 %height.m1, i32 %pitch.m1, i32 %88, i32 %83, i32 16, i32 16, i32 8, i32 2, i1 false, i1 false, i32 0)
  %90 = shufflevector <16 x i16> %89, <16 x i16> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %91 = shufflevector <16 x i16> %89, <16 x i16> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %92 = or i32 %88, 32
  %93 = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %85, i32 %width.m1, i32 %height.m1, i32 %pitch.m1, i32 %92, i32 %83, i32 16, i32 16, i32 8, i32 2, i1 false, i1 false, i32 0)
  %94 = shufflevector <16 x i16> %93, <16 x i16> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %95 = shufflevector <16 x i16> %93, <16 x i16> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %96 = icmp sgt i32 %29, -1
  %97 = icmp slt i32 %45, %72
  %98 = and i1 %96, %97
  %99 = sext i32 %46 to i64
  %100 = getelementptr [4 x i8], ptr addrspace(1) %77, i64 %99
  %101 = icmp slt i32 %46, %72
  %102 = and i1 %96, %101
  %103 = sext i32 %47 to i64
  %104 = getelementptr [4 x i8], ptr addrspace(1) %77, i64 %103
  %105 = icmp slt i32 %47, %72
  %106 = and i1 %96, %105
  %107 = sext i32 %48 to i64
  %108 = getelementptr [4 x i8], ptr addrspace(1) %77, i64 %107
  %109 = icmp slt i32 %48, %72
  %110 = and i1 %96, %109
  %111 = sext i32 %49 to i64
  %112 = getelementptr [4 x i8], ptr addrspace(1) %77, i64 %111
  %113 = icmp slt i32 %49, %72
  %114 = and i1 %96, %113
  %115 = sext i32 %50 to i64
  %116 = getelementptr [4 x i8], ptr addrspace(1) %77, i64 %115
  %117 = icmp slt i32 %50, %72
  %118 = and i1 %96, %117
  %119 = sext i32 %51 to i64
  %120 = getelementptr [4 x i8], ptr addrspace(1) %77, i64 %119
  %121 = icmp slt i32 %51, %72
  %122 = and i1 %96, %121
  %123 = sext i32 %52 to i64
  %124 = getelementptr [4 x i8], ptr addrspace(1) %77, i64 %123
  %125 = icmp slt i32 %52, %72
  %126 = and i1 %96, %125
  br i1 %98, label %127, label %._crit_edge140

127:                                              ; preds = %39
  %128 = zext i32 %45 to i64
  %129 = getelementptr [4 x i8], ptr addrspace(1) %77, i64 %128
  %130 = load i32, ptr addrspace(1) %129, align 4
  br label %._crit_edge140

._crit_edge140:                                   ; preds = %39, %127
  %131 = phi i32 [ %130, %127 ], [ 0, %39 ]
  br i1 %102, label %132, label %._crit_edge141

132:                                              ; preds = %._crit_edge140
  %133 = load i32, ptr addrspace(1) %100, align 4
  br label %._crit_edge141

._crit_edge141:                                   ; preds = %._crit_edge140, %132
  %134 = phi i32 [ %133, %132 ], [ 0, %._crit_edge140 ]
  br i1 %106, label %135, label %._crit_edge142

135:                                              ; preds = %._crit_edge141
  %136 = load i32, ptr addrspace(1) %104, align 4
  br label %._crit_edge142

._crit_edge142:                                   ; preds = %._crit_edge141, %135
  %137 = phi i32 [ %136, %135 ], [ 0, %._crit_edge141 ]
  br i1 %110, label %138, label %._crit_edge143

138:                                              ; preds = %._crit_edge142
  %139 = load i32, ptr addrspace(1) %108, align 4
  br label %._crit_edge143

._crit_edge143:                                   ; preds = %._crit_edge142, %138
  %140 = phi i32 [ %139, %138 ], [ 0, %._crit_edge142 ]
  br i1 %114, label %141, label %._crit_edge144

141:                                              ; preds = %._crit_edge143
  %142 = load i32, ptr addrspace(1) %112, align 4
  br label %._crit_edge144

._crit_edge144:                                   ; preds = %._crit_edge143, %141
  %143 = phi i32 [ %142, %141 ], [ 0, %._crit_edge143 ]
  br i1 %118, label %144, label %._crit_edge145

144:                                              ; preds = %._crit_edge144
  %145 = load i32, ptr addrspace(1) %116, align 4
  br label %._crit_edge145

._crit_edge145:                                   ; preds = %._crit_edge144, %144
  %146 = phi i32 [ %145, %144 ], [ 0, %._crit_edge144 ]
  br i1 %122, label %147, label %._crit_edge146

147:                                              ; preds = %._crit_edge145
  %148 = load i32, ptr addrspace(1) %120, align 4
  br label %._crit_edge146

._crit_edge146:                                   ; preds = %._crit_edge145, %147
  %149 = phi i32 [ %148, %147 ], [ 0, %._crit_edge145 ]
  br i1 %126, label %150, label %.lr.ph

150:                                              ; preds = %._crit_edge146
  %151 = load i32, ptr addrspace(1) %124, align 4
  br label %.lr.ph

.lr.ph:                                           ; preds = %._crit_edge146, %150
  %152 = phi i32 [ %151, %150 ], [ 0, %._crit_edge146 ]
  %153 = or i32 %42, 16
  %154 = shl i32 %10, 7
  %155 = shl nuw nsw i32 %localThreadId56, 1
  %156 = and i32 %155, 28
  %157 = shl nuw nsw i32 %localThreadId56, 5
  %158 = and i32 %157, 32
  %159 = or i32 %156, %29
  %160 = ptrtoint ptr addrspace(1) %66 to i64
  %161 = and i64 %160, -64
  %162 = trunc i64 %160 to i32
  %163 = and i32 %162, 63
  %164 = lshr i32 %163, 1
  %165 = or i32 %158, %164
  %166 = add nuw nsw i32 %163, 127
  %167 = add i32 %73, -1
  %168 = add i32 %154, -1
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %161, i32 %166, i32 %167, i32 %168, i32 %165, i32 %159, i32 16, i32 32, i32 4, i32 1, i1 false, i1 false, i32 4) #8
  %169 = ptrtoint ptr addrspace(1) %68 to i64
  %170 = and i64 %169, -64
  %171 = trunc i64 %169 to i32
  %172 = and i32 %171, 63
  %173 = lshr i32 %172, 1
  %174 = or i32 %158, %173
  %175 = add nuw nsw i32 %172, 127
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %170, i32 %175, i32 %167, i32 %168, i32 %174, i32 %159, i32 16, i32 32, i32 4, i32 1, i1 false, i1 false, i32 4) #8
  %176 = or i32 %29, 32
  %177 = or i32 %156, %176
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %161, i32 %166, i32 %167, i32 %168, i32 %165, i32 %177, i32 16, i32 32, i32 4, i32 1, i1 false, i1 false, i32 4) #8
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %170, i32 %175, i32 %167, i32 %168, i32 %174, i32 %177, i32 16, i32 32, i32 4, i32 1, i1 false, i1 false, i32 4) #8
  %178 = lshr i32 %163, 2
  %179 = add nuw nsw i32 %178, 8
  %180 = or i32 %178, 16
  %181 = add nuw nsw i32 %178, 24
  %182 = or i32 %173, 32
  br label %183

183:                                              ; preds = %._crit_edge148, %.lr.ph
  %184 = phi i32 [ %176, %.lr.ph ], [ %236, %._crit_edge148 ]
  %185 = phi i32 [ %29, %.lr.ph ], [ %184, %._crit_edge148 ]
  %186 = phi float [ 0.000000e+00, %.lr.ph ], [ %675, %._crit_edge148 ]
  %187 = phi float [ 0.000000e+00, %.lr.ph ], [ %676, %._crit_edge148 ]
  %188 = phi float [ 0.000000e+00, %.lr.ph ], [ %677, %._crit_edge148 ]
  %189 = phi float [ 0.000000e+00, %.lr.ph ], [ %678, %._crit_edge148 ]
  %190 = phi float [ 0.000000e+00, %.lr.ph ], [ %679, %._crit_edge148 ]
  %191 = phi float [ 0.000000e+00, %.lr.ph ], [ %680, %._crit_edge148 ]
  %192 = phi float [ 0.000000e+00, %.lr.ph ], [ %681, %._crit_edge148 ]
  %193 = phi float [ 0.000000e+00, %.lr.ph ], [ %682, %._crit_edge148 ]
  %194 = phi float [ 0xC1D0000000000000, %.lr.ph ], [ %433, %._crit_edge148 ]
  %195 = phi float [ 0xC1D0000000000000, %.lr.ph ], [ %434, %._crit_edge148 ]
  %196 = phi float [ 0xC1D0000000000000, %.lr.ph ], [ %435, %._crit_edge148 ]
  %197 = phi float [ 0xC1D0000000000000, %.lr.ph ], [ %436, %._crit_edge148 ]
  %198 = phi float [ 0xC1D0000000000000, %.lr.ph ], [ %437, %._crit_edge148 ]
  %199 = phi float [ 0xC1D0000000000000, %.lr.ph ], [ %438, %._crit_edge148 ]
  %200 = phi float [ 0xC1D0000000000000, %.lr.ph ], [ %439, %._crit_edge148 ]
  %201 = phi float [ 0xC1D0000000000000, %.lr.ph ], [ %440, %._crit_edge148 ]
  %202 = phi float [ 0.000000e+00, %.lr.ph ], [ %632, %._crit_edge148 ]
  %203 = phi float [ 0.000000e+00, %.lr.ph ], [ %633, %._crit_edge148 ]
  %204 = phi float [ 0.000000e+00, %.lr.ph ], [ %634, %._crit_edge148 ]
  %205 = phi float [ 0.000000e+00, %.lr.ph ], [ %635, %._crit_edge148 ]
  %206 = phi float [ 0.000000e+00, %.lr.ph ], [ %636, %._crit_edge148 ]
  %207 = phi float [ 0.000000e+00, %.lr.ph ], [ %637, %._crit_edge148 ]
  %208 = phi float [ 0.000000e+00, %.lr.ph ], [ %638, %._crit_edge148 ]
  %209 = phi float [ 0.000000e+00, %.lr.ph ], [ %639, %._crit_edge148 ]
  %210 = phi float [ 0.000000e+00, %.lr.ph ], [ %641, %._crit_edge148 ]
  %211 = phi float [ 0.000000e+00, %.lr.ph ], [ %642, %._crit_edge148 ]
  %212 = phi float [ 0.000000e+00, %.lr.ph ], [ %643, %._crit_edge148 ]
  %213 = phi float [ 0.000000e+00, %.lr.ph ], [ %644, %._crit_edge148 ]
  %214 = phi float [ 0.000000e+00, %.lr.ph ], [ %645, %._crit_edge148 ]
  %215 = phi float [ 0.000000e+00, %.lr.ph ], [ %646, %._crit_edge148 ]
  %216 = phi float [ 0.000000e+00, %.lr.ph ], [ %647, %._crit_edge148 ]
  %217 = phi float [ 0.000000e+00, %.lr.ph ], [ %648, %._crit_edge148 ]
  %218 = phi float [ 0.000000e+00, %.lr.ph ], [ %650, %._crit_edge148 ]
  %219 = phi float [ 0.000000e+00, %.lr.ph ], [ %651, %._crit_edge148 ]
  %220 = phi float [ 0.000000e+00, %.lr.ph ], [ %652, %._crit_edge148 ]
  %221 = phi float [ 0.000000e+00, %.lr.ph ], [ %653, %._crit_edge148 ]
  %222 = phi float [ 0.000000e+00, %.lr.ph ], [ %654, %._crit_edge148 ]
  %223 = phi float [ 0.000000e+00, %.lr.ph ], [ %655, %._crit_edge148 ]
  %224 = phi float [ 0.000000e+00, %.lr.ph ], [ %656, %._crit_edge148 ]
  %225 = phi float [ 0.000000e+00, %.lr.ph ], [ %657, %._crit_edge148 ]
  %226 = phi float [ 0.000000e+00, %.lr.ph ], [ %659, %._crit_edge148 ]
  %227 = phi float [ 0.000000e+00, %.lr.ph ], [ %660, %._crit_edge148 ]
  %228 = phi float [ 0.000000e+00, %.lr.ph ], [ %661, %._crit_edge148 ]
  %229 = phi float [ 0.000000e+00, %.lr.ph ], [ %662, %._crit_edge148 ]
  %230 = phi float [ 0.000000e+00, %.lr.ph ], [ %663, %._crit_edge148 ]
  %231 = phi float [ 0.000000e+00, %.lr.ph ], [ %664, %._crit_edge148 ]
  %232 = phi float [ 0.000000e+00, %.lr.ph ], [ %665, %._crit_edge148 ]
  %233 = phi float [ 0.000000e+00, %.lr.ph ], [ %666, %._crit_edge148 ]
  %234 = phi i64 [ %30, %.lr.ph ], [ %683, %._crit_edge148 ]
  %235 = trunc i64 %234 to i32
  %236 = add i32 %235, 64
  %237 = or i32 %156, %236
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %161, i32 %166, i32 %167, i32 %168, i32 %165, i32 %237, i32 16, i32 32, i32 4, i32 1, i1 false, i1 false, i32 4) #8
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %170, i32 %175, i32 %167, i32 %168, i32 %174, i32 %237, i32 16, i32 32, i32 4, i32 1, i1 false, i1 false, i32 4) #8
  %238 = add i32 %185, %42
  %239 = icmp sgt i32 %238, -1
  %240 = icmp slt i32 %238, %73
  %241 = and i1 %240, %239
  %242 = add i32 %185, %153
  %243 = sext i32 %242 to i64
  %244 = getelementptr [4 x i8], ptr addrspace(1) %78, i64 %243
  %245 = icmp sgt i32 %242, -1
  %246 = icmp slt i32 %242, %73
  %247 = and i1 %246, %245
  br i1 %241, label %248, label %._crit_edge147

248:                                              ; preds = %183
  %249 = zext i32 %238 to i64
  %250 = getelementptr [4 x i8], ptr addrspace(1) %78, i64 %249
  %251 = load i32, ptr addrspace(1) %250, align 4
  br label %._crit_edge147

._crit_edge147:                                   ; preds = %183, %248
  %252 = phi i32 [ %251, %248 ], [ 0, %183 ]
  br i1 %247, label %253, label %._crit_edge148

253:                                              ; preds = %._crit_edge147
  %254 = load i32, ptr addrspace(1) %244, align 4
  br label %._crit_edge148

._crit_edge148:                                   ; preds = %._crit_edge147, %253
  %255 = phi i32 [ %254, %253 ], [ 0, %._crit_edge147 ]
  %256 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %161, i32 %166, i32 %167, i32 %168, i32 %178, i32 %185, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %257 = add nuw nsw i32 %185, 16
  %258 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %161, i32 %166, i32 %167, i32 %168, i32 %178, i32 %257, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %259 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %161, i32 %166, i32 %167, i32 %168, i32 %179, i32 %185, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %260 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %161, i32 %166, i32 %167, i32 %168, i32 %179, i32 %257, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %261 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %161, i32 %166, i32 %167, i32 %168, i32 %180, i32 %185, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %262 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %161, i32 %166, i32 %167, i32 %168, i32 %180, i32 %257, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %263 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %161, i32 %166, i32 %167, i32 %168, i32 %181, i32 %185, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %264 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %161, i32 %166, i32 %167, i32 %168, i32 %181, i32 %257, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %265 = call <32 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v32i32(i64 %170, i32 %175, i32 %167, i32 %168, i32 %173, i32 %185, i32 16, i32 16, i32 32, i32 2, i1 false, i1 true, i32 0)
  %266 = shufflevector <32 x i32> %265, <32 x i32> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %267 = shufflevector <32 x i32> %265, <32 x i32> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %268 = shufflevector <32 x i32> %265, <32 x i32> undef, <8 x i32> <i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23>
  %269 = shufflevector <32 x i32> %265, <32 x i32> undef, <8 x i32> <i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31>
  %270 = call <32 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v32i32(i64 %170, i32 %175, i32 %167, i32 %168, i32 %182, i32 %185, i32 16, i32 16, i32 32, i32 2, i1 false, i1 true, i32 0)
  %271 = shufflevector <32 x i32> %270, <32 x i32> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %272 = shufflevector <32 x i32> %270, <32 x i32> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %273 = shufflevector <32 x i32> %270, <32 x i32> undef, <8 x i32> <i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23>
  %274 = shufflevector <32 x i32> %270, <32 x i32> undef, <8 x i32> <i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31>
  %275 = icmp eq i32 %131, %252
  %276 = icmp eq i32 %134, %252
  %277 = icmp eq i32 %137, %252
  %278 = icmp eq i32 %140, %252
  %279 = icmp eq i32 %143, %252
  %280 = icmp eq i32 %146, %252
  %281 = icmp eq i32 %149, %252
  %282 = icmp eq i32 %152, %252
  %283 = icmp eq i32 %131, %255
  %284 = icmp eq i32 %134, %255
  %285 = icmp eq i32 %137, %255
  %286 = icmp eq i32 %140, %255
  %287 = icmp eq i32 %143, %255
  %288 = icmp eq i32 %146, %255
  %289 = icmp eq i32 %149, %255
  %290 = icmp eq i32 %152, %255
  %291 = icmp eq i32 %252, 0
  %292 = icmp eq i32 %255, 0
  %293 = or i1 %276, %291
  %294 = or i1 %277, %291
  %295 = or i1 %278, %291
  %296 = or i1 %279, %291
  %297 = or i1 %280, %291
  %298 = or i1 %281, %291
  %299 = or i1 %282, %291
  %300 = or i1 %283, %292
  %301 = or i1 %284, %292
  %302 = or i1 %285, %292
  %303 = or i1 %286, %292
  %304 = or i1 %287, %292
  %305 = or i1 %288, %292
  %306 = or i1 %289, %292
  %307 = or i1 %290, %292
  %308 = icmp eq i32 %45, %238
  %309 = icmp eq i32 %46, %238
  %310 = icmp eq i32 %47, %238
  %311 = icmp eq i32 %48, %238
  %312 = icmp eq i32 %49, %238
  %313 = icmp eq i32 %50, %238
  %314 = icmp eq i32 %51, %238
  %315 = icmp eq i32 %52, %238
  %316 = icmp eq i32 %45, %242
  %317 = icmp eq i32 %46, %242
  %318 = icmp eq i32 %47, %242
  %319 = icmp eq i32 %48, %242
  %320 = icmp eq i32 %49, %242
  %321 = icmp eq i32 %50, %242
  %322 = icmp eq i32 %51, %242
  %323 = icmp eq i32 %52, %242
  %324 = icmp slt i32 %45, %238
  %.not79 = icmp sle i32 %47, %238
  %.not80 = icmp sle i32 %48, %238
  %.not81 = icmp sle i32 %49, %238
  %.not82 = icmp sle i32 %50, %238
  %.not83 = icmp sle i32 %51, %238
  %.not84 = icmp sle i32 %52, %238
  %.not85 = icmp sle i32 %45, %242
  %325 = icmp slt i32 %45, %242
  %.not86 = icmp sle i32 %47, %242
  %.not87 = icmp sle i32 %48, %242
  %.not88 = icmp sle i32 %49, %242
  %.not89 = icmp sle i32 %50, %242
  %.not90 = icmp sle i32 %51, %242
  %.not91 = icmp sle i32 %52, %242
  %326 = or i1 %275, %291
  %.not = icmp sle i32 %45, %238
  %327 = and i1 %324, %293
  %328 = and i1 %.not79, %294
  %329 = and i1 %.not80, %295
  %330 = and i1 %.not81, %296
  %331 = and i1 %.not82, %297
  %332 = and i1 %.not83, %298
  %333 = and i1 %.not84, %299
  %334 = and i1 %.not85, %300
  %335 = and i1 %325, %301
  %336 = and i1 %.not86, %302
  %337 = and i1 %.not87, %303
  %338 = and i1 %.not88, %304
  %339 = and i1 %.not89, %305
  %340 = and i1 %.not90, %306
  %341 = and i1 %.not91, %307
  %342 = select i1 %326, i1 %.not, i1 %308
  %343 = or i1 %309, %327
  %344 = or i1 %310, %328
  %345 = or i1 %311, %329
  %346 = or i1 %312, %330
  %347 = or i1 %313, %331
  %348 = or i1 %314, %332
  %349 = or i1 %315, %333
  %350 = or i1 %316, %334
  %351 = or i1 %317, %335
  %352 = or i1 %318, %336
  %353 = or i1 %319, %337
  %354 = or i1 %320, %338
  %355 = or i1 %321, %339
  %356 = or i1 %322, %340
  %357 = or i1 %323, %341
  %358 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %90, <8 x i32> %256, i32 12, i32 12, i32 8, i32 8, i1 false)
  %359 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %90, <8 x i32> %258, i32 12, i32 12, i32 8, i32 8, i1 false)
  %360 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %358, <8 x i16> %91, <8 x i32> %259, i32 12, i32 12, i32 8, i32 8, i1 false)
  %361 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %359, <8 x i16> %91, <8 x i32> %260, i32 12, i32 12, i32 8, i32 8, i1 false)
  %362 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %360, <8 x i16> %94, <8 x i32> %261, i32 12, i32 12, i32 8, i32 8, i1 false)
  %363 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %361, <8 x i16> %94, <8 x i32> %262, i32 12, i32 12, i32 8, i32 8, i1 false)
  %364 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %362, <8 x i16> %95, <8 x i32> %263, i32 12, i32 12, i32 8, i32 8, i1 false)
  %365 = extractelement <8 x float> %364, i64 0
  %366 = extractelement <8 x float> %364, i64 1
  %367 = extractelement <8 x float> %364, i64 2
  %368 = extractelement <8 x float> %364, i64 3
  %369 = extractelement <8 x float> %364, i64 4
  %370 = extractelement <8 x float> %364, i64 5
  %371 = extractelement <8 x float> %364, i64 6
  %372 = extractelement <8 x float> %364, i64 7
  %373 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %363, <8 x i16> %95, <8 x i32> %264, i32 12, i32 12, i32 8, i32 8, i1 false)
  %374 = extractelement <8 x float> %373, i64 0
  %375 = extractelement <8 x float> %373, i64 1
  %376 = extractelement <8 x float> %373, i64 2
  %377 = extractelement <8 x float> %373, i64 3
  %378 = extractelement <8 x float> %373, i64 4
  %379 = extractelement <8 x float> %373, i64 5
  %380 = extractelement <8 x float> %373, i64 6
  %381 = extractelement <8 x float> %373, i64 7
  %382 = sext i32 %238 to i64
  %383 = icmp sgt i64 %37, %382
  %384 = icmp sgt i64 %37, %243
  %385 = and i1 %383, %342
  %386 = and i1 %383, %343
  %387 = and i1 %383, %344
  %388 = and i1 %383, %345
  %389 = and i1 %383, %346
  %390 = and i1 %383, %347
  %391 = and i1 %383, %348
  %392 = and i1 %383, %349
  %393 = and i1 %384, %350
  %394 = and i1 %384, %351
  %395 = and i1 %384, %352
  %396 = and i1 %384, %353
  %397 = and i1 %384, %354
  %398 = and i1 %384, %355
  %399 = and i1 %384, %356
  %400 = and i1 %384, %357
  %401 = select i1 %385, float %365, float 0xC1D0000000000000
  %402 = select i1 %386, float %366, float 0xC1D0000000000000
  %403 = select i1 %387, float %367, float 0xC1D0000000000000
  %404 = select i1 %388, float %368, float 0xC1D0000000000000
  %405 = select i1 %389, float %369, float 0xC1D0000000000000
  %406 = select i1 %390, float %370, float 0xC1D0000000000000
  %407 = select i1 %391, float %371, float 0xC1D0000000000000
  %408 = select i1 %392, float %372, float 0xC1D0000000000000
  %409 = select i1 %393, float %374, float 0xC1D0000000000000
  %410 = select i1 %394, float %375, float 0xC1D0000000000000
  %411 = select i1 %395, float %376, float 0xC1D0000000000000
  %412 = select i1 %396, float %377, float 0xC1D0000000000000
  %413 = select i1 %397, float %378, float 0xC1D0000000000000
  %414 = select i1 %398, float %379, float 0xC1D0000000000000
  %415 = select i1 %399, float %380, float 0xC1D0000000000000
  %416 = select i1 %400, float %381, float 0xC1D0000000000000
  %417 = call float @llvm.maxnum.f32(float %401, float %409)
  %418 = call float @llvm.maxnum.f32(float %402, float %410)
  %419 = call float @llvm.maxnum.f32(float %403, float %411)
  %420 = call float @llvm.maxnum.f32(float %404, float %412)
  %421 = call float @llvm.maxnum.f32(float %405, float %413)
  %422 = call float @llvm.maxnum.f32(float %406, float %414)
  %423 = call float @llvm.maxnum.f32(float %407, float %415)
  %424 = call float @llvm.maxnum.f32(float %408, float %416)
  %425 = call float @llvm.genx.GenISA.WaveAll.f32(float %417, i8 12, i1 true, i32 0)
  %426 = call float @llvm.genx.GenISA.WaveAll.f32(float %418, i8 12, i1 true, i32 0)
  %427 = call float @llvm.genx.GenISA.WaveAll.f32(float %419, i8 12, i1 true, i32 0)
  %428 = call float @llvm.genx.GenISA.WaveAll.f32(float %420, i8 12, i1 true, i32 0)
  %429 = call float @llvm.genx.GenISA.WaveAll.f32(float %421, i8 12, i1 true, i32 0)
  %430 = call float @llvm.genx.GenISA.WaveAll.f32(float %422, i8 12, i1 true, i32 0)
  %431 = call float @llvm.genx.GenISA.WaveAll.f32(float %423, i8 12, i1 true, i32 0)
  %432 = call float @llvm.genx.GenISA.WaveAll.f32(float %424, i8 12, i1 true, i32 0)
  %433 = call float @llvm.maxnum.f32(float %194, float %425)
  %434 = call float @llvm.maxnum.f32(float %195, float %426)
  %435 = call float @llvm.maxnum.f32(float %196, float %427)
  %436 = call float @llvm.maxnum.f32(float %197, float %428)
  %437 = call float @llvm.maxnum.f32(float %198, float %429)
  %438 = call float @llvm.maxnum.f32(float %199, float %430)
  %439 = call float @llvm.maxnum.f32(float %200, float %431)
  %440 = call float @llvm.maxnum.f32(float %201, float %432)
  %441 = fsub float %194, %433
  %442 = fsub float %195, %434
  %443 = fsub float %196, %435
  %444 = fsub float %197, %436
  %445 = fsub float %198, %437
  %446 = fsub float %199, %438
  %447 = fsub float %200, %439
  %448 = fsub float %201, %440
  %449 = fmul contract float %41, %441
  %450 = fmul contract float %41, %442
  %451 = fmul contract float %41, %443
  %452 = fmul contract float %41, %444
  %453 = fmul contract float %41, %445
  %454 = fmul contract float %41, %446
  %455 = fmul contract float %41, %447
  %456 = fmul contract float %41, %448
  %457 = call float @llvm.exp2.f32(float %449)
  %458 = call float @llvm.exp2.f32(float %450)
  %459 = call float @llvm.exp2.f32(float %451)
  %460 = call float @llvm.exp2.f32(float %452)
  %461 = call float @llvm.exp2.f32(float %453)
  %462 = call float @llvm.exp2.f32(float %454)
  %463 = call float @llvm.exp2.f32(float %455)
  %464 = call float @llvm.exp2.f32(float %456)
  %465 = fsub float %401, %433
  %466 = fsub float %402, %434
  %467 = fsub float %403, %435
  %468 = fsub float %404, %436
  %469 = fsub float %405, %437
  %470 = fsub float %406, %438
  %471 = fsub float %407, %439
  %472 = fsub float %408, %440
  %473 = fsub float %409, %433
  %474 = fsub float %410, %434
  %475 = fsub float %411, %435
  %476 = fsub float %412, %436
  %477 = fsub float %413, %437
  %478 = fsub float %414, %438
  %479 = fsub float %415, %439
  %480 = fsub float %416, %440
  %481 = fmul contract float %41, %465
  %482 = fmul contract float %41, %466
  %483 = fmul contract float %41, %467
  %484 = fmul contract float %41, %468
  %485 = fmul contract float %41, %469
  %486 = fmul contract float %41, %470
  %487 = fmul contract float %41, %471
  %488 = fmul contract float %41, %472
  %489 = fmul contract float %41, %473
  %490 = fmul contract float %41, %474
  %491 = fmul contract float %41, %475
  %492 = fmul contract float %41, %476
  %493 = fmul contract float %41, %477
  %494 = fmul contract float %41, %478
  %495 = fmul contract float %41, %479
  %496 = fmul contract float %41, %480
  %497 = call float @llvm.exp2.f32(float %481)
  %498 = call float @llvm.exp2.f32(float %482)
  %499 = call float @llvm.exp2.f32(float %483)
  %500 = call float @llvm.exp2.f32(float %484)
  %501 = call float @llvm.exp2.f32(float %485)
  %502 = call float @llvm.exp2.f32(float %486)
  %503 = call float @llvm.exp2.f32(float %487)
  %504 = call float @llvm.exp2.f32(float %488)
  %505 = call float @llvm.exp2.f32(float %489)
  %506 = call float @llvm.exp2.f32(float %490)
  %507 = call float @llvm.exp2.f32(float %491)
  %508 = call float @llvm.exp2.f32(float %492)
  %509 = call float @llvm.exp2.f32(float %493)
  %510 = call float @llvm.exp2.f32(float %494)
  %511 = call float @llvm.exp2.f32(float %495)
  %512 = call float @llvm.exp2.f32(float %496)
  %513 = fadd contract float %497, %505
  %514 = fadd contract float %498, %506
  %515 = fadd contract float %499, %507
  %516 = fadd contract float %500, %508
  %517 = fadd contract float %501, %509
  %518 = fadd contract float %502, %510
  %519 = fadd contract float %503, %511
  %520 = fadd contract float %504, %512
  %521 = call float @llvm.genx.GenISA.WaveAll.f32(float %513, i8 9, i1 true, i32 0)
  %522 = call float @llvm.genx.GenISA.WaveAll.f32(float %514, i8 9, i1 true, i32 0)
  %523 = call float @llvm.genx.GenISA.WaveAll.f32(float %515, i8 9, i1 true, i32 0)
  %524 = call float @llvm.genx.GenISA.WaveAll.f32(float %516, i8 9, i1 true, i32 0)
  %525 = call float @llvm.genx.GenISA.WaveAll.f32(float %517, i8 9, i1 true, i32 0)
  %526 = call float @llvm.genx.GenISA.WaveAll.f32(float %518, i8 9, i1 true, i32 0)
  %527 = call float @llvm.genx.GenISA.WaveAll.f32(float %519, i8 9, i1 true, i32 0)
  %528 = call float @llvm.genx.GenISA.WaveAll.f32(float %520, i8 9, i1 true, i32 0)
  %529 = fmul contract float %202, %457
  %530 = fmul contract float %203, %458
  %531 = fmul contract float %204, %459
  %532 = fmul contract float %205, %460
  %533 = fmul contract float %206, %461
  %534 = fmul contract float %207, %462
  %535 = fmul contract float %208, %463
  %536 = fmul contract float %209, %464
  %537 = fmul contract float %210, %457
  %538 = fmul contract float %211, %458
  %539 = fmul contract float %212, %459
  %540 = fmul contract float %213, %460
  %541 = fmul contract float %214, %461
  %542 = fmul contract float %215, %462
  %543 = fmul contract float %216, %463
  %544 = fmul contract float %217, %464
  %545 = fmul contract float %218, %457
  %546 = fmul contract float %219, %458
  %547 = fmul contract float %220, %459
  %548 = fmul contract float %221, %460
  %549 = fmul contract float %222, %461
  %550 = fmul contract float %223, %462
  %551 = fmul contract float %224, %463
  %552 = fmul contract float %225, %464
  %553 = fmul contract float %226, %457
  %554 = fmul contract float %227, %458
  %555 = fmul contract float %228, %459
  %556 = fmul contract float %229, %460
  %557 = fmul contract float %230, %461
  %558 = fmul contract float %231, %462
  %559 = fmul contract float %232, %463
  %560 = fmul contract float %233, %464
  %561 = fptrunc float %497 to half
  %562 = fptrunc float %498 to half
  %563 = fptrunc float %499 to half
  %564 = fptrunc float %500 to half
  %565 = fptrunc float %501 to half
  %566 = fptrunc float %502 to half
  %567 = fptrunc float %503 to half
  %568 = fptrunc float %504 to half
  %569 = fptrunc float %505 to half
  %570 = fptrunc float %506 to half
  %571 = fptrunc float %507 to half
  %572 = fptrunc float %508 to half
  %573 = fptrunc float %509 to half
  %574 = fptrunc float %510 to half
  %575 = fptrunc float %511 to half
  %576 = fptrunc float %512 to half
  %577 = insertelement <8 x float> undef, float %529, i64 0
  %578 = insertelement <8 x float> %577, float %530, i64 1
  %579 = insertelement <8 x float> %578, float %531, i64 2
  %580 = insertelement <8 x float> %579, float %532, i64 3
  %581 = insertelement <8 x float> %580, float %533, i64 4
  %582 = insertelement <8 x float> %581, float %534, i64 5
  %583 = insertelement <8 x float> %582, float %535, i64 6
  %584 = insertelement <8 x float> %583, float %536, i64 7
  %585 = insertelement <8 x float> undef, float %537, i64 0
  %586 = insertelement <8 x float> %585, float %538, i64 1
  %587 = insertelement <8 x float> %586, float %539, i64 2
  %588 = insertelement <8 x float> %587, float %540, i64 3
  %589 = insertelement <8 x float> %588, float %541, i64 4
  %590 = insertelement <8 x float> %589, float %542, i64 5
  %591 = insertelement <8 x float> %590, float %543, i64 6
  %592 = insertelement <8 x float> %591, float %544, i64 7
  %593 = insertelement <8 x float> undef, float %545, i64 0
  %594 = insertelement <8 x float> %593, float %546, i64 1
  %595 = insertelement <8 x float> %594, float %547, i64 2
  %596 = insertelement <8 x float> %595, float %548, i64 3
  %597 = insertelement <8 x float> %596, float %549, i64 4
  %598 = insertelement <8 x float> %597, float %550, i64 5
  %599 = insertelement <8 x float> %598, float %551, i64 6
  %600 = insertelement <8 x float> %599, float %552, i64 7
  %601 = insertelement <8 x float> undef, float %553, i64 0
  %602 = insertelement <8 x float> %601, float %554, i64 1
  %603 = insertelement <8 x float> %602, float %555, i64 2
  %604 = insertelement <8 x float> %603, float %556, i64 3
  %605 = insertelement <8 x float> %604, float %557, i64 4
  %606 = insertelement <8 x float> %605, float %558, i64 5
  %607 = insertelement <8 x float> %606, float %559, i64 6
  %608 = insertelement <8 x float> %607, float %560, i64 7
  %609 = insertelement <8 x half> undef, half %561, i64 0
  %610 = insertelement <8 x half> %609, half %562, i64 1
  %611 = insertelement <8 x half> %610, half %563, i64 2
  %612 = insertelement <8 x half> %611, half %564, i64 3
  %613 = insertelement <8 x half> %612, half %565, i64 4
  %614 = insertelement <8 x half> %613, half %566, i64 5
  %615 = insertelement <8 x half> %614, half %567, i64 6
  %616 = insertelement <8 x half> %615, half %568, i64 7
  %617 = bitcast <8 x half> %616 to <8 x i16>
  %618 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %584, <8 x i16> %617, <8 x i32> %266, i32 12, i32 12, i32 8, i32 8, i1 false)
  %619 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %592, <8 x i16> %617, <8 x i32> %268, i32 12, i32 12, i32 8, i32 8, i1 false)
  %620 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %600, <8 x i16> %617, <8 x i32> %271, i32 12, i32 12, i32 8, i32 8, i1 false)
  %621 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %608, <8 x i16> %617, <8 x i32> %273, i32 12, i32 12, i32 8, i32 8, i1 false)
  %622 = insertelement <8 x half> undef, half %569, i64 0
  %623 = insertelement <8 x half> %622, half %570, i64 1
  %624 = insertelement <8 x half> %623, half %571, i64 2
  %625 = insertelement <8 x half> %624, half %572, i64 3
  %626 = insertelement <8 x half> %625, half %573, i64 4
  %627 = insertelement <8 x half> %626, half %574, i64 5
  %628 = insertelement <8 x half> %627, half %575, i64 6
  %629 = insertelement <8 x half> %628, half %576, i64 7
  %630 = bitcast <8 x half> %629 to <8 x i16>
  %631 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %618, <8 x i16> %630, <8 x i32> %267, i32 12, i32 12, i32 8, i32 8, i1 false)
  %632 = extractelement <8 x float> %631, i64 0
  %633 = extractelement <8 x float> %631, i64 1
  %634 = extractelement <8 x float> %631, i64 2
  %635 = extractelement <8 x float> %631, i64 3
  %636 = extractelement <8 x float> %631, i64 4
  %637 = extractelement <8 x float> %631, i64 5
  %638 = extractelement <8 x float> %631, i64 6
  %639 = extractelement <8 x float> %631, i64 7
  %640 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %619, <8 x i16> %630, <8 x i32> %269, i32 12, i32 12, i32 8, i32 8, i1 false)
  %641 = extractelement <8 x float> %640, i64 0
  %642 = extractelement <8 x float> %640, i64 1
  %643 = extractelement <8 x float> %640, i64 2
  %644 = extractelement <8 x float> %640, i64 3
  %645 = extractelement <8 x float> %640, i64 4
  %646 = extractelement <8 x float> %640, i64 5
  %647 = extractelement <8 x float> %640, i64 6
  %648 = extractelement <8 x float> %640, i64 7
  %649 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %620, <8 x i16> %630, <8 x i32> %272, i32 12, i32 12, i32 8, i32 8, i1 false)
  %650 = extractelement <8 x float> %649, i64 0
  %651 = extractelement <8 x float> %649, i64 1
  %652 = extractelement <8 x float> %649, i64 2
  %653 = extractelement <8 x float> %649, i64 3
  %654 = extractelement <8 x float> %649, i64 4
  %655 = extractelement <8 x float> %649, i64 5
  %656 = extractelement <8 x float> %649, i64 6
  %657 = extractelement <8 x float> %649, i64 7
  %658 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %621, <8 x i16> %630, <8 x i32> %274, i32 12, i32 12, i32 8, i32 8, i1 false)
  %659 = extractelement <8 x float> %658, i64 0
  %660 = extractelement <8 x float> %658, i64 1
  %661 = extractelement <8 x float> %658, i64 2
  %662 = extractelement <8 x float> %658, i64 3
  %663 = extractelement <8 x float> %658, i64 4
  %664 = extractelement <8 x float> %658, i64 5
  %665 = extractelement <8 x float> %658, i64 6
  %666 = extractelement <8 x float> %658, i64 7
  %667 = fmul contract float %186, %457
  %668 = fmul contract float %187, %458
  %669 = fmul contract float %188, %459
  %670 = fmul contract float %189, %460
  %671 = fmul contract float %190, %461
  %672 = fmul contract float %191, %462
  %673 = fmul contract float %192, %463
  %674 = fmul contract float %193, %464
  %675 = fadd contract float %667, %521
  %676 = fadd contract float %668, %522
  %677 = fadd contract float %669, %523
  %678 = fadd contract float %670, %524
  %679 = fadd contract float %671, %525
  %680 = fadd contract float %672, %526
  %681 = fadd contract float %673, %527
  %682 = fadd contract float %674, %528
  %683 = add i64 %234, 32
  %684 = icmp slt i64 %683, %37
  br i1 %684, label %183, label %._crit_edge

._crit_edge:                                      ; preds = %._crit_edge148
  %685 = fdiv float %632, %675
  %686 = fdiv float %633, %676
  %687 = fdiv float %634, %677
  %688 = fdiv float %635, %678
  %689 = fdiv float %636, %679
  %690 = fdiv float %637, %680
  %691 = fdiv float %638, %681
  %692 = fdiv float %639, %682
  %693 = fdiv float %641, %675
  %694 = fdiv float %642, %676
  %695 = fdiv float %643, %677
  %696 = fdiv float %644, %678
  %697 = fdiv float %645, %679
  %698 = fdiv float %646, %680
  %699 = fdiv float %647, %681
  %700 = fdiv float %648, %682
  %701 = fdiv float %650, %675
  %702 = fdiv float %651, %676
  %703 = fdiv float %652, %677
  %704 = fdiv float %653, %678
  %705 = fdiv float %654, %679
  %706 = fdiv float %655, %680
  %707 = fdiv float %656, %681
  %708 = fdiv float %657, %682
  %709 = fdiv float %659, %675
  %710 = fdiv float %660, %676
  %711 = fdiv float %661, %677
  %712 = fdiv float %662, %678
  %713 = fdiv float %663, %679
  %714 = fdiv float %664, %680
  %715 = fdiv float %665, %681
  %716 = fdiv float %666, %682
  %717 = fmul contract float %433, %40
  %718 = fmul contract float %434, %40
  %719 = fmul contract float %435, %40
  %720 = fmul contract float %436, %40
  %721 = fmul contract float %437, %40
  %722 = fmul contract float %438, %40
  %723 = fmul contract float %439, %40
  %724 = fmul contract float %440, %40
  %725 = bitcast float %675 to i32
  %726 = and i32 %725, 2147483647
  %727 = bitcast i32 %726 to float
  %728 = fcmp uge float %727, 0x7FF0000000000000
  %729 = fcmp ule float %675, 0.000000e+00
  %730 = or i1 %728, %729
  br i1 %730, label %757, label %731

731:                                              ; preds = %._crit_edge
  %732 = fmul float %675, 0x4160000000000000
  %733 = fcmp olt float %675, 0x3810000000000000
  %734 = select i1 %733, float -2.300000e+01, float 0.000000e+00
  %735 = select i1 %733, float %732, float %675
  %736 = bitcast float %735 to i32
  %737 = add nsw i32 %736, -1059760811
  %738 = and i32 %737, 8388607
  %739 = add nuw nsw i32 %738, 1059760811
  %740 = ashr i32 %737, 23
  %741 = sitofp i32 %740 to float
  %742 = fadd float %734, %741
  %743 = bitcast i32 %739 to float
  %744 = fadd float %743, -1.000000e+00
  %745 = call float @llvm.fma.f32(float %744, float 0xBFC0805900000000, float 0x3FC1E66BA0000000)
  %746 = call float @llvm.fma.f32(float %745, float %744, float 0xBFBF3113C0000000)
  %747 = call float @llvm.fma.f32(float %746, float %744, float 0x3FC1ED7180000000)
  %748 = call float @llvm.fma.f32(float %747, float %744, float 0xBFC559DCC0000000)
  %749 = call float @llvm.fma.f32(float %748, float %744, float 0x3FC99D0280000000)
  %750 = call float @llvm.fma.f32(float %749, float %744, float 0xBFCFFFEF00000000)
  %751 = call float @llvm.fma.f32(float %750, float %744, float 0x3FD5555060000000)
  %752 = call float @llvm.fma.f32(float %751, float %744, float -5.000000e-01)
  %753 = fmul float %744, %752
  %754 = call float @llvm.fma.f32(float %753, float %744, float %744)
  %755 = call float @llvm.fma.f32(float %742, float 0x3EB7F7D1C0000000, float %754)
  %756 = call float @llvm.fma.f32(float %742, float 0x3FE62E4000000000, float %755)
  br label %_Z15__spirv_ocl_logf.exit

757:                                              ; preds = %._crit_edge
  %758 = call float @llvm.log2.f32(float %675)
  br label %_Z15__spirv_ocl_logf.exit

_Z15__spirv_ocl_logf.exit:                        ; preds = %731, %757
  %759 = phi float [ %756, %731 ], [ %758, %757 ]
  %760 = bitcast float %676 to i32
  %761 = and i32 %760, 2147483647
  %762 = bitcast i32 %761 to float
  %763 = fcmp uge float %762, 0x7FF0000000000000
  %764 = fcmp ule float %676, 0.000000e+00
  %765 = or i1 %763, %764
  br i1 %765, label %792, label %766

766:                                              ; preds = %_Z15__spirv_ocl_logf.exit
  %767 = fmul float %676, 0x4160000000000000
  %768 = fcmp olt float %676, 0x3810000000000000
  %769 = select i1 %768, float -2.300000e+01, float 0.000000e+00
  %770 = select i1 %768, float %767, float %676
  %771 = bitcast float %770 to i32
  %772 = add nsw i32 %771, -1059760811
  %773 = and i32 %772, 8388607
  %774 = add nuw nsw i32 %773, 1059760811
  %775 = ashr i32 %772, 23
  %776 = sitofp i32 %775 to float
  %777 = fadd float %769, %776
  %778 = bitcast i32 %774 to float
  %779 = fadd float %778, -1.000000e+00
  %780 = call float @llvm.fma.f32(float %779, float 0xBFC0805900000000, float 0x3FC1E66BA0000000)
  %781 = call float @llvm.fma.f32(float %780, float %779, float 0xBFBF3113C0000000)
  %782 = call float @llvm.fma.f32(float %781, float %779, float 0x3FC1ED7180000000)
  %783 = call float @llvm.fma.f32(float %782, float %779, float 0xBFC559DCC0000000)
  %784 = call float @llvm.fma.f32(float %783, float %779, float 0x3FC99D0280000000)
  %785 = call float @llvm.fma.f32(float %784, float %779, float 0xBFCFFFEF00000000)
  %786 = call float @llvm.fma.f32(float %785, float %779, float 0x3FD5555060000000)
  %787 = call float @llvm.fma.f32(float %786, float %779, float -5.000000e-01)
  %788 = fmul float %779, %787
  %789 = call float @llvm.fma.f32(float %788, float %779, float %779)
  %790 = call float @llvm.fma.f32(float %777, float 0x3EB7F7D1C0000000, float %789)
  %791 = call float @llvm.fma.f32(float %777, float 0x3FE62E4000000000, float %790)
  br label %_Z15__spirv_ocl_logf.exit94

792:                                              ; preds = %_Z15__spirv_ocl_logf.exit
  %793 = call float @llvm.log2.f32(float %676)
  br label %_Z15__spirv_ocl_logf.exit94

_Z15__spirv_ocl_logf.exit94:                      ; preds = %766, %792
  %794 = phi float [ %791, %766 ], [ %793, %792 ]
  %795 = bitcast float %677 to i32
  %796 = and i32 %795, 2147483647
  %797 = bitcast i32 %796 to float
  %798 = fcmp uge float %797, 0x7FF0000000000000
  %799 = fcmp ule float %677, 0.000000e+00
  %800 = or i1 %798, %799
  br i1 %800, label %827, label %801

801:                                              ; preds = %_Z15__spirv_ocl_logf.exit94
  %802 = fmul float %677, 0x4160000000000000
  %803 = fcmp olt float %677, 0x3810000000000000
  %804 = select i1 %803, float -2.300000e+01, float 0.000000e+00
  %805 = select i1 %803, float %802, float %677
  %806 = bitcast float %805 to i32
  %807 = add nsw i32 %806, -1059760811
  %808 = and i32 %807, 8388607
  %809 = add nuw nsw i32 %808, 1059760811
  %810 = ashr i32 %807, 23
  %811 = sitofp i32 %810 to float
  %812 = fadd float %804, %811
  %813 = bitcast i32 %809 to float
  %814 = fadd float %813, -1.000000e+00
  %815 = call float @llvm.fma.f32(float %814, float 0xBFC0805900000000, float 0x3FC1E66BA0000000)
  %816 = call float @llvm.fma.f32(float %815, float %814, float 0xBFBF3113C0000000)
  %817 = call float @llvm.fma.f32(float %816, float %814, float 0x3FC1ED7180000000)
  %818 = call float @llvm.fma.f32(float %817, float %814, float 0xBFC559DCC0000000)
  %819 = call float @llvm.fma.f32(float %818, float %814, float 0x3FC99D0280000000)
  %820 = call float @llvm.fma.f32(float %819, float %814, float 0xBFCFFFEF00000000)
  %821 = call float @llvm.fma.f32(float %820, float %814, float 0x3FD5555060000000)
  %822 = call float @llvm.fma.f32(float %821, float %814, float -5.000000e-01)
  %823 = fmul float %814, %822
  %824 = call float @llvm.fma.f32(float %823, float %814, float %814)
  %825 = call float @llvm.fma.f32(float %812, float 0x3EB7F7D1C0000000, float %824)
  %826 = call float @llvm.fma.f32(float %812, float 0x3FE62E4000000000, float %825)
  br label %_Z15__spirv_ocl_logf.exit95

827:                                              ; preds = %_Z15__spirv_ocl_logf.exit94
  %828 = call float @llvm.log2.f32(float %677)
  br label %_Z15__spirv_ocl_logf.exit95

_Z15__spirv_ocl_logf.exit95:                      ; preds = %801, %827
  %829 = phi float [ %826, %801 ], [ %828, %827 ]
  %830 = bitcast float %678 to i32
  %831 = and i32 %830, 2147483647
  %832 = bitcast i32 %831 to float
  %833 = fcmp uge float %832, 0x7FF0000000000000
  %834 = fcmp ule float %678, 0.000000e+00
  %835 = or i1 %833, %834
  br i1 %835, label %862, label %836

836:                                              ; preds = %_Z15__spirv_ocl_logf.exit95
  %837 = fmul float %678, 0x4160000000000000
  %838 = fcmp olt float %678, 0x3810000000000000
  %839 = select i1 %838, float -2.300000e+01, float 0.000000e+00
  %840 = select i1 %838, float %837, float %678
  %841 = bitcast float %840 to i32
  %842 = add nsw i32 %841, -1059760811
  %843 = and i32 %842, 8388607
  %844 = add nuw nsw i32 %843, 1059760811
  %845 = ashr i32 %842, 23
  %846 = sitofp i32 %845 to float
  %847 = fadd float %839, %846
  %848 = bitcast i32 %844 to float
  %849 = fadd float %848, -1.000000e+00
  %850 = call float @llvm.fma.f32(float %849, float 0xBFC0805900000000, float 0x3FC1E66BA0000000)
  %851 = call float @llvm.fma.f32(float %850, float %849, float 0xBFBF3113C0000000)
  %852 = call float @llvm.fma.f32(float %851, float %849, float 0x3FC1ED7180000000)
  %853 = call float @llvm.fma.f32(float %852, float %849, float 0xBFC559DCC0000000)
  %854 = call float @llvm.fma.f32(float %853, float %849, float 0x3FC99D0280000000)
  %855 = call float @llvm.fma.f32(float %854, float %849, float 0xBFCFFFEF00000000)
  %856 = call float @llvm.fma.f32(float %855, float %849, float 0x3FD5555060000000)
  %857 = call float @llvm.fma.f32(float %856, float %849, float -5.000000e-01)
  %858 = fmul float %849, %857
  %859 = call float @llvm.fma.f32(float %858, float %849, float %849)
  %860 = call float @llvm.fma.f32(float %847, float 0x3EB7F7D1C0000000, float %859)
  %861 = call float @llvm.fma.f32(float %847, float 0x3FE62E4000000000, float %860)
  br label %_Z15__spirv_ocl_logf.exit96

862:                                              ; preds = %_Z15__spirv_ocl_logf.exit95
  %863 = call float @llvm.log2.f32(float %678)
  br label %_Z15__spirv_ocl_logf.exit96

_Z15__spirv_ocl_logf.exit96:                      ; preds = %836, %862
  %864 = phi float [ %861, %836 ], [ %863, %862 ]
  %865 = bitcast float %679 to i32
  %866 = and i32 %865, 2147483647
  %867 = bitcast i32 %866 to float
  %868 = fcmp uge float %867, 0x7FF0000000000000
  %869 = fcmp ule float %679, 0.000000e+00
  %870 = or i1 %868, %869
  br i1 %870, label %897, label %871

871:                                              ; preds = %_Z15__spirv_ocl_logf.exit96
  %872 = fmul float %679, 0x4160000000000000
  %873 = fcmp olt float %679, 0x3810000000000000
  %874 = select i1 %873, float -2.300000e+01, float 0.000000e+00
  %875 = select i1 %873, float %872, float %679
  %876 = bitcast float %875 to i32
  %877 = add nsw i32 %876, -1059760811
  %878 = and i32 %877, 8388607
  %879 = add nuw nsw i32 %878, 1059760811
  %880 = ashr i32 %877, 23
  %881 = sitofp i32 %880 to float
  %882 = fadd float %874, %881
  %883 = bitcast i32 %879 to float
  %884 = fadd float %883, -1.000000e+00
  %885 = call float @llvm.fma.f32(float %884, float 0xBFC0805900000000, float 0x3FC1E66BA0000000)
  %886 = call float @llvm.fma.f32(float %885, float %884, float 0xBFBF3113C0000000)
  %887 = call float @llvm.fma.f32(float %886, float %884, float 0x3FC1ED7180000000)
  %888 = call float @llvm.fma.f32(float %887, float %884, float 0xBFC559DCC0000000)
  %889 = call float @llvm.fma.f32(float %888, float %884, float 0x3FC99D0280000000)
  %890 = call float @llvm.fma.f32(float %889, float %884, float 0xBFCFFFEF00000000)
  %891 = call float @llvm.fma.f32(float %890, float %884, float 0x3FD5555060000000)
  %892 = call float @llvm.fma.f32(float %891, float %884, float -5.000000e-01)
  %893 = fmul float %884, %892
  %894 = call float @llvm.fma.f32(float %893, float %884, float %884)
  %895 = call float @llvm.fma.f32(float %882, float 0x3EB7F7D1C0000000, float %894)
  %896 = call float @llvm.fma.f32(float %882, float 0x3FE62E4000000000, float %895)
  br label %_Z15__spirv_ocl_logf.exit97

897:                                              ; preds = %_Z15__spirv_ocl_logf.exit96
  %898 = call float @llvm.log2.f32(float %679)
  br label %_Z15__spirv_ocl_logf.exit97

_Z15__spirv_ocl_logf.exit97:                      ; preds = %871, %897
  %899 = phi float [ %896, %871 ], [ %898, %897 ]
  %900 = bitcast float %680 to i32
  %901 = and i32 %900, 2147483647
  %902 = bitcast i32 %901 to float
  %903 = fcmp uge float %902, 0x7FF0000000000000
  %904 = fcmp ule float %680, 0.000000e+00
  %905 = or i1 %903, %904
  br i1 %905, label %932, label %906

906:                                              ; preds = %_Z15__spirv_ocl_logf.exit97
  %907 = fmul float %680, 0x4160000000000000
  %908 = fcmp olt float %680, 0x3810000000000000
  %909 = select i1 %908, float -2.300000e+01, float 0.000000e+00
  %910 = select i1 %908, float %907, float %680
  %911 = bitcast float %910 to i32
  %912 = add nsw i32 %911, -1059760811
  %913 = and i32 %912, 8388607
  %914 = add nuw nsw i32 %913, 1059760811
  %915 = ashr i32 %912, 23
  %916 = sitofp i32 %915 to float
  %917 = fadd float %909, %916
  %918 = bitcast i32 %914 to float
  %919 = fadd float %918, -1.000000e+00
  %920 = call float @llvm.fma.f32(float %919, float 0xBFC0805900000000, float 0x3FC1E66BA0000000)
  %921 = call float @llvm.fma.f32(float %920, float %919, float 0xBFBF3113C0000000)
  %922 = call float @llvm.fma.f32(float %921, float %919, float 0x3FC1ED7180000000)
  %923 = call float @llvm.fma.f32(float %922, float %919, float 0xBFC559DCC0000000)
  %924 = call float @llvm.fma.f32(float %923, float %919, float 0x3FC99D0280000000)
  %925 = call float @llvm.fma.f32(float %924, float %919, float 0xBFCFFFEF00000000)
  %926 = call float @llvm.fma.f32(float %925, float %919, float 0x3FD5555060000000)
  %927 = call float @llvm.fma.f32(float %926, float %919, float -5.000000e-01)
  %928 = fmul float %919, %927
  %929 = call float @llvm.fma.f32(float %928, float %919, float %919)
  %930 = call float @llvm.fma.f32(float %917, float 0x3EB7F7D1C0000000, float %929)
  %931 = call float @llvm.fma.f32(float %917, float 0x3FE62E4000000000, float %930)
  br label %_Z15__spirv_ocl_logf.exit98

932:                                              ; preds = %_Z15__spirv_ocl_logf.exit97
  %933 = call float @llvm.log2.f32(float %680)
  br label %_Z15__spirv_ocl_logf.exit98

_Z15__spirv_ocl_logf.exit98:                      ; preds = %906, %932
  %934 = phi float [ %931, %906 ], [ %933, %932 ]
  %935 = bitcast float %681 to i32
  %936 = and i32 %935, 2147483647
  %937 = bitcast i32 %936 to float
  %938 = fcmp uge float %937, 0x7FF0000000000000
  %939 = fcmp ule float %681, 0.000000e+00
  %940 = or i1 %938, %939
  br i1 %940, label %967, label %941

941:                                              ; preds = %_Z15__spirv_ocl_logf.exit98
  %942 = fmul float %681, 0x4160000000000000
  %943 = fcmp olt float %681, 0x3810000000000000
  %944 = select i1 %943, float -2.300000e+01, float 0.000000e+00
  %945 = select i1 %943, float %942, float %681
  %946 = bitcast float %945 to i32
  %947 = add nsw i32 %946, -1059760811
  %948 = and i32 %947, 8388607
  %949 = add nuw nsw i32 %948, 1059760811
  %950 = ashr i32 %947, 23
  %951 = sitofp i32 %950 to float
  %952 = fadd float %944, %951
  %953 = bitcast i32 %949 to float
  %954 = fadd float %953, -1.000000e+00
  %955 = call float @llvm.fma.f32(float %954, float 0xBFC0805900000000, float 0x3FC1E66BA0000000)
  %956 = call float @llvm.fma.f32(float %955, float %954, float 0xBFBF3113C0000000)
  %957 = call float @llvm.fma.f32(float %956, float %954, float 0x3FC1ED7180000000)
  %958 = call float @llvm.fma.f32(float %957, float %954, float 0xBFC559DCC0000000)
  %959 = call float @llvm.fma.f32(float %958, float %954, float 0x3FC99D0280000000)
  %960 = call float @llvm.fma.f32(float %959, float %954, float 0xBFCFFFEF00000000)
  %961 = call float @llvm.fma.f32(float %960, float %954, float 0x3FD5555060000000)
  %962 = call float @llvm.fma.f32(float %961, float %954, float -5.000000e-01)
  %963 = fmul float %954, %962
  %964 = call float @llvm.fma.f32(float %963, float %954, float %954)
  %965 = call float @llvm.fma.f32(float %952, float 0x3EB7F7D1C0000000, float %964)
  %966 = call float @llvm.fma.f32(float %952, float 0x3FE62E4000000000, float %965)
  br label %_Z15__spirv_ocl_logf.exit99

967:                                              ; preds = %_Z15__spirv_ocl_logf.exit98
  %968 = call float @llvm.log2.f32(float %681)
  br label %_Z15__spirv_ocl_logf.exit99

_Z15__spirv_ocl_logf.exit99:                      ; preds = %941, %967
  %969 = phi float [ %966, %941 ], [ %968, %967 ]
  %970 = bitcast float %682 to i32
  %971 = and i32 %970, 2147483647
  %972 = bitcast i32 %971 to float
  %973 = fcmp uge float %972, 0x7FF0000000000000
  %974 = fcmp ule float %682, 0.000000e+00
  %975 = or i1 %973, %974
  br i1 %975, label %1002, label %976

976:                                              ; preds = %_Z15__spirv_ocl_logf.exit99
  %977 = fmul float %682, 0x4160000000000000
  %978 = fcmp olt float %682, 0x3810000000000000
  %979 = select i1 %978, float -2.300000e+01, float 0.000000e+00
  %980 = select i1 %978, float %977, float %682
  %981 = bitcast float %980 to i32
  %982 = add nsw i32 %981, -1059760811
  %983 = and i32 %982, 8388607
  %984 = add nuw nsw i32 %983, 1059760811
  %985 = ashr i32 %982, 23
  %986 = sitofp i32 %985 to float
  %987 = fadd float %979, %986
  %988 = bitcast i32 %984 to float
  %989 = fadd float %988, -1.000000e+00
  %990 = call float @llvm.fma.f32(float %989, float 0xBFC0805900000000, float 0x3FC1E66BA0000000)
  %991 = call float @llvm.fma.f32(float %990, float %989, float 0xBFBF3113C0000000)
  %992 = call float @llvm.fma.f32(float %991, float %989, float 0x3FC1ED7180000000)
  %993 = call float @llvm.fma.f32(float %992, float %989, float 0xBFC559DCC0000000)
  %994 = call float @llvm.fma.f32(float %993, float %989, float 0x3FC99D0280000000)
  %995 = call float @llvm.fma.f32(float %994, float %989, float 0xBFCFFFEF00000000)
  %996 = call float @llvm.fma.f32(float %995, float %989, float 0x3FD5555060000000)
  %997 = call float @llvm.fma.f32(float %996, float %989, float -5.000000e-01)
  %998 = fmul float %989, %997
  %999 = call float @llvm.fma.f32(float %998, float %989, float %989)
  %1000 = call float @llvm.fma.f32(float %987, float 0x3EB7F7D1C0000000, float %999)
  %1001 = call float @llvm.fma.f32(float %987, float 0x3FE62E4000000000, float %1000)
  br label %_Z15__spirv_ocl_logf.exit100

1002:                                             ; preds = %_Z15__spirv_ocl_logf.exit99
  %1003 = call float @llvm.log2.f32(float %682)
  br label %_Z15__spirv_ocl_logf.exit100

_Z15__spirv_ocl_logf.exit100:                     ; preds = %976, %1002
  %1004 = phi float [ %1001, %976 ], [ %1003, %1002 ]
  %1005 = fadd contract float %717, %759
  %1006 = fadd contract float %718, %794
  %1007 = fadd contract float %719, %829
  %1008 = fadd contract float %720, %864
  %1009 = fadd contract float %721, %899
  %1010 = fadd contract float %722, %934
  %1011 = fadd contract float %723, %969
  %1012 = fadd contract float %724, %1004
  %1013 = fptrunc float %685 to half
  %1014 = fptrunc float %686 to half
  %1015 = fptrunc float %687 to half
  %1016 = fptrunc float %688 to half
  %1017 = fptrunc float %689 to half
  %1018 = fptrunc float %690 to half
  %1019 = fptrunc float %691 to half
  %1020 = fptrunc float %692 to half
  %1021 = fptrunc float %693 to half
  %1022 = fptrunc float %694 to half
  %1023 = fptrunc float %695 to half
  %1024 = fptrunc float %696 to half
  %1025 = fptrunc float %697 to half
  %1026 = fptrunc float %698 to half
  %1027 = fptrunc float %699 to half
  %1028 = fptrunc float %700 to half
  %1029 = fptrunc float %701 to half
  %1030 = fptrunc float %702 to half
  %1031 = fptrunc float %703 to half
  %1032 = fptrunc float %704 to half
  %1033 = fptrunc float %705 to half
  %1034 = fptrunc float %706 to half
  %1035 = fptrunc float %707 to half
  %1036 = fptrunc float %708 to half
  %1037 = fptrunc float %709 to half
  %1038 = fptrunc float %710 to half
  %1039 = fptrunc float %711 to half
  %1040 = fptrunc float %712 to half
  %1041 = fptrunc float %713 to half
  %1042 = fptrunc float %714 to half
  %1043 = fptrunc float %715 to half
  %1044 = fptrunc float %716 to half
  %1045 = insertelement <8 x half> undef, half %1013, i64 0
  %1046 = insertelement <8 x half> %1045, half %1014, i64 1
  %1047 = insertelement <8 x half> %1046, half %1015, i64 2
  %1048 = insertelement <8 x half> %1047, half %1016, i64 3
  %1049 = insertelement <8 x half> %1048, half %1017, i64 4
  %1050 = insertelement <8 x half> %1049, half %1018, i64 5
  %1051 = insertelement <8 x half> %1050, half %1019, i64 6
  %1052 = insertelement <8 x half> %1051, half %1020, i64 7
  %1053 = bitcast <8 x half> %1052 to <8 x i16>
  %1054 = ptrtoint ptr addrspace(1) %70 to i64
  %1055 = and i64 %1054, -64
  %1056 = trunc i64 %1054 to i32
  %1057 = and i32 %1056, 63
  %1058 = lshr i32 %1057, 1
  %width.m168 = add nuw nsw i32 %1057, 127
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i16(i64 %1055, i32 %width.m168, i32 %height.m1, i32 %pitch.m1, i32 %1058, i32 %83, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i16> %1053)
  %1059 = insertelement <8 x half> undef, half %1021, i64 0
  %1060 = insertelement <8 x half> %1059, half %1022, i64 1
  %1061 = insertelement <8 x half> %1060, half %1023, i64 2
  %1062 = insertelement <8 x half> %1061, half %1024, i64 3
  %1063 = insertelement <8 x half> %1062, half %1025, i64 4
  %1064 = insertelement <8 x half> %1063, half %1026, i64 5
  %1065 = insertelement <8 x half> %1064, half %1027, i64 6
  %1066 = insertelement <8 x half> %1065, half %1028, i64 7
  %1067 = bitcast <8 x half> %1066 to <8 x i16>
  %1068 = add nuw nsw i32 %1058, 16
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i16(i64 %1055, i32 %width.m168, i32 %height.m1, i32 %pitch.m1, i32 %1068, i32 %83, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i16> %1067)
  %1069 = insertelement <8 x half> undef, half %1029, i64 0
  %1070 = insertelement <8 x half> %1069, half %1030, i64 1
  %1071 = insertelement <8 x half> %1070, half %1031, i64 2
  %1072 = insertelement <8 x half> %1071, half %1032, i64 3
  %1073 = insertelement <8 x half> %1072, half %1033, i64 4
  %1074 = insertelement <8 x half> %1073, half %1034, i64 5
  %1075 = insertelement <8 x half> %1074, half %1035, i64 6
  %1076 = insertelement <8 x half> %1075, half %1036, i64 7
  %1077 = bitcast <8 x half> %1076 to <8 x i16>
  %1078 = or i32 %1058, 32
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i16(i64 %1055, i32 %width.m168, i32 %height.m1, i32 %pitch.m1, i32 %1078, i32 %83, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i16> %1077)
  %1079 = insertelement <8 x half> undef, half %1037, i64 0
  %1080 = insertelement <8 x half> %1079, half %1038, i64 1
  %1081 = insertelement <8 x half> %1080, half %1039, i64 2
  %1082 = insertelement <8 x half> %1081, half %1040, i64 3
  %1083 = insertelement <8 x half> %1082, half %1041, i64 4
  %1084 = insertelement <8 x half> %1083, half %1042, i64 5
  %1085 = insertelement <8 x half> %1084, half %1043, i64 6
  %1086 = insertelement <8 x half> %1085, half %1044, i64 7
  %1087 = bitcast <8 x half> %1086 to <8 x i16>
  %1088 = add nuw nsw i32 %1058, 48
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i16(i64 %1055, i32 %width.m168, i32 %height.m1, i32 %pitch.m1, i32 %1088, i32 %83, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i16> %1087)
  %1089 = sext i32 %45 to i64
  %1090 = mul nsw i64 %1089, %53
  %1091 = mul nsw i64 %99, %53
  %1092 = mul nsw i64 %103, %53
  %1093 = mul nsw i64 %107, %53
  %1094 = mul nsw i64 %111, %53
  %1095 = mul nsw i64 %115, %53
  %1096 = mul nsw i64 %119, %53
  %1097 = mul nsw i64 %123, %53
  %1098 = icmp sgt i64 %28, %1089
  %1099 = icmp sgt i64 %28, %99
  %1100 = icmp sgt i64 %28, %103
  %1101 = icmp sgt i64 %28, %107
  %1102 = icmp sgt i64 %28, %111
  %1103 = icmp sgt i64 %28, %115
  %1104 = icmp sgt i64 %28, %119
  %1105 = icmp sgt i64 %28, %123
  %1106 = and i1 %96, %1098
  %1107 = and i1 %96, %1099
  %1108 = and i1 %96, %1100
  %1109 = and i1 %96, %1101
  %1110 = and i1 %96, %1102
  %1111 = and i1 %96, %1103
  %1112 = and i1 %96, %1104
  %1113 = and i1 %96, %1105
  %1114 = getelementptr [4 x i8], ptr addrspace(1) %76, i64 %1090
  %1115 = getelementptr [4 x i8], ptr addrspace(1) %76, i64 %1091
  %1116 = getelementptr [4 x i8], ptr addrspace(1) %76, i64 %1092
  %1117 = getelementptr [4 x i8], ptr addrspace(1) %76, i64 %1093
  %1118 = getelementptr [4 x i8], ptr addrspace(1) %76, i64 %1094
  %1119 = getelementptr [4 x i8], ptr addrspace(1) %76, i64 %1095
  %1120 = getelementptr [4 x i8], ptr addrspace(1) %76, i64 %1096
  %1121 = getelementptr [4 x i8], ptr addrspace(1) %76, i64 %1097
  %1122 = icmp eq i32 %42, 0
  %1123 = bitcast float %1005 to i32
  %1124 = and i1 %1122, %1106
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) %1114, i32 %1123, i64 4, i1 %1124)
  %1125 = bitcast float %1006 to i32
  %1126 = and i1 %1122, %1107
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) %1115, i32 %1125, i64 4, i1 %1126)
  %1127 = bitcast float %1007 to i32
  %1128 = and i1 %1122, %1108
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) %1116, i32 %1127, i64 4, i1 %1128)
  %1129 = bitcast float %1008 to i32
  %1130 = and i1 %1122, %1109
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) %1117, i32 %1129, i64 4, i1 %1130)
  %1131 = bitcast float %1009 to i32
  %1132 = and i1 %1122, %1110
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) %1118, i32 %1131, i64 4, i1 %1132)
  %1133 = bitcast float %1010 to i32
  %1134 = and i1 %1122, %1111
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) %1119, i32 %1133, i64 4, i1 %1134)
  %1135 = bitcast float %1011 to i32
  %1136 = and i1 %1122, %1112
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) %1120, i32 %1135, i64 4, i1 %1136)
  %1137 = bitcast float %1012 to i32
  %1138 = and i1 %1122, %1113
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) %1121, i32 %1137, i64 4, i1 %1138)
  br label %common.ret
}

declare void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)

; Function Attrs: nounwind memory(readwrite)
declare <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32) #1

; Function Attrs: nounwind memory(readwrite)
declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32) #1

; Function Attrs: nounwind memory(readwrite)
declare <32 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v32i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32) #1

; Function Attrs: nounwind memory(readwrite)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i16>) #1

declare <8 x float> @__builtin_IB_sub_group16_fdpas_f_f_hf_hf_8_8(<8 x float>, <8 x i16>, <8 x i32>)

; Function Attrs: nounwind memory(readwrite)
declare void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1), i32, i64, i1) #1

; Function Attrs: convergent mustprogress nofree nounwind willreturn memory(none)
declare spir_func float @__builtin_IB_fmax(float noundef, float noundef) local_unnamed_addr #2

; Function Attrs: convergent mustprogress nofree nounwind willreturn memory(none)
declare spir_func float @__builtin_IB_native_log2f(float noundef) local_unnamed_addr #2

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare float @llvm.fma.f32(float, float, float) #3

; Function Attrs: convergent mustprogress nofree nounwind willreturn memory(none)
declare spir_func float @__builtin_IB_native_exp2f(float noundef) local_unnamed_addr #2

; Function Attrs: convergent mustprogress nofree nounwind willreturn memory(none)
declare spir_func float @__builtin_IB_sub_group_reduce_FAdd_f32(float noundef) local_unnamed_addr #2

; Function Attrs: convergent mustprogress nofree nounwind willreturn memory(none)
declare spir_func float @__builtin_IB_sub_group_reduce_FMax_f32(float noundef) local_unnamed_addr #2

; Function Attrs: convergent mustprogress nofree nounwind willreturn memory(none)
declare spir_func i32 @__builtin_IB_get_group_id(i32 noundef) local_unnamed_addr #2

; Function Attrs: convergent mustprogress nofree nounwind willreturn memory(none)
declare spir_func i32 @__builtin_IB_get_local_id_x() local_unnamed_addr #2

; Function Attrs: convergent mustprogress nofree nounwind willreturn memory(none)
declare spir_func i32 @__builtin_IB_get_local_id_y() local_unnamed_addr #2

; Function Attrs: convergent mustprogress nofree nounwind willreturn memory(none)
declare spir_func i32 @__builtin_IB_get_local_id_z() local_unnamed_addr #2

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: readwrite)
declare void @llvm.assume(i1 noundef) #4

; Function Attrs: convergent mustprogress nofree nounwind willreturn memory(none)
declare spir_func i32 @__builtin_IB_get_local_thread_id() local_unnamed_addr #2

declare i32 @printf(ptr addrspace(2), ...)

; Function Attrs: convergent nounwind memory(inaccessiblemem: readwrite)
declare float @llvm.genx.GenISA.WaveAll.f32(float, i8, i1, i32) #5

; Function Attrs: convergent nounwind willreturn memory(none)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #6

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare float @llvm.maxnum.f32(float, float) #3

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare float @llvm.exp2.f32(float) #3

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare float @llvm.log2.f32(float) #3

; Function Attrs: nounwind willreturn memory(none)
declare void @llvm.genx.GenISA.CatchAllDebugLine() #7

attributes #0 = { convergent nounwind }
attributes #1 = { nounwind memory(readwrite) }
attributes #2 = { convergent mustprogress nofree nounwind willreturn memory(none) "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #3 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #4 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: readwrite) }
attributes #5 = { convergent nounwind memory(inaccessiblemem: readwrite) }
attributes #6 = { convergent nounwind willreturn memory(none) }
attributes #7 = { nounwind willreturn memory(none) }
attributes #8 = { nounwind }

!igc.functions = !{!0}
!IGCMetadata = !{!2}

!0 = !{ptr @fa_fwd_kernel, !1}
!1 = !{!7}
!2 = !{!"ModuleMD", !3}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", ptr @fa_fwd_kernel}
!5 = !{!"FuncMDValue[0]", !6}
!6 = !{!"requiredSubGroupSize", i32 16}
!7 = !{!"function_type", i32 0}
