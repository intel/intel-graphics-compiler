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
; CHAR2_I16-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; CHAR2_I16-DAG: .decl [[INPUT]] v_type=G type=b num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x i8>/g' -e 's/OUTPUT_TYPE/half/g' \
; RUN:   -e 's/TEST_NAME/test_char2_half/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_c/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=CHAR2_HALF

; CHAR2_HALF: .function "test_char2_half_1"
; CHAR2_HALF: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; CHAR2_HALF-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; CHAR2_HALF-DAG: .decl [[INPUT]] v_type=G type=b num_elts=32 align=wordx32

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
; CHAR4_V2I16-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
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
; CHAR4_I32-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; CHAR4_I32-DAG: .decl [[INPUT]] v_type=G type=b num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x i8>/g' -e 's/OUTPUT_TYPE/<2 x half>/g' \
; RUN:   -e 's/TEST_NAME/test_char4_v2half/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_c/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=CHAR4_V2HALF

; CHAR4_V2HALF: .function "test_char4_v2half_1"
; CHAR4_V2HALF: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; CHAR4_V2HALF: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; CHAR4_V2HALF-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; CHAR4_V2HALF-DAG: .decl [[INPUT]] v_type=G type=b num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x i8>/g' -e 's/OUTPUT_TYPE/float/g' \
; RUN:   -e 's/TEST_NAME/test_char4_float/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_c/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=CHAR4_FLOAT

; CHAR4_FLOAT: .function "test_char4_float_1"
; CHAR4_FLOAT: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; CHAR4_FLOAT-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; CHAR4_FLOAT-DAG: .decl [[INPUT]] v_type=G type=b num_elts=64 align=wordx32

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
; CHAR8_V4I16-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
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
; CHAR8_V2I32-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
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
; CHAR8_I64-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; CHAR8_I64-DAG: .decl [[INPUT]] v_type=G type=b num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x i8>/g' -e 's/OUTPUT_TYPE/<4 x half>/g' \
; RUN:   -e 's/TEST_NAME/test_char8_v4half/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_c/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=CHAR8_V4HALF

; CHAR8_V4HALF: .function "test_char8_v4half_1"
; CHAR8_V4HALF: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; CHAR8_V4HALF: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; CHAR8_V4HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; CHAR8_V4HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; CHAR8_V4HALF-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; CHAR8_V4HALF-DAG: .decl [[INPUT]] v_type=G type=b num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x i8>/g' -e 's/OUTPUT_TYPE/<2 x float>/g' \
; RUN:   -e 's/TEST_NAME/test_char8_v2float/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_c/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=CHAR8_V2FLOAT

; CHAR8_V2FLOAT: .function "test_char8_v2float_1"
; CHAR8_V2FLOAT: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; CHAR8_V2FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; CHAR8_V2FLOAT-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; CHAR8_V2FLOAT-DAG: .decl [[INPUT]] v_type=G type=b num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x i8>/g' -e 's/OUTPUT_TYPE/double/g' \
; RUN:   -e 's/TEST_NAME/test_char8_double/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_c/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=CHAR8_DOUBLE

; CHAR8_DOUBLE: .function "test_char8_double_1"
; CHAR8_DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; CHAR8_DOUBLE-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; CHAR8_DOUBLE-DAG: .decl [[INPUT]] v_type=G type=b num_elts=128 align=wordx32

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
; CHAR16_V8I16-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
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
; CHAR16_V4I32-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
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
; CHAR16_V2I64-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; CHAR16_V2I64-DAG: .decl [[INPUT]] v_type=G type=b num_elts=256 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<16 x i8>/g' -e 's/OUTPUT_TYPE/<8 x half>/g' \
; RUN:   -e 's/TEST_NAME/test_char16_v8half/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv16_c/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=CHAR16_V8HALF

; CHAR16_V8HALF: .function "test_char16_v8half_1"
; CHAR16_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; CHAR16_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; CHAR16_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; CHAR16_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; CHAR16_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; CHAR16_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; CHAR16_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; CHAR16_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; CHAR16_V8HALF-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; CHAR16_V8HALF-DAG: .decl [[INPUT]] v_type=G type=b num_elts=256 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<16 x i8>/g' -e 's/OUTPUT_TYPE/<4 x float>/g' \
; RUN:   -e 's/TEST_NAME/test_char16_v4float/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv16_c/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=CHAR16_V4FLOAT

; CHAR16_V4FLOAT: .function "test_char16_v4float_1"
; CHAR16_V4FLOAT: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; CHAR16_V4FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; CHAR16_V4FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; CHAR16_V4FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; CHAR16_V4FLOAT-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; CHAR16_V4FLOAT-DAG: .decl [[INPUT]] v_type=G type=b num_elts=256 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<16 x i8>/g' -e 's/OUTPUT_TYPE/<2 x double>/g' \
; RUN:   -e 's/TEST_NAME/test_char16_v2double/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv16_c/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=CHAR16_V2DOUBLE

; CHAR16_V2DOUBLE: .function "test_char16_v2double_1"
; CHAR16_V2DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; CHAR16_V2DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; CHAR16_V2DOUBLE-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; CHAR16_V2DOUBLE-DAG: .decl [[INPUT]] v_type=G type=b num_elts=256 align=wordx32

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
; SHORT_V2I8-DAG: .decl [[OUTPUT]] v_type=G type=ub num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT_V2I8-DAG: .decl [[INPUT]] v_type=G type=w num_elts=16 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/i16/g' -e 's/OUTPUT_TYPE/half/g' \
; RUN:   -e 's/TEST_NAME/test_short_half/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELs/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=SHORT_HALF

; SHORT_HALF: .function "test_short_half_1"
; SHORT_HALF: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; SHORT_HALF-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT_HALF-DAG: .decl [[INPUT]] v_type=G type=w num_elts=16 align=wordx32

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
; SHORT2_V4I8-DAG: .decl [[OUTPUT]] v_type=G type=ub num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
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
; SHORT2_I32-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT2_I32-DAG: .decl [[INPUT]] v_type=G type=w num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x i16>/g' -e 's/OUTPUT_TYPE/<2 x half>/g' \
; RUN:   -e 's/TEST_NAME/test_short2_v2half/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_s/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=SHORT2_V2HALF

; SHORT2_V2HALF: .function "test_short2_v2half_1"
; SHORT2_V2HALF: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; SHORT2_V2HALF: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; SHORT2_V2HALF-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT2_V2HALF-DAG: .decl [[INPUT]] v_type=G type=w num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x i16>/g' -e 's/OUTPUT_TYPE/float/g' \
; RUN:   -e 's/TEST_NAME/test_short2_float/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_s/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=SHORT2_FLOAT

; SHORT2_FLOAT: .function "test_short2_float_1"
; SHORT2_FLOAT: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; SHORT2_FLOAT-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT2_FLOAT-DAG: .decl [[INPUT]] v_type=G type=w num_elts=32 align=wordx32

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
; SHORT4_V8I8-DAG: .decl [[OUTPUT]] v_type=G type=ub num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
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
; SHORT4_V2I32-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
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
; SHORT4_I64-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT4_I64-DAG: .decl [[INPUT]] v_type=G type=w num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x i16>/g' -e 's/OUTPUT_TYPE/<4 x half>/g' \
; RUN:   -e 's/TEST_NAME/test_short4_v4half/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_s/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=SHORT4_V4HALF

; SHORT4_V4HALF: .function "test_short4_v4half_1"
; SHORT4_V4HALF: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; SHORT4_V4HALF: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; SHORT4_V4HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; SHORT4_V4HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; SHORT4_V4HALF-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT4_V4HALF-DAG: .decl [[INPUT]] v_type=G type=w num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x i16>/g' -e 's/OUTPUT_TYPE/<2 x float>/g' \
; RUN:   -e 's/TEST_NAME/test_short4_v2float/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_s/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=SHORT4_V2FLOAT

; SHORT4_V2FLOAT: .function "test_short4_v2float_1"
; SHORT4_V2FLOAT: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; SHORT4_V2FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; SHORT4_V2FLOAT-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT4_V2FLOAT-DAG: .decl [[INPUT]] v_type=G type=w num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x i16>/g' -e 's/OUTPUT_TYPE/double/g' \
; RUN:   -e 's/TEST_NAME/test_short4_double/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_s/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=SHORT4_DOUBLE

; SHORT4_DOUBLE: .function "test_short4_double_1"
; SHORT4_DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; SHORT4_DOUBLE-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT4_DOUBLE-DAG: .decl [[INPUT]] v_type=G type=w num_elts=64 align=wordx32

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
; SHORT8_V16I8-DAG: .decl [[OUTPUT]] v_type=G type=ub num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
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
; SHORT8_V4I32-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
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
; SHORT8_V2I64-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT8_V2I64-DAG: .decl [[INPUT]] v_type=G type=w num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x i16>/g' -e 's/OUTPUT_TYPE/<8 x half>/g' \
; RUN:   -e 's/TEST_NAME/test_short8_v8half/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_s/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=SHORT8_V8HALF

