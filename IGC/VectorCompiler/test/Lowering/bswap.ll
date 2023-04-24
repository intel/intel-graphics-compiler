;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen11 -mtriple=spir64-unknown-unknown -S < %s | FileCheck --check-prefix CHECK-ROTATE %s
; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck  --check-prefix CHECK-ROTATE64 %s

declare i16 @llvm.bswap.i16(i16)
declare i64 @llvm.bswap.i64(i64)
declare <3 x i32> @llvm.bswap.v3i32(<3 x i32>)

define spir_func i16 @scalar_16(i16 %arg) {
; CHECK-LABEL: define spir_func i16 @scalar_16
; CHECK-SAME: (i16 %[[ARG_I16:[^ )]+]])
; CHECK-NOT: call i16 @llvm.bswap.16
; CHECK: %[[CAST_V1I16:[^ )]+]] = bitcast i16 %[[ARG_I16]] to <1 x i16>
; CHECK: %[[CAST_V2I8:[^ )]+]] = bitcast <1 x i16> %[[CAST_V1I16]] to <2 x i8>
; CHECK: %[[READ_V1I8LO:[^ )]+]] = call <1 x i8> @llvm.genx.rdregioni.v1i8.v2i8.i16(<2 x i8> %[[CAST_V2I8]], i32 2, i32 1, i32 0, i16 0, i32 undef)
; CHECK: %[[READ_V1I8HI:[^ )]+]] = call <1 x i8> @llvm.genx.rdregioni.v1i8.v2i8.i16(<2 x i8> %[[CAST_V2I8]], i32 2, i32 1, i32 0, i16 1, i32 undef)
; CHECK: %[[WRITE_V1I8HI:[^ )]+]] = call <2 x i8> @llvm.genx.wrregioni.v2i8.v1i8.i16.i1(<2 x i8> undef, <1 x i8> %[[READ_V1I8LO]], i32 2, i32 1, i32 0, i16 1, i32 undef, i1 true)
; CHECK: %[[WRITE_V1I8LO:[^ )]+]] = call <2 x i8> @llvm.genx.wrregioni.v2i8.v1i8.i16.i1(<2 x i8> %[[WRITE_V1I8HI]], <1 x i8> %[[READ_V1I8HI]], i32 2, i32 1, i32 0, i16 0, i32 undef, i1 true)
; CHECK: %[[CAST_I16:[^ )]+]] = bitcast <2 x i8> %[[WRITE_V1I8LO]] to i16
; CHECK: ret i16 %[[CAST_I16]]
; CHECK-ROTATE-LABEL: define spir_func i16 @scalar_16
; CHECK-ROTATE-SAME: (i16 %[[ARG_I16:[^ )]+]])
; CHECK-ROTATE-NOT: call i16 @llvm.bswap.16
; CHECK-ROTATE: %[[CAST_V1I16:[^ )]+]] = bitcast i16 %[[ARG_I16]] to <1 x i16>
; CHECK-ROTATE: %[[ROL_V1I16:[^ )]+]] = call <1 x i16> @llvm.genx.rol.v1i16.v1i16(<1 x i16> %[[CAST_V1I16]], <1 x i16> <i16 8>)
; CHECK-ROTATE: %[[CAST_I16:[^ )]+]] = bitcast <1 x i16> %[[ROL_V1I16]] to i16
; CHECK-ROTATE: ret i16 %[[CAST_I16]]
; CHECK-ROTATE64-LABEL: define spir_func i16 @scalar_16
; CHECK-ROTATE64-SAME: (i16 %[[ARG_I16:[^ )]+]])
; CHECK-ROTATE64-NOT: call i16 @llvm.bswap.16
; CHECK-ROTATE64: %[[CAST_V1I16:[^ )]+]] = bitcast i16 %[[ARG_I16]] to <1 x i16>
; CHECK-ROTATE64: %[[ROL_V1I16:[^ )]+]] = call <1 x i16> @llvm.genx.rol.v1i16.v1i16(<1 x i16> %[[CAST_V1I16]], <1 x i16> <i16 8>)
; CHECK-ROTATE64: %[[CAST_I16:[^ )]+]] = bitcast <1 x i16> %[[ROL_V1I16]] to i16
; CHECK-ROTATE64: ret i16 %[[CAST_I16]]
  %res = tail call i16 @llvm.bswap.i16(i16 %arg)
  ret i16 %res
}

