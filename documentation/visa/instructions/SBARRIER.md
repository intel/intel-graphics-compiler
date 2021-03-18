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

  SBARRIER = 0x7c

## Format

| |
| --- |
| 0x7c(SBARRIER) | Mode |


## Semantics




      split-phase barrier synchronization within a thread group.

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

    SBARRIER.signal		// barrier signal
```



## Notes



      This instruction may only be used with the thread-group execution model.
