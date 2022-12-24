<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

## Opcode

  VME_IME = 0x54

## Format

| | | | | | | |
| --- | --- | --- | --- | --- | --- | --- |
| 0x54(VME_IME) | StreamMode | SearchCtrl | UNIInput | IMEInput | Surface | Ref0 |
|               | Ref1       | CostCenter | Output   |          |         |      |


## Semantics


```

      Video Motion Estimation - integer motion estimation
```

## Description






    Performs Integer Motion Estimation. See [4] for more detailed information on the VME functionality. This instruction may not appear inside a SIMD control flow block.


- **StreamMode(ub):** VME stream mode. Valid values are:

  - 0: VME_STREAM_DISABLE
  - 1: VME_STREAM_OUT
  - 2: VME_STREAM_IN
  - 3: VME_STREAM_IN_OUT

- **SearchCtrl(ub):** VME search control state. Valid values are:

  - 0: VME_SEARCH_SINGLE_REF_SINGLE_REC_SINGLE_START
  - 1: VME_SEARCH_SINGLE_REF_SINGLE_REC_DUAL_START
  - 2: VME_SEARCH_SINGLE_REF_DUAL_REC
  - 3: VME_SEARCH_DUAL_REF_DUAL_REC

- **UNIInput(raw_operand):** The raw operand of a general variable that stores the universal VME payload data. Must have type UB. Must have 128 elements


- **IMEInput(raw_operand):** The raw operand of a general variable that stores the IME payload data. Must have type UB. Number of elements is:

      - 128 (field_value(StreamMode)!=0) and (field_value(SearchCtrl)!=3)
      - 64 field_value(StreamMode)==0
      - 192 (field_value(StreamMode)!=0) and (field_value(SearchCtrl)==3)

- **Surface(ub):** The index of the surface variable


- **Ref0(raw_operand):** The raw operand of a general variable containing the position of the left-top integer corner of the first reference window located in the first reference surface (in unit of pixels, relative to the surface origin). Must have type UW. Must have 2 elements


- **Ref1(raw_operand):** The raw operand of a general variable containing the position of the left-top integer corner of the second reference window located in the second reference surface (in unit of pixels, relative to the surface origin; ignored in single reference mode). Must have type UW. Must have 2 elements


- **CostCenter(raw_operand):** The raw operand of a general variable containing the coordinates for the cost centers relative to the picture source MB. Must have type UW. Must have 16 elements


- **Output(raw_operand):** The raw operand of a general variable used to store the VME output data. Must have type UB. Number of elements is:

      - 288 (field_value(StreamMode)!=0) and (field_value(SearchCtrl)!=3)
      - 224 field_value(StreamMode)==0
      - 352 (field_value(StreamMode)!=0) and (field_value(SearchCtrl)==3)

#### Properties




## Text
```



    VME_IME (<streamMode>, <searchCtrl>) <surface> <UNIInput> <IMEInput> <ref0> <ref1> <costCenter> <output>
```
## Notes





