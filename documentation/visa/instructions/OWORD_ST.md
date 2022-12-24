<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  OWORD_ST = 0x36

## Format

| | | | | |
| --- | --- | --- | --- | --- |
| 0x36(OWORD_ST) | Size | Surface | Offset | Src |


## Semantics


```

      for (i = 0; i < num_owords; ++i) {
        surface[offset+i] = src[i]; //16 byte, oword-aligned
      }
```

## Description





```
    Writes contiguous owords (one oword is 16 byte) to <surface> starting at <offset>, taking the values from <Src>. The execution mask is set to 'NoMask' (i.e., every element is written to).
```


- **Size(ub):**

  - Bit[2..0]: Number of owords to write

    - 0b000:  1 oword
    - 0b001:  2 owords
    - 0b010:  4 owords
    - 0b011:  8 owords
    - {XEHP+}0b100:  16 owords. may only be used when surface is T0

- **Surface(ub):** Index of the surface variable. It must be a buffer.

                - T0 (SLM): {ICLLP+} Yes. No for earlier platforms
                - T5 (stateless): yes

- **Offset(scalar):** The offset of the write in units of owords. Must have type UD


- **Src(raw_operand):** The raw operand of a general variable storing the values to be written


#### Properties
- **Out-of-bound Access:** On write: data is dropped.




## Text
```



    OWORD_ST (<size>) <surface> <offset> <src>
```

