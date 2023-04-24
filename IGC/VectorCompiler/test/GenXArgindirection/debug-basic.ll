;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXModule -GenXLiveRangesWrapper -GenXArgIndirectionWrapper -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXArgIndirection
; ------------------------------------------------
; This test checks that GenXArgIndirection pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"

; Function Attrs: nounwind
declare !genx_intrinsic_id !20 <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32>, i32, i32, i32, i16, i32) #0

; Function Attrs: nounwind
declare !genx_intrinsic_id !21 <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32>, <16 x i32>, i32, i32, i32, i16, i32, i1) #0

; Function Attrs: nounwind readonly
declare <16 x i32> @llvm.genx.gather.masked.scaled2.v16i32.v16i32.v16i1(i32, i16, i32, i32, <16 x i32>, <16 x i1>) #1

; CHECK: define internal spir_func <64 x i32> @IP_AlphaBlendRound{{.*}} !dbg [[SCOPE1:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL1_V:%[A-z0-9\.]*]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC:![0-9]*]]
; CHECK-DAG: [[VAL1_V]] = {{.*}}, !dbg [[VAL1_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL2_V:%[A-z0-9\.]*]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL2_V]] = {{.*}}, !dbg [[VAL2_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL3_V:%[A-z0-9\.]*]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: [[VAL3_V]] = {{.*}}, !dbg [[VAL3_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL4_V:%[A-z0-9\.]*]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL5_V:%[A-z0-9\.]*]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK-DAG: [[VAL5_V]] = {{.*}}, !dbg [[VAL5_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL6_V:%[A-z0-9\.]*]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC:![0-9]*]]
; CHECK-DAG: [[VAL6_V]] = {{.*}}, !dbg [[VAL6_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL7_V:%[A-z0-9\.]*]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC:![0-9]*]]
; CHECK-DAG: [[VAL7_V]] = {{.*}}, !dbg [[VAL7_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL8_V:%[A-z0-9\.]*]], metadata [[VAL8_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL8_LOC:![0-9]*]]
; CHECK-DAG: [[VAL8_V]] = {{.*}}, !dbg [[VAL8_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL9_V:%[A-z0-9\.]*]], metadata [[VAL9_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL9_LOC:![0-9]*]]
; CHECK-DAG: [[VAL9_V]] = {{.*}}, !dbg [[VAL9_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL10_V:%[A-z0-9\.]*]], metadata [[VAL10_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL10_LOC:![0-9]*]]
; CHECK-DAG: [[VAL10_V]] = {{.*}}, !dbg [[VAL10_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL11_V:%[A-z0-9\.]*]], metadata [[VAL11_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL11_LOC:![0-9]*]]
; CHECK-DAG: [[VAL11_V]] = {{.*}}, !dbg [[VAL11_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL12_V:%[A-z0-9\.]*]], metadata [[VAL12_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL12_LOC:![0-9]*]]
; CHECK-DAG: [[VAL12_V]] = {{.*}}, !dbg [[VAL12_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL13_V:%[A-z0-9\.]*]], metadata [[VAL13_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL13_LOC:![0-9]*]]
; CHECK-DAG: [[VAL13_V]] = {{.*}}, !dbg [[VAL13_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL14_V:%[A-z0-9\.]*]], metadata [[VAL14_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL14_LOC:![0-9]*]]
; CHECK-DAG: [[VAL14_V]] = {{.*}}, !dbg [[VAL14_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL15_V:%[A-z0-9\.]*]], metadata [[VAL15_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL15_LOC:![0-9]*]]
; CHECK-DAG: [[VAL15_V]] = {{.*}}, !dbg [[VAL15_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL16_V:%[A-z0-9\.]*]], metadata [[VAL16_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL16_LOC:![0-9]*]]
; CHECK-DAG: [[VAL16_V]] = {{.*}}, !dbg [[VAL16_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL17_V:%[A-z0-9\.]*]], metadata [[VAL17_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL17_LOC:![0-9]*]]
; CHECK-DAG: [[VAL17_V]] = {{.*}}, !dbg [[VAL17_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL18_V:%[A-z0-9\.]*]], metadata [[VAL18_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL18_LOC:![0-9]*]]
; CHECK-DAG: [[VAL18_V]] = {{.*}}, !dbg [[VAL18_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <64 x i32> [[VAL19_V:%[A-z0-9\.]*]], metadata [[VAL19_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL19_LOC:![0-9]*]]
; CHECK-DAG: [[VAL19_V]] = {{.*}}, !dbg [[VAL19_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL20_V:%[A-z0-9\.]*]], metadata [[VAL20_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL20_LOC:![0-9]*]]
; CHECK-DAG: [[VAL20_V]] = {{.*}}, !dbg [[VAL20_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL21_V:%[A-z0-9\.]*]], metadata [[VAL21_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL21_LOC:![0-9]*]]
; CHECK-DAG: [[VAL21_V]] = {{.*}}, !dbg [[VAL21_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <64 x i32> [[VAL22_V:%[A-z0-9\.]*]], metadata [[VAL22_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL22_LOC:![0-9]*]]
; CHECK-DAG: [[VAL22_V]] = {{.*}}, !dbg [[VAL22_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL23_V:%[A-z0-9\.]*]], metadata [[VAL23_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL23_LOC:![0-9]*]]
; CHECK-DAG: [[VAL23_V]] = {{.*}}, !dbg [[VAL23_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL24_V:%[A-z0-9\.]*]], metadata [[VAL24_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL24_LOC:![0-9]*]]
; CHECK-DAG: [[VAL24_V]] = {{.*}}, !dbg [[VAL24_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <64 x i32> [[VAL25_V:%[A-z0-9\.]*]], metadata [[VAL25_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL25_LOC:![0-9]*]]
; CHECK-DAG: [[VAL25_V]] = {{.*}}, !dbg [[VAL25_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL26_V:%[A-z0-9\.]*]], metadata [[VAL26_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL26_LOC:![0-9]*]]
; CHECK-DAG: [[VAL26_V]] = {{.*}}, !dbg [[VAL26_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL27_V:%[A-z0-9\.]*]], metadata [[VAL27_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL27_LOC:![0-9]*]]
; CHECK-DAG: [[VAL27_V]] = {{.*}}, !dbg [[VAL27_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <64 x i32> [[VAL28_V:%[A-z0-9\.]*]], metadata [[VAL28_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL28_LOC:![0-9]*]]
; CHECK-DAG: [[VAL28_V]] = {{.*}}, !dbg [[VAL28_LOC]]

; Function Attrs: noinline norecurse nounwind readnone
define internal spir_func <64 x i32> @IP_AlphaBlendRound(<64 x i32> %arg, <64 x i32> %arg67, <64 x i32> %arg68) unnamed_addr #2 !dbg !22 {
  %.split051 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %arg, i32 16, i32 16, i32 1, i16 0, i32 undef), !dbg !55
  call void @llvm.dbg.value(metadata <16 x i32> %.split051, metadata !25, metadata !DIExpression()), !dbg !55
  %.split052 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %arg67, i32 16, i32 16, i32 1, i16 0, i32 undef), !dbg !56
  call void @llvm.dbg.value(metadata <16 x i32> %.split052, metadata !27, metadata !DIExpression()), !dbg !56
  %.split1654 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %arg, i32 16, i32 16, i32 1, i16 64, i32 undef), !dbg !57
  call void @llvm.dbg.value(metadata <16 x i32> %.split1654, metadata !28, metadata !DIExpression()), !dbg !57
  %.split1655 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %arg67, i32 16, i32 16, i32 1, i16 64, i32 undef), !dbg !58
  call void @llvm.dbg.value(metadata <16 x i32> %.split1655, metadata !29, metadata !DIExpression()), !dbg !58
  %.split3257 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %arg, i32 16, i32 16, i32 1, i16 128, i32 undef), !dbg !59
  call void @llvm.dbg.value(metadata <16 x i32> %.split3257, metadata !30, metadata !DIExpression()), !dbg !59
  %.split3258 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %arg67, i32 16, i32 16, i32 1, i16 128, i32 undef), !dbg !60
  call void @llvm.dbg.value(metadata <16 x i32> %.split3258, metadata !31, metadata !DIExpression()), !dbg !60
  %.split4860 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %arg, i32 16, i32 16, i32 1, i16 192, i32 undef), !dbg !61
  call void @llvm.dbg.value(metadata <16 x i32> %.split4860, metadata !32, metadata !DIExpression()), !dbg !61
  %.split4861 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %arg67, i32 16, i32 16, i32 1, i16 192, i32 undef), !dbg !62
  call void @llvm.dbg.value(metadata <16 x i32> %.split4861, metadata !33, metadata !DIExpression()), !dbg !62
  %.split040 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %arg68, i32 16, i32 16, i32 1, i16 0, i32 undef), !dbg !63
  call void @llvm.dbg.value(metadata <16 x i32> %.split040, metadata !34, metadata !DIExpression()), !dbg !63
  %.split1643 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %arg68, i32 16, i32 16, i32 1, i16 64, i32 undef), !dbg !64
  call void @llvm.dbg.value(metadata <16 x i32> %.split1643, metadata !35, metadata !DIExpression()), !dbg !64
  %.split3246 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %arg68, i32 16, i32 16, i32 1, i16 128, i32 undef), !dbg !65
  call void @llvm.dbg.value(metadata <16 x i32> %.split3246, metadata !36, metadata !DIExpression()), !dbg !65
  %.split4849 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %arg68, i32 16, i32 16, i32 1, i16 192, i32 undef), !dbg !66
  call void @llvm.dbg.value(metadata <16 x i32> %.split4849, metadata !37, metadata !DIExpression()), !dbg !66
  %.split05266 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %arg67, i32 16, i32 16, i32 1, i16 0, i32 undef), !dbg !67
  call void @llvm.dbg.value(metadata <16 x i32> %.split05266, metadata !38, metadata !DIExpression()), !dbg !67
  %.split165565 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %arg67, i32 16, i32 16, i32 1, i16 64, i32 undef), !dbg !68
  call void @llvm.dbg.value(metadata <16 x i32> %.split165565, metadata !39, metadata !DIExpression()), !dbg !68
  %.split325864 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %arg67, i32 16, i32 16, i32 1, i16 128, i32 undef), !dbg !69
  call void @llvm.dbg.value(metadata <16 x i32> %.split325864, metadata !40, metadata !DIExpression()), !dbg !69
  %.split486163 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %arg67, i32 16, i32 16, i32 1, i16 192, i32 undef), !dbg !70
  call void @llvm.dbg.value(metadata <16 x i32> %.split486163, metadata !41, metadata !DIExpression()), !dbg !70
  %.in = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %arg68, i32 0, i32 16, i32 1, i16 0, i32 undef), !dbg !71
  call void @llvm.dbg.value(metadata <16 x i32> %.in, metadata !42, metadata !DIExpression()), !dbg !71
  %.in4 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %arg, i32 0, i32 16, i32 1, i16 0, i32 undef), !dbg !72
  call void @llvm.dbg.value(metadata <16 x i32> %.in4, metadata !43, metadata !DIExpression()), !dbg !72
  %.join = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %arg, <16 x i32> <i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8>, i32 16, i32 16, i32 1, i16 0, i32 undef, i1 true), !dbg !73
  call void @llvm.dbg.value(metadata <64 x i32> %.join, metadata !44, metadata !DIExpression()), !dbg !73
  %.in1 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %arg68, i32 0, i32 16, i32 1, i16 64, i32 undef), !dbg !74
  call void @llvm.dbg.value(metadata <16 x i32> %.in1, metadata !46, metadata !DIExpression()), !dbg !74
  %.in6 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %.join, i32 0, i32 16, i32 1, i16 64, i32 undef), !dbg !75
  call void @llvm.dbg.value(metadata <16 x i32> %.in6, metadata !47, metadata !DIExpression()), !dbg !75
  %.join12 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %.join, <16 x i32> <i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8>, i32 16, i32 16, i32 1, i16 64, i32 undef, i1 true), !dbg !76
  call void @llvm.dbg.value(metadata <64 x i32> %.join12, metadata !48, metadata !DIExpression()), !dbg !76
  %.in2 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %arg68, i32 0, i32 16, i32 1, i16 128, i32 undef), !dbg !77
  call void @llvm.dbg.value(metadata <16 x i32> %.in2, metadata !49, metadata !DIExpression()), !dbg !77
  %.in8 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %.join12, i32 0, i32 16, i32 1, i16 128, i32 undef), !dbg !78
  call void @llvm.dbg.value(metadata <16 x i32> %.in8, metadata !50, metadata !DIExpression()), !dbg !78
  %.join13 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %.join12, <16 x i32> <i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8>, i32 16, i32 16, i32 1, i16 128, i32 undef, i1 true), !dbg !79
  call void @llvm.dbg.value(metadata <64 x i32> %.join13, metadata !51, metadata !DIExpression()), !dbg !79
  %.in3 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %arg68, i32 0, i32 16, i32 1, i16 192, i32 undef), !dbg !80
  call void @llvm.dbg.value(metadata <16 x i32> %.in3, metadata !52, metadata !DIExpression()), !dbg !80
  %.in10 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %.join13, i32 0, i32 16, i32 1, i16 192, i32 undef), !dbg !81
  call void @llvm.dbg.value(metadata <16 x i32> %.in10, metadata !53, metadata !DIExpression()), !dbg !81
  %.join14 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %.join13, <16 x i32> %.in10, i32 16, i32 16, i32 1, i16 192, i32 undef, i1 true), !dbg !82
  call void @llvm.dbg.value(metadata <64 x i32> %.join14, metadata !54, metadata !DIExpression()), !dbg !82
  ret <64 x i32> %.join14, !dbg !83
}

