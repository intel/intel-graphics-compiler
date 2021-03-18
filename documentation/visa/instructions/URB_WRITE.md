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

  URB_WRITE = 0x72

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x72(URB_WRITE) | Exec_size       | Pred        | Num_out | Channel_mask | Global_offset | URB_handle |
|                 | Per_slot_offset | Vertex_data |         |              |               |            |


## Semantics




    Write to the Unified Resource Buffer (URB)

## Description


- **Exec_size(ub):** Execution size
 
  - Bit[2..0]: size of the region for source and destination operands
 
    - 0b011:  8 elements 
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
- **Pred(uw):** Predication control

- **Num_out(ub):** 
 
  - Bit[2..0]: specifies the number of output parameters. Valid values are [1-8]

- **Channel_mask(raw_operand):** First 8 elements represent the 8-bit channel mask for each vertex. 1 means the corresponding channel will be written, 0 means not. Should be set to V0 if all channels are on. Must have type UD

- **Global_offset(uw):** global offset to each URB handle in 128-bit units. Valid values are  [0-2047]

- **URB_handle(raw_operand):** First 8 elements represent the URB handles where each channel's results are written to. Must have type UD

- **Per_slot_offset(raw_operand):** First 8 elements represent the per-slot offset to each URB handle, in 128-bit units. Must be set to V0 if per-slot offset is not used. Must have type UD. Valid values are  [0-2047]

- **Vertex_data(raw_operand):** num_out GRFs from the operands are written to the URB subject to the channel mask. Each GRF represents an output parameter, and each dword in the GRF stores the value of the corresponding vertex. Must have type UD,D,F

#### Properties


## Text
```
    



[(<P>)] URB_WRITE (M1, 8) <num_out> <global_offset> <channel_mask> <URB_handle> <per_slot_offset> <vertex_data>

//<channel_mask> is an integer representing the bit mask value
```



## Notes


