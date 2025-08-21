/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The test checks if zeinfo "has_printf_calls" and "has_indirect_calls" and it fields are set.

// UNSUPPORTED: sys32
// REQUIRES: pvc-supported, regkeys

// RUN: ocloc compile -file %s -options "-igc_opts 'DumpZEInfoToConsole=1'" \
// RUN:     -device pvc | FileCheck %s

// CHECK:       kernels:
// CHECK-NEXT:  - name:            test
// CHECK:       execution_env:
// CHECK:       has_indirect_calls: true

// CHECK:       functions:
// CHECK:       - name:            even
// CHECK:       execution_env
// CHECK:       has_printf_calls: true
// CHECK:       - name:            odd
// CHECK:       execution_env
// CHECK:       has_printf_calls: true

char *__builtin_IB_get_function_pointer(__constant char *function_name);
void __builtin_IB_call_function_pointer(char *function_pointer,
                                        char *argument_structure);

void even(char *argument_structure) {
  int *pio = (int *)argument_structure;
  printf("%d", *pio * 2);
}

void odd(char *argument_structure) {
  int *pio = (int *)argument_structure;
  printf("%d", *pio * 3);
}

#define EVEN_FUNC_NAME "even"
#define ODD_FUNC_NAME "odd"

__kernel void test() {
  int gid = (int)get_global_id(0);

  char* fp = 0;
  if(gid % 2)
    fp = __builtin_IB_get_function_pointer(ODD_FUNC_NAME);
  else
    fp = __builtin_IB_get_function_pointer(EVEN_FUNC_NAME);

  int x = 5;

  __builtin_IB_call_function_pointer(fp, (char *)&x);
}
