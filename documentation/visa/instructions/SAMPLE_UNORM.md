<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  SAMPLE_UNORM = 0x41

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x41(SAMPLE_UNORM) | Channel_mask | Sampler | Surface | U_offset | V_offset | DeltaU |
|                    | DeltaV       | Dst     |         |          |          |        |


## Semantics


```

                    Sample a UNORM surface
```

## Description





```
    Samples an UNORM <surface> using the indicated sampler state.
```


- **Channel_mask(ub):**

  - Bit[3..0]: determines the write masks for the RGBA channel, with R being bit 0 and A bit 3. At least one channel must be enabled (i.e., "0000" is not allowed).

  - Bit[5..4]: is Output format control.

    - 0b00:  "16-bit full". Two bytes will be returned for each pixel
    - 0b01:  "16-bit chrominance downsampled". Like the previous one, except only even pixels are returned for R and B channels
    - 0b10:  "8-bit full". One byte is returned for each pixel
    - 0b11:  "8-bit chrominance downsampled". Like the previous one, except only even pixels are returned for R and B channels

- **Sampler(ub):** Index of the sampler variable


- **Surface(ub):** Index of the surface variable


- **U_offset(scalar):** the normalized x coordinate of pixel 0. Must have type F


- **V_offset(scalar):** the normalized y coordinate of pixel 0. Must have type F


- **DeltaU(scalar):** the difference in coordinates for adjacent pixels in the X direction. Must have type F


- **DeltaV(scalar):** the difference in coordinates for adjacent pixels in the Y direction. Must have type F


- **Dst(raw_operand):** The raw operand of a general variable storing the result of the sample. The variable must have 32 * num_enabled_channels elements, with the disabled channels skipped in the results. Must have type UW


#### Properties
- **SIMD Control Flow:** channel enable is ignored




## Text
```



    SAMPLE_UNORM.<channel>.<output_format> <sampler> <surface> <u_offset> <v_offset> <deltaU> <deltaV> <dst>

    //<output_format> is one of "16-full", "16-downsampled", "8-full", "8-downsampled"
```
## Notes





