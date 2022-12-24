<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  VME_SIC = 0x55

## Format

| | | | | |
| --- | --- | --- | --- | --- |
| 0x55(VME_SIC) | UNIInput | SICInput | Surface | Output |


## Semantics


```

      Video Motion Estimation - skip and intra check.
```

## Description






    Performs Video Motion Estimation with skip and intra check (SIC). See [4] for more detailed information on the VME functionality. This instruction may not appear inside a SIMD control flow block.


- **UNIInput(raw_operand):** The raw operand of a general variable that stores the universal VME payload data. Must have type UB. Must have 128 elements


- **SICInput(raw_operand):** The raw operand of a general variable that stores the SIC specific payload data. Must have type UB. Must have 128 elements


- **Surface(ub):** The index of the surface variable


- **Output(raw_operand):** The raw operand of a general variable used to store the VME output data. Must have type UB. Must have 224 elements


#### Properties




## Text
```



    VME_SIC <surface> <UNIInput> <SICInput> <output>
```
## Notes





