<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  GATHER4_TYPED = 0x4b

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x4b(GATHER4_TYPED) | Exec_size | Pred | Channels | Surface | U | V |
|                     | R         | LOD  | Dst      |         |   |   |


## Semantics


```

                    UD ch_pos = 0;
                    for (c = 0; c < 4; ++c) {
                      if (ch_mask[c] == 1) {
                        for (i = 0; i < exec_size; ++i) {
                          if (ChEn[i]) {
                            UD ch_start = ch_pos * max(exec_size, GRF_SIZE / 4);  // GRF_SIZE is the register size in byte
                            dst[ch_start+i] = *(&surface[u,v,r] + c); //one pixel, with type conversion
                          }
                        }
                        ch_pos++;
                      }
                    }
```

## Description





```
    Performs <exec_size>*<num_enabled_channels> dword scattered read from <surface>, which must be typed, and stores the result into <dst>.
```


- **Exec_size(ub):** Execution size

  - Bit[2..0]: size of the region for source and destination operands

    - 0b011:  8 elements
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


- **Surface(ub):** Index of the surface variable. It must be a 1D, 2D, or 3D surface.

            - T0 (SLM): no
            - T5 (stateless): no

- **U(raw_operand):** The first Exec_size elements contain the U offset. Restriction: type UD


- **V(raw_operand):** The first Exec_size elements contain the V offset. Restriction: type UD


- **R(raw_operand):** The first Exec_size elements contain the R offset. Restriction: type UD


- **LOD(raw_operand):** The first Exec_size elements contain the LOD. Restriction: type UD


- **Dst(raw_operand):** The variable storing the results of the read. For each enabled channel in RGBA order, <exec_size> elements will be returned to <dst> subject to predication. The next enabled channel will have its return data starting at the next register. If a channel's return data do not occupy the entire register, the remaining part of the register has undefined values. The operand must have one of UD, D, F type


#### Properties
- **Out-of-bound Access:** On read: ones are returned in the alpha channel, zeros in the other channels.




## Text
```



    [(<P>)] GATHER4_TYPED.<channels> (<exec_size>) <surface> <u> <v> <r> <lod> <dst>

    //<channels> is one of R, G, B, A, RG, RB, RA, RGB, RGBA, GB, GA, GBA, BA
```
## Notes





    If an offset operand is not applicable for the surface accessed (e.g., V and R for 1D surfaces), it should be set to V0 (the null variable).