define spir_func i64 @scalar_64(i64 %arg) {
; CHECK-LABEL: define spir_func i64 @scalar_64
; CHECK-SAME: (i64 %[[ARG_I64:[^ )]+]])
; CHECK-NOT: call i64 @llvm.bswap.i64
; CHECK: %[[VCAST_V4I16:[^ )]+]] = bitcast i64 %[[ARG_I64]] to <4 x i16>
; CHECK: %[[CAST_V8I8:[^ )]+]] = bitcast <4 x i16> %[[VCAST_V4I16]] to <8 x i8>
; CHECK: %[[READ_V4I8LO:[^ )]+]] = call <4 x i8> @llvm.genx.rdregioni.v4i8.v8i8.i16(<8 x i8> %[[CAST_V8I8]], i32 2, i32 1, i32 0, i16 0, i32 undef)
; CHECK: %[[READ_V4I8HI:[^ )]+]] = call <4 x i8> @llvm.genx.rdregioni.v4i8.v8i8.i16(<8 x i8> %[[CAST_V8I8]], i32 2, i32 1, i32 0, i16 1, i32 undef)
; CHECK: %[[WRITE_V4I8HI:[^ )]+]] = call <8 x i8> @llvm.genx.wrregioni.v8i8.v4i8.i16.i1(<8 x i8> undef, <4 x i8> %[[READ_V4I8LO]], i32 2, i32 1, i32 0, i16 1, i32 undef, i1 true)
; CHECK: %[[WRITE_V4I8LO:[^ )]+]] = call <8 x i8> @llvm.genx.wrregioni.v8i8.v4i8.i16.i1(<8 x i8> %[[WRITE_V4I8HI]], <4 x i8> %[[READ_V4I8HI]], i32 2, i32 1, i32 0, i16 0, i32 undef, i1 true)
; CHECK: %[[CAST_V2I32:[^ )]+]] = bitcast <8 x i8> %[[WRITE_V4I8LO]] to <2 x i32>
; CHECK: %[[CAST_V4I16:[^ )]+]] = bitcast <2 x i32> %[[CAST_V2I32]] to <4 x i16>
; CHECK: %[[READ_V2I16LO:[^ )]+]] = call <2 x i16> @llvm.genx.rdregioni.v2i16.v4i16.i16(<4 x i16> %[[CAST_V4I16]], i32 2, i32 1, i32 0, i16 0, i32 undef)
; CHECK: %[[READ_V2I16HI:[^ )]+]] = call <2 x i16> @llvm.genx.rdregioni.v2i16.v4i16.i16(<4 x i16> %[[CAST_V4I16]], i32 2, i32 1, i32 0, i16 2, i32 undef)
; CHECK: %[[WRITE_V2I16HI:[^ )]+]] = call <4 x i16> @llvm.genx.wrregioni.v4i16.v2i16.i16.i1(<4 x i16> undef, <2 x i16> %[[READ_V2I16LO]], i32 2, i32 1, i32 0, i16 2, i32 undef, i1 true)
; CHECK: %[[WRITE_V2I16LO:[^ )]+]] = call <4 x i16> @llvm.genx.wrregioni.v4i16.v2i16.i16.i1(<4 x i16> %[[WRITE_V2I16HI]], <2 x i16> %[[READ_V2I16HI]], i32 2, i32 1, i32 0, i16 0, i32 undef, i1 true)
; CHECK: %[[CAST_V1I64:[^ )]+]] = bitcast <4 x i16> %[[WRITE_V2I16LO]] to <1 x i64>
; CHECK: %[[CAST_V2I32:[^ )]+]] = bitcast <1 x i64> %[[CAST_V1I64]] to <2 x i32>
; CHECK: %[[READ_V1I32LO:[^ )]+]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> %[[CAST_V2I32]], i32 2, i32 1, i32 0, i16 0, i32 undef)
; CHECK: %[[READ_V1I32HI:[^ )]+]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> %[[CAST_V2I32]], i32 2, i32 1, i32 0, i16 4, i32 undef)
; CHECK: %[[WRITE_V1I32HI:[^ )]+]] = call <2 x i32> @llvm.genx.wrregioni.v2i32.v1i32.i16.i1(<2 x i32> undef, <1 x i32> %[[READ_V1I32LO]], i32 2, i32 1, i32 0, i16 4, i32 undef, i1 true)
; CHECK: %[[WRITE_V1I32LO:[^ )]+]] = call <2 x i32> @llvm.genx.wrregioni.v2i32.v1i32.i16.i1(<2 x i32> %[[WRITE_V1I32HI]], <1 x i32> %[[READ_V1I32HI]], i32 2, i32 1, i32 0, i16 0, i32 undef, i1 true)
; CHECK: %[[CAST_I64:[^ )]+]] = bitcast <2 x i32> %[[WRITE_V1I32LO]] to i64
; CHECK: ret i64 %[[CAST_I64]]
; CHECK-ROTATE-LABEL: define spir_func i64 @scalar_64
; CHECK-ROTATE-SAME: (i64 %[[ARG_I64:[^ )]+]])
; CHECK-ROTATE-NOT: call i64 @llvm.bswap.i64
; CHECK-ROTATE: %[[CAST_V4I16:[^ )]+]] = bitcast i64 %[[ARG_I64]] to <4 x i16>
; CHECK-ROTATE: %[[ROL_V4I16:[^ )]+]] = call <4 x i16> @llvm.genx.rol.v4i16.v4i16(<4 x i16> %[[CAST_V4I16]], <4 x i16> <i16 8, i16 8, i16 8, i16 8>)
; CHECK-ROTATE: %[[CAST_V2I32:[^ )]+]] = bitcast <4 x i16> %[[ROL_V4I16]] to <2 x i32>
; CHECK-ROTATE: %[[ROL_V2I32:[^ )]+]] = call <2 x i32> @llvm.genx.rol.v2i32.v2i32(<2 x i32> %[[CAST_V2I32]], <2 x i32> <i32 16, i32 16>)
; CHECK-ROTATE: %[[CAST_V1I64:[^ )]+]] = bitcast <2 x i32> %[[ROL_V2I32]] to <1 x i64>
; CHECK-ROTATE: %[[CAST_V2I32:[^ )]+]] = bitcast <1 x i64> %[[CAST_V1I64]] to <2 x i32>
; CHECK-ROTATE: %[[READ_V1I32LO:[^ )]+]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> %[[CAST_V2I32]], i32 2, i32 1, i32 0, i16 0, i32 undef)
; CHECK-ROTATE: %[[READ_V1I32HI:[^ )]+]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> %[[CAST_V2I32]], i32 2, i32 1, i32 0, i16 4, i32 undef)
; CHECK-ROTATE: %[[WRITE_V1I32HI:[^ )]+]] = call <2 x i32> @llvm.genx.wrregioni.v2i32.v1i32.i16.i1(<2 x i32> undef, <1 x i32> %[[READ_V1I32LO]], i32 2, i32 1, i32 0, i16 4, i32 undef, i1 true)
; CHECK-ROTATE: %[[WRITE_V1I32LO:[^ )]+]] = call <2 x i32> @llvm.genx.wrregioni.v2i32.v1i32.i16.i1(<2 x i32> %[[WRITE_V1I32HI]], <1 x i32> %[[READ_V1I32HI]], i32 2, i32 1, i32 0, i16 0, i32 undef, i1 true)
; CHECK-ROTATE: %[[CAST_I64:[^ )]+]] = bitcast <2 x i32> %[[WRITE_V1I32LO]] to i64
; CHECK-ROTATE: ret i64 %[[CAST_I64]]
; CHECK-ROTATE64-LABEL: define spir_func i64 @scalar_64
; CHECK-ROTATE64-SAME: (i64 %[[ARG_I64:[^ )]+]])
; CHECK-ROTATE64-NOT: call i64 @llvm.bswap.i64
; CHECK-ROTATE64: %[[CAST_V4I16:[^ )]+]] = bitcast i64 %[[ARG_I64]] to <4 x i16>
; CHECK-ROTATE64: %[[ROL_V4I16:[^ )]+]] = call <4 x i16> @llvm.genx.rol.v4i16.v4i16(<4 x i16> %[[CAST_V4I16]], <4 x i16> <i16 8, i16 8, i16 8, i16 8>)
; CHECK-ROTATE64: %[[CAST_V2I32:[^ )]+]] = bitcast <4 x i16> %[[ROL_V4I16]] to <2 x i32>
; CHECK-ROTATE64: %[[ROL_V2I32:[^ )]+]] = call <2 x i32> @llvm.genx.rol.v2i32.v2i32(<2 x i32> %[[CAST_V2I32]], <2 x i32> <i32 16, i32 16>)
; CHECK-ROTATE64: %[[CAST_V1I64:[^ )]+]] = bitcast <2 x i32> %[[ROL_V2I32]] to <1 x i64>
; CHECK-ROTATE64: %[[ROL_V1I64:[^ )]+]] = call <1 x i64> @llvm.genx.rol.v1i64.v1i64(<1 x i64> %[[CAST_V1I64]], <1 x i64> <i64 32>)
; CHECK-ROTATE64: %[[CAST_I64:[^ )]+]] = bitcast <1 x i64> %[[ROL_V1I64]] to i64
; CHECK-ROTATE64: ret i64 %[[CAST_I64]]
  %res = tail call i64 @llvm.bswap.i64(i64 %arg)
  ret i64 %res
}