; SHORT8_V8HALF: .function "test_short8_v8half_1"
; SHORT8_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; SHORT8_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; SHORT8_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; SHORT8_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; SHORT8_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; SHORT8_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; SHORT8_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; SHORT8_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; SHORT8_V8HALF-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT8_V8HALF-DAG: .decl [[INPUT]] v_type=G type=w num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x i16>/g' -e 's/OUTPUT_TYPE/<4 x float>/g' \
; RUN:   -e 's/TEST_NAME/test_short8_v4float/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_s/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=SHORT8_V4FLOAT

; SHORT8_V4FLOAT: .function "test_short8_v4float_1"
; SHORT8_V4FLOAT: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; SHORT8_V4FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; SHORT8_V4FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; SHORT8_V4FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; SHORT8_V4FLOAT-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT8_V4FLOAT-DAG: .decl [[INPUT]] v_type=G type=w num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x i16>/g' -e 's/OUTPUT_TYPE/<2 x double>/g' \
; RUN:   -e 's/TEST_NAME/test_short8_v2double/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_s/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=SHORT8_V2DOUBLE

; SHORT8_V2DOUBLE: .function "test_short8_v2double_1"
; SHORT8_V2DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; SHORT8_V2DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; SHORT8_V2DOUBLE-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT8_V2DOUBLE-DAG: .decl [[INPUT]] v_type=G type=w num_elts=128 align=wordx32

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
; SHORT16_V8I32-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
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
; SHORT16_V4I64-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT16_V4I64-DAG: .decl [[INPUT]] v_type=G type=w num_elts=256 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<16 x i16>/g' -e 's/OUTPUT_TYPE/<16 x half>/g' \
; RUN:   -e 's/TEST_NAME/test_short16_v16half/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv16_s/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=SHORT16_V16HALF

; SHORT16_V16HALF: .function "test_short16_v16half_1"
; SHORT16_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; SHORT16_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; SHORT16_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; SHORT16_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; SHORT16_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; SHORT16_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; SHORT16_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; SHORT16_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; SHORT16_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; SHORT16_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](4,16)<1;1,0>
; SHORT16_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; SHORT16_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](5,16)<1;1,0>
; SHORT16_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; SHORT16_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](6,16)<1;1,0>
; SHORT16_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; SHORT16_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](7,16)<1;1,0>
; SHORT16_V16HALF-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT16_V16HALF-DAG: .decl [[INPUT]] v_type=G type=w num_elts=256 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<16 x i16>/g' -e 's/OUTPUT_TYPE/<8 x float>/g' \
; RUN:   -e 's/TEST_NAME/test_short16_v8float/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv16_s/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=SHORT16_V8FLOAT

; SHORT16_V8FLOAT: .function "test_short16_v8float_1"
; SHORT16_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; SHORT16_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; SHORT16_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; SHORT16_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; SHORT16_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; SHORT16_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; SHORT16_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; SHORT16_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; SHORT16_V8FLOAT-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT16_V8FLOAT-DAG: .decl [[INPUT]] v_type=G type=w num_elts=256 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<16 x i16>/g' -e 's/OUTPUT_TYPE/<4 x double>/g' \
; RUN:   -e 's/TEST_NAME/test_short16_v4double/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv16_s/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=SHORT16_V4DOUBLE

; SHORT16_V4DOUBLE: .function "test_short16_v4double_1"
; SHORT16_V4DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; SHORT16_V4DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; SHORT16_V4DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; SHORT16_V4DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; SHORT16_V4DOUBLE-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; SHORT16_V4DOUBLE-DAG: .decl [[INPUT]] v_type=G type=w num_elts=256 align=wordx32

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
; INT_V4I8-DAG: .decl [[OUTPUT]] v_type=G type=ub num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
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
; INT_V2I16-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT_V2I16-DAG: .decl [[INPUT]] v_type=G type=d num_elts=16 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/i32/g' -e 's/OUTPUT_TYPE/<2 x half>/g' \
; RUN:   -e 's/TEST_NAME/test_int_v2half/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELi/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=INT_V2HALF

; INT_V2HALF: .function "test_int_v2half_1"
; INT_V2HALF: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; INT_V2HALF: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; INT_V2HALF-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT_V2HALF-DAG: .decl [[INPUT]] v_type=G type=d num_elts=16 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/i32/g' -e 's/OUTPUT_TYPE/float/g' \
; RUN:   -e 's/TEST_NAME/test_int_float/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELi/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=INT_FLOAT

; INT_FLOAT: .function "test_int_float_1"
; INT_FLOAT: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; INT_FLOAT-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT_FLOAT-DAG: .decl [[INPUT]] v_type=G type=d num_elts=16 align=wordx32

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
; INT2_V8I8-DAG: .decl [[OUTPUT]] v_type=G type=ub num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
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
; INT2_V4I16-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
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
; INT2_I64-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT2_I64-DAG: .decl [[INPUT]] v_type=G type=d num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x i32>/g' -e 's/OUTPUT_TYPE/<4 x half>/g' \
; RUN:   -e 's/TEST_NAME/test_int2_v4half/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_i/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=INT2_V4HALF

; INT2_V4HALF: .function "test_int2_v4half_1"
; INT2_V4HALF: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; INT2_V4HALF: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; INT2_V4HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; INT2_V4HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; INT2_V4HALF-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT2_V4HALF-DAG: .decl [[INPUT]] v_type=G type=d num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x i32>/g' -e 's/OUTPUT_TYPE/<2 x float>/g' \
; RUN:   -e 's/TEST_NAME/test_int2_v2float/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_i/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=INT2_V2FLOAT

; INT2_V2FLOAT: .function "test_int2_v2float_1"
; INT2_V2FLOAT: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; INT2_V2FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; INT2_V2FLOAT-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT2_V2FLOAT-DAG: .decl [[INPUT]] v_type=G type=d num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x i32>/g' -e 's/OUTPUT_TYPE/double/g' \
; RUN:   -e 's/TEST_NAME/test_int2_double/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_i/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=INT2_DOUBLE

; INT2_DOUBLE: .function "test_int2_double_1"
; INT2_DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; INT2_DOUBLE-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT2_DOUBLE-DAG: .decl [[INPUT]] v_type=G type=d num_elts=32 align=wordx32

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
; INT4_V16I8-DAG: .decl [[OUTPUT]] v_type=G type=ub num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
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
; INT4_V8I16-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
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
; INT4_V2I64-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT4_V2I64-DAG: .decl [[INPUT]] v_type=G type=d num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x i32>/g' -e 's/OUTPUT_TYPE/<8 x half>/g' \
; RUN:   -e 's/TEST_NAME/test_int4_v8half/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_i/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=INT4_V8HALF

; INT4_V8HALF: .function "test_int4_v8half_1"
; INT4_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; INT4_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; INT4_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; INT4_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; INT4_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; INT4_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; INT4_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; INT4_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; INT4_V8HALF-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT4_V8HALF-DAG: .decl [[INPUT]] v_type=G type=d num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x i32>/g' -e 's/OUTPUT_TYPE/<4 x float>/g' \
; RUN:   -e 's/TEST_NAME/test_int4_v4float/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_i/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=INT4_V4FLOAT

; INT4_V4FLOAT: .function "test_int4_v4float_1"
; INT4_V4FLOAT: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; INT4_V4FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; INT4_V4FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; INT4_V4FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; INT4_V4FLOAT-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT4_V4FLOAT-DAG: .decl [[INPUT]] v_type=G type=d num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x i32>/g' -e 's/OUTPUT_TYPE/<2 x double>/g' \
; RUN:   -e 's/TEST_NAME/test_int4_v2double/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_i/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=INT4_V2DOUBLE

; INT4_V2DOUBLE: .function "test_int4_v2double_1"
; INT4_V2DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; INT4_V2DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; INT4_V2DOUBLE-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT4_V2DOUBLE-DAG: .decl [[INPUT]] v_type=G type=d num_elts=64 align=wordx32

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
; INT8_V16I16-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
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
; INT8_V4I64-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT8_V4I64-DAG: .decl [[INPUT]] v_type=G type=d num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x i32>/g' -e 's/OUTPUT_TYPE/<16 x half>/g' \
; RUN:   -e 's/TEST_NAME/test_int8_v16half/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_i/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=INT8_V16HALF

; INT8_V16HALF: .function "test_int8_v16half_1"
; INT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; INT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; INT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; INT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; INT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; INT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; INT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; INT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; INT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; INT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](4,16)<1;1,0>
; INT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; INT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](5,16)<1;1,0>
; INT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; INT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](6,16)<1;1,0>
; INT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; INT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](7,16)<1;1,0>
; INT8_V16HALF-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT8_V16HALF-DAG: .decl [[INPUT]] v_type=G type=d num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x i32>/g' -e 's/OUTPUT_TYPE/<8 x float>/g' \
; RUN:   -e 's/TEST_NAME/test_int8_v8float/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_i/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=INT8_V8FLOAT

; INT8_V8FLOAT: .function "test_int8_v8float_1"
; INT8_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; INT8_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; INT8_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; INT8_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; INT8_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; INT8_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; INT8_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; INT8_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; INT8_V8FLOAT-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT8_V8FLOAT-DAG: .decl [[INPUT]] v_type=G type=d num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x i32>/g' -e 's/OUTPUT_TYPE/<4 x double>/g' \
; RUN:   -e 's/TEST_NAME/test_int8_v4double/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_i/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=INT8_V4DOUBLE

