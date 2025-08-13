/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// REQUIRES: regkeys, pvc-supported

// windows unsupported due to issues on 32bit build, to be debugged.
// UNSUPPORTED: system-windows
// Disable loop unroll so that the private memory is not optimized out.

// checking the asm dump file
// RUN: ocloc compile -file %s -options " -g -igc_opts 'DisableLoopUnroll=1 VISAOptions=-asmToConsole'" -device pvc 2>&1 | FileCheck %s --check-prefix=CHECK-ASM
// checking the llvm-IR after EmitVISAPass
// RUN: ocloc compile -file %s -options " -g -igc_opts 'DisableLoopUnroll=1 PrintToConsole=1 PrintMDBeforeModule=1 PrintAfter=EmitPass'" -device pvc 2>&1 | FileCheck %s --check-prefix=CHECK-LLVM
// RUN: ocloc compile -file %s -options " -g -igc_opts 'DisableLoopUnroll=1 PrintToConsole=1 PrintMDBeforeModule=1 PrintAfter=EmitPass'" -device pvc 2>&1 | FileCheck %s --check-prefix=CHECK-LLVM2
// RUN: ocloc compile -file %s -options " -g -igc_opts 'DisableLoopUnroll=1 PrintToConsole=1 PrintMDBeforeModule=1 PrintAfter=EmitPass'" -device pvc 2>&1 | FileCheck %s --check-prefix=CHECK-LLVM3
// RUN: ocloc compile -file %s -options " -g -igc_opts 'DisableLoopUnroll=1 PrintToConsole=1 PrintMDBeforeModule=1 PrintAfter=EmitPass'" -device pvc 2>&1 | FileCheck %s --check-prefix=CHECK-LLVM4

// Looking for the comment which informs about the amount of spill size
// CHECK-ASM: //.private memory size
// Looking for the comment attached to the instructions ex:
// (W)     store.ugm .... //  address space: private; ; $193
// CHECK-ASM: address space: private;

// Looking for the MD attached to the instructions ex:
// store <4 x i32> %39, <4 x i32>* %40, align 4, !user_as_priv !348   ; visa id: 82
// %55 = load i32, i32* %54, align 4, !user_as_priv !348   ; visa id: 127
// Looking for the reason of not promoting the user variable to grf


kernel void test_simple(global int* output)
{
  // To force spill of the variable private_array.
  // This will generate the metadata !user_as_priv in LLVM-IR
  // and comment in asm dump "address space: private;"
  int private_array[1024];
  int private_array_2[1024];

  for(int i = 0; i < 1024; ++i)
  {
// Look first for the debugInfo mark which indetify the asign place
// CHECK-LLVM: [[PA1:![0-9]+]] = !DILocation(line: [[@LINE+7]], column: 22,
// Look if we have any load/store pointing to the dbg loc which don't have
// md attribute !user_as_priv
// regex in POSIX: !([^u]|u[^s]|us[^e]|use[^r]|user[^_]|user_[^a]|user_a[^s]|user_as[^_]|user_as_[^p])
// equals negative lookahead !(?!user_as_p)
// CHECK-LLVM-NOT: {{^ store .*, !dbg }}[[PA1]]{{, !([^u]|u[^s]|us[^e]|use[^r]|user[^_]|user_[^a]|user_a[^s]|user_as[^_]|user_as_[^p])}}
// CHECK-LLVM-NOT: {{^ .* = load .*, !dbg }}[[PA1]]{{, !([^u]|u[^s]|us[^e]|use[^r]|user[^_]|user_[^a]|user_a[^s]|user_as[^_]|user_as_[^p])}}
    private_array[i] = i;
  }

  for(int i = 1023; i >= 0 ; --i)
  {
// CHECK-LLVM2: [[PA2:![0-9]+]] = !DILocation(line: [[@LINE+3]], column: 21,
// CHECK-LLVM2-NOT: {{^ store .*, !dbg }}[[PA2]]{{, !([^u]|u[^s]|us[^e]|use[^r]|user[^_]|user_[^a]|user_a[^s]|user_as[^_]|user_as_[^p])}}
// CHECK-LLVM2-NOT: {{^ .* = load .*, !dbg }}[[PA2]]{{, !([^u]|u[^s]|us[^e]|use[^r]|user[^_]|user_[^a]|user_a[^s]|user_as[^_]|user_as_[^p])}}
    const int tmp = private_array[i];
// CHECK-LLVM3: [[PA3:![0-9]+]] = !DILocation(line: [[@LINE+3]], column: 24,
// CHECK-LLVM3-NOT: {{^ store .*, !dbg }}[[PA3]]{{, !([^u]|u[^s]|us[^e]|use[^r]|user[^_]|user_[^a]|user_a[^s]|user_as[^_]|user_as_[^p])}}
// CHECK-LLVM3-NOT: {{^ .* = load .*, !dbg }}[[PA3]]{{, !([^u]|u[^s]|us[^e]|use[^r]|user[^_]|user_[^a]|user_a[^s]|user_as[^_]|user_as_[^p])}}
    private_array_2[i] = tmp;
  }

  for(int i = 0; i < 1024; ++i)
  {
// CHECK-LLVM4: [[PA4:![0-9]+]] = !DILocation(line: [[@LINE+3]], column: 21,
// CHECK-LLVM4-NOT: {{^ store .*, !dbg }}[[PA4]]{{, !([^u]|u[^s]|us[^e]|use[^r]|user[^_]|user_[^a]|user_a[^s]|user_as[^_]|user_as_[^p])}}
// CHECK-LLVM4-NOT: {{^ .* = load .*, !dbg }}[[PA4]]{{, !([^u]|u[^s]|us[^e]|use[^r]|user[^_]|user_[^a]|user_a[^s]|user_as[^_]|user_as_[^p])}}
    const int tmp = private_array_2[i];
    output[i] = tmp;
  }
}
