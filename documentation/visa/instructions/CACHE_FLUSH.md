<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  CACHE_FLUSH = 0x5a

## Format

## Semantics


```

                    Flush the textural cache.
```

## Description





    The texture caches in the sampling engine are flushed.


#### Properties




## Text
```
CACHE_FLUSH
```

## Notes





    This message flushes all levels of texture cache, while the fence
    instruction's texture flush bit will only ensure that L3 textural cache
    is flushed.

