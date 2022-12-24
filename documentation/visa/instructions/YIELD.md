<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  YIELD = 0x5f

## Format

## Semantics


```

    Causes the EU to switch to another thread. The hardware selects the thread to yield to.
```

## Description






#### Properties




## Text
```
YIELD
```

## Notes





    This instruction is strictly for performance tuning and does not affect
    program correctness.

