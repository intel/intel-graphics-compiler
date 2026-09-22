;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: cri-supported, llvm-spirv

; RUN: sed -e 's/INPUT_TYPE/<2 x i8>/g' -e 's/OUTPUT_TYPE/i16/g' \
; RUN:   -e 's/TEST_NAME/test_char2_i16/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_c/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=CHAR2_I16

; CHAR2_I16: .function "test_char2_i16_1"
; CHAR2_I16: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; CHAR2_I16-DAG: .decl [[OUTPUT]] v_type=G type=w num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; CHAR2_I16-DAG: .decl [[INPUT]] v_type=G type=b num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x i8>/g' -e 's/OUTPUT_TYPE/<2 x i16>/g' \
; RUN:   -e 's/TEST_NAME/test_char4_v2i16/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_c/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=CHAR4_V2I16

; CHAR4_V2I16: .function "test_char4_v2i16_1"
; CHAR4_V2I16: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; CHAR4_V2I16: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; CHAR4_V2I16-DAG: .decl [[OUTPUT]] v_type=G type=w num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; CHAR4_V2I16-DAG: .decl [[INPUT]] v_type=G type=b num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x i8>/g' -e 's/OUTPUT_TYPE/i32/g' \
; RUN:   -e 's/TEST_NAME/test_char4_i32/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_c/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=CHAR4_I32

; CHAR4_I32: .function "test_char4_i32_1"
; CHAR4_I32: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; CHAR4_I32-DAG: .decl [[OUTPUT]] v_type=G type=d num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; CHAR4_I32-DAG: .decl [[INPUT]] v_type=G type=b num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x i8>/g' -e 's/OUTPUT_TYPE/<4 x i16>/g' \
; RUN:   -e 's/TEST_NAME/test_char8_v4i16/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_c/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=CHAR8_V4I16

; CHAR8_V4I16: .function "test_char8_v4i16_1"
; CHAR8_V4I16: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; CHAR8_V4I16: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; CHAR8_V4I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; CHAR8_V4I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; CHAR8_V4I16-DAG: .decl [[OUTPUT]] v_type=G type=w num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; CHAR8_V4I16-DAG: .decl [[INPUT]] v_type=G type=b num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x i8>/g' -e 's/OUTPUT_TYPE/<2 x i32>/g' \
; RUN:   -e 's/TEST_NAME/test_char8_v2i32/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_c/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=CHAR8_V2I32

; CHAR8_V2I32: .function "test_char8_v2i32_1"
; CHAR8_V2I32: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; CHAR8_V2I32: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; CHAR8_V2I32-DAG: .decl [[OUTPUT]] v_type=G type=d num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; CHAR8_V2I32-DAG: .decl [[INPUT]] v_type=G type=b num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x i8>/g' -e 's/OUTPUT_TYPE/i64/g' \
; RUN:   -e 's/TEST_NAME/test_char8_i64/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_c/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=CHAR8_I64

; CHAR8_I64: .function "test_char8_i64_1"
; CHAR8_I64: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; CHAR8_I64-DAG: .decl [[OUTPUT]] v_type=G type=q num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; CHAR8_I64-DAG: .decl [[INPUT]] v_type=G type=b num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<16 x i8>/g' -e 's/OUTPUT_TYPE/<8 x i16>/g' \
; RUN:   -e 's/TEST_NAME/test_char16_v8i16/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv16_c/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=CHAR16_V8I16

; CHAR16_V8I16: .function "test_char16_v8i16_1"
; CHAR16_V8I16: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; CHAR16_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; CHAR16_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; CHAR16_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; CHAR16_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; CHAR16_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; CHAR16_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; CHAR16_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; CHAR16_V8I16-DAG: .decl [[OUTPUT]] v_type=G type=w num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; CHAR16_V8I16-DAG: .decl [[INPUT]] v_type=G type=b num_elts=256 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<16 x i8>/g' -e 's/OUTPUT_TYPE/<4 x i32>/g' \
; RUN:   -e 's/TEST_NAME/test_char16_v4i32/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv16_c/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=CHAR16_V4I32

