<!---======================= begin_copyright_notice ============================

Copyright (c) 2019-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ==========================-->

 

## Opcode

  FENCE = 0x5c

## Format

| |
| --- |
| 0x5c(FENCE) | Mask |


## Semantics




      Memory fence.

## Description



    Performs scattered write into <surface>, using the values from <src>.

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
