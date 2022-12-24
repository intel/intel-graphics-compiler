<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  FENCE = 0x5c

## Format

| | |
| --- | --- |
| 0x5c(FENCE) | Mask |


## Semantics


```

      Memory fence.
```

## Description





```
    Performs scattered write into <surface>, using the values from <src>.
```


- **Mask(ub):**  Controls whether the various caches should be flushed.

  - Bit[0]: the "commit enable" bit. If set, the fence is guaranteed to be globally observable.

  - Bit[1]: flush instruction cache if set.

  - Bit[2]: flush sampler cache if set.

  - Bit[3]: flush constant cache if set.

  - Bit[4]: flush read-write cache if set.

  - {ICLLP+}Bit[5]:

    - 0b0:  fence is applied to global memory (surface-based accesses and SVM)
    - 0b1:  fence applies to shared local memory only
  - Bit[6]: flush L1 read-only data cache if set

  - Bit[7]: indicates this is a scheduling barrier but will not generate an actual fence instruction.


#### Properties




## Text
```



    FENCE_{GLOBAL|LOCAL}.{E?I?S?C?R?L1?}

    FENCE_SW

    //E - commit enable

    //I - Instruction Cache

    //S - Sampler Cache

    //C - Constant Cache

    //R - Read-Write Cache

    //L1 - L1 Cache

    //FENCE_SW means a software only scheduling barrier
```
## Notes





      Cache flush is normally not needed for correctness.