; CHECK: dllexport spir_kernel void @Compute_2D_STD_UV{{.*}} !dbg [[SCOPE2:![0-9]*]]
; CHECK: [[CALL_V:%[A-z0-9.]*]] = {{.*}}call <64 x i32> @IP_AlphaBlendRound{{.*}}, !dbg [[CALL_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <64 x i32> [[CALL_V]], metadata [[CALL_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALL_LOC]]

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @Compute_2D_STD_UV() local_unnamed_addr #3 !dbg !84 {
  %.categoryconv1206 = call i32 @llvm.genx.convert.i32(i32 8), !dbg !144
  call void @llvm.dbg.value(metadata i32 %.categoryconv1206, metadata !86, metadata !DIExpression()), !dbg !144
  %scaled21205 = tail call <16 x i32> @llvm.genx.gather.masked.scaled2.v16i32.v16i32.v16i1(i32 2, i16 0, i32 %.categoryconv1206, i32 0, <16 x i32> <i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120>, <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>), !dbg !145
  call void @llvm.dbg.value(metadata <16 x i32> %scaled21205, metadata !88, metadata !DIExpression()), !dbg !145
  %wrregioni1123 = tail call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> undef, <16 x i32> %scaled21205, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true), !dbg !146
  call void @llvm.dbg.value(metadata <64 x i32> %wrregioni1123, metadata !89, metadata !DIExpression()), !dbg !146
  %.categoryconv1203 = call i32 @llvm.genx.convert.i32(i32 8), !dbg !147
  call void @llvm.dbg.value(metadata i32 %.categoryconv1203, metadata !90, metadata !DIExpression()), !dbg !147
  %scaled21202 = tail call <16 x i32> @llvm.genx.gather.masked.scaled2.v16i32.v16i32.v16i1(i32 2, i16 0, i32 %.categoryconv1203, i32 0, <16 x i32> <i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120>, <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>), !dbg !148
  call void @llvm.dbg.value(metadata <16 x i32> %scaled21202, metadata !91, metadata !DIExpression()), !dbg !148
  %wrregioni1125 = tail call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %wrregioni1123, <16 x i32> %scaled21202, i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true), !dbg !149
  call void @llvm.dbg.value(metadata <64 x i32> %wrregioni1125, metadata !92, metadata !DIExpression()), !dbg !149
  %.categoryconv1200 = call i32 @llvm.genx.convert.i32(i32 8), !dbg !150
  call void @llvm.dbg.value(metadata i32 %.categoryconv1200, metadata !93, metadata !DIExpression()), !dbg !150
  %scaled21199 = tail call <16 x i32> @llvm.genx.gather.masked.scaled2.v16i32.v16i32.v16i1(i32 2, i16 0, i32 %.categoryconv1200, i32 0, <16 x i32> <i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120>, <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>), !dbg !151
  call void @llvm.dbg.value(metadata <16 x i32> %scaled21199, metadata !94, metadata !DIExpression()), !dbg !151
  %wrregioni1127 = tail call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %wrregioni1125, <16 x i32> %scaled21199, i32 0, i32 16, i32 1, i16 128, i32 undef, i1 true), !dbg !152
  call void @llvm.dbg.value(metadata <64 x i32> %wrregioni1127, metadata !95, metadata !DIExpression()), !dbg !152
  %.categoryconv1197 = call i32 @llvm.genx.convert.i32(i32 8), !dbg !153
  call void @llvm.dbg.value(metadata i32 %.categoryconv1197, metadata !96, metadata !DIExpression()), !dbg !153
  %scaled21196 = tail call <16 x i32> @llvm.genx.gather.masked.scaled2.v16i32.v16i32.v16i1(i32 2, i16 0, i32 %.categoryconv1197, i32 0, <16 x i32> <i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120>, <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>), !dbg !154
  call void @llvm.dbg.value(metadata <16 x i32> %scaled21196, metadata !97, metadata !DIExpression()), !dbg !154
  %wrregioni1129 = tail call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %wrregioni1127, <16 x i32> %scaled21196, i32 0, i32 16, i32 1, i16 192, i32 undef, i1 true), !dbg !155
  call void @llvm.dbg.value(metadata <64 x i32> %wrregioni1129, metadata !98, metadata !DIExpression()), !dbg !155
  %.categoryconv1194 = call i32 @llvm.genx.convert.i32(i32 9), !dbg !156
  call void @llvm.dbg.value(metadata i32 %.categoryconv1194, metadata !99, metadata !DIExpression()), !dbg !156
  %scaled21193 = tail call <16 x i32> @llvm.genx.gather.masked.scaled2.v16i32.v16i32.v16i1(i32 2, i16 0, i32 %.categoryconv1194, i32 0, <16 x i32> <i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512>, <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>), !dbg !157
  call void @llvm.dbg.value(metadata <16 x i32> %scaled21193, metadata !100, metadata !DIExpression()), !dbg !157
  %wrregioni1131 = tail call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> undef, <16 x i32> %scaled21193, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true), !dbg !158
  call void @llvm.dbg.value(metadata <64 x i32> %wrregioni1131, metadata !101, metadata !DIExpression()), !dbg !158
  %.categoryconv1191 = call i32 @llvm.genx.convert.i32(i32 9), !dbg !159
  call void @llvm.dbg.value(metadata i32 %.categoryconv1191, metadata !102, metadata !DIExpression()), !dbg !159
  %scaled21190 = tail call <16 x i32> @llvm.genx.gather.masked.scaled2.v16i32.v16i32.v16i1(i32 2, i16 0, i32 %.categoryconv1191, i32 0, <16 x i32> <i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512>, <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>), !dbg !160
  call void @llvm.dbg.value(metadata <16 x i32> %scaled21190, metadata !103, metadata !DIExpression()), !dbg !160
  %wrregioni1133 = tail call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %wrregioni1131, <16 x i32> %scaled21190, i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true), !dbg !161
  call void @llvm.dbg.value(metadata <64 x i32> %wrregioni1133, metadata !104, metadata !DIExpression()), !dbg !161
  %.categoryconv1188 = call i32 @llvm.genx.convert.i32(i32 9), !dbg !162
  call void @llvm.dbg.value(metadata i32 %.categoryconv1188, metadata !105, metadata !DIExpression()), !dbg !162
  %scaled21187 = tail call <16 x i32> @llvm.genx.gather.masked.scaled2.v16i32.v16i32.v16i1(i32 2, i16 0, i32 %.categoryconv1188, i32 0, <16 x i32> <i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512>, <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>), !dbg !163
  call void @llvm.dbg.value(metadata <16 x i32> %scaled21187, metadata !106, metadata !DIExpression()), !dbg !163
  %wrregioni1135 = tail call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %wrregioni1133, <16 x i32> %scaled21187, i32 0, i32 16, i32 1, i16 128, i32 undef, i1 true), !dbg !164
  call void @llvm.dbg.value(metadata <64 x i32> %wrregioni1135, metadata !107, metadata !DIExpression()), !dbg !164
  %.categoryconv1185 = call i32 @llvm.genx.convert.i32(i32 9), !dbg !165
  call void @llvm.dbg.value(metadata i32 %.categoryconv1185, metadata !108, metadata !DIExpression()), !dbg !165
  %scaled21184 = tail call <16 x i32> @llvm.genx.gather.masked.scaled2.v16i32.v16i32.v16i1(i32 2, i16 0, i32 %.categoryconv1185, i32 0, <16 x i32> <i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512, i32 512>, <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>), !dbg !166
  call void @llvm.dbg.value(metadata <16 x i32> %scaled21184, metadata !109, metadata !DIExpression()), !dbg !166
  %wrregioni1137 = tail call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %wrregioni1135, <16 x i32> %scaled21184, i32 0, i32 16, i32 1, i16 192, i32 undef, i1 true), !dbg !167
  call void @llvm.dbg.value(metadata <64 x i32> %wrregioni1137, metadata !110, metadata !DIExpression()), !dbg !167
  %.categoryconv1182 = call i32 @llvm.genx.convert.i32(i32 10), !dbg !168
  call void @llvm.dbg.value(metadata i32 %.categoryconv1182, metadata !111, metadata !DIExpression()), !dbg !168
  %scaled21181 = tail call <16 x i32> @llvm.genx.gather.masked.scaled2.v16i32.v16i32.v16i1(i32 2, i16 0, i32 %.categoryconv1182, i32 0, <16 x i32> <i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120>, <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>), !dbg !169
  call void @llvm.dbg.value(metadata <16 x i32> %scaled21181, metadata !112, metadata !DIExpression()), !dbg !169
  %wrregioni1138 = tail call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> undef, <16 x i32> %scaled21181, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true), !dbg !170
  call void @llvm.dbg.value(metadata <64 x i32> %wrregioni1138, metadata !113, metadata !DIExpression()), !dbg !170
  %.categoryconv1179 = call i32 @llvm.genx.convert.i32(i32 10), !dbg !171
  call void @llvm.dbg.value(metadata i32 %.categoryconv1179, metadata !114, metadata !DIExpression()), !dbg !171
  %scaled21178 = tail call <16 x i32> @llvm.genx.gather.masked.scaled2.v16i32.v16i32.v16i1(i32 2, i16 0, i32 %.categoryconv1179, i32 0, <16 x i32> <i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120>, <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>), !dbg !172
  call void @llvm.dbg.value(metadata <16 x i32> %scaled21178, metadata !115, metadata !DIExpression()), !dbg !172
  %wrregioni1139 = tail call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %wrregioni1138, <16 x i32> %scaled21178, i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true), !dbg !173
  call void @llvm.dbg.value(metadata <64 x i32> %wrregioni1139, metadata !116, metadata !DIExpression()), !dbg !173
  %.categoryconv1176 = call i32 @llvm.genx.convert.i32(i32 10), !dbg !174
  call void @llvm.dbg.value(metadata i32 %.categoryconv1176, metadata !117, metadata !DIExpression()), !dbg !174
  %scaled21175 = tail call <16 x i32> @llvm.genx.gather.masked.scaled2.v16i32.v16i32.v16i1(i32 2, i16 0, i32 %.categoryconv1176, i32 0, <16 x i32> <i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120>, <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>), !dbg !175
  call void @llvm.dbg.value(metadata <16 x i32> %scaled21175, metadata !118, metadata !DIExpression()), !dbg !175
  %wrregioni1140 = tail call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %wrregioni1139, <16 x i32> %scaled21175, i32 0, i32 16, i32 1, i16 128, i32 undef, i1 true), !dbg !176
  call void @llvm.dbg.value(metadata <64 x i32> %wrregioni1140, metadata !119, metadata !DIExpression()), !dbg !176
  %.categoryconv1173 = call i32 @llvm.genx.convert.i32(i32 10), !dbg !177
  call void @llvm.dbg.value(metadata i32 %.categoryconv1173, metadata !120, metadata !DIExpression()), !dbg !177
  %scaled21172 = tail call <16 x i32> @llvm.genx.gather.masked.scaled2.v16i32.v16i32.v16i1(i32 2, i16 0, i32 %.categoryconv1173, i32 0, <16 x i32> <i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120, i32 120>, <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>), !dbg !178
  call void @llvm.dbg.value(metadata <16 x i32> %scaled21172, metadata !121, metadata !DIExpression()), !dbg !178
  %wrregioni1141 = tail call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %wrregioni1140, <16 x i32> %scaled21172, i32 0, i32 16, i32 1, i16 192, i32 undef, i1 true), !dbg !179
  call void @llvm.dbg.value(metadata <64 x i32> %wrregioni1141, metadata !122, metadata !DIExpression()), !dbg !179
  %.split0431.join0 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> undef, <16 x i32> <i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255>, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true), !dbg !180
  call void @llvm.dbg.value(metadata <64 x i32> %.split0431.join0, metadata !123, metadata !DIExpression()), !dbg !180
  %.split16433.join16 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %.split0431.join0, <16 x i32> <i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255>, i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true), !dbg !181
  call void @llvm.dbg.value(metadata <64 x i32> %.split16433.join16, metadata !124, metadata !DIExpression()), !dbg !181
  %.split32435.join32 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %.split16433.join16, <16 x i32> <i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255>, i32 0, i32 16, i32 1, i16 128, i32 undef, i1 true), !dbg !182
  call void @llvm.dbg.value(metadata <64 x i32> %.split32435.join32, metadata !125, metadata !DIExpression()), !dbg !182
  %.split48437.join48 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %.split32435.join32, <16 x i32> <i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255>, i32 0, i32 16, i32 1, i16 192, i32 undef, i1 true), !dbg !183
  call void @llvm.dbg.value(metadata <64 x i32> %.split48437.join48, metadata !126, metadata !DIExpression()), !dbg !183
  %callarg.precopy1236 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %.split48437.join48, i32 0, i32 16, i32 1, i16 0, i32 undef), !dbg !184
  call void @llvm.dbg.value(metadata <16 x i32> %callarg.precopy1236, metadata !127, metadata !DIExpression()), !dbg !184
  %callarg.precopy1237 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> undef, <16 x i32> %callarg.precopy1236, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true), !dbg !185
  call void @llvm.dbg.value(metadata <64 x i32> %callarg.precopy1237, metadata !128, metadata !DIExpression()), !dbg !185
  %callarg.precopy1238 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %.split48437.join48, i32 0, i32 16, i32 1, i16 64, i32 undef), !dbg !186
  call void @llvm.dbg.value(metadata <16 x i32> %callarg.precopy1238, metadata !129, metadata !DIExpression()), !dbg !186
  %callarg.precopy1239 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %callarg.precopy1237, <16 x i32> %callarg.precopy1238, i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true), !dbg !187
  call void @llvm.dbg.value(metadata <64 x i32> %callarg.precopy1239, metadata !130, metadata !DIExpression()), !dbg !187
  %callarg.precopy1240 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %.split48437.join48, i32 0, i32 16, i32 1, i16 128, i32 undef), !dbg !188
  call void @llvm.dbg.value(metadata <16 x i32> %callarg.precopy1240, metadata !131, metadata !DIExpression()), !dbg !188
  %callarg.precopy1241 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %callarg.precopy1239, <16 x i32> %callarg.precopy1240, i32 0, i32 16, i32 1, i16 128, i32 undef, i1 true), !dbg !189
  call void @llvm.dbg.value(metadata <64 x i32> %callarg.precopy1241, metadata !132, metadata !DIExpression()), !dbg !189
  %callarg.precopy1242 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %.split48437.join48, i32 0, i32 16, i32 1, i16 192, i32 undef), !dbg !190
  call void @llvm.dbg.value(metadata <16 x i32> %callarg.precopy1242, metadata !133, metadata !DIExpression()), !dbg !190
  %callarg.precopy1243 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %callarg.precopy1241, <16 x i32> %callarg.precopy1242, i32 0, i32 16, i32 1, i16 192, i32 undef, i1 true), !dbg !191
  call void @llvm.dbg.value(metadata <64 x i32> %callarg.precopy1243, metadata !134, metadata !DIExpression()), !dbg !191
  %call1154 = tail call spir_func <64 x i32> @IP_AlphaBlendRound(<64 x i32> %wrregioni1141, <64 x i32> %wrregioni1129, <64 x i32> %callarg.precopy1243) #5, !dbg !192
  call void @llvm.dbg.value(metadata <64 x i32> %call1154, metadata !135, metadata !DIExpression()), !dbg !192
  %retval.postcopy = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %call1154, i32 0, i32 16, i32 1, i16 0, i32 undef), !dbg !193
  call void @llvm.dbg.value(metadata <16 x i32> %retval.postcopy, metadata !136, metadata !DIExpression()), !dbg !193
  %retval.postcopy1244 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> undef, <16 x i32> %retval.postcopy, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true), !dbg !194
  call void @llvm.dbg.value(metadata <64 x i32> %retval.postcopy1244, metadata !137, metadata !DIExpression()), !dbg !194
  %retval.postcopy1245 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %call1154, i32 0, i32 16, i32 1, i16 64, i32 undef), !dbg !195
  call void @llvm.dbg.value(metadata <16 x i32> %retval.postcopy1245, metadata !138, metadata !DIExpression()), !dbg !195
  %retval.postcopy1246 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %retval.postcopy1244, <16 x i32> %retval.postcopy1245, i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true), !dbg !196
  call void @llvm.dbg.value(metadata <64 x i32> %retval.postcopy1246, metadata !139, metadata !DIExpression()), !dbg !196
  %retval.postcopy1247 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %call1154, i32 0, i32 16, i32 1, i16 128, i32 undef), !dbg !197
  call void @llvm.dbg.value(metadata <16 x i32> %retval.postcopy1247, metadata !140, metadata !DIExpression()), !dbg !197
  %retval.postcopy1248 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %retval.postcopy1246, <16 x i32> %retval.postcopy1247, i32 0, i32 16, i32 1, i16 128, i32 undef, i1 true), !dbg !198
  call void @llvm.dbg.value(metadata <64 x i32> %retval.postcopy1248, metadata !141, metadata !DIExpression()), !dbg !198
  %retval.postcopy1249 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %call1154, i32 0, i32 16, i32 1, i16 192, i32 undef), !dbg !199
  call void @llvm.dbg.value(metadata <16 x i32> %retval.postcopy1249, metadata !142, metadata !DIExpression()), !dbg !199
  %retval.postcopy1250 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %retval.postcopy1248, <16 x i32> %retval.postcopy1249, i32 0, i32 16, i32 1, i16 192, i32 undef, i1 true), !dbg !200
  call void @llvm.dbg.value(metadata <64 x i32> %retval.postcopy1250, metadata !143, metadata !DIExpression()), !dbg !200
  ret void, !dbg !201
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "basic.ll", directory: "/")
; CHECK-DAG: [[SCOPE1]] = distinct !DISubprogram(name: "IP_AlphaBlendRound", linkageName: "IP_AlphaBlendRound", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE1]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE1]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE1]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE1]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE1]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE1]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE1]], file: [[FILE]], line: 7
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL8_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE1]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL8_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL9_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE1]], file: [[FILE]], line: 9
; CHECK-DAG: [[VAL9_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL10_MD]] = !DILocalVariable(name: "10", scope: [[SCOPE1]], file: [[FILE]], line: 10
; CHECK-DAG: [[VAL10_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL11_MD]] = !DILocalVariable(name: "11", scope: [[SCOPE1]], file: [[FILE]], line: 11
; CHECK-DAG: [[VAL11_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL12_MD]] = !DILocalVariable(name: "12", scope: [[SCOPE1]], file: [[FILE]], line: 12
; CHECK-DAG: [[VAL12_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL13_MD]] = !DILocalVariable(name: "13", scope: [[SCOPE1]], file: [[FILE]], line: 13
; CHECK-DAG: [[VAL13_LOC]] = !DILocation(line: 13, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL14_MD]] = !DILocalVariable(name: "14", scope: [[SCOPE1]], file: [[FILE]], line: 14
; CHECK-DAG: [[VAL14_LOC]] = !DILocation(line: 14, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL15_MD]] = !DILocalVariable(name: "15", scope: [[SCOPE1]], file: [[FILE]], line: 15
; CHECK-DAG: [[VAL15_LOC]] = !DILocation(line: 15, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL16_MD]] = !DILocalVariable(name: "16", scope: [[SCOPE1]], file: [[FILE]], line: 16
; CHECK-DAG: [[VAL16_LOC]] = !DILocation(line: 16, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL17_MD]] = !DILocalVariable(name: "17", scope: [[SCOPE1]], file: [[FILE]], line: 17
; CHECK-DAG: [[VAL17_LOC]] = !DILocation(line: 17, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL18_MD]] = !DILocalVariable(name: "18", scope: [[SCOPE1]], file: [[FILE]], line: 18
; CHECK-DAG: [[VAL18_LOC]] = !DILocation(line: 18, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL19_MD]] = !DILocalVariable(name: "19", scope: [[SCOPE1]], file: [[FILE]], line: 19
; CHECK-DAG: [[VAL19_LOC]] = !DILocation(line: 19, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL20_MD]] = !DILocalVariable(name: "20", scope: [[SCOPE1]], file: [[FILE]], line: 20
; CHECK-DAG: [[VAL20_LOC]] = !DILocation(line: 20, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL21_MD]] = !DILocalVariable(name: "21", scope: [[SCOPE1]], file: [[FILE]], line: 21
; CHECK-DAG: [[VAL21_LOC]] = !DILocation(line: 21, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL22_MD]] = !DILocalVariable(name: "22", scope: [[SCOPE1]], file: [[FILE]], line: 22
; CHECK-DAG: [[VAL22_LOC]] = !DILocation(line: 22, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL23_MD]] = !DILocalVariable(name: "23", scope: [[SCOPE1]], file: [[FILE]], line: 23
; CHECK-DAG: [[VAL23_LOC]] = !DILocation(line: 23, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL24_MD]] = !DILocalVariable(name: "24", scope: [[SCOPE1]], file: [[FILE]], line: 24
; CHECK-DAG: [[VAL24_LOC]] = !DILocation(line: 24, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL25_MD]] = !DILocalVariable(name: "25", scope: [[SCOPE1]], file: [[FILE]], line: 25
; CHECK-DAG: [[VAL25_LOC]] = !DILocation(line: 25, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL26_MD]] = !DILocalVariable(name: "26", scope: [[SCOPE1]], file: [[FILE]], line: 26
; CHECK-DAG: [[VAL26_LOC]] = !DILocation(line: 26, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL27_MD]] = !DILocalVariable(name: "27", scope: [[SCOPE1]], file: [[FILE]], line: 27
; CHECK-DAG: [[VAL27_LOC]] = !DILocation(line: 27, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL28_MD]] = !DILocalVariable(name: "28", scope: [[SCOPE1]], file: [[FILE]], line: 28
; CHECK-DAG: [[VAL28_LOC]] = !DILocation(line: 28, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[SCOPE2]] = distinct !DISubprogram(name: "Compute_2D_STD_UV", linkageName: "Compute_2D_STD_UV", scope: null, file: [[FILE]], line: 30
; CHECK-DAG: [[CALL_MD]] = !DILocalVariable(name: "77", scope: [[SCOPE2]], file: [[FILE]], line: 78
; CHECK-DAG: [[CALL_LOC]] = !DILocation(line: 78, column: 1, scope: [[SCOPE2]])

; Function Attrs: nounwind
declare !genx_intrinsic_id !21 i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64, i64, i32, i32, i32, i16, i32, i1) #0

; Function Attrs: nounwind
declare !genx_intrinsic_id !21 <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.i16.v16i1(<16 x i32>, <16 x i32>, i32, i32, i32, i16, i32, <16 x i1>) #0

; Function Attrs: nounwind
declare !genx_intrinsic_id !202 i32 @llvm.genx.convert.i32(i32) #0

; Function Attrs: nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #4

attributes #0 = { nounwind }
attributes #1 = { nounwind readonly }
attributes #2 = { noinline norecurse nounwind readnone }
attributes #3 = { noinline nounwind "CMGenxMain" "VC.Stack.Amount"="0" "oclrt"="1" }
attributes #4 = { nounwind readnone speculatable willreturn }
attributes #5 = { noinline nounwind }

!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!0}
!opencl.spir.version = !{!1, !2, !2}
!opencl.ocl.version = !{!0, !2, !2}
!opencl.used.extensions = !{!3}
!opencl.used.optional.core.features = !{!3}
!spirv.Generator = !{!4}
!genx.kernels = !{!5}
!genx.kernel.internal = !{!10}
!llvm.ident = !{!13, !13}
!llvm.module.flags = !{!14, !15}
!llvm.dbg.cu = !{!16}
!llvm.debugify = !{!18, !19}

!0 = !{i32 0, i32 0}
!1 = !{i32 1, i32 2}
!2 = !{i32 2, i32 0}
!3 = !{}
!4 = !{i16 6, i16 14}
!5 = !{void ()* @Compute_2D_STD_UV, !"Compute_2D_STD_UV", !6, i32 0, !7, !8, !9, i32 0}
!6 = !{i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 96}
!7 = !{i32 72, i32 80, i32 88, i32 96, i32 104, i32 112, i32 120, i32 128, i32 136, i32 144, i32 152, i32 160, i32 64}
!8 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!9 = !{!"buffer_t read_write", !"buffer_t read_write", !"buffer_t read_write", !"buffer_t read_write", !"buffer_t read_write", !"buffer_t read_write", !"buffer_t read_write", !"buffer_t read_write", !"buffer_t read_write", !"buffer_t read_write", !"buffer_t read_write", !"buffer_t read_write"}
!10 = !{void ()* @Compute_2D_STD_UV, !11, !12, !3, !12}
!11 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!12 = !{i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12}
!13 = !{!"Ubuntu clang version 10.0.1"}
!14 = !{i32 1, !"wchar_size", i32 4}
!15 = !{i32 2, !"Debug Info Version", i32 3}
!16 = distinct !DICompileUnit(language: DW_LANG_C, file: !17, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !3)
!17 = !DIFile(filename: "basic.ll", directory: "/")
!18 = !{i32 87}
!19 = !{i32 85}
!20 = !{i32 7747}
!21 = !{i32 7952}
!22 = distinct !DISubprogram(name: "IP_AlphaBlendRound", linkageName: "IP_AlphaBlendRound", scope: null, file: !17, line: 1, type: !23, scopeLine: 1, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition | DISPFlagOptimized, unit: !16, retainedNodes: !24)
!23 = !DISubroutineType(types: !3)
!24 = !{!25, !27, !28, !29, !30, !31, !32, !33, !34, !35, !36, !37, !38, !39, !40, !41, !42, !43, !44, !46, !47, !48, !49, !50, !51, !52, !53, !54}
!25 = !DILocalVariable(name: "1", scope: !22, file: !17, line: 1, type: !26)
!26 = !DIBasicType(name: "ty512", size: 512, encoding: DW_ATE_unsigned)
!27 = !DILocalVariable(name: "2", scope: !22, file: !17, line: 2, type: !26)
!28 = !DILocalVariable(name: "3", scope: !22, file: !17, line: 3, type: !26)
!29 = !DILocalVariable(name: "4", scope: !22, file: !17, line: 4, type: !26)
!30 = !DILocalVariable(name: "5", scope: !22, file: !17, line: 5, type: !26)
!31 = !DILocalVariable(name: "6", scope: !22, file: !17, line: 6, type: !26)
!32 = !DILocalVariable(name: "7", scope: !22, file: !17, line: 7, type: !26)
!33 = !DILocalVariable(name: "8", scope: !22, file: !17, line: 8, type: !26)
!34 = !DILocalVariable(name: "9", scope: !22, file: !17, line: 9, type: !26)
!35 = !DILocalVariable(name: "10", scope: !22, file: !17, line: 10, type: !26)
!36 = !DILocalVariable(name: "11", scope: !22, file: !17, line: 11, type: !26)
!37 = !DILocalVariable(name: "12", scope: !22, file: !17, line: 12, type: !26)
!38 = !DILocalVariable(name: "13", scope: !22, file: !17, line: 13, type: !26)
!39 = !DILocalVariable(name: "14", scope: !22, file: !17, line: 14, type: !26)
!40 = !DILocalVariable(name: "15", scope: !22, file: !17, line: 15, type: !26)
!41 = !DILocalVariable(name: "16", scope: !22, file: !17, line: 16, type: !26)
!42 = !DILocalVariable(name: "17", scope: !22, file: !17, line: 17, type: !26)
!43 = !DILocalVariable(name: "18", scope: !22, file: !17, line: 18, type: !26)
!44 = !DILocalVariable(name: "19", scope: !22, file: !17, line: 19, type: !45)
!45 = !DIBasicType(name: "ty2048", size: 2048, encoding: DW_ATE_unsigned)
!46 = !DILocalVariable(name: "20", scope: !22, file: !17, line: 20, type: !26)
!47 = !DILocalVariable(name: "21", scope: !22, file: !17, line: 21, type: !26)
!48 = !DILocalVariable(name: "22", scope: !22, file: !17, line: 22, type: !45)
!49 = !DILocalVariable(name: "23", scope: !22, file: !17, line: 23, type: !26)
!50 = !DILocalVariable(name: "24", scope: !22, file: !17, line: 24, type: !26)
!51 = !DILocalVariable(name: "25", scope: !22, file: !17, line: 25, type: !45)
!52 = !DILocalVariable(name: "26", scope: !22, file: !17, line: 26, type: !26)
!53 = !DILocalVariable(name: "27", scope: !22, file: !17, line: 27, type: !26)
!54 = !DILocalVariable(name: "28", scope: !22, file: !17, line: 28, type: !45)
!55 = !DILocation(line: 1, column: 1, scope: !22)
!56 = !DILocation(line: 2, column: 1, scope: !22)
!57 = !DILocation(line: 3, column: 1, scope: !22)
!58 = !DILocation(line: 4, column: 1, scope: !22)
!59 = !DILocation(line: 5, column: 1, scope: !22)
!60 = !DILocation(line: 6, column: 1, scope: !22)
!61 = !DILocation(line: 7, column: 1, scope: !22)
!62 = !DILocation(line: 8, column: 1, scope: !22)
!63 = !DILocation(line: 9, column: 1, scope: !22)
!64 = !DILocation(line: 10, column: 1, scope: !22)
!65 = !DILocation(line: 11, column: 1, scope: !22)
!66 = !DILocation(line: 12, column: 1, scope: !22)
!67 = !DILocation(line: 13, column: 1, scope: !22)
!68 = !DILocation(line: 14, column: 1, scope: !22)
!69 = !DILocation(line: 15, column: 1, scope: !22)
!70 = !DILocation(line: 16, column: 1, scope: !22)
!71 = !DILocation(line: 17, column: 1, scope: !22)
!72 = !DILocation(line: 18, column: 1, scope: !22)
!73 = !DILocation(line: 19, column: 1, scope: !22)
!74 = !DILocation(line: 20, column: 1, scope: !22)
!75 = !DILocation(line: 21, column: 1, scope: !22)
!76 = !DILocation(line: 22, column: 1, scope: !22)
!77 = !DILocation(line: 23, column: 1, scope: !22)
!78 = !DILocation(line: 24, column: 1, scope: !22)
!79 = !DILocation(line: 25, column: 1, scope: !22)
!80 = !DILocation(line: 26, column: 1, scope: !22)
!81 = !DILocation(line: 27, column: 1, scope: !22)
!82 = !DILocation(line: 28, column: 1, scope: !22)
!83 = !DILocation(line: 29, column: 1, scope: !22)
!84 = distinct !DISubprogram(name: "Compute_2D_STD_UV", linkageName: "Compute_2D_STD_UV", scope: null, file: !17, line: 30, type: !23, scopeLine: 30, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !16, retainedNodes: !85)
!85 = !{!86, !88, !89, !90, !91, !92, !93, !94, !95, !96, !97, !98, !99, !100, !101, !102, !103, !104, !105, !106, !107, !108, !109, !110, !111, !112, !113, !114, !115, !116, !117, !118, !119, !120, !121, !122, !123, !124, !125, !126, !127, !128, !129, !130, !131, !132, !133, !134, !135, !136, !137, !138, !139, !140, !141, !142, !143}
!86 = !DILocalVariable(name: "29", scope: !84, file: !17, line: 30, type: !87)
!87 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!88 = !DILocalVariable(name: "30", scope: !84, file: !17, line: 31, type: !26)
!89 = !DILocalVariable(name: "31", scope: !84, file: !17, line: 32, type: !45)
!90 = !DILocalVariable(name: "32", scope: !84, file: !17, line: 33, type: !87)
!91 = !DILocalVariable(name: "33", scope: !84, file: !17, line: 34, type: !26)
!92 = !DILocalVariable(name: "34", scope: !84, file: !17, line: 35, type: !45)
!93 = !DILocalVariable(name: "35", scope: !84, file: !17, line: 36, type: !87)
!94 = !DILocalVariable(name: "36", scope: !84, file: !17, line: 37, type: !26)
!95 = !DILocalVariable(name: "37", scope: !84, file: !17, line: 38, type: !45)
!96 = !DILocalVariable(name: "38", scope: !84, file: !17, line: 39, type: !87)
!97 = !DILocalVariable(name: "39", scope: !84, file: !17, line: 40, type: !26)
!98 = !DILocalVariable(name: "40", scope: !84, file: !17, line: 41, type: !45)
!99 = !DILocalVariable(name: "41", scope: !84, file: !17, line: 42, type: !87)
!100 = !DILocalVariable(name: "42", scope: !84, file: !17, line: 43, type: !26)
!101 = !DILocalVariable(name: "43", scope: !84, file: !17, line: 44, type: !45)
!102 = !DILocalVariable(name: "44", scope: !84, file: !17, line: 45, type: !87)
!103 = !DILocalVariable(name: "45", scope: !84, file: !17, line: 46, type: !26)
!104 = !DILocalVariable(name: "46", scope: !84, file: !17, line: 47, type: !45)
!105 = !DILocalVariable(name: "47", scope: !84, file: !17, line: 48, type: !87)
!106 = !DILocalVariable(name: "48", scope: !84, file: !17, line: 49, type: !26)
!107 = !DILocalVariable(name: "49", scope: !84, file: !17, line: 50, type: !45)
!108 = !DILocalVariable(name: "50", scope: !84, file: !17, line: 51, type: !87)
!109 = !DILocalVariable(name: "51", scope: !84, file: !17, line: 52, type: !26)
!110 = !DILocalVariable(name: "52", scope: !84, file: !17, line: 53, type: !45)
!111 = !DILocalVariable(name: "53", scope: !84, file: !17, line: 54, type: !87)
!112 = !DILocalVariable(name: "54", scope: !84, file: !17, line: 55, type: !26)
!113 = !DILocalVariable(name: "55", scope: !84, file: !17, line: 56, type: !45)
!114 = !DILocalVariable(name: "56", scope: !84, file: !17, line: 57, type: !87)
!115 = !DILocalVariable(name: "57", scope: !84, file: !17, line: 58, type: !26)
!116 = !DILocalVariable(name: "58", scope: !84, file: !17, line: 59, type: !45)
!117 = !DILocalVariable(name: "59", scope: !84, file: !17, line: 60, type: !87)
!118 = !DILocalVariable(name: "60", scope: !84, file: !17, line: 61, type: !26)
!119 = !DILocalVariable(name: "61", scope: !84, file: !17, line: 62, type: !45)
!120 = !DILocalVariable(name: "62", scope: !84, file: !17, line: 63, type: !87)
!121 = !DILocalVariable(name: "63", scope: !84, file: !17, line: 64, type: !26)
!122 = !DILocalVariable(name: "64", scope: !84, file: !17, line: 65, type: !45)
!123 = !DILocalVariable(name: "65", scope: !84, file: !17, line: 66, type: !45)
!124 = !DILocalVariable(name: "66", scope: !84, file: !17, line: 67, type: !45)
!125 = !DILocalVariable(name: "67", scope: !84, file: !17, line: 68, type: !45)
!126 = !DILocalVariable(name: "68", scope: !84, file: !17, line: 69, type: !45)
!127 = !DILocalVariable(name: "69", scope: !84, file: !17, line: 70, type: !26)
!128 = !DILocalVariable(name: "70", scope: !84, file: !17, line: 71, type: !45)
!129 = !DILocalVariable(name: "71", scope: !84, file: !17, line: 72, type: !26)
!130 = !DILocalVariable(name: "72", scope: !84, file: !17, line: 73, type: !45)
!131 = !DILocalVariable(name: "73", scope: !84, file: !17, line: 74, type: !26)
!132 = !DILocalVariable(name: "74", scope: !84, file: !17, line: 75, type: !45)
!133 = !DILocalVariable(name: "75", scope: !84, file: !17, line: 76, type: !26)
!134 = !DILocalVariable(name: "76", scope: !84, file: !17, line: 77, type: !45)
!135 = !DILocalVariable(name: "77", scope: !84, file: !17, line: 78, type: !45)
!136 = !DILocalVariable(name: "78", scope: !84, file: !17, line: 79, type: !26)
!137 = !DILocalVariable(name: "79", scope: !84, file: !17, line: 80, type: !45)
!138 = !DILocalVariable(name: "80", scope: !84, file: !17, line: 81, type: !26)
!139 = !DILocalVariable(name: "81", scope: !84, file: !17, line: 82, type: !45)
!140 = !DILocalVariable(name: "82", scope: !84, file: !17, line: 83, type: !26)
!141 = !DILocalVariable(name: "83", scope: !84, file: !17, line: 84, type: !45)
!142 = !DILocalVariable(name: "84", scope: !84, file: !17, line: 85, type: !26)
!143 = !DILocalVariable(name: "85", scope: !84, file: !17, line: 86, type: !45)
!144 = !DILocation(line: 30, column: 1, scope: !84)
!145 = !DILocation(line: 31, column: 1, scope: !84)
!146 = !DILocation(line: 32, column: 1, scope: !84)
!147 = !DILocation(line: 33, column: 1, scope: !84)
!148 = !DILocation(line: 34, column: 1, scope: !84)
!149 = !DILocation(line: 35, column: 1, scope: !84)
!150 = !DILocation(line: 36, column: 1, scope: !84)
!151 = !DILocation(line: 37, column: 1, scope: !84)
!152 = !DILocation(line: 38, column: 1, scope: !84)
!153 = !DILocation(line: 39, column: 1, scope: !84)
!154 = !DILocation(line: 40, column: 1, scope: !84)
!155 = !DILocation(line: 41, column: 1, scope: !84)
!156 = !DILocation(line: 42, column: 1, scope: !84)
!157 = !DILocation(line: 43, column: 1, scope: !84)
!158 = !DILocation(line: 44, column: 1, scope: !84)
!159 = !DILocation(line: 45, column: 1, scope: !84)
!160 = !DILocation(line: 46, column: 1, scope: !84)
!161 = !DILocation(line: 47, column: 1, scope: !84)
!162 = !DILocation(line: 48, column: 1, scope: !84)
!163 = !DILocation(line: 49, column: 1, scope: !84)
!164 = !DILocation(line: 50, column: 1, scope: !84)
!165 = !DILocation(line: 51, column: 1, scope: !84)
!166 = !DILocation(line: 52, column: 1, scope: !84)
!167 = !DILocation(line: 53, column: 1, scope: !84)
!168 = !DILocation(line: 54, column: 1, scope: !84)
!169 = !DILocation(line: 55, column: 1, scope: !84)
!170 = !DILocation(line: 56, column: 1, scope: !84)
!171 = !DILocation(line: 57, column: 1, scope: !84)
!172 = !DILocation(line: 58, column: 1, scope: !84)
!173 = !DILocation(line: 59, column: 1, scope: !84)
!174 = !DILocation(line: 60, column: 1, scope: !84)
!175 = !DILocation(line: 61, column: 1, scope: !84)
!176 = !DILocation(line: 62, column: 1, scope: !84)
!177 = !DILocation(line: 63, column: 1, scope: !84)
!178 = !DILocation(line: 64, column: 1, scope: !84)
!179 = !DILocation(line: 65, column: 1, scope: !84)
!180 = !DILocation(line: 66, column: 1, scope: !84)
!181 = !DILocation(line: 67, column: 1, scope: !84)
!182 = !DILocation(line: 68, column: 1, scope: !84)
!183 = !DILocation(line: 69, column: 1, scope: !84)
!184 = !DILocation(line: 70, column: 1, scope: !84)
!185 = !DILocation(line: 71, column: 1, scope: !84)
!186 = !DILocation(line: 72, column: 1, scope: !84)
!187 = !DILocation(line: 73, column: 1, scope: !84)
!188 = !DILocation(line: 74, column: 1, scope: !84)
!189 = !DILocation(line: 75, column: 1, scope: !84)
!190 = !DILocation(line: 76, column: 1, scope: !84)
!191 = !DILocation(line: 77, column: 1, scope: !84)
!192 = !DILocation(line: 78, column: 1, scope: !84)
!193 = !DILocation(line: 79, column: 1, scope: !84)
!194 = !DILocation(line: 80, column: 1, scope: !84)
!195 = !DILocation(line: 81, column: 1, scope: !84)
!196 = !DILocation(line: 82, column: 1, scope: !84)
!197 = !DILocation(line: 83, column: 1, scope: !84)
!198 = !DILocation(line: 84, column: 1, scope: !84)
!199 = !DILocation(line: 85, column: 1, scope: !84)
!200 = !DILocation(line: 86, column: 1, scope: !84)
!201 = !DILocation(line: 87, column: 1, scope: !84)
!202 = !{i32 7572}
