<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  SCATTER_SCALED = 0x79

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x79(SCATTER_SCALED) | Exec_size | Pred           | Block_size | Num_blocks | Scale | Surface |
|                      | Offset    | Element_offset | Src        |            |       |         |


## Semantics


```

      for (i = 0; i < exec_size; ++i) {
        if (ChEn[i]) {
          surface[offset+element_offset[i]] = src[i]; //<num_blocks> bytes
        }
      }
```

## Description





```
    Performs <exec_size> element scattered write into <surface> using the values from <src>. For each channel <num_blocks> bytes are written. The address to be written is determined by the offset and each element offset.
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


- **Block_size(ub):** This field is ignored and block size is always one byte


- **Num_blocks(ub):**

  - Bit[1..0]: encodes the number of blocks to read at each address.

    - 0b00:  1 block
    - 0b01:  2 blocks
    - 0b10:  4 blocks

- **Scale(uw):** This field is ignored and scale is always zero


- **Surface(ub):** Index of the surface variable.


- **Offset(scalar):** The global byte offset. Must have type UD


- **Element_offset(raw_operand):** The first Exec_size elements of the operand will be used as the byte offsets into the surface. Must have type UD


- **Src(raw_operand):** The variable providing the values to be written. The first exec_size elements will be used. For 1 and 2 byte accesses the upper bytes will be ignored. Must have type UD,D,F


#### Properties
- **Out-of-bound Access:** On write: data is dropped.




## Text
```



    [(<P>)] SCATTER_SCALED.<num_blocks> (<exec_size>) <surface> <offset> <element_offset> <src>
```
## Notes





    The behavior is undefined if more than one channel writes to the same address.

