<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  GOTO = 0x6c

## Format

| | | | |
| --- | --- | --- | --- |
| 0x6c(GOTO) | Exec_size | Pred | Label |


## Semantics


```

                    Unstructured SIMD jump to basic block at <label>
```

## Description





```
    Each channel performs a conditional jump to the basic block <label> based on its own predicate value.
```


- **Exec_size(ub):** Execution size

  - Bit[2..0]: size of the region for source and destination operands

    - 0b000:  1 element (scalar)
    - 0b001:  2 elements
    - 0b010:  4 elements
    - 0b011:  8 elements
    - 0b100:  16 elements
    - 0b101:  32 elements
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


- **Label(uw):** Index of the label variable.  It must be a block label


#### Properties
- **Source Modifier:** false




## Text
```
[(<P>)] GOTO (<exec_size>) <label>
```

## Notes





```
    For a forward jump (the label is lexicographically after the goto): The active channels that are predicated will have their execution mask turned off, and they will be reactivated when execution reaches the label instruction. The active channels that are not predicated will execute the next instruction. If all active channels are predicated, execution jumps to the next program point where some channels are waiting to be reactivated (e.g., the target label of another goto).

    For a backward jump (the label is lexicographically before the goto): Execution jumps to the label if any active channel is predicated. The active channels that are not predicated will have their execution mask turned off, and they will be reactivated when execution reaches the next instruction.

    It is the application's responsibility to ensure that divergent control flow due to the goto instructions will converge eventually. The label must be in the same subroutine as the goto.

    A <exec_size> of 1 indicates that the branch is uniform, and all channels would either branch or not based on the predicate value.
```

