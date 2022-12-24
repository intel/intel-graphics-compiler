<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  WAIT = 0x5b

## Format

| | |
| --- | --- |
| 0x5b(WAIT) | Mask |


## Semantics


```

    //Each TDR is a struct with two fields: a bool valid bit and a thread id.
    //It is initialized during thread dispatch
    for (i = 0; i < 8; ++i) {
        if (mask & (1<< i)) {
            TDR[i].valid = false;
        }
        if (TDR[i].valid) {
            wait for TDR[i].thread to finish;
        }
    }
```

## Description





    If a thread dependency pattern is specified during the creation of the
    kernel's thread space, this instruction will cause the thread to wait
    until all of its dependency threads have finished their execution. The
    8-bit thread dependency clear mask provides finer-grained dependency
    control. Each bit corresponds to one of the eight threads this thread
    may depend on; if set, dependency is cleared, and this thread will not
    wait for the corresponding thread's termination.


- **Mask(scalar):** The thread dependency clear mask. Must have type UB


#### Properties




## Text
```



    WAIT <Mask>
```
## Notes





    This instruction may only be used with the global thread space execution model.

