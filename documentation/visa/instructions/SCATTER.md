<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

 

## Opcode

  SCATTER = 0x3a

## Format

| | | | | | |
| --- | --- | --- | --- | --- | --- |
| 0x3a(SCATTER) | Elt_size | Num_elts | Surface | Global_offset | Element_offset | Src |


## Semantics




      for (i = 0; i < exec_size; ++i) {
        if (ChEn[i]) {
          surface[global_offset+element_offset[i]] = src[i]; //1, 2, or 4 byte
        }
      }

## Description


    Performs scattered write into <surface>, using the values from <src>.

- **Elt_size(ub):** 
 
  - Bit[1..0]: encodes the byte size of each element
 
    - 0b00:  1 byte 
    - 0b01:  2 bytes 
    - 0b10:  4 bytes
- **Num_elts(ub):** 
 
  - Bit[1..0]: encodes the number of elements that will be written
 
    - 0b00:  8 elements 
    - 0b01:  16 elements 
    - 0b10:  1 element 
  - Bit[7..4]: encodes the execution mask as described in Table 4.

- **Surface(ub):** Index of the surface variable.  It must be a buffer. Valid values are:
 
  - 0: T0 - Shared Local Memory (SLM) access 
  - 5: T255 - Stateless surface access
- **Global_offset(scalar):** The global offset of all the elements in element size. Must have type UD

- **Element_offset(raw_operand):** The raw operand of a general variable storing the offset values. The first Num_elts elements will be used as the offsets into the surface, and they are in the unit of element size. Must have type UD

- **Src(raw_operand):** The variable providing the values to be written. The first num_elts elements will be used. The operand must have one of UD, D, F type; for 1 and 2 byte accesses the upper bits will be ignored

#### Properties
- **Out-of-bound Access:** On write: data is dropped.


## Text
```
    

		SCATTER.<elt_size> <surface> <global_offset> <element_offset> <src>
```



## Notes



    The behavior is undefined if more than one channel writes to the same address.
