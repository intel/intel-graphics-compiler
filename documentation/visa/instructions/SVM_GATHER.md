<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  SVM = 0x4e
  GATHER = 0x03

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x4e(SVM) | 0x03(GATHER) | Exec_size | Pred | Block_size | Num_blocks | Addresses | Dst |


## Semantics


```

                    for (i = 0; i < exec_size; ++i) {
                        if (ChEn[i]) {
                            if (block_size == 4 || block_size == 8) {
                                for (j = 0; j < num_blocks; ++j) {
                                    dst[j*exec_size+i] = *(Addresses[i] + j*block_size); //4 or 8 byte
                                }
                            }
                            else { // block_size == 1
                                UD min_block_size = num_blocks < 4 ? 4 : num_blocks;
                                for (j = 0; j < num_blocks; ++j) {
                                    dst[i*min_block_size+j] = *(Addresses[i] + j); //1 byte
                                }
                            }
                        }
                    }
```

## Description





```
    Performs 8 element scattered read from virtual addresses and stores the
    result into <dst>. Each element may have multiple blocks.
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


- **Block_size(ub):**

  - Bit[1..0]: encodes the byte size of each block

    - 0b00:  1 byte
    - 0b01:  4 byte
    - 0b11:  8 byte

- **Num_blocks(ub):**

  - Bit[1..0]: encodes the number of blocks to read for each address

    - 0b00:  1 block
    - 0b01:  2 blocks
    - 0b10:  4 blocks
    - 0b11:  8 blocks. It is only valid for 4 byte blocks and execution size 8

- **Addresses(raw_operand):** The general variable storing the addresses to read. The first 8 elements of the operand will be used as the virtual addresses to read from, and they are in the unit of bytes. Each address must be aligned to the <block_size>. Must have type UQ


- **Dst(raw_operand):** The general variable storing the results of the read. The format of the return value depends on <block_size> and <num_blocks>


#### Properties




## Text
```



    [(<P>)] SVM_GATHER.<block_size>.<num_blocks> (<exec_size>) <addresses> <dst>
```
## Notes





```
    4 byte gather:

        +-----------------+-----------------+----+----+-----+----+----+-----------------+
        | A0[0]           | A1[0]           |    |    | ... |    |    | A7[0]           |
        +-----------------+-----------------+----+----+-----+----+----+-----------------+
        |                                ...                                            |
        +-----------------+-----------------+----+----+-----+----+----+-----------------+
        | A0[#blocks-1]   | A1[#blocks-1]   |    |    | ... |    |    | A7[#blocks-1]   |
        +-----------------+-----------------+----+----+-----+----+----+-----------------+

    8 byte gather:

        +-----------------+-----------------+-----------------+-----------------+
        | A0[0]           | A1[0]           | A2[0]           | A3[0]           |
        +-----------------+-----------------+-----------------+-----------------+
        | A4[0]           | A5[0]           | A6[0]           | A7[0]           |
        +-----------------+-----------------+-----------------+-----------------+
        | ...                                                                   |
        +-----------------+-----------------+-----------------+-----------------+
        | A0[#blocks-1]   | A1[#blocks-1]   | A2[#blocks-1]   | A3[#blocks-1]   |
        +-----------------+-----------------+-----------------+-----------------+
        | A4[#blocks-1]   | A5[#blocks-1]   | A6[#blocks-1]   | A7[#blocks-1]   |
        +-----------------+-----------------+-----------------+-----------------+

    1 byte gather, 1/2/4 block (Ai[N] has undefined value if N>=num_blocks)

        +-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+
        | A0[0:3]   | A1[0:3]   | A2[0:3]   | A3[0:3]   | A4[0:3]   | A5[0:3]   | A6[0:3]   | A7[0:3]   |
        +-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+

    1 byte gather, 8 block

        +-----------+-----------+-----------+-----------+
        | A0[0:7]   | A1[0:7]   | A2[0:7]   | A3[0:7]   |
        +-----------+-----------+-----------+-----------+
        | A4[0:7]   | A5[0:7]   | A6[0:7]   | A7[0:7]   |
        +-----------+-----------+-----------+-----------+

    Each table row represents one GRF, and A0[0] corresponds to the
    first element in <dst> in all of the above tables. The operand's
    type must be the same size as <block_size>.
```

