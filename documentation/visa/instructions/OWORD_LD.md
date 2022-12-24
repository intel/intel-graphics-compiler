<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  OWORD_LD = 0x35

## Format

| | | | | | |
| --- | --- | --- | --- | --- | --- |
| 0x35(OWORD_LD) | Size | Is_modified | Surface | Offset | Dst |


## Semantics


```

      for (i = 0; i < num_owords; ++i) {
        dst[i] = surface[offset+i]; //16 byte, oword-aligned
      }
```

## Description





```
    Reads contiguous owords (one oword is 16 byte) from <surface> starting at <offset>, and stores the result into <dst>.  The execution mask is set to 'NoMask' (i.e., every element is returned).
```


- **Size(ub):**

  - Bit[2..0]: Number of owords to read

    - 0b000:  1 oword
    - 0b001:  2 owords
    - 0b010:  4 owords
    - 0b011:  8 owords
    - {XEHP+}0b100:  16 owords. may only be used when surface is T0

- **Is_modified(ub):** The field is ignored, the read always return the last write from this thread


- **Surface(ub):** Index of the surface variable. It must be a buffer.

                - T0 (SLM): {ICLLP+} Yes. No for earlier platforms
                - T5 (stateless): yes

- **Offset(scalar):** The offset of the read in units of owords. Must have type UD


- **Dst(raw_operand):** The raw operand of a general variable storing the results of the read


#### Properties
- **Out-of-bound Access:** On read: zeros are returned.




## Text
```



    OWORD_LD (<size>) <surface> <offset> <dst>
```