; CHAR16_V4I32: .function "test_char16_v4i32_1"
; CHAR16_V4I32: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; CHAR16_V4I32: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; CHAR16_V4I32: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; CHAR16_V4I32: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; CHAR16_V4I32-DAG: .decl [[OUTPUT]] v_type=G type=d num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; CHAR16_V4I32-DAG: .decl [[INPUT]] v_type=G type=b num_elts=256 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<16 x i8>/g' -e 's/OUTPUT_TYPE/<2 x i64>/g' \
; RUN:   -e 's/TEST_NAME/test_char16_v2i64/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv16_c/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=CHAR16_V2I64

; CHAR16_V2I64: .function "test_char16_v2i64_1"
; CHAR16_V2I64: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; CHAR16_V2I64: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; CHAR16_V2I64-DAG: .decl [[OUTPUT]] v_type=G type=q num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; CHAR16_V2I64-DAG: .decl [[INPUT]] v_type=G type=b num_elts=256 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/i16/g' -e 's/OUTPUT_TYPE/<2 x i8>/g' \
; RUN:   -e 's/TEST_NAME/test_short_v2i8/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELs/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=SHORT_V2I8

; SHORT_V2I8: .function "test_short_v2i8_1"
; SHORT_V2I8: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; SHORT_V2I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; SHORT_V2I8-DAG: .decl [[OUTPUT]] v_type=G type=b num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT_V2I8-DAG: .decl [[INPUT]] v_type=G type=w num_elts=16 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x i16>/g' -e 's/OUTPUT_TYPE/<4 x i8>/g' \
; RUN:   -e 's/TEST_NAME/test_short2_v4i8/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_s/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=SHORT2_V4I8

; SHORT2_V4I8: .function "test_short2_v4i8_1"
; SHORT2_V4I8: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; SHORT2_V4I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; SHORT2_V4I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,32)<1;1,0>
; SHORT2_V4I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,48)<1;1,0>
; SHORT2_V4I8-DAG: .decl [[OUTPUT]] v_type=G type=b num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT2_V4I8-DAG: .decl [[INPUT]] v_type=G type=w num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x i16>/g' -e 's/OUTPUT_TYPE/i32/g' \
; RUN:   -e 's/TEST_NAME/test_short2_i32/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_s/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=SHORT2_I32

; SHORT2_I32: .function "test_short2_i32_1"
; SHORT2_I32: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; SHORT2_I32-DAG: .decl [[OUTPUT]] v_type=G type=d num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT2_I32-DAG: .decl [[INPUT]] v_type=G type=w num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x i16>/g' -e 's/OUTPUT_TYPE/<8 x i8>/g' \
; RUN:   -e 's/TEST_NAME/test_short4_v8i8/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_s/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=SHORT4_V8I8

; SHORT4_V8I8: .function "test_short4_v8i8_1"
; SHORT4_V8I8: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; SHORT4_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; SHORT4_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,32)<1;1,0>
; SHORT4_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,48)<1;1,0>
; SHORT4_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; SHORT4_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; SHORT4_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,32)<1;1,0>
; SHORT4_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,48)<1;1,0>
; SHORT4_V8I8-DAG: .decl [[OUTPUT]] v_type=G type=b num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT4_V8I8-DAG: .decl [[INPUT]] v_type=G type=w num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x i16>/g' -e 's/OUTPUT_TYPE/<2 x i32>/g' \
; RUN:   -e 's/TEST_NAME/test_short4_v2i32/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_s/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=SHORT4_V2I32

; SHORT4_V2I32: .function "test_short4_v2i32_1"
; SHORT4_V2I32: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; SHORT4_V2I32: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; SHORT4_V2I32-DAG: .decl [[OUTPUT]] v_type=G type=d num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT4_V2I32-DAG: .decl [[INPUT]] v_type=G type=w num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x i16>/g' -e 's/OUTPUT_TYPE/i64/g' \
; RUN:   -e 's/TEST_NAME/test_short4_i64/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_s/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=SHORT4_I64

; SHORT4_I64: .function "test_short4_i64_1"
; SHORT4_I64: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; SHORT4_I64-DAG: .decl [[OUTPUT]] v_type=G type=q num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT4_I64-DAG: .decl [[INPUT]] v_type=G type=w num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x i16>/g' -e 's/OUTPUT_TYPE/<16 x i8>/g' \
; RUN:   -e 's/TEST_NAME/test_short8_v16i8/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_s/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=SHORT8_V16I8

