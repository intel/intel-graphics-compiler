//========================== begin_copyright_notice ============================
//
// Copyright (C) 2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
//=========================== end_copyright_notice =============================

// UNSUPPORTED: sys32
// REQUIRES: temporarily-disabled, oneapi-readelf, dg2-supported

// RUN: ocloc compile -file %s -options "-g -igc_opts 'PrintToConsole=1 PrintAfter=EmitPass'" -device dg2 2>&1 | FileCheck %s --check-prefix=CHECK-LLVM
// RUN: ocloc compile -file %s -options "-g -igc_opts 'ElfDumpEnable=1 DumpUseShorterName=0 DebugDumpNamePrefix=%t_dg2_'" -device dg2
// RUN: oneapi-readelf --debug-dump %t_dg2_OCL_simd32_entry_0001.elf | FileCheck %s --check-prefix=CHECK-DWARF

// CHECK-LLVM-LABEL: define spir_kernel void @test
// CHECK-LLVM-SAME:      ({{.*}} %in,{{.*}} %out,{{.*}} i16 %localIdX{{.*}})
__attribute__((intel_reqd_sub_group_size(32)))
kernel void test(global int* in, global int* out) {
  // COM: The routine instructions for local ID extraction are largely skipped in the checks below
  // CHECK-LLVM: %[[LOCAL_ID_X:.+]] = zext i16 %localIdX to i32, !dbg !{{[0-9]+}}
  // CHECK-LLVM: %[[LOCAL_ID_TMP_0:.+]] = add i32 %{{.+}}, %localIdX4, !dbg !{{[0-9]+}}
  // CHECK-LLVM: %[[LOCAL_ID_TMP_1:.+]] = add i32 %[[LOCAL_ID_TMP_0]], %{{.*}}, !dbg !{{[0-9]+}}
  // COM: 'gid' is implicitly marked as a stack value before the emitter
  // CHECK-LLVM: call void @llvm.dbg.value(metadata i32 %[[LOCAL_ID_TMP_1]], metadata ![[GID_DI_VAR_MD:[0-9]+]]
  // CHECK-LLVM-SAME: metadata !DIExpression(DW_OP_LLVM_convert, 32, DW_ATE_unsigned, DW_OP_LLVM_convert, 64, DW_ATE_unsigned, DW_OP_stack_value))
  size_t gid = get_global_id(0);
  // CHECK-LLVM: %[[IN_LOAD:.+]] = call i32 @llvm.genx.GenISA.ldraw.indexed.i32{{.*}}({{.*}}), !dbg !{{.*}}
  // CHECK-LLVM: %[[MUL:.+]] = mul nsw i32 %[[IN_LOAD]], 42, !dbg !{{.*}}
  // COM: 'mul' is to be marked as a stack value during the emitter phase
  // CHECK-LLVM: call void @llvm.dbg.value(metadata i32 %[[MUL]], metadata ![[MUL_DI_VAR_MD:[0-9]+]], metadata !DIExpression())
  int mul = in[gid] * 42;
  out[gid] = mul;
}
// CHECK-LLVM-DAG: !{{[0-9]+}} = !{!"sub_group_size", i32 32}
//
// CHECK-LLVM-DAG: ![[GID_DI_VAR_MD]] = !DILocalVariable(name: "gid", {{.+}}, type: ![[SIZE_T_DI_TY_MD:[0-9]+]])
// CHECK-LLVM-DAG: ![[SIZE_T_DI_TY_MD]] = !DIDerivedType(tag: DW_TAG_typedef, name: "size_t", file: !{{[0-9]+}}, baseType: !{{[0-9]+}})
// CHECK-LLVM-DAG: ![[MUL_DI_VAR_MD]] = !DILocalVariable(name: "mul", {{.+}}, type: ![[INT_DI_TY_MD:[0-9]+]])
// CHECK-LLVM-DAG: ![[INT_DI_TY_MD]] = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)

