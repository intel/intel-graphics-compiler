<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  VME_IDM = 0x57

## Format

| | | | | |
| --- | --- | --- | --- | --- |
| 0x57(VME_IDM) | UNIInput | IDMInput | Surface | Output |


## Semantics


```

      Video Motion Estimation - distortion mesh.
```

## Description






    Performs Video Motion Estimation with distortion mesh output (IDM). See [4] for more detailed information on the VME functionality. This instruction may not appear inside a SIMD control flow block.


- **UNIInput(raw_operand):** The raw operand of a general variable that stores the universal VME payload data. Must have type UB. Must have 128 elements


- **IDMInput(raw_operand):** The raw operand of a general variable that stores the SIC specific payload data. Must have type UB. Must have 32 elements


- **Surface(ub):** The index of the surface variable


- **Output(raw_operand):** The raw operand of a general variable used to store the VME output data. Must have type UB. Must have 512 elements


#### Properties




## Text
```



    VME_IDM <surface> <UNIInput> <IDMInput> <output>
```
## Notes





