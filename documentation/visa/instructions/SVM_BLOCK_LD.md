<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  SVM = 0x4e
  BLOCK_LD = 0x01

## Format

| | | | |
| --- | --- | --- | --- |
| 0x4e(SVM) | 0x01(BLOCK_LD) | Properties | Address | Dst |


## Semantics


```

                    for (i = 0; i < num_owords; ++i) {
                        dst[i] = *(address+i*16); //16 byte, oword- or dword-aligned
                    }
```

## Description





```
    Reads contiguous owords (one oword is 16 byte) from the virtual address
    <address>, and stores the result into <dst>. The execution mask is set
    to "NoMask" (i.e., every element is returned).
```


- **Properties(ub):**

  - Bit[2..0]: Number of owords to read

    - 0b000:  1 oword
    - 0b001:  2 owords
    - 0b010:  4 owords
    - 0b011:  8 owords
  - Bit[3]: indicates whether the address needs to be oword aligned.

    - 0b0:  address must be oword aligned
    - 0b1:  address can be dword aligned

- **Address(scalar):** The read address in units of bytes. Must have type UQ


- **Dst(raw_operand):** The raw operand of a general variable storing the results of the read


#### Properties




## Text
```



    SVM_BLOCK_LD[.unaligned] (<size>) <address> <dst>
```
## Notes