; INT8_V4DOUBLE: .function "test_int8_v4double_1"
; INT8_V4DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; INT8_V4DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; INT8_V4DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; INT8_V4DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; INT8_V4DOUBLE-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT8_V4DOUBLE-DAG: .decl [[INPUT]] v_type=G type=d num_elts=128 align=wordx32

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
; INT16_V8I64-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT16_V8I64-DAG: .decl [[INPUT]] v_type=G type=d num_elts=256 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<16 x i32>/g' -e 's/OUTPUT_TYPE/<16 x float>/g' \
; RUN:   -e 's/TEST_NAME/test_int16_v16float/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv16_i/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=INT16_V16FLOAT

; INT16_V16FLOAT: .function "test_int16_v16float_1"
; INT16_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; INT16_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; INT16_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; INT16_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; INT16_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; INT16_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; INT16_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; INT16_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; INT16_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](8,0)<1;1,0>
; INT16_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](9,0)<1;1,0>
; INT16_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](10,0)<1;1,0>
; INT16_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](11,0)<1;1,0>
; INT16_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](12,0)<1;1,0>
; INT16_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](13,0)<1;1,0>
; INT16_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](14,0)<1;1,0>
; INT16_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](15,0)<1;1,0>
; INT16_V16FLOAT-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT16_V16FLOAT-DAG: .decl [[INPUT]] v_type=G type=d num_elts=256 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<16 x i32>/g' -e 's/OUTPUT_TYPE/<8 x double>/g' \
; RUN:   -e 's/TEST_NAME/test_int16_v8double/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv16_i/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=INT16_V8DOUBLE

; INT16_V8DOUBLE: .function "test_int16_v8double_1"
; INT16_V8DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; INT16_V8DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; INT16_V8DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; INT16_V8DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; INT16_V8DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](8,0)<1;1,0>
; INT16_V8DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](10,0)<1;1,0>
; INT16_V8DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](12,0)<1;1,0>
; INT16_V8DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](14,0)<1;1,0>
; INT16_V8DOUBLE-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; INT16_V8DOUBLE-DAG: .decl [[INPUT]] v_type=G type=d num_elts=256 align=wordx32

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
; LONG_V8I8-DAG: .decl [[OUTPUT]] v_type=G type=ub num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
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
; LONG_V4I16-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
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
; LONG_V2I32-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG_V2I32-DAG: .decl [[INPUT]] v_type=G type=q num_elts=16 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/i64/g' -e 's/OUTPUT_TYPE/<4 x half>/g' \
; RUN:   -e 's/TEST_NAME/test_long_v4half/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELl/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=LONG_V4HALF

; LONG_V4HALF: .function "test_long_v4half_1"
; LONG_V4HALF: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; LONG_V4HALF: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; LONG_V4HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; LONG_V4HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; LONG_V4HALF-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG_V4HALF-DAG: .decl [[INPUT]] v_type=G type=q num_elts=16 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/i64/g' -e 's/OUTPUT_TYPE/<2 x float>/g' \
; RUN:   -e 's/TEST_NAME/test_long_v2float/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELl/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=LONG_V2FLOAT

; LONG_V2FLOAT: .function "test_long_v2float_1"
; LONG_V2FLOAT: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; LONG_V2FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; LONG_V2FLOAT-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG_V2FLOAT-DAG: .decl [[INPUT]] v_type=G type=q num_elts=16 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/i64/g' -e 's/OUTPUT_TYPE/double/g' \
; RUN:   -e 's/TEST_NAME/test_long_double/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELl/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=LONG_DOUBLE

; LONG_DOUBLE: .function "test_long_double_1"
; LONG_DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; LONG_DOUBLE-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG_DOUBLE-DAG: .decl [[INPUT]] v_type=G type=q num_elts=16 align=wordx32

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
; LONG2_V16I8-DAG: .decl [[OUTPUT]] v_type=G type=ub num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
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
; LONG2_V8I16-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
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
; LONG2_V4I32-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG2_V4I32-DAG: .decl [[INPUT]] v_type=G type=q num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x i64>/g' -e 's/OUTPUT_TYPE/<8 x half>/g' \
; RUN:   -e 's/TEST_NAME/test_long2_v8half/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_l/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=LONG2_V8HALF

; LONG2_V8HALF: .function "test_long2_v8half_1"
; LONG2_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; LONG2_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; LONG2_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; LONG2_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; LONG2_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; LONG2_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; LONG2_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; LONG2_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; LONG2_V8HALF-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG2_V8HALF-DAG: .decl [[INPUT]] v_type=G type=q num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x i64>/g' -e 's/OUTPUT_TYPE/<4 x float>/g' \
; RUN:   -e 's/TEST_NAME/test_long2_v4float/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_l/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=LONG2_V4FLOAT

; LONG2_V4FLOAT: .function "test_long2_v4float_1"
; LONG2_V4FLOAT: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; LONG2_V4FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; LONG2_V4FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; LONG2_V4FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; LONG2_V4FLOAT-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG2_V4FLOAT-DAG: .decl [[INPUT]] v_type=G type=q num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x i64>/g' -e 's/OUTPUT_TYPE/<2 x double>/g' \
; RUN:   -e 's/TEST_NAME/test_long2_v2double/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_l/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=LONG2_V2DOUBLE

; LONG2_V2DOUBLE: .function "test_long2_v2double_1"
; LONG2_V2DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; LONG2_V2DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; LONG2_V2DOUBLE-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG2_V2DOUBLE-DAG: .decl [[INPUT]] v_type=G type=q num_elts=32 align=wordx32

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
; LONG4_V16I16-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
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
; LONG4_V8I32-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG4_V8I32-DAG: .decl [[INPUT]] v_type=G type=q num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x i64>/g' -e 's/OUTPUT_TYPE/<16 x half>/g' \
; RUN:   -e 's/TEST_NAME/test_long4_v16half/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_l/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=LONG4_V16HALF

; LONG4_V16HALF: .function "test_long4_v16half_1"
; LONG4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; LONG4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; LONG4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; LONG4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; LONG4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; LONG4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; LONG4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; LONG4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; LONG4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; LONG4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](4,16)<1;1,0>
; LONG4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; LONG4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](5,16)<1;1,0>
; LONG4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; LONG4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](6,16)<1;1,0>
; LONG4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; LONG4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](7,16)<1;1,0>
; LONG4_V16HALF-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG4_V16HALF-DAG: .decl [[INPUT]] v_type=G type=q num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x i64>/g' -e 's/OUTPUT_TYPE/<8 x float>/g' \
; RUN:   -e 's/TEST_NAME/test_long4_v8float/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_l/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=LONG4_V8FLOAT

; LONG4_V8FLOAT: .function "test_long4_v8float_1"
; LONG4_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; LONG4_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; LONG4_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; LONG4_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; LONG4_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; LONG4_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; LONG4_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; LONG4_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; LONG4_V8FLOAT-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG4_V8FLOAT-DAG: .decl [[INPUT]] v_type=G type=q num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x i64>/g' -e 's/OUTPUT_TYPE/<4 x double>/g' \
; RUN:   -e 's/TEST_NAME/test_long4_v4double/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_l/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=LONG4_V4DOUBLE

; LONG4_V4DOUBLE: .function "test_long4_v4double_1"
; LONG4_V4DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; LONG4_V4DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; LONG4_V4DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; LONG4_V4DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; LONG4_V4DOUBLE-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG4_V4DOUBLE-DAG: .decl [[INPUT]] v_type=G type=q num_elts=64 align=wordx32

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
; LONG8_V16I32-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG8_V16I32-DAG: .decl [[INPUT]] v_type=G type=q num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x i64>/g' -e 's/OUTPUT_TYPE/<16 x float>/g' \
; RUN:   -e 's/TEST_NAME/test_long8_v16float/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_l/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=LONG8_V16FLOAT

; LONG8_V16FLOAT: .function "test_long8_v16float_1"
; LONG8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; LONG8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; LONG8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; LONG8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; LONG8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; LONG8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; LONG8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; LONG8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; LONG8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](8,0)<1;1,0>
; LONG8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](9,0)<1;1,0>
; LONG8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](10,0)<1;1,0>
; LONG8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](11,0)<1;1,0>
; LONG8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](12,0)<1;1,0>
; LONG8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](13,0)<1;1,0>
; LONG8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](14,0)<1;1,0>
; LONG8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](15,0)<1;1,0>
; LONG8_V16FLOAT-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG8_V16FLOAT-DAG: .decl [[INPUT]] v_type=G type=q num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x i64>/g' -e 's/OUTPUT_TYPE/<8 x double>/g' \
; RUN:   -e 's/TEST_NAME/test_long8_v8double/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_l/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=LONG8_V8DOUBLE

; LONG8_V8DOUBLE: .function "test_long8_v8double_1"
; LONG8_V8DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; LONG8_V8DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; LONG8_V8DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; LONG8_V8DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; LONG8_V8DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](8,0)<1;1,0>
; LONG8_V8DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](10,0)<1;1,0>
; LONG8_V8DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](12,0)<1;1,0>
; LONG8_V8DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](14,0)<1;1,0>
; LONG8_V8DOUBLE-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG8_V8DOUBLE-DAG: .decl [[INPUT]] v_type=G type=q num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<16 x i64>/g' -e 's/OUTPUT_TYPE/<16 x double>/g' \
; RUN:   -e 's/TEST_NAME/test_long16_v16double/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv16_l/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=LONG16_V16DOUBLE

