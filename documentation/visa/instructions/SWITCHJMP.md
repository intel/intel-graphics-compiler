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

  SWITCHJMP = 0x69

## Format

| | | | | | |
| --- | --- | --- | --- | --- | --- |
| 0x69(SWITCHJMP) | Exec_size | Num_labels | Index | Label0 | LabelN | Label(Num_labels-1) |


## Semantics




                    Jumps to one of the labels based on the index value.

## Description


    Implements a multiway branch by performing a jump to one of the labels in the table based on the given index.

- **Exec_size(ub):** Execution size
 
  - Bit[2..0]: size of the region for source and destination operands
 
    - 0b000:  1 element (scalar) 
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
- **Num_labels(ub):** Number of labels in the table. Must be in the range [1..32]

- **Index(scalar):** Index of the label in the table to jump to. It must have unsigned integer type, and its value must be in the range of [0..<num_labels>-1]

- **Label0(uw):** The 0th label in the jump table, represented by the label variable's id. It must be a block label

- **LabelN(uw):** The Nth label in the jump table, represented by the label variable's id. It must be a block label

- **Label(Num_labels-1)(unknown):** The last label in the jump table, represented by the label variable's id. It must be a block label

#### Properties


## Text
```
    

		SWITCHJMP (<exec_size>) <index> (Label0, Label1, ..., LabelN-1)
```



## Notes