define spir_func <3 x i32> @vector_32(<3 x i32> %arg) {
; CHECK-LABEL: define spir_func <3 x i32> @vector_32
; CHECK-SAME: (<3 x i32> %[[ARG_V3I32:[^ )]+]])
; CHECK-NOT: call <3 x i32> @llvm.bswap.i64
; CHECK: %[[VCAST_V6I16:[^ )]+]] = bitcast <3 x i32> %[[ARG_I64]] to <6 x i16>
; CHECK: %[[CAST_V12I8:[^ )]+]] = bitcast <6 x i16> %[[VCAST_V4I16]] to <12 x i8>
; CHECK: %[[READ_V6I8LO:[^ )]+]] = call <6 x i8> @llvm.genx.rdregioni.v6i8.v12i8.i16(<12 x i8> %[[CAST_V12I8]], i32 2, i32 1, i32 0, i16 0, i32 undef)
; CHECK: %[[READ_V6I8HI:[^ )]+]] = call <6 x i8> @llvm.genx.rdregioni.v6i8.v12i8.i16(<12 x i8> %[[CAST_V12I8]], i32 2, i32 1, i32 0, i16 1, i32 undef)
; CHECK: %[[WRITE_V6I8HI:[^ )]+]] = call <12 x i8> @llvm.genx.wrregioni.v12i8.v6i8.i16.i1(<12 x i8> undef, <6 x i8> %[[READ_V6I8LO]], i32 2, i32 1, i32 0, i16 1, i32 undef, i1 true)
; CHECK: %[[WRITE_V6I8LO:[^ )]+]] = call <12 x i8> @llvm.genx.wrregioni.v12i8.v6i8.i16.i1(<12 x i8> %[[WRITE_V6I8HI]], <6 x i8> %[[READ_V6I8HI]], i32 2, i32 1, i32 0, i16 0, i32 undef, i1 true)
; CHECK: %[[CAST_V3I32:[^ )]+]] = bitcast <12 x i8> %[[WRITE_V6I8LO]] to <3 x i32>
; CHECK: %[[CAST_V6I16:[^ )]+]] = bitcast <3 x i32> %[[CAST_V3I32]] to <6 x i16>
; CHECK: %[[READ_V3I16LO:[^ )]+]] = call <3 x i16> @llvm.genx.rdregioni.v3i16.v6i16.i16(<6 x i16> %[[CAST_V6I16]], i32 2, i32 1, i32 0, i16 0, i32 undef)
; CHECK: %[[READ_V3I16HI:[^ )]+]] = call <3 x i16> @llvm.genx.rdregioni.v3i16.v6i16.i16(<6 x i16> %[[CAST_V6I16]], i32 2, i32 1, i32 0, i16 2, i32 undef)
; CHECK: %[[WRITE_V3I16HI:[^ )]+]] = call <6 x i16> @llvm.genx.wrregioni.v6i16.v3i16.i16.i1(<6 x i16> undef, <3 x i16> %[[READ_V3I16LO]], i32 2, i32 1, i32 0, i16 2, i32 undef, i1 true)
; CHECK: %[[WRITE_V3I16LO:[^ )]+]] = call <6 x i16> @llvm.genx.wrregioni.v6i16.v3i16.i16.i1(<6 x i16> %[[WRITE_V3I16HI]], <3 x i16> %[[READ_V3I16HI]], i32 2, i32 1, i32 0, i16 0, i32 undef, i1 true)
; CHECK: %[[RES_V3I32:[^ )]+]] = bitcast <6 x i16> %[[WRITE_V3I16LO]] to <3 x i32>
; CHECK: ret <3 x i32> %[[RES_V3I32]]
; CHECK-ROTATE-LABEL: define spir_func <3 x i32> @vector_32
; CHECK-ROTATE-SAME: (<3 x i32> %[[ARG_V3I32:[^ )]+]])
; CHECK-ROTATE-NOT: call <3 x i32> @llvm.bswap.i64
; CHECK-ROTATE: %[[CAST_V6I16:[^ )]+]] = bitcast <3 x i32> %[[ARG_I64]] to <6 x i16>
; CHECK-ROTATE: %[[ROL_V6I16:[^ )]+]] = call <6 x i16> @llvm.genx.rol.v6i16.v6i16(<6 x i16> %[[CAST_V6I16]], <6 x i16> <i16 8, i16 8, i16 8, i16 8, i16 8, i16 8>)
; CHECK-ROTATE: %[[CAST_V3I32:[^ )]+]] = bitcast <6 x i16> %[[ROL_V6I16]] to <3 x i32>
; CHECK-ROTATE: %[[ROL_V3I32:[^ )]+]] = call <3 x i32> @llvm.genx.rol.v3i32.v3i32(<3 x i32> %[[CAST_V3I32]], <3 x i32> <i32 16, i32 16, i32 16>)
; CHECK-ROTATE: ret <3 x i32> %[[ROL_V3I32]]
; CHECK-ROTATE64-LABEL: define spir_func <3 x i32> @vector_32
; CHECK-ROTATE64-SAME: (<3 x i32> %[[ARG_V3I32:[^ )]+]])
; CHECK-ROTATE64-NOT: call <3 x i32> @llvm.bswap.i64
; CHECK-ROTATE64: %[[CAST_V6I16:[^ )]+]] = bitcast <3 x i32> %[[ARG_I64]] to <6 x i16>
; CHECK-ROTATE64: %[[ROL_V6I16:[^ )]+]] = call <6 x i16> @llvm.genx.rol.v6i16.v6i16(<6 x i16> %[[CAST_V6I16]], <6 x i16> <i16 8, i16 8, i16 8, i16 8, i16 8, i16 8>)
; CHECK-ROTATE64: %[[CAST_V3I32:[^ )]+]] = bitcast <6 x i16> %[[ROL_V6I16]] to <3 x i32>
; CHECK-ROTATE64: %[[ROL_V3I32:[^ )]+]] = call <3 x i32> @llvm.genx.rol.v3i32.v3i32(<3 x i32> %[[CAST_V3I32]], <3 x i32> <i32 16, i32 16, i32 16>)
; CHECK-ROTATE64: ret <3 x i32> %[[ROL_V3I32]]
  %res = tail call <3 x i32> @llvm.bswap.v3i32(<3 x i32> %arg)
  ret <3 x i32> %res
}