; LONG16_V16DOUBLE: .function "test_long16_v16double_1"
; LONG16_V16DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; LONG16_V16DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; LONG16_V16DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; LONG16_V16DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; LONG16_V16DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](8,0)<1;1,0>
; LONG16_V16DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](10,0)<1;1,0>
; LONG16_V16DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](12,0)<1;1,0>
; LONG16_V16DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](14,0)<1;1,0>
; LONG16_V16DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](16,0)<1;1,0>
; LONG16_V16DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](18,0)<1;1,0>
; LONG16_V16DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](20,0)<1;1,0>
; LONG16_V16DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](22,0)<1;1,0>
; LONG16_V16DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](24,0)<1;1,0>
; LONG16_V16DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](26,0)<1;1,0>
; LONG16_V16DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](28,0)<1;1,0>
; LONG16_V16DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](30,0)<1;1,0>
; LONG16_V16DOUBLE-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; LONG16_V16DOUBLE-DAG: .decl [[INPUT]] v_type=G type=q num_elts=256 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/half/g' -e 's/OUTPUT_TYPE/<2 x i8>/g' \
; RUN:   -e 's/TEST_NAME/test_half_v2i8/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDh/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=HALF_V2I8

; HALF_V2I8: .function "test_half_v2i8_1"
; HALF_V2I8: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; HALF_V2I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; HALF_V2I8-DAG: .decl [[OUTPUT]] v_type=G type=ub num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; HALF_V2I8-DAG: .decl [[INPUT]] v_type=G type=hf num_elts=16 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/half/g' -e 's/OUTPUT_TYPE/i16/g' \
; RUN:   -e 's/TEST_NAME/test_half_i16/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDh/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=HALF_I16

; HALF_I16: .function "test_half_i16_1"
; HALF_I16: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; HALF_I16-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; HALF_I16-DAG: .decl [[INPUT]] v_type=G type=hf num_elts=16 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x half>/g' -e 's/OUTPUT_TYPE/<4 x i8>/g' \
; RUN:   -e 's/TEST_NAME/test_half2_v4i8/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_Dh/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=HALF2_V4I8

; HALF2_V4I8: .function "test_half2_v4i8_1"
; HALF2_V4I8: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; HALF2_V4I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; HALF2_V4I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,32)<1;1,0>
; HALF2_V4I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,48)<1;1,0>
; HALF2_V4I8-DAG: .decl [[OUTPUT]] v_type=G type=ub num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; HALF2_V4I8-DAG: .decl [[INPUT]] v_type=G type=hf num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x half>/g' -e 's/OUTPUT_TYPE/<2 x i16>/g' \
; RUN:   -e 's/TEST_NAME/test_half2_v2i16/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_Dh/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=HALF2_V2I16

; HALF2_V2I16: .function "test_half2_v2i16_1"
; HALF2_V2I16: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; HALF2_V2I16: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; HALF2_V2I16-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; HALF2_V2I16-DAG: .decl [[INPUT]] v_type=G type=hf num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x half>/g' -e 's/OUTPUT_TYPE/i32/g' \
; RUN:   -e 's/TEST_NAME/test_half2_i32/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_Dh/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=HALF2_I32

; HALF2_I32: .function "test_half2_i32_1"
; HALF2_I32: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; HALF2_I32-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; HALF2_I32-DAG: .decl [[INPUT]] v_type=G type=hf num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x half>/g' -e 's/OUTPUT_TYPE/float/g' \
; RUN:   -e 's/TEST_NAME/test_half2_float/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_Dh/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=HALF2_FLOAT

; HALF2_FLOAT: .function "test_half2_float_1"
; HALF2_FLOAT: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; HALF2_FLOAT-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; HALF2_FLOAT-DAG: .decl [[INPUT]] v_type=G type=hf num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x half>/g' -e 's/OUTPUT_TYPE/<8 x i8>/g' \
; RUN:   -e 's/TEST_NAME/test_half4_v8i8/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_Dh/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=HALF4_V8I8

; HALF4_V8I8: .function "test_half4_v8i8_1"
; HALF4_V8I8: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; HALF4_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; HALF4_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,32)<1;1,0>
; HALF4_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,48)<1;1,0>
; HALF4_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; HALF4_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; HALF4_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,32)<1;1,0>
; HALF4_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,48)<1;1,0>
; HALF4_V8I8-DAG: .decl [[OUTPUT]] v_type=G type=ub num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; HALF4_V8I8-DAG: .decl [[INPUT]] v_type=G type=hf num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x half>/g' -e 's/OUTPUT_TYPE/<4 x i16>/g' \
; RUN:   -e 's/TEST_NAME/test_half4_v4i16/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_Dh/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=HALF4_V4I16

; HALF4_V4I16: .function "test_half4_v4i16_1"
; HALF4_V4I16: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; HALF4_V4I16: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; HALF4_V4I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; HALF4_V4I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; HALF4_V4I16-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; HALF4_V4I16-DAG: .decl [[INPUT]] v_type=G type=hf num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x half>/g' -e 's/OUTPUT_TYPE/<2 x i32>/g' \
; RUN:   -e 's/TEST_NAME/test_half4_v2i32/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_Dh/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=HALF4_V2I32

; HALF4_V2I32: .function "test_half4_v2i32_1"
; HALF4_V2I32: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; HALF4_V2I32: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; HALF4_V2I32-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; HALF4_V2I32-DAG: .decl [[INPUT]] v_type=G type=hf num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x half>/g' -e 's/OUTPUT_TYPE/i64/g' \
; RUN:   -e 's/TEST_NAME/test_half4_i64/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_Dh/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=HALF4_I64

; HALF4_I64: .function "test_half4_i64_1"
; HALF4_I64: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; HALF4_I64-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; HALF4_I64-DAG: .decl [[INPUT]] v_type=G type=hf num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x half>/g' -e 's/OUTPUT_TYPE/<2 x float>/g' \
; RUN:   -e 's/TEST_NAME/test_half4_v2float/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_Dh/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=HALF4_V2FLOAT

; HALF4_V2FLOAT: .function "test_half4_v2float_1"
; HALF4_V2FLOAT: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; HALF4_V2FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; HALF4_V2FLOAT-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; HALF4_V2FLOAT-DAG: .decl [[INPUT]] v_type=G type=hf num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x half>/g' -e 's/OUTPUT_TYPE/double/g' \
; RUN:   -e 's/TEST_NAME/test_half4_double/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_Dh/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=HALF4_DOUBLE

; HALF4_DOUBLE: .function "test_half4_double_1"
; HALF4_DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; HALF4_DOUBLE-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; HALF4_DOUBLE-DAG: .decl [[INPUT]] v_type=G type=hf num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x half>/g' -e 's/OUTPUT_TYPE/<16 x i8>/g' \
; RUN:   -e 's/TEST_NAME/test_half8_v16i8/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_Dh/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=HALF8_V16I8

; HALF8_V16I8: .function "test_half8_v16i8_1"
; HALF8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; HALF8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; HALF8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,32)<1;1,0>
; HALF8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,48)<1;1,0>
; HALF8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; HALF8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; HALF8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,32)<1;1,0>
; HALF8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,48)<1;1,0>
; HALF8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; HALF8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; HALF8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](2,32)<1;1,0>
; HALF8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](2,48)<1;1,0>
; HALF8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; HALF8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; HALF8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](3,32)<1;1,0>
; HALF8_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](3,48)<1;1,0>
; HALF8_V16I8-DAG: .decl [[OUTPUT]] v_type=G type=ub num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; HALF8_V16I8-DAG: .decl [[INPUT]] v_type=G type=hf num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x half>/g' -e 's/OUTPUT_TYPE/<8 x i16>/g' \
; RUN:   -e 's/TEST_NAME/test_half8_v8i16/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_Dh/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=HALF8_V8I16

; HALF8_V8I16: .function "test_half8_v8i16_1"
; HALF8_V8I16: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; HALF8_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; HALF8_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; HALF8_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; HALF8_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; HALF8_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; HALF8_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; HALF8_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; HALF8_V8I16-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; HALF8_V8I16-DAG: .decl [[INPUT]] v_type=G type=hf num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x half>/g' -e 's/OUTPUT_TYPE/<4 x i32>/g' \
; RUN:   -e 's/TEST_NAME/test_half8_v4i32/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_Dh/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=HALF8_V4I32

; HALF8_V4I32: .function "test_half8_v4i32_1"
; HALF8_V4I32: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; HALF8_V4I32: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; HALF8_V4I32: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; HALF8_V4I32: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; HALF8_V4I32-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; HALF8_V4I32-DAG: .decl [[INPUT]] v_type=G type=hf num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x half>/g' -e 's/OUTPUT_TYPE/<2 x i64>/g' \
; RUN:   -e 's/TEST_NAME/test_half8_v2i64/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_Dh/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=HALF8_V2I64

; HALF8_V2I64: .function "test_half8_v2i64_1"
; HALF8_V2I64: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; HALF8_V2I64: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; HALF8_V2I64-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; HALF8_V2I64-DAG: .decl [[INPUT]] v_type=G type=hf num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x half>/g' -e 's/OUTPUT_TYPE/<4 x float>/g' \
; RUN:   -e 's/TEST_NAME/test_half8_v4float/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_Dh/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=HALF8_V4FLOAT