; SHORT8_V16I8: .function "test_short8_v16i8_1"
; SHORT8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; SHORT8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; SHORT8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,32)<1;1,0>
; SHORT8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,48)<1;1,0>
; SHORT8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; SHORT8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; SHORT8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,32)<1;1,0>
; SHORT8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,48)<1;1,0>
; SHORT8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; SHORT8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; SHORT8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](2,32)<1;1,0>
; SHORT8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](2,48)<1;1,0>
; SHORT8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; SHORT8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; SHORT8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](3,32)<1;1,0>
; SHORT8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](3,48)<1;1,0>
; SHORT8_V16I8-DAG: .decl [[OUTPUT]] v_type=G type=b num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT8_V16I8-DAG: .decl [[INPUT]] v_type=G type=w num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x i16>/g' -e 's/OUTPUT_TYPE/<4 x i32>/g' \
; RUN:   -e 's/TEST_NAME/test_short8_v4i32/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_s/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=SHORT8_V4I32

; SHORT8_V4I32: .function "test_short8_v4i32_1"
; SHORT8_V4I32: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; SHORT8_V4I32: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; SHORT8_V4I32: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; SHORT8_V4I32: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; SHORT8_V4I32-DAG: .decl [[OUTPUT]] v_type=G type=d num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT8_V4I32-DAG: .decl [[INPUT]] v_type=G type=w num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x i16>/g' -e 's/OUTPUT_TYPE/<2 x i64>/g' \
; RUN:   -e 's/TEST_NAME/test_short8_v2i64/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_s/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=SHORT8_V2I64

; SHORT8_V2I64: .function "test_short8_v2i64_1"
; SHORT8_V2I64: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; SHORT8_V2I64: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; SHORT8_V2I64-DAG: .decl [[OUTPUT]] v_type=G type=q num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT8_V2I64-DAG: .decl [[INPUT]] v_type=G type=w num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<16 x i16>/g' -e 's/OUTPUT_TYPE/<8 x i32>/g' \
; RUN:   -e 's/TEST_NAME/test_short16_v8i32/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv16_s/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=SHORT16_V8I32

; SHORT16_V8I32: .function "test_short16_v8i32_1"
; SHORT16_V8I32: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; SHORT16_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; SHORT16_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; SHORT16_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; SHORT16_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; SHORT16_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; SHORT16_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; SHORT16_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; SHORT16_V8I32-DAG: .decl [[OUTPUT]] v_type=G type=d num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT16_V8I32-DAG: .decl [[INPUT]] v_type=G type=w num_elts=256 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<16 x i16>/g' -e 's/OUTPUT_TYPE/<4 x i64>/g' \
; RUN:   -e 's/TEST_NAME/test_short16_v4i64/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv16_s/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=SHORT16_V4I64

; SHORT16_V4I64: .function "test_short16_v4i64_1"
; SHORT16_V4I64: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; SHORT16_V4I64: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; SHORT16_V4I64: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; SHORT16_V4I64: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; SHORT16_V4I64-DAG: .decl [[OUTPUT]] v_type=G type=q num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT16_V4I64-DAG: .decl [[INPUT]] v_type=G type=w num_elts=256 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/i32/g' -e 's/OUTPUT_TYPE/<4 x i8>/g' \
; RUN:   -e 's/TEST_NAME/test_int_v4i8/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELi/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=INT_V4I8

; INT_V4I8: .function "test_int_v4i8_1"
; INT_V4I8: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; INT_V4I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; INT_V4I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,32)<1;1,0>
; INT_V4I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,48)<1;1,0>
; INT_V4I8-DAG: .decl [[OUTPUT]] v_type=G type=b num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT_V4I8-DAG: .decl [[INPUT]] v_type=G type=d num_elts=16 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/i32/g' -e 's/OUTPUT_TYPE/<2 x i16>/g' \
; RUN:   -e 's/TEST_NAME/test_int_v2i16/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELi/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=INT_V2I16

; INT_V2I16: .function "test_int_v2i16_1"
; INT_V2I16: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; INT_V2I16: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; INT_V2I16-DAG: .decl [[OUTPUT]] v_type=G type=w num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT_V2I16-DAG: .decl [[INPUT]] v_type=G type=d num_elts=16 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x i32>/g' -e 's/OUTPUT_TYPE/<8 x i8>/g' \
; RUN:   -e 's/TEST_NAME/test_int2_v8i8/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_i/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=INT2_V8I8

