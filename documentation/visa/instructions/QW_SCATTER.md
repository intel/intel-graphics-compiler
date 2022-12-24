<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  QW_SCATTER = 0x87

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x87(QW_SCATTER) | Exec_size | Pred | Num_blocks | Surface | Offset | Src |


## Semantics


```

                    for (i = 0; i < exec_size; ++i) {
                        if (ChEn[i]) {
                            for (j = 0; j < num_blocks; ++j) {
                                Surface[Offset[i] + j] = src[j*exec_size+i]; // 8 byte
                            }
                        }
                    }
```

## Description





```
    Performs <exec_size> element scattered write into <surface> using the values from <src>. For each channel
    <num_blocks> * 8 bytes are written.
```


- **Exec_size(ub):** Execution size

  - Bit[2..0]: size of the region for source and destination operands

    - 0b000:  1 element (scalar)
    - 0b001:  2 elements
    - 0b010:  4 elements
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

- **Pred(uw):** Predication control


- **Num_blocks(ub):**

  - Bit[1..0]: encodes the number of blocks to read for each address

    - 0b00:  1 block

- **Surface(ub):** Index of the surface variable. T0 (SLM) is supported


- **Offset(raw_operand):** The first Exec_size elements of the operand will be used as the byte offsets into the surface. Must have type UD


- **Src(raw_operand):** The general variable providing the values to be written to each address. Must have type Q,UQ,DF


#### Properties
- **Out-of-bound Access:** On write: data is dropped.




## Text
```



    [(<P>)] QW_SCATTER.<Num_blocks> (<Exec_size>) <Surface> <Offset> <Src>
```
## Notes





    The behavior is undefined if more than one channel writes to the same address.

