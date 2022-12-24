<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  JMP = 0x32

## Format

| | | | |
| --- | --- | --- | --- |
| 0x32(JMP) | Exec_size | Pred | Label |


## Semantics


```

                    Jump to the basic block at <label>.
```

## Description





```
    Performs a conditional jump to the basic block <label> based on the predicate value.
```


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

- **Pred(uw):** Predication control


- **Label(uw):** Index of the label variable. It must be a block label


#### Properties
- **Source Modifier:** false




## Text
```
[(<P>)] JMP (<exec_size>) <label>
```

## Notes





    An execution size of 1 indicates the jump is convergent, and the first element of the predicate variable is used to determine if the jump will be executed. The label must be in the same subroutine as the jump.

    Indexed jump through a variable is not supported.

