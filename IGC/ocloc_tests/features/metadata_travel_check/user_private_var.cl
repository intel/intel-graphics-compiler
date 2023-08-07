/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// This test checks if calls to stack overflow detection subroutines are present
// in the generated vISA for a kernel with stack calls.

// REQUIRES: regkeys

// checking the asm dump file
// RUN: ocloc compile -file %s -options " -igc_opts 'VISAOptions=-asmToConsole'" -device pvc | FileCheck %s --check-prefix=CHECK-ASM
// checking the llvm-IR after EmitVISAPass
// RUN: ocloc compile -file %s -options " -igc_opts 'PrintToConsole=1 PrintBefore=EmitPass'" -device pvc 2>&1 | FileCheck %s --check-prefix=CHECK-LLVM

// Looking for the comment attached to the instructions ex:
// (W)     store.ugm .... //  address space: private; ; $193
// CHECK-ASM: address space: private;

// Looking for the MD attached to the instructions ex:
// store <4 x i32> %39, <4 x i32>* %40, align 4, !user_addrspace_priv !348   ; visa id: 82
// %55 = load i32, i32* %54, align 4, !user_addrspace_priv !348   ; visa id: 127
// CHECK-LLVM: !user_addrspace_priv
// Looking for the reason of not promoting the user variable to grf
// CHECK-LLVM: = !{!"OutOfAllocSizeLimit"}


kernel void test_simple(global int* output)
{
  // To force spill of the variable private_array.
  // This will generate the metadata !user_addrspace_priv in LLVM-IR
  // and comment in asm dump "address space: private;"
  int private_array[1024];
  int private_array_2[1024];

  for(int i = 0; i < 1024; ++i)
  {
    private_array[i] = i;
  }

  for(int i = 1023; i >= 0 ; --i)
  {
    const int tmp = private_array[i];
    private_array_2[i] = tmp;
  }

  for(int i = 0; i < 1024; ++i)
  {
    const int tmp = private_array_2[i];
    output[i] = tmp;
  }
}
