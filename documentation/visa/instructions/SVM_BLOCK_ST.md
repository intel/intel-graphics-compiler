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

  SVM = 0x4e

  BLOCK_ST = 0x02

## Format

| | | |
| --- | --- | --- |
| 0x4e(SVM) | 0x02(BLOCK_ST) | Properties | Address | Src |


## Semantics




                    for (i = 0; i < num_owords; ++i) {
                        *(address+i*16) = src[i]; //16 byte, oword-aligned
                    }

## Description



    Write contiguous owords (one oword is 16 byte) to the virtual address
    <address>, taking the values from <src>. The execution mask is set to
    "NoMask" (i.e., every element is returned).

- **Properties(ub):** 
 
  - Bit[2..0]: encodes the number of owords to read
 
    - 0b000:  1 oword 
    - 0b001:  2 oword 
    - 0b010:  4 oword 
    - 0b011:  8 oword
- **Address(scalar):** The write address in units of bytes. The address must be oword-aligned. Must have type UQ

- **Src(raw_operand):** The raw operand of a general variable storing the values to be written

#### Properties


## Text
```
    

		SVM_BLOCK_ST (<size>) <address> <src>
```



## Notes


