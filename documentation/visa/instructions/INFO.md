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

  INFO = 0x70

## Format

| | | | | |
| --- | --- | --- | --- | --- |
| 0x70(INFO) | Op | Exec_size | Surface | LOD | Dst |


## Semantics




    Returns information about the surface

## Description



    <exec_size>*4 elements are returned in RGBA order with the format described below. Only the enabled pixels are written to in <dst>.

    **Return format for resinfo:**

        +----------------+------------------------------------+--------------------------------------+------------------+------------+
        | Surface type   | R                                  | G                                    | B                | A          |
        +----------------+------------------------------------+--------------------------------------+------------------+------------+
        | 1D             | (Width+1)>>LOD                     | 0                                    | 0                | MIPCount   |
        +----------------+------------------------------------+--------------------------------------+------------------+------------+
        | 1D_array       | (Width+1)>>LOD                     | Depth+1                              | 0                | MIPCount   |
        +----------------+------------------------------------+--------------------------------------+------------------+------------+
        | 2D             | ((Width+1)>>LOD)*(QuiltWidth+1)    | ((Height+1)>>LOD)*(QuiltHeight+1)    | 0                | MIPCount   |
        +----------------+------------------------------------+--------------------------------------+------------------+------------+
        | 2D_array       | ((Width+1)>>LOD)*(QuiltWidth+1)    | ((Height+1)>>LOD)*(QuiltHeight+1)    | Depth+1          | MIPCount   |
        +----------------+------------------------------------+--------------------------------------+------------------+------------+
        | 3D             | (Width+1)>>LOD                     | (Height+1)>>LOD                      | (Depth+1)>>LOD   | MIPCount   |
        +----------------+------------------------------------+--------------------------------------+------------------+------------+
        | CUBE           | Buffer size                        | (Height+1)>>LOD                      | Depth+1          | MIPCount   |
        +----------------+------------------------------------+--------------------------------------+------------------+------------+

    Return format for sampleinfo:

        +----------------+---------------------+-------+-------+---------------------------------+
        | Surface type   | R                   | G     | B     | A                               |
        +----------------+---------------------+-------+-------+---------------------------------+
        | 2D             | Number of samples   | N/A   | N/A   | Sample position palette index   |
        +----------------+---------------------+-------+-------+---------------------------------+

- **Op(ub):** 
 
  - Bit[4..0]: encodes the sampler operation
 
    - 0b01010:  resinfo 
    - 0b01011:  sampleinfo
- **Exec_size(ub):** Execution size
 
  - Bit[2..0]: size of the region for source and destination operands
 
    - 0b011:  8 elements 
    - 0b100:  16 elements 
  - Bit[7..4]: execution mask (explicit control over the enabled channels)
 
    - 0b0000:  M1 
    - 0b0001:  M2 
    - 0b0010:  M3 
    - 0b0011:  M4 
    - 0b0100:  M5 
    - 0b0101:  M6 
    - 0b0110:  M7 
    - 0b0111:  M8 
    - 0b1000:  M1_NM 
    - 0b1001:  M2_NM 
    - 0b1010:  M3_NM 
    - 0b1011:  M4_NM 
    - 0b1100:  M5_NM 
    - 0b1101:  M6_NM 
    - 0b1110:  M7_NM 
    - 0b1111:  M8_NM
- **Surface(ub):** Index of the surface variable

- **LOD(raw_operand):** The LOD parameter. The field is present only for resinfo. Must have type UD

- **Dst(raw_operand):** results of the sampler operation. Must have type UD

#### Properties


## Text
```
    



RESINFO (exec_size) <surface> <lod> <dst>

SAMPLEINFO (exec_size) <surface> <dst>
```



## Notes