; INT2_V8I8: .function "test_int2_v8i8_1"
; INT2_V8I8: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; INT2_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; INT2_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,32)<1;1,0>
; INT2_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,48)<1;1,0>
; INT2_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; INT2_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; INT2_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,32)<1;1,0>
; INT2_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,48)<1;1,0>
; INT2_V8I8-DAG: .decl [[OUTPUT]] v_type=G type=b num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT2_V8I8-DAG: .decl [[INPUT]] v_type=G type=d num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x i32>/g' -e 's/OUTPUT_TYPE/<4 x i16>/g' \
; RUN:   -e 's/TEST_NAME/test_int2_v4i16/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_i/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=INT2_V4I16

; INT2_V4I16: .function "test_int2_v4i16_1"
; INT2_V4I16: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; INT2_V4I16: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; INT2_V4I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; INT2_V4I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; INT2_V4I16-DAG: .decl [[OUTPUT]] v_type=G type=w num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT2_V4I16-DAG: .decl [[INPUT]] v_type=G type=d num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x i32>/g' -e 's/OUTPUT_TYPE/i64/g' \
; RUN:   -e 's/TEST_NAME/test_int2_i64/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_i/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=INT2_I64

; INT2_I64: .function "test_int2_i64_1"
; INT2_I64: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; INT2_I64-DAG: .decl [[OUTPUT]] v_type=G type=q num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT2_I64-DAG: .decl [[INPUT]] v_type=G type=d num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x i32>/g' -e 's/OUTPUT_TYPE/<16 x i8>/g' \
; RUN:   -e 's/TEST_NAME/test_int4_v16i8/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_i/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=INT4_V16I8

; INT4_V16I8: .function "test_int4_v16i8_1"
; INT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; INT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; INT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,32)<1;1,0>
; INT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,48)<1;1,0>
; INT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; INT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; INT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,32)<1;1,0>
; INT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,48)<1;1,0>
; INT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; INT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; INT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](2,32)<1;1,0>
; INT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](2,48)<1;1,0>
; INT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; INT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; INT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](3,32)<1;1,0>
; INT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](3,48)<1;1,0>
; INT4_V16I8-DAG: .decl [[OUTPUT]] v_type=G type=b num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT4_V16I8-DAG: .decl [[INPUT]] v_type=G type=d num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x i32>/g' -e 's/OUTPUT_TYPE/<8 x i16>/g' \
; RUN:   -e 's/TEST_NAME/test_int4_v8i16/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_i/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=INT4_V8I16

; INT4_V8I16: .function "test_int4_v8i16_1"
; INT4_V8I16: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; INT4_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; INT4_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; INT4_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; INT4_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; INT4_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; INT4_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; INT4_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; INT4_V8I16-DAG: .decl [[OUTPUT]] v_type=G type=w num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT4_V8I16-DAG: .decl [[INPUT]] v_type=G type=d num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x i32>/g' -e 's/OUTPUT_TYPE/<2 x i64>/g' \
; RUN:   -e 's/TEST_NAME/test_int4_v2i64/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_i/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=INT4_V2I64

; INT4_V2I64: .function "test_int4_v2i64_1"
; INT4_V2I64: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; INT4_V2I64: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; INT4_V2I64-DAG: .decl [[OUTPUT]] v_type=G type=q num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT4_V2I64-DAG: .decl [[INPUT]] v_type=G type=d num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x i32>/g' -e 's/OUTPUT_TYPE/<16 x i16>/g' \
; RUN:   -e 's/TEST_NAME/test_int8_v16i16/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_i/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=INT8_V16I16

; INT8_V16I16: .function "test_int8_v16i16_1"
; INT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; INT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; INT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; INT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; INT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; INT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; INT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; INT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; INT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; INT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](4,16)<1;1,0>
; INT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; INT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](5,16)<1;1,0>
; INT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; INT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](6,16)<1;1,0>
; INT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; INT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](7,16)<1;1,0>
; INT8_V16I16-DAG: .decl [[OUTPUT]] v_type=G type=w num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT8_V16I16-DAG: .decl [[INPUT]] v_type=G type=d num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x i32>/g' -e 's/OUTPUT_TYPE/<4 x i64>/g' \
; RUN:   -e 's/TEST_NAME/test_int8_v4i64/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_i/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=INT8_V4I64