; HALF8_V4FLOAT: .function "test_half8_v4float_1"
; HALF8_V4FLOAT: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; HALF8_V4FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; HALF8_V4FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; HALF8_V4FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; HALF8_V4FLOAT-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; HALF8_V4FLOAT-DAG: .decl [[INPUT]] v_type=G type=hf num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x half>/g' -e 's/OUTPUT_TYPE/<2 x double>/g' \
; RUN:   -e 's/TEST_NAME/test_half8_v2double/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_Dh/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=HALF8_V2DOUBLE

; HALF8_V2DOUBLE: .function "test_half8_v2double_1"
; HALF8_V2DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; HALF8_V2DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; HALF8_V2DOUBLE-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; HALF8_V2DOUBLE-DAG: .decl [[INPUT]] v_type=G type=hf num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<16 x half>/g' -e 's/OUTPUT_TYPE/<16 x i16>/g' \
; RUN:   -e 's/TEST_NAME/test_half16_v16i16/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv16_Dh/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=HALF16_V16I16

; HALF16_V16I16: .function "test_half16_v16i16_1"
; HALF16_V16I16: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; HALF16_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; HALF16_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; HALF16_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; HALF16_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; HALF16_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; HALF16_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; HALF16_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; HALF16_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; HALF16_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](4,16)<1;1,0>
; HALF16_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; HALF16_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](5,16)<1;1,0>
; HALF16_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; HALF16_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](6,16)<1;1,0>
; HALF16_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; HALF16_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](7,16)<1;1,0>
; HALF16_V16I16-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; HALF16_V16I16-DAG: .decl [[INPUT]] v_type=G type=hf num_elts=256 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<16 x half>/g' -e 's/OUTPUT_TYPE/<8 x i32>/g' \
; RUN:   -e 's/TEST_NAME/test_half16_v8i32/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv16_Dh/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=HALF16_V8I32

; HALF16_V8I32: .function "test_half16_v8i32_1"
; HALF16_V8I32: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; HALF16_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; HALF16_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; HALF16_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; HALF16_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; HALF16_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; HALF16_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; HALF16_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; HALF16_V8I32-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; HALF16_V8I32-DAG: .decl [[INPUT]] v_type=G type=hf num_elts=256 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<16 x half>/g' -e 's/OUTPUT_TYPE/<4 x i64>/g' \
; RUN:   -e 's/TEST_NAME/test_half16_v4i64/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv16_Dh/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=HALF16_V4I64

; HALF16_V4I64: .function "test_half16_v4i64_1"
; HALF16_V4I64: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; HALF16_V4I64: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; HALF16_V4I64: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; HALF16_V4I64: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; HALF16_V4I64-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; HALF16_V4I64-DAG: .decl [[INPUT]] v_type=G type=hf num_elts=256 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<16 x half>/g' -e 's/OUTPUT_TYPE/<8 x float>/g' \
; RUN:   -e 's/TEST_NAME/test_half16_v8float/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv16_Dh/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=HALF16_V8FLOAT

; HALF16_V8FLOAT: .function "test_half16_v8float_1"
; HALF16_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; HALF16_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; HALF16_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; HALF16_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; HALF16_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; HALF16_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; HALF16_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; HALF16_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; HALF16_V8FLOAT-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; HALF16_V8FLOAT-DAG: .decl [[INPUT]] v_type=G type=hf num_elts=256 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<16 x half>/g' -e 's/OUTPUT_TYPE/<4 x double>/g' \
; RUN:   -e 's/TEST_NAME/test_half16_v4double/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv16_Dh/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=HALF16_V4DOUBLE

; HALF16_V4DOUBLE: .function "test_half16_v4double_1"
; HALF16_V4DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; HALF16_V4DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; HALF16_V4DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; HALF16_V4DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; HALF16_V4DOUBLE-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; HALF16_V4DOUBLE-DAG: .decl [[INPUT]] v_type=G type=hf num_elts=256 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/float/g' -e 's/OUTPUT_TYPE/<4 x i8>/g' \
; RUN:   -e 's/TEST_NAME/test_float_v4i8/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELf/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=FLOAT_V4I8

; FLOAT_V4I8: .function "test_float_v4i8_1"
; FLOAT_V4I8: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; FLOAT_V4I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; FLOAT_V4I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,32)<1;1,0>
; FLOAT_V4I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,48)<1;1,0>
; FLOAT_V4I8-DAG: .decl [[OUTPUT]] v_type=G type=ub num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; FLOAT_V4I8-DAG: .decl [[INPUT]] v_type=G type=f num_elts=16 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/float/g' -e 's/OUTPUT_TYPE/<2 x i16>/g' \
; RUN:   -e 's/TEST_NAME/test_float_v2i16/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELf/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=FLOAT_V2I16

; FLOAT_V2I16: .function "test_float_v2i16_1"
; FLOAT_V2I16: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; FLOAT_V2I16: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; FLOAT_V2I16-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; FLOAT_V2I16-DAG: .decl [[INPUT]] v_type=G type=f num_elts=16 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/float/g' -e 's/OUTPUT_TYPE/i32/g' \
; RUN:   -e 's/TEST_NAME/test_float_i32/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELf/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=FLOAT_I32

; FLOAT_I32: .function "test_float_i32_1"
; FLOAT_I32: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; FLOAT_I32-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; FLOAT_I32-DAG: .decl [[INPUT]] v_type=G type=f num_elts=16 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/float/g' -e 's/OUTPUT_TYPE/<2 x half>/g' \
; RUN:   -e 's/TEST_NAME/test_float_v2half/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELf/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=FLOAT_V2HALF

; FLOAT_V2HALF: .function "test_float_v2half_1"
; FLOAT_V2HALF: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; FLOAT_V2HALF: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; FLOAT_V2HALF-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; FLOAT_V2HALF-DAG: .decl [[INPUT]] v_type=G type=f num_elts=16 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x float>/g' -e 's/OUTPUT_TYPE/<8 x i8>/g' \
; RUN:   -e 's/TEST_NAME/test_float2_v8i8/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_f/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=FLOAT2_V8I8

; FLOAT2_V8I8: .function "test_float2_v8i8_1"
; FLOAT2_V8I8: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; FLOAT2_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; FLOAT2_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,32)<1;1,0>
; FLOAT2_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,48)<1;1,0>
; FLOAT2_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; FLOAT2_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; FLOAT2_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,32)<1;1,0>
; FLOAT2_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,48)<1;1,0>
; FLOAT2_V8I8-DAG: .decl [[OUTPUT]] v_type=G type=ub num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; FLOAT2_V8I8-DAG: .decl [[INPUT]] v_type=G type=f num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x float>/g' -e 's/OUTPUT_TYPE/<4 x i16>/g' \
; RUN:   -e 's/TEST_NAME/test_float2_v4i16/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_f/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=FLOAT2_V4I16

; FLOAT2_V4I16: .function "test_float2_v4i16_1"
; FLOAT2_V4I16: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; FLOAT2_V4I16: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; FLOAT2_V4I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; FLOAT2_V4I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; FLOAT2_V4I16-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; FLOAT2_V4I16-DAG: .decl [[INPUT]] v_type=G type=f num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x float>/g' -e 's/OUTPUT_TYPE/<2 x i32>/g' \
; RUN:   -e 's/TEST_NAME/test_float2_v2i32/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_f/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=FLOAT2_V2I32

; FLOAT2_V2I32: .function "test_float2_v2i32_1"
; FLOAT2_V2I32: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; FLOAT2_V2I32: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; FLOAT2_V2I32-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; FLOAT2_V2I32-DAG: .decl [[INPUT]] v_type=G type=f num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x float>/g' -e 's/OUTPUT_TYPE/i64/g' \
; RUN:   -e 's/TEST_NAME/test_float2_i64/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_f/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=FLOAT2_I64

; FLOAT2_I64: .function "test_float2_i64_1"
; FLOAT2_I64: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; FLOAT2_I64-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; FLOAT2_I64-DAG: .decl [[INPUT]] v_type=G type=f num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x float>/g' -e 's/OUTPUT_TYPE/<4 x half>/g' \
; RUN:   -e 's/TEST_NAME/test_float2_v4half/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_f/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=FLOAT2_V4HALF

; FLOAT2_V4HALF: .function "test_float2_v4half_1"
; FLOAT2_V4HALF: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; FLOAT2_V4HALF: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; FLOAT2_V4HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; FLOAT2_V4HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; FLOAT2_V4HALF-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; FLOAT2_V4HALF-DAG: .decl [[INPUT]] v_type=G type=f num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x float>/g' -e 's/OUTPUT_TYPE/double/g' \
; RUN:   -e 's/TEST_NAME/test_float2_double/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_f/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=FLOAT2_DOUBLE

; FLOAT2_DOUBLE: .function "test_float2_double_1"
; FLOAT2_DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; FLOAT2_DOUBLE-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; FLOAT2_DOUBLE-DAG: .decl [[INPUT]] v_type=G type=f num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x float>/g' -e 's/OUTPUT_TYPE/<16 x i8>/g' \
; RUN:   -e 's/TEST_NAME/test_float4_v16i8/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_f/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=FLOAT4_V16I8

