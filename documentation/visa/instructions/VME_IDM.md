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

  VME_IDM = 0x57

## Format

| | | | |
| --- | --- | --- | --- |
| 0x57(VME_IDM) | UNIInput | IDMInput | Surface | Output |


## Semantics




      Video Motion Estimation - distortion mesh.

## Description



    Performs Video Motion Estimation with distortion mesh output (IDM). See [4] for more detailed information on the VME functionality. This instruction may not appear inside a SIMD control flow block.

- **UNIInput(raw_operand):** The raw operand of a general variable that stores the universal VME payload data. Must have type UB. Must have 128 elements

- **IDMInput(raw_operand):** The raw operand of a general variable that stores the SIC specific payload data. Must have type UB. Must have 32 elements

- **Surface(ub):** The index of the surface variable

- **Output(raw_operand):** The raw operand of a general variable used to store the VME output data. Must have type UB. Must have 512 elements

#### Properties


## Text
```
    

    VME_IDM <surface> <UNIInput> <IDMInput> <output>
```



## Notes


