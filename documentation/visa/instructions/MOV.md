 

## Opcode

  MOV = 0x29

## Format

| | | | |
| --- | --- | --- | --- |
| 0x29(MOV) | Exec_size | Pred | Dst | Src0 |


## Semantics




                    for (i = 0; < exec_size; ++i) {
                      if (ChEn[i]) {
                        dst[i] = src0[i]; //with type conversion
                      }
                    }

## Description


    Component-wise move from <src0> to <dst>, with type conversion if necessary.

- **Exec_size(ub):** Execution size
 
  - Bit[2..0]: size of the region for source and destination operands
 
    - 0b000:  1 element (scalar) 
    - 0b001:  2 elements 
    - 0b010:  4 elements 
    - 0b011:  8 elements 
    - 0b100:  16 elements 
    - 0b101:  32 elements 
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

- **Dst(vec_operand):** The destination operand. Operand class: general,indirect

- **Src0(vec_operand):** The first source operand. Operand class: general,indirect,immediate,predicate

#### Properties
- **Saturation:** Yes 
- **Source Modifier:** arithmetic 


## Text
```
    

		[(<P>)] MOV [.sat] (<exec_size>) <dst> <src0>
```



## Notes



    - **Supported Type:** BOOL (for predicate), HF, F, DF, B, UB, W, UW, D, UD, Q, UQ

    - If <dst> and <src0> have different type, format conversion will be performed following the type conversion rules in Section X. If <src0> is a predicate operand, <exec_size> must be one, and <dst> must have one of the types UB, UW, UD whose bit-size is greater than or equal to that of the source predicate variable's size. The value of the predicate variable is interpreted as an unsigned integer, with element 0 corresponding to the LSB, and copied to <dst>. Neither predication nor saturation is supported in this mode. Note that if the predicate is declared to have fewer than 16 elements, the upper bits in the <dst> variable will have undefined values.
