<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  SVM = 0x4e
  BLOCK_ST = 0x02

## Format

| | | | |
| --- | --- | --- | --- |
| 0x4e(SVM) | 0x02(BLOCK_ST) | Properties | Address | Src |


## Semantics


```

                    for (i = 0; i < num_owords; ++i) {
                        *(address+i*16) = src[i]; //16 byte, oword-aligned
                    }
```

## Description





```
    Write contiguous owords (one oword is 16 byte) to the virtual address
    <address>, taking the values from <src>. The execution mask is set to
    "NoMask" (i.e., every element is returned).
```


- **Properties(ub):**

  - Bit[2..0]: encodes the number of owords to read

    - 0b000:  1 oword
    - 0b001:  2 oword
    - 0b010:  4 oword
    - 0b011:  8 oword

- **Address(scalar):** The write address in units of bytes. The address must be oword-aligned. Must have type UQ


- **Src(raw_operand):** The raw operand of a general variable storing the values to be written


#### Properties




## Text
```



    SVM_BLOCK_ST (<size>) <address> <src>
```
## Notes





