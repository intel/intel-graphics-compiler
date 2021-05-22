<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

 

## Opcode

  GATHER = 0x39

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x39(GATHER) | Elt_size | Is_modified | Num_elts | Surface | Global_offset | Element_offset |
|              | Dst      |             |          |         |               |                |


## Semantics




                    for (i = 0; i < exec_size; ++i) {
                      if (ChEn[i]) {
                        dst[i] = surface[global_offset+element_offset[i]]; //1, 2, or 4 byte
                      }
                    }

## Description


    Performs 1, 8, or 16 element scattered read from <surface> and stores the result into <dst>.

- **Elt_size(ub):** 
 
  - Bit[1..0]: encodes the byte size of each element
 
    - 0b00:  1 byte 
    - 0b01:  2 bytes 
    - 0b10:  4 bytes
- **Is_modified(ub):** The field is ignored, the read always return the last write from this thread

- **Num_elts(ub):** 
 
  - Bit[1..0]: encodes the number of elements that will be read
 
    - 0b00:  8 elements 
    - 0b01:  16 elements 
    - 0b10:  1 element 
  - Bit[7..4]: encodes the execution mask as described in Table 4.

- **Surface(ub):** Index of the surface variable. It must be a buffer. Valid values are:
 
  - 0: T0 - Shared Local Memory (SLM) access 
  - 5: T255 - Stateless surface access
- **Global_offset(scalar):** The global offset of all elements, in the unit of element size. Must have type UD

- **Element_offset(raw_operand):** The first Num_elts elements will be used as the offsets (after adding the global offset) into the surface, and they are in the unit of element size. Must have type UD

- **Dst(raw_operand):** The variable storing the results of the read. The first num_elts elements will be written to. For 1 and 2 byte accesses the upper bytes have undefined values. Must have type UD,D,F

#### Properties
- **Out-of-bound Access:** On read: zeros are returned. 


## Text
```
    

		GATHER.<elt_size> <surface> <global_offset> <element_offset> <dst>
```



## Notes


