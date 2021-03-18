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

  OWORD_ST = 0x36

## Format

| | | | |
| --- | --- | --- | --- |
| 0x36(OWORD_ST) | Size | Surface | Offset | Src |


## Semantics




      for (i = 0; i < num_owords; ++i) {
        surface[offset+i] = src[i]; //16 byte, oword-aligned
      }

## Description


    Writes contiguous owords (one oword is 16 byte) to <surface> starting at <offset>, taking the values from <Src>. The execution mask is set to 'NoMask' (i.e., every element is written to).

- **Size(ub):** 
 
  - Bit[2..0]: Number of owords to write
 
    - 0b000:  1 oword 
    - 0b001:  2 owords 
    - 0b010:  4 owords 
    - 0b011:  8 owords
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