// CHECK-DWARF: Contents of the .debug_info section:
// COM: Briefly check global layout
// CHECK-DWARF: Abbrev Number: 1 (DW_TAG_compile_unit)
// CHECK-DWARF: DW_AT_name{{ *}}: simd32-sliced-stack-value.cl
// CHECK-DWARF: DW_AT_INTEL_simd_width{{ *}}: 32
// CHECK-DWARF: Abbrev Number: 2 (DW_TAG_subprogram)
// CHECK-DWARF: DW_AT_name{{ *}}: test
// CHECK-DWARF: DW_AT_INTEL_simd_width{{ *}}: 32
// CHECK-DWARF: Abbrev Number: 3 (DW_TAG_formal_parameter)
// CHECK-DWARF: DW_AT_name{{ *}}: in
// CHECK-DWARF: Abbrev Number: 3 (DW_TAG_formal_parameter)
// CHECK-DWARF: DW_AT_name{{ *}}: out
// COM: Relevant variable checks/type captures
// CHECK-DWARF: Abbrev Number: 4 (DW_TAG_variable)
// CHECK-DWARF: DW_AT_name{{ *}}: gid
// CHECK-DWARF: DW_AT_type{{ *}}: <0x[[SIZE_T_TY:[0-9a-f]+]]>
// CHECK-DWARF: DW_AT_location{{ *}}: [[GID_LOC:0]] (location list)
// CHECK-DWARF: Abbrev Number: 4 (DW_TAG_variable)
// CHECK-DWARF: DW_AT_name{{ *}}: mul
// CHECK-DWARF: DW_AT_type{{ *}}: <0x[[INT_TY:[0-9a-f]+]]>
// CHECK-DWARF: DW_AT_location{{ *}}: 0x[[MUL_LOC:[0-9a-f]+]] (location list)
// COM: Type checks
// CHECK-DWARF: <[[INT_TY]]>: Abbrev Number: 6 (DW_TAG_base_type)
// CHECK-DWARF-NEXT: DW_AT_name{{ *}}: int
// CHECK-DWARF-NEXT: DW_AT_encoding{{ *}}: 5{{ *}} (signed)
// CHECK-DWARF: <[[SIZE_T_TY]]>: Abbrev Number: 7 (DW_TAG_typedef)
// CHECK-DWARF-NEXT: DW_AT_type
// CHECK-DWARF-NEXT: DW_AT_name{{ *}}: size_t
//
// CHECK-DWARF: Contents of the .debug_loc section:
// COM: Check SIMD 32 location expressions. We only expect DW_OP_stack_value at the end of each
//      expression, never before a DW_OP_skip.
// CHECK-DWARF-NOT: DW_OP_stack_value; DW_OP_skip
// COM: 'gid' source variable (implicit stack value)
// CHECK-DWARF: {{0+}}[[GID_LOC]] {{[0-9a-f]+}} {{[0-9a-f]+}}
// CHECK-DWARF-SAME: (DW_OP_INTEL_push_simd_lane; DW_OP_lit16; DW_OP_ge; DW_OP_bra: [[GID_BR:[0-9]+]];
// CHECK-DWARF-SAME: DW_OP_INTEL_push_simd_lane; DW_OP_lit3; DW_OP_shr; DW_OP_plus_uconst: {{[0-9]+}};
// CHECK-DWARF-SAME: DW_OP_INTEL_push_simd_lane; [[GID_MAIN_EXPR:DW_OP_lit7; DW_OP_and; DW_OP_const1u: 32; DW_OP_mul; DW_OP_INTEL_regval_bits: 32; DW_OP_const4u: 4294967295; DW_OP_and]];
// COM: skip to DW_OP_stack_value, where all lanes should meet
// CHECK-DWARF-SAME: DW_OP_skip: 21
// CHECK-DWARF-SAME: DW_OP_INTEL_push_simd_lane; DW_OP_lit16; DW_OP_minus; DW_OP_lit3; DW_OP_shr; DW_OP_plus_uconst: {{[0-9]+}};
// CHECK-DWARF-SAME: DW_OP_INTEL_push_simd_lane; [[GID_MAIN_EXPR]]; DW_OP_stack_value)
// CHECK-DWARF-NEXT: <End of list>
// COM: 'mul' source variable (explicitly marked as stack value)
// CHECK-DWARF: {{0+}}[[MUL_LOC]] {{[0-9a-f]+}} {{[0-9a-f]+}}
// CHECK-DWARF-SAME: (DW_OP_INTEL_push_simd_lane; DW_OP_lit16; DW_OP_ge; DW_OP_bra: [[MUL_BR:[0-9]+]];
// CHECK-DWARF-SAME: DW_OP_INTEL_push_simd_lane; DW_OP_lit3; DW_OP_shr; DW_OP_plus_uconst: {{[0-9]+}};
// CHECK-DWARF-SAME: DW_OP_INTEL_push_simd_lane; [[MUL_MAIN_EXPR:DW_OP_lit7; DW_OP_and; DW_OP_const1u: 32; DW_OP_mul; DW_OP_INTEL_regval_bits: 32]];
// COM: skip to DW_OP_stack_value, where all lanes should meet
// CHECK-DWARF-SAME: DW_OP_skip: 15
// CHECK-DWARF-SAME: DW_OP_INTEL_push_simd_lane; DW_OP_lit16; DW_OP_minus; DW_OP_lit3; DW_OP_shr; DW_OP_plus_uconst: {{[0-9]+}};
// CHECK-DWARF-SAME: DW_OP_INTEL_push_simd_lane; [[MUL_MAIN_EXPR]]; DW_OP_stack_value)
// CHECK-DWARF-NEXT: <End of list>
