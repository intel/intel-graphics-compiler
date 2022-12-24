<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  SWITCHJMP = 0x69

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x69(SWITCHJMP) | Exec_size | Num_labels | Index | Label0 | LabelN | Label(Num_labels-1) |


## Semantics


```

                    Jumps to one of the labels based on the index value.
```

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


- **Index(scalar):** Index of the label in the table to jump to. It must have unsigned integer type, and its value must be in the range of [0..<num_labels>-1]. Must have type UB


- **Label0(uw):** The 0th label in the jump table, represented by the label variable's id. It must be a block label


- **LabelN(uw):** The Nth label in the jump table, represented by the label variable's id. It must be a block label


- **Label(Num_labels-1)(unknown):** The last label in the jump table, represented by the label variable's id. It must be a block label


#### Properties




## Text
```



    SWITCHJMP (<exec_size>) <index> (Label0, Label1, ..., LabelN-1)
```
## Notes





