<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  SVM = 0x4e
  SCATTER4_SCALED = 0x07

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x4e(SVM) | 0x07(SCATTER4_SCALED) | Exec_size | Pred | Channels | Scale | Address | Element_offset |
|           |                       | Src       |      |          |       |         |                |


## Semantics


```

                    UD ch_pos = 0;
                    for (c = 0; c < 4; ++c) {
                        if (ch_mask[c] == 1) {
                            for (i = 0; i < exec_size; ++i) {
                                if (ChEn[i]) {
                                    UD ch_start = ch_pos * max(exec_size, GRF_SIZE / 4);  // GRF_SIZE is the register size in bytes
                                    *(address + element_offset[i] + (c * 4)) = src[ch_start+i]; //4 byte
                                }
                            }
                            ch_pos++;
                        }
                    }
```

## Description





```
    Performs <exec_size>*<num_enabled_channels> element scattered write
    into virtual addresses, using the values from <src>. The address to be
    written is determined by the offset, scaling pitch, and each element
    offset, and the final address must be dword-aligned.
```


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

- **Pred(uw):** Predication control


- **Channels(ub):**

  - Bit[3..0]: determines the write masks for the RGBA channel, with R being bit 0 and A bit 3. At least one channel must be enabled (i.e., "0000" is not allowed)


- **Scale(uw):** This field is ignored and scale is always zero


- **Address(scalar):** The global byte offset. Must have type UQ


- **Element_offset(raw_operand):** The first Exec_size elements of the variable will be used as the byte offsets for the corresponding channel. Must have type UQ


- **Src(raw_operand):** The values to be written.  For each enabled channel in RGBA order, exec_size elements will be written to the surface subject to predication. The next enabled channel will get its data from the next register. Must have type UD,D,F


#### Properties




## Text
```



    [(<P>)] SVM_SCATTER4_SCALED.<channels> (<exec_size>) <address> <element_offset> <src>
```
## Notes