; INT8_V4I64: .function "test_int8_v4i64_1"
; INT8_V4I64: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; INT8_V4I64: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; INT8_V4I64: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; INT8_V4I64: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; INT8_V4I64-DAG: .decl [[OUTPUT]] v_type=G type=q num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT8_V4I64-DAG: .decl [[INPUT]] v_type=G type=d num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<16 x i32>/g' -e 's/OUTPUT_TYPE/<8 x i64>/g' \
; RUN:   -e 's/TEST_NAME/test_int16_v8i64/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv16_i/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=INT16_V8I64

; INT16_V8I64: .function "test_int16_v8i64_1"
; INT16_V8I64: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; INT16_V8I64: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; INT16_V8I64: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; INT16_V8I64: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; INT16_V8I64: mov (M1, 16) {{.*}} [[OUTPUT]](8,0)<1;1,0>
; INT16_V8I64: mov (M1, 16) {{.*}} [[OUTPUT]](10,0)<1;1,0>
; INT16_V8I64: mov (M1, 16) {{.*}} [[OUTPUT]](12,0)<1;1,0>
; INT16_V8I64: mov (M1, 16) {{.*}} [[OUTPUT]](14,0)<1;1,0>
; INT16_V8I64-DAG: .decl [[OUTPUT]] v_type=G type=q num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT16_V8I64-DAG: .decl [[INPUT]] v_type=G type=d num_elts=256 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/i64/g' -e 's/OUTPUT_TYPE/<8 x i8>/g' \
; RUN:   -e 's/TEST_NAME/test_long_v8i8/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELl/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=LONG_V8I8

; LONG_V8I8: .function "test_long_v8i8_1"
; LONG_V8I8: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; LONG_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; LONG_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,32)<1;1,0>
; LONG_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,48)<1;1,0>
; LONG_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; LONG_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; LONG_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,32)<1;1,0>
; LONG_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,48)<1;1,0>
; LONG_V8I8-DAG: .decl [[OUTPUT]] v_type=G type=b num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG_V8I8-DAG: .decl [[INPUT]] v_type=G type=q num_elts=16 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/i64/g' -e 's/OUTPUT_TYPE/<4 x i16>/g' \
; RUN:   -e 's/TEST_NAME/test_long_v4i16/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELl/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=LONG_V4I16

; LONG_V4I16: .function "test_long_v4i16_1"
; LONG_V4I16: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; LONG_V4I16: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; LONG_V4I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; LONG_V4I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; LONG_V4I16-DAG: .decl [[OUTPUT]] v_type=G type=w num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG_V4I16-DAG: .decl [[INPUT]] v_type=G type=q num_elts=16 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/i64/g' -e 's/OUTPUT_TYPE/<2 x i32>/g' \
; RUN:   -e 's/TEST_NAME/test_long_v2i32/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELl/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=LONG_V2I32

; LONG_V2I32: .function "test_long_v2i32_1"
; LONG_V2I32: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; LONG_V2I32: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; LONG_V2I32-DAG: .decl [[OUTPUT]] v_type=G type=d num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG_V2I32-DAG: .decl [[INPUT]] v_type=G type=q num_elts=16 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x i64>/g' -e 's/OUTPUT_TYPE/<16 x i8>/g' \
; RUN:   -e 's/TEST_NAME/test_long2_v16i8/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_l/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=LONG2_V16I8

; LONG2_V16I8: .function "test_long2_v16i8_1"
; LONG2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; LONG2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; LONG2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,32)<1;1,0>
; LONG2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,48)<1;1,0>
; LONG2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; LONG2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; LONG2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,32)<1;1,0>
; LONG2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,48)<1;1,0>
; LONG2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; LONG2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; LONG2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](2,32)<1;1,0>
; LONG2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](2,48)<1;1,0>
; LONG2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; LONG2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; LONG2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](3,32)<1;1,0>
; LONG2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](3,48)<1;1,0>
; LONG2_V16I8-DAG: .decl [[OUTPUT]] v_type=G type=b num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG2_V16I8-DAG: .decl [[INPUT]] v_type=G type=q num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x i64>/g' -e 's/OUTPUT_TYPE/<8 x i16>/g' \
; RUN:   -e 's/TEST_NAME/test_long2_v8i16/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_l/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=LONG2_V8I16

