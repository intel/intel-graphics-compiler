<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  BARRIER = 0x59

## Format

## Semantics


```

                    Barrier synchronization within a thread group.
```

## Description






    Performs barrier synchronization for all threads within the same thread group.
    The barrier instruction causes the executing thread to wait until all threads
    in the same thread group have executed the barrier instruction. Memory
    ordering is also guaranteed by this instruction. The behavior is undefined
    if this instruction is executed in divergent control flow.


#### Properties




## Text
```
BARRIER
```

## Notes





    This instruction may only be used with the thread-group execution model.