; FLOAT4_V16I8: .function "test_float4_v16i8_1"
; FLOAT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; FLOAT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; FLOAT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,32)<1;1,0>
; FLOAT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,48)<1;1,0>
; FLOAT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; FLOAT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; FLOAT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,32)<1;1,0>
; FLOAT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,48)<1;1,0>
; FLOAT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; FLOAT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; FLOAT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](2,32)<1;1,0>
; FLOAT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](2,48)<1;1,0>
; FLOAT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; FLOAT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; FLOAT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](3,32)<1;1,0>
; FLOAT4_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](3,48)<1;1,0>
; FLOAT4_V16I8-DAG: .decl [[OUTPUT]] v_type=G type=ub num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; FLOAT4_V16I8-DAG: .decl [[INPUT]] v_type=G type=f num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x float>/g' -e 's/OUTPUT_TYPE/<8 x i16>/g' \
; RUN:   -e 's/TEST_NAME/test_float4_v8i16/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_f/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=FLOAT4_V8I16

; FLOAT4_V8I16: .function "test_float4_v8i16_1"
; FLOAT4_V8I16: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; FLOAT4_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; FLOAT4_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; FLOAT4_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; FLOAT4_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; FLOAT4_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; FLOAT4_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; FLOAT4_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; FLOAT4_V8I16-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; FLOAT4_V8I16-DAG: .decl [[INPUT]] v_type=G type=f num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x float>/g' -e 's/OUTPUT_TYPE/<4 x i32>/g' \
; RUN:   -e 's/TEST_NAME/test_float4_v4i32/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_f/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=FLOAT4_V4I32

; FLOAT4_V4I32: .function "test_float4_v4i32_1"
; FLOAT4_V4I32: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; FLOAT4_V4I32: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; FLOAT4_V4I32: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; FLOAT4_V4I32: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; FLOAT4_V4I32-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; FLOAT4_V4I32-DAG: .decl [[INPUT]] v_type=G type=f num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x float>/g' -e 's/OUTPUT_TYPE/<2 x i64>/g' \
; RUN:   -e 's/TEST_NAME/test_float4_v2i64/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_f/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=FLOAT4_V2I64

; FLOAT4_V2I64: .function "test_float4_v2i64_1"
; FLOAT4_V2I64: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; FLOAT4_V2I64: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; FLOAT4_V2I64-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; FLOAT4_V2I64-DAG: .decl [[INPUT]] v_type=G type=f num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x float>/g' -e 's/OUTPUT_TYPE/<8 x half>/g' \
; RUN:   -e 's/TEST_NAME/test_float4_v8half/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_f/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=FLOAT4_V8HALF

; FLOAT4_V8HALF: .function "test_float4_v8half_1"
; FLOAT4_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; FLOAT4_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; FLOAT4_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; FLOAT4_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; FLOAT4_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; FLOAT4_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; FLOAT4_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; FLOAT4_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; FLOAT4_V8HALF-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; FLOAT4_V8HALF-DAG: .decl [[INPUT]] v_type=G type=f num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x float>/g' -e 's/OUTPUT_TYPE/<2 x double>/g' \
; RUN:   -e 's/TEST_NAME/test_float4_v2double/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_f/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=FLOAT4_V2DOUBLE

; FLOAT4_V2DOUBLE: .function "test_float4_v2double_1"
; FLOAT4_V2DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; FLOAT4_V2DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; FLOAT4_V2DOUBLE-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; FLOAT4_V2DOUBLE-DAG: .decl [[INPUT]] v_type=G type=f num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x float>/g' -e 's/OUTPUT_TYPE/<16 x i16>/g' \
; RUN:   -e 's/TEST_NAME/test_float8_v16i16/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_f/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=FLOAT8_V16I16

; FLOAT8_V16I16: .function "test_float8_v16i16_1"
; FLOAT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; FLOAT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; FLOAT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; FLOAT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; FLOAT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; FLOAT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; FLOAT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; FLOAT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; FLOAT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; FLOAT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](4,16)<1;1,0>
; FLOAT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; FLOAT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](5,16)<1;1,0>
; FLOAT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; FLOAT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](6,16)<1;1,0>
; FLOAT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; FLOAT8_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](7,16)<1;1,0>
; FLOAT8_V16I16-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; FLOAT8_V16I16-DAG: .decl [[INPUT]] v_type=G type=f num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x float>/g' -e 's/OUTPUT_TYPE/<8 x i32>/g' \
; RUN:   -e 's/TEST_NAME/test_float8_v8i32/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_f/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=FLOAT8_V8I32

; FLOAT8_V8I32: .function "test_float8_v8i32_1"
; FLOAT8_V8I32: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; FLOAT8_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; FLOAT8_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; FLOAT8_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; FLOAT8_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; FLOAT8_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; FLOAT8_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; FLOAT8_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; FLOAT8_V8I32-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; FLOAT8_V8I32-DAG: .decl [[INPUT]] v_type=G type=f num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x float>/g' -e 's/OUTPUT_TYPE/<4 x i64>/g' \
; RUN:   -e 's/TEST_NAME/test_float8_v4i64/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_f/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=FLOAT8_V4I64

; FLOAT8_V4I64: .function "test_float8_v4i64_1"
; FLOAT8_V4I64: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; FLOAT8_V4I64: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; FLOAT8_V4I64: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; FLOAT8_V4I64: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; FLOAT8_V4I64-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; FLOAT8_V4I64-DAG: .decl [[INPUT]] v_type=G type=f num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x float>/g' -e 's/OUTPUT_TYPE/<16 x half>/g' \
; RUN:   -e 's/TEST_NAME/test_float8_v16half/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_f/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=FLOAT8_V16HALF

; FLOAT8_V16HALF: .function "test_float8_v16half_1"
; FLOAT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; FLOAT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; FLOAT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; FLOAT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; FLOAT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; FLOAT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; FLOAT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; FLOAT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; FLOAT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; FLOAT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](4,16)<1;1,0>
; FLOAT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; FLOAT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](5,16)<1;1,0>
; FLOAT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; FLOAT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](6,16)<1;1,0>
; FLOAT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; FLOAT8_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](7,16)<1;1,0>
; FLOAT8_V16HALF-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; FLOAT8_V16HALF-DAG: .decl [[INPUT]] v_type=G type=f num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x float>/g' -e 's/OUTPUT_TYPE/<4 x double>/g' \
; RUN:   -e 's/TEST_NAME/test_float8_v4double/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_f/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=FLOAT8_V4DOUBLE

; FLOAT8_V4DOUBLE: .function "test_float8_v4double_1"
; FLOAT8_V4DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; FLOAT8_V4DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; FLOAT8_V4DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; FLOAT8_V4DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; FLOAT8_V4DOUBLE-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; FLOAT8_V4DOUBLE-DAG: .decl [[INPUT]] v_type=G type=f num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<16 x float>/g' -e 's/OUTPUT_TYPE/<16 x i32>/g' \
; RUN:   -e 's/TEST_NAME/test_float16_v16i32/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv16_f/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=FLOAT16_V16I32

; FLOAT16_V16I32: .function "test_float16_v16i32_1"
; FLOAT16_V16I32: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; FLOAT16_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; FLOAT16_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; FLOAT16_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; FLOAT16_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; FLOAT16_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; FLOAT16_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; FLOAT16_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; FLOAT16_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](8,0)<1;1,0>
; FLOAT16_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](9,0)<1;1,0>
; FLOAT16_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](10,0)<1;1,0>
; FLOAT16_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](11,0)<1;1,0>
; FLOAT16_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](12,0)<1;1,0>
; FLOAT16_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](13,0)<1;1,0>
; FLOAT16_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](14,0)<1;1,0>
; FLOAT16_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](15,0)<1;1,0>
; FLOAT16_V16I32-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; FLOAT16_V16I32-DAG: .decl [[INPUT]] v_type=G type=f num_elts=256 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<16 x float>/g' -e 's/OUTPUT_TYPE/<8 x i64>/g' \
; RUN:   -e 's/TEST_NAME/test_float16_v8i64/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv16_f/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=FLOAT16_V8I64

; FLOAT16_V8I64: .function "test_float16_v8i64_1"
; FLOAT16_V8I64: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; FLOAT16_V8I64: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; FLOAT16_V8I64: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; FLOAT16_V8I64: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; FLOAT16_V8I64: mov (M1, 16) {{.*}} [[OUTPUT]](8,0)<1;1,0>
; FLOAT16_V8I64: mov (M1, 16) {{.*}} [[OUTPUT]](10,0)<1;1,0>
; FLOAT16_V8I64: mov (M1, 16) {{.*}} [[OUTPUT]](12,0)<1;1,0>
; FLOAT16_V8I64: mov (M1, 16) {{.*}} [[OUTPUT]](14,0)<1;1,0>
; FLOAT16_V8I64-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; FLOAT16_V8I64-DAG: .decl [[INPUT]] v_type=G type=f num_elts=256 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<16 x float>/g' -e 's/OUTPUT_TYPE/<8 x double>/g' \
; RUN:   -e 's/TEST_NAME/test_float16_v8double/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv16_f/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=FLOAT16_V8DOUBLE