; LONG2_V8I16: .function "test_long2_v8i16_1"
; LONG2_V8I16: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; LONG2_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; LONG2_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; LONG2_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; LONG2_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; LONG2_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; LONG2_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; LONG2_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; LONG2_V8I16-DAG: .decl [[OUTPUT]] v_type=G type=w num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG2_V8I16-DAG: .decl [[INPUT]] v_type=G type=q num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x i64>/g' -e 's/OUTPUT_TYPE/<4 x i32>/g' \
; RUN:   -e 's/TEST_NAME/test_long2_v4i32/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_l/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=LONG2_V4I32

; LONG2_V4I32: .function "test_long2_v4i32_1"
; LONG2_V4I32: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; LONG2_V4I32: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; LONG2_V4I32: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; LONG2_V4I32: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; LONG2_V4I32-DAG: .decl [[OUTPUT]] v_type=G type=d num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG2_V4I32-DAG: .decl [[INPUT]] v_type=G type=q num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x i64>/g' -e 's/OUTPUT_TYPE/<16 x i16>/g' \
; RUN:   -e 's/TEST_NAME/test_long4_v16i16/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_l/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=LONG4_V16I16

; LONG4_V16I16: .function "test_long4_v16i16_1"
; LONG4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; LONG4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; LONG4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; LONG4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; LONG4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; LONG4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; LONG4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; LONG4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; LONG4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; LONG4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](4,16)<1;1,0>
; LONG4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; LONG4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](5,16)<1;1,0>
; LONG4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; LONG4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](6,16)<1;1,0>
; LONG4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; LONG4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](7,16)<1;1,0>
; LONG4_V16I16-DAG: .decl [[OUTPUT]] v_type=G type=w num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG4_V16I16-DAG: .decl [[INPUT]] v_type=G type=q num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x i64>/g' -e 's/OUTPUT_TYPE/<8 x i32>/g' \
; RUN:   -e 's/TEST_NAME/test_long4_v8i32/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_l/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=LONG4_V8I32

; LONG4_V8I32: .function "test_long4_v8i32_1"
; LONG4_V8I32: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; LONG4_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; LONG4_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; LONG4_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; LONG4_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; LONG4_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; LONG4_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; LONG4_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; LONG4_V8I32-DAG: .decl [[OUTPUT]] v_type=G type=d num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG4_V8I32-DAG: .decl [[INPUT]] v_type=G type=q num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x i64>/g' -e 's/OUTPUT_TYPE/<16 x i32>/g' \
; RUN:   -e 's/TEST_NAME/test_long8_v16i32/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_l/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=LONG8_V16I32

; LONG8_V16I32: .function "test_long8_v16i32_1"
; LONG8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; LONG8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; LONG8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; LONG8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; LONG8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; LONG8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; LONG8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; LONG8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; LONG8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](8,0)<1;1,0>
; LONG8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](9,0)<1;1,0>
; LONG8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](10,0)<1;1,0>
; LONG8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](11,0)<1;1,0>
; LONG8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](12,0)<1;1,0>
; LONG8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](13,0)<1;1,0>
; LONG8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](14,0)<1;1,0>
; LONG8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](15,0)<1;1,0>
; LONG8_V16I32-DAG: .decl [[OUTPUT]] v_type=G type=d num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG8_V16I32-DAG: .decl [[INPUT]] v_type=G type=q num_elts=128 align=wordx32

target triple = "spir64-unknown-unknown"

declare spir_func OUTPUT_TYPE @SPIRV_INTRINSIC(INPUT_TYPE) #0

define spir_func OUTPUT_TYPE @TEST_NAME(INPUT_TYPE %x) #1 {
  entry:
  %res = call OUTPUT_TYPE @SPIRV_INTRINSIC(INPUT_TYPE %x)
  ret OUTPUT_TYPE %res
}

define spir_kernel void @test(INPUT_TYPE %x, OUTPUT_TYPE addrspace(1)* %out) {
entry:
  %result = call OUTPUT_TYPE @TEST_NAME(INPUT_TYPE %x)
  store OUTPUT_TYPE %result, OUTPUT_TYPE addrspace(1)* %out
  ret void
}

attributes #0 = { nounwind }
attributes #1 = { noinline nounwind }


!24 = ! { i32 16 }
