<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  SBARRIER = 0x7c

## Format

| | |
| --- | --- |
| 0x7c(SBARRIER) | Mode |


## Semantics


```

      split-phase barrier synchronization within a thread group.
```

## Description






    Split-phase barrier. A signal operation notifies the other threads that it has reached the barrier,
    while a wait operation waits for other threads to reach the barrier.


- **Mode(ub):**

  - Bit[0]: indicates whether this instruction is a barrier signal or wait

    - 0b0:  wait
    - 0b1:  signal

#### Properties




## Text
```



    SBARRIER.wait     // barrier wait

    SBARRIER.signal   // barrier signal
```
## Notes





      This instruction may only be used with the thread-group execution model.

