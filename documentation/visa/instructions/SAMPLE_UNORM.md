<!---======================= begin_copyright_notice ============================

Copyright (c) 2019-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ==========================-->

 

## Opcode

  SAMPLE_UNORM = 0x41

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x41(SAMPLE_UNORM) | Channel_mask | Sampler | Surface | U_offset | V_offset | DeltaU |
|                    | DeltaV       | Dst     |         |          |          |        |


## Semantics




                    Sample a UNORM surface

## Description


    Samples an UNORM <surface> using the indicated sampler state.

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


