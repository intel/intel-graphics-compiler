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

  WAIT = 0x5b

## Format

| |
| --- |
| 0x5b(WAIT) | Mask |


## Semantics




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
