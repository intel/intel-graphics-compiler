 

## Opcode

  LOC = 0x52

## Format

| |
| --- |
| 0x52(LOC) | Line_number |


## Semantics




      Source line number for the subsequent instructions.

## Description



    Specify the source line number for the subsequent instructions. The
    source line number remains in effect until the next LOC instruction
    changes it. This instruction is used for debugging purposes.

- **Line_number(ud):** Source line number for the subsequent instruction

#### Properties


## Text
```
    

		LOC <line_number>
```



## Notes


