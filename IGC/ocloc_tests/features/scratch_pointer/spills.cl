/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Verify that scratch_pointer is removed from implicit kernel arguments if it is beneficial and kernel has no spills.

// REQUIRES: cri-supported, regkeys
// RUN: ocloc compile -file %s -options "-igc_opts 'DumpZEInfoToConsole=1, RemoveUnusedIdImplicitArguments=1, RemoveImplicitScratchPointer=1, TotalGRFNum=32, AllowSIMD16DropForXE2Plus=0'" -device cri | FileCheck %s --check-prefix=CHECK-SPILL
// RUN: ocloc compile -file %s -options "-igc_opts 'DumpZEInfoToConsole=1, RemoveUnusedIdImplicitArguments=1, RemoveImplicitScratchPointer=1, TotalGRFNum=512, AllowSIMD16DropForXE2Plus=0'" -device cri | FileCheck %s --check-prefix=CHECK-NO-SPILL

// CHECK-SPILL:      payload_arguments:
// CHECK-SPILL-NEXT: - arg_type:        indirect_data_pointer
// CHECK-SPILL-NEXT:   offset:          0
// CHECK-SPILL-NEXT:   size:            8
// CHECK-SPILL-NEXT: - arg_type:        scratch_pointer
// CHECK-SPILL-NEXT:   offset:          8
// CHECK-SPILL-NEXT:   size:            8
//
// CHECK-SPILL:      Start recompilation of the kernel

// CHECK-NO-SPILL:      payload_arguments:
// CHECK-NO-SPILL-NEXT: - arg_type:        indirect_data_pointer
// CHECK-NO-SPILL-NEXT:   offset:          0
// CHECK-NO-SPILL-NEXT:   size:            8
// CHECK-NO-SPILL-NEXT: - arg_type:        global_id_offset
// CHECK-NO-SPILL-NEXT:   offset:          8
// CHECK-NO-SPILL-NEXT:   size:            12
//
// CHECK-NO-SPILL-NOT:      scratch_pointer
// CHECK-NO-SPILL-NOT:      Start recompilation of the kernel

kernel void foo(global float4* a1, global float4* a2, global float4* a3, global float4* a4) {

    int  i = get_global_id(0);

    float4 float_var_1 = {1+i, 2+i, 3+i, 4+i};
    float4 float_var_2 = {1+i, 2+i, 3+i, 4+i};
    float4 float_var_3 = {1+i, 2+i, 3+i, 4+i};
    float4 float_var_4 = {1+i, 2+i, 3+i, 4+i};

    #pragma nounroll
    for (int i = 0; i < 1500; i++)
    {
        float_var_1 += (float4){4, 3, 2, 1};
        float_var_2 += (float4){4, 3, 2, 1};
        float_var_3 += (float4){4, 3, 2, 1};
        float_var_4 += (float4){4, 3, 2, 1};
    }

    a1[i+1] = float_var_1;
    a1[i+2] = float_var_2;
    a1[i+3] = float_var_3;
    a1[i+4] = float_var_4;
}