; FLOAT16_V8DOUBLE: .function "test_float16_v8double_1"
; FLOAT16_V8DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; FLOAT16_V8DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; FLOAT16_V8DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; FLOAT16_V8DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; FLOAT16_V8DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](8,0)<1;1,0>
; FLOAT16_V8DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](10,0)<1;1,0>
; FLOAT16_V8DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](12,0)<1;1,0>
; FLOAT16_V8DOUBLE: mov (M1, 16) {{.*}} [[OUTPUT]](14,0)<1;1,0>
; FLOAT16_V8DOUBLE-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; FLOAT16_V8DOUBLE-DAG: .decl [[INPUT]] v_type=G type=f num_elts=256 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/double/g' -e 's/OUTPUT_TYPE/<8 x i8>/g' \
; RUN:   -e 's/TEST_NAME/test_double_v8i8/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELd/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=DOUBLE_V8I8

; DOUBLE_V8I8: .function "test_double_v8i8_1"
; DOUBLE_V8I8: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; DOUBLE_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; DOUBLE_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,32)<1;1,0>
; DOUBLE_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,48)<1;1,0>
; DOUBLE_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; DOUBLE_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; DOUBLE_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,32)<1;1,0>
; DOUBLE_V8I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,48)<1;1,0>
; DOUBLE_V8I8-DAG: .decl [[OUTPUT]] v_type=G type=ub num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; DOUBLE_V8I8-DAG: .decl [[INPUT]] v_type=G type=df num_elts=16 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/double/g' -e 's/OUTPUT_TYPE/<4 x i16>/g' \
; RUN:   -e 's/TEST_NAME/test_double_v4i16/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELd/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=DOUBLE_V4I16

; DOUBLE_V4I16: .function "test_double_v4i16_1"
; DOUBLE_V4I16: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; DOUBLE_V4I16: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; DOUBLE_V4I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; DOUBLE_V4I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; DOUBLE_V4I16-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; DOUBLE_V4I16-DAG: .decl [[INPUT]] v_type=G type=df num_elts=16 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/double/g' -e 's/OUTPUT_TYPE/<2 x i32>/g' \
; RUN:   -e 's/TEST_NAME/test_double_v2i32/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELd/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=DOUBLE_V2I32

; DOUBLE_V2I32: .function "test_double_v2i32_1"
; DOUBLE_V2I32: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; DOUBLE_V2I32: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; DOUBLE_V2I32-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; DOUBLE_V2I32-DAG: .decl [[INPUT]] v_type=G type=df num_elts=16 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/double/g' -e 's/OUTPUT_TYPE/i64/g' \
; RUN:   -e 's/TEST_NAME/test_double_i64/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELd/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=DOUBLE_I64

; DOUBLE_I64: .function "test_double_i64_1"
; DOUBLE_I64: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; DOUBLE_I64-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=16 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; DOUBLE_I64-DAG: .decl [[INPUT]] v_type=G type=df num_elts=16 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/double/g' -e 's/OUTPUT_TYPE/<4 x half>/g' \
; RUN:   -e 's/TEST_NAME/test_double_v4half/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELd/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=DOUBLE_V4HALF

; DOUBLE_V4HALF: .function "test_double_v4half_1"
; DOUBLE_V4HALF: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; DOUBLE_V4HALF: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; DOUBLE_V4HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; DOUBLE_V4HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; DOUBLE_V4HALF-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; DOUBLE_V4HALF-DAG: .decl [[INPUT]] v_type=G type=df num_elts=16 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/double/g' -e 's/OUTPUT_TYPE/<2 x float>/g' \
; RUN:   -e 's/TEST_NAME/test_double_v2float/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELd/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=DOUBLE_V2FLOAT

; DOUBLE_V2FLOAT: .function "test_double_v2float_1"
; DOUBLE_V2FLOAT: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; DOUBLE_V2FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; DOUBLE_V2FLOAT-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; DOUBLE_V2FLOAT-DAG: .decl [[INPUT]] v_type=G type=df num_elts=16 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x double>/g' -e 's/OUTPUT_TYPE/<16 x i8>/g' \
; RUN:   -e 's/TEST_NAME/test_double2_v16i8/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_d/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=DOUBLE2_V16I8

; DOUBLE2_V16I8: .function "test_double2_v16i8_1"
; DOUBLE2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; DOUBLE2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; DOUBLE2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,32)<1;1,0>
; DOUBLE2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](0,48)<1;1,0>
; DOUBLE2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; DOUBLE2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; DOUBLE2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,32)<1;1,0>
; DOUBLE2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](1,48)<1;1,0>
; DOUBLE2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; DOUBLE2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; DOUBLE2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](2,32)<1;1,0>
; DOUBLE2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](2,48)<1;1,0>
; DOUBLE2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; DOUBLE2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; DOUBLE2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](3,32)<1;1,0>
; DOUBLE2_V16I8: mov (M1, 16) {{.*}} [[OUTPUT]](3,48)<1;1,0>
; DOUBLE2_V16I8-DAG: .decl [[OUTPUT]] v_type=G type=ub num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; DOUBLE2_V16I8-DAG: .decl [[INPUT]] v_type=G type=df num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x double>/g' -e 's/OUTPUT_TYPE/<8 x i16>/g' \
; RUN:   -e 's/TEST_NAME/test_double2_v8i16/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_d/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=DOUBLE2_V8I16

; DOUBLE2_V8I16: .function "test_double2_v8i16_1"
; DOUBLE2_V8I16: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; DOUBLE2_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; DOUBLE2_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; DOUBLE2_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; DOUBLE2_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; DOUBLE2_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; DOUBLE2_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; DOUBLE2_V8I16: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; DOUBLE2_V8I16-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; DOUBLE2_V8I16-DAG: .decl [[INPUT]] v_type=G type=df num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x double>/g' -e 's/OUTPUT_TYPE/<4 x i32>/g' \
; RUN:   -e 's/TEST_NAME/test_double2_v4i32/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_d/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=DOUBLE2_V4I32

; DOUBLE2_V4I32: .function "test_double2_v4i32_1"
; DOUBLE2_V4I32: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; DOUBLE2_V4I32: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; DOUBLE2_V4I32: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; DOUBLE2_V4I32: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; DOUBLE2_V4I32-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; DOUBLE2_V4I32-DAG: .decl [[INPUT]] v_type=G type=df num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x double>/g' -e 's/OUTPUT_TYPE/<2 x i64>/g' \
; RUN:   -e 's/TEST_NAME/test_double2_v2i64/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_d/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=DOUBLE2_V2I64

; DOUBLE2_V2I64: .function "test_double2_v2i64_1"
; DOUBLE2_V2I64: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; DOUBLE2_V2I64: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; DOUBLE2_V2I64-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=32 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; DOUBLE2_V2I64-DAG: .decl [[INPUT]] v_type=G type=df num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x double>/g' -e 's/OUTPUT_TYPE/<8 x half>/g' \
; RUN:   -e 's/TEST_NAME/test_double2_v8half/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_d/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=DOUBLE2_V8HALF

; DOUBLE2_V8HALF: .function "test_double2_v8half_1"
; DOUBLE2_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; DOUBLE2_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; DOUBLE2_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; DOUBLE2_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; DOUBLE2_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; DOUBLE2_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; DOUBLE2_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; DOUBLE2_V8HALF: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; DOUBLE2_V8HALF-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; DOUBLE2_V8HALF-DAG: .decl [[INPUT]] v_type=G type=df num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<2 x double>/g' -e 's/OUTPUT_TYPE/<4 x float>/g' \
; RUN:   -e 's/TEST_NAME/test_double2_v4float/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv2_d/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=DOUBLE2_V4FLOAT

; DOUBLE2_V4FLOAT: .function "test_double2_v4float_1"
; DOUBLE2_V4FLOAT: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; DOUBLE2_V4FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; DOUBLE2_V4FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; DOUBLE2_V4FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; DOUBLE2_V4FLOAT-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; DOUBLE2_V4FLOAT-DAG: .decl [[INPUT]] v_type=G type=df num_elts=32 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x double>/g' -e 's/OUTPUT_TYPE/<16 x i16>/g' \
; RUN:   -e 's/TEST_NAME/test_double4_v16i16/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_d/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=DOUBLE4_V16I16

; DOUBLE4_V16I16: .function "test_double4_v16i16_1"
; DOUBLE4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; DOUBLE4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; DOUBLE4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; DOUBLE4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; DOUBLE4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; DOUBLE4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; DOUBLE4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; DOUBLE4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; DOUBLE4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; DOUBLE4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](4,16)<1;1,0>
; DOUBLE4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; DOUBLE4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](5,16)<1;1,0>
; DOUBLE4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; DOUBLE4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](6,16)<1;1,0>
; DOUBLE4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; DOUBLE4_V16I16: mov (M1, 16) {{.*}} [[OUTPUT]](7,16)<1;1,0>
; DOUBLE4_V16I16-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; DOUBLE4_V16I16-DAG: .decl [[INPUT]] v_type=G type=df num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x double>/g' -e 's/OUTPUT_TYPE/<8 x i32>/g' \
; RUN:   -e 's/TEST_NAME/test_double4_v8i32/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_d/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=DOUBLE4_V8I32

; DOUBLE4_V8I32: .function "test_double4_v8i32_1"
; DOUBLE4_V8I32: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; DOUBLE4_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; DOUBLE4_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; DOUBLE4_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; DOUBLE4_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; DOUBLE4_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; DOUBLE4_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; DOUBLE4_V8I32: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; DOUBLE4_V8I32-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; DOUBLE4_V8I32-DAG: .decl [[INPUT]] v_type=G type=df num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x double>/g' -e 's/OUTPUT_TYPE/<4 x i64>/g' \
; RUN:   -e 's/TEST_NAME/test_double4_v4i64/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_d/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=DOUBLE4_V4I64

; DOUBLE4_V4I64: .function "test_double4_v4i64_1"
; DOUBLE4_V4I64: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; DOUBLE4_V4I64: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; DOUBLE4_V4I64: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; DOUBLE4_V4I64: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; DOUBLE4_V4I64-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=64 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; DOUBLE4_V4I64-DAG: .decl [[INPUT]] v_type=G type=df num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x double>/g' -e 's/OUTPUT_TYPE/<16 x half>/g' \
; RUN:   -e 's/TEST_NAME/test_double4_v16half/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_d/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=DOUBLE4_V16HALF

; DOUBLE4_V16HALF: .function "test_double4_v16half_1"
; DOUBLE4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; DOUBLE4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](0,16)<1;1,0>
; DOUBLE4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; DOUBLE4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](1,16)<1;1,0>
; DOUBLE4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; DOUBLE4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](2,16)<1;1,0>
; DOUBLE4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; DOUBLE4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](3,16)<1;1,0>
; DOUBLE4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; DOUBLE4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](4,16)<1;1,0>
; DOUBLE4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; DOUBLE4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](5,16)<1;1,0>
; DOUBLE4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; DOUBLE4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](6,16)<1;1,0>
; DOUBLE4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; DOUBLE4_V16HALF: mov (M1, 16) {{.*}} [[OUTPUT]](7,16)<1;1,0>
; DOUBLE4_V16HALF-DAG: .decl [[OUTPUT]] v_type=G type=uw num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; DOUBLE4_V16HALF-DAG: .decl [[INPUT]] v_type=G type=df num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<4 x double>/g' -e 's/OUTPUT_TYPE/<8 x float>/g' \
; RUN:   -e 's/TEST_NAME/test_double4_v8float/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv4_d/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=DOUBLE4_V8FLOAT

; DOUBLE4_V8FLOAT: .function "test_double4_v8float_1"
; DOUBLE4_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; DOUBLE4_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; DOUBLE4_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; DOUBLE4_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; DOUBLE4_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; DOUBLE4_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; DOUBLE4_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; DOUBLE4_V8FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; DOUBLE4_V8FLOAT-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; DOUBLE4_V8FLOAT-DAG: .decl [[INPUT]] v_type=G type=df num_elts=64 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x double>/g' -e 's/OUTPUT_TYPE/<16 x i32>/g' \
; RUN:   -e 's/TEST_NAME/test_double8_v16i32/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_d/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=DOUBLE8_V16I32

; DOUBLE8_V16I32: .function "test_double8_v16i32_1"
; DOUBLE8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; DOUBLE8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; DOUBLE8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; DOUBLE8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; DOUBLE8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; DOUBLE8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; DOUBLE8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; DOUBLE8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; DOUBLE8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](8,0)<1;1,0>
; DOUBLE8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](9,0)<1;1,0>
; DOUBLE8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](10,0)<1;1,0>
; DOUBLE8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](11,0)<1;1,0>
; DOUBLE8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](12,0)<1;1,0>
; DOUBLE8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](13,0)<1;1,0>
; DOUBLE8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](14,0)<1;1,0>
; DOUBLE8_V16I32: mov (M1, 16) {{.*}} [[OUTPUT]](15,0)<1;1,0>
; DOUBLE8_V16I32-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; DOUBLE8_V16I32-DAG: .decl [[INPUT]] v_type=G type=df num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x double>/g' -e 's/OUTPUT_TYPE/<8 x i64>/g' \
; RUN:   -e 's/TEST_NAME/test_double8_v8i64/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_d/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=DOUBLE8_V8I64

; DOUBLE8_V8I64: .function "test_double8_v8i64_1"
; DOUBLE8_V8I64: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; DOUBLE8_V8I64: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; DOUBLE8_V8I64: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; DOUBLE8_V8I64: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; DOUBLE8_V8I64: mov (M1, 16) {{.*}} [[OUTPUT]](8,0)<1;1,0>
; DOUBLE8_V8I64: mov (M1, 16) {{.*}} [[OUTPUT]](10,0)<1;1,0>
; DOUBLE8_V8I64: mov (M1, 16) {{.*}} [[OUTPUT]](12,0)<1;1,0>
; DOUBLE8_V8I64: mov (M1, 16) {{.*}} [[OUTPUT]](14,0)<1;1,0>
; DOUBLE8_V8I64-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=128 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; DOUBLE8_V8I64-DAG: .decl [[INPUT]] v_type=G type=df num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<8 x double>/g' -e 's/OUTPUT_TYPE/<16 x float>/g' \
; RUN:   -e 's/TEST_NAME/test_double8_v16float/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv8_d/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=DOUBLE8_V16FLOAT

; DOUBLE8_V16FLOAT: .function "test_double8_v16float_1"
; DOUBLE8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; DOUBLE8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](1,0)<1;1,0>
; DOUBLE8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; DOUBLE8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](3,0)<1;1,0>
; DOUBLE8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; DOUBLE8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](5,0)<1;1,0>
; DOUBLE8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; DOUBLE8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](7,0)<1;1,0>
; DOUBLE8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](8,0)<1;1,0>
; DOUBLE8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](9,0)<1;1,0>
; DOUBLE8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](10,0)<1;1,0>
; DOUBLE8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](11,0)<1;1,0>
; DOUBLE8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](12,0)<1;1,0>
; DOUBLE8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](13,0)<1;1,0>
; DOUBLE8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](14,0)<1;1,0>
; DOUBLE8_V16FLOAT: mov (M1, 16) {{.*}} [[OUTPUT]](15,0)<1;1,0>
; DOUBLE8_V16FLOAT-DAG: .decl [[OUTPUT]] v_type=G type=ud num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; DOUBLE8_V16FLOAT-DAG: .decl [[INPUT]] v_type=G type=df num_elts=128 align=wordx32

; RUN: sed -e 's/INPUT_TYPE/<16 x double>/g' -e 's/OUTPUT_TYPE/<16 x i64>/g' \
; RUN:   -e 's/TEST_NAME/test_double16_v16i64/g' \
; RUN:   -e 's/SPIRV_INTRINSIC/_Z35__spirv_SubgroupBitcastShuffleINTELDv16_d/g' %s > %t.ll
; RUN: llvm-as %t.ll -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv
; TODO: switch to the line below once SPV_INTEL_subgroup_bitcast_shuffle is supported in llvm-spirv translator
; llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_subgroup_bitcast_shuffle -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" | FileCheck %s --check-prefix=DOUBLE16_V16I64

; DOUBLE16_V16I64: .function "test_double16_v16i64_1"
; DOUBLE16_V16I64: mov (M1, 16) {{.*}} [[OUTPUT:[a-zA-Z0-9_]+]](0,0)<1;1,0>
; DOUBLE16_V16I64: mov (M1, 16) {{.*}} [[OUTPUT]](2,0)<1;1,0>
; DOUBLE16_V16I64: mov (M1, 16) {{.*}} [[OUTPUT]](4,0)<1;1,0>
; DOUBLE16_V16I64: mov (M1, 16) {{.*}} [[OUTPUT]](6,0)<1;1,0>
; DOUBLE16_V16I64: mov (M1, 16) {{.*}} [[OUTPUT]](8,0)<1;1,0>
; DOUBLE16_V16I64: mov (M1, 16) {{.*}} [[OUTPUT]](10,0)<1;1,0>
; DOUBLE16_V16I64: mov (M1, 16) {{.*}} [[OUTPUT]](12,0)<1;1,0>
; DOUBLE16_V16I64: mov (M1, 16) {{.*}} [[OUTPUT]](14,0)<1;1,0>
; DOUBLE16_V16I64: mov (M1, 16) {{.*}} [[OUTPUT]](16,0)<1;1,0>
; DOUBLE16_V16I64: mov (M1, 16) {{.*}} [[OUTPUT]](18,0)<1;1,0>
; DOUBLE16_V16I64: mov (M1, 16) {{.*}} [[OUTPUT]](20,0)<1;1,0>
; DOUBLE16_V16I64: mov (M1, 16) {{.*}} [[OUTPUT]](22,0)<1;1,0>
; DOUBLE16_V16I64: mov (M1, 16) {{.*}} [[OUTPUT]](24,0)<1;1,0>
; DOUBLE16_V16I64: mov (M1, 16) {{.*}} [[OUTPUT]](26,0)<1;1,0>
; DOUBLE16_V16I64: mov (M1, 16) {{.*}} [[OUTPUT]](28,0)<1;1,0>
; DOUBLE16_V16I64: mov (M1, 16) {{.*}} [[OUTPUT]](30,0)<1;1,0>
; DOUBLE16_V16I64-DAG: .decl [[OUTPUT]] v_type=G type=uq num_elts=256 align=wordx32 alias=<[[INPUT:[a-zA-Z0-9_]+]], 0>
; DOUBLE16_V16I64-DAG: .decl [[INPUT]] v_type=G type=df num_elts=256 align=wordx32

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
