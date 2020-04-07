# Instruction Set

Format of Instruction Descriptions
----------------------------------

This section describes each virtual ISA instruction. Instructions are
grouped based on their functionality (e.g., arithmetic, data movement),
and instructions in the same group generally share a common format.
Descriptions for each instruction are organized in the following format:

-   **MNEMONIC**: Short description of the instruction
-   **Opcode**: NAME=&lt;opcode\_value&gt;. Each instruction has a
    distinct opcode with values from 0 to 255 (0xFF).
-   **Format**: The binary format of the instruction.
-   **Semantics**: Semantic description of the instruction, usually in
    terms of C operations.
-   **Description**: A longer description detailing the constraints on
    the source/destination operands, the operation performed, etc.
-   **Text**: Assembly format of the instruction.
-   **Notes**: Comments that are not strictly part of the
    specification. They provide implementation hints and insights to
    the design rationale for the instruction.

3D Extensions 
-------------

## 

- [3D_LOAD - 3D sampler load](instructions/3D_LOAD.md)

- [3D_SAMPLE - 3D sampler](instructions/3D_SAMPLE.md)

- [3D_SAMPLE4 - 3D sampler gather4](instructions/3D_SAMPLE4.md)

- [INFO - surface info](instructions/INFO.md)

- [RT_WRITE - render target write](instructions/RT_WRITE.md)

- [TYPED_ATOMIC - atomic operations for typed surfaces](instructions/TYPED_ATOMIC.md)

- [URB_WRITE - URB write](instructions/URB_WRITE.md)


Address Instructions 
--------------------

## 

- [ADDR_ADD - address arithmetic](instructions/ADDR_ADD.md)


Arithmetic Instructions 
-----------------------

## 

- [ADD - addition](instructions/ADD.md)

- [ADDC - addition with carry](instructions/ADDC.md)

- [AVG - integer average](instructions/AVG.md)

- [COS - cosine](instructions/COS.md)

- [DIV - division](instructions/DIV.md)

- [DIVM - IEEE floating point divide {-ICLLP}](instructions/DIVM.md)

- [EXP - exponent](instructions/EXP.md)

- [FRC - fraction](instructions/FRC.md)

- [INV - reciprocal](instructions/INV.md)

- [LOG - logarithm](instructions/LOG.md)

- [LRP - linear interpolation {-ICLLP}](instructions/LRP.md)

- [LZD - leading zero detection](instructions/LZD.md)

- [MAD - multiply and add](instructions/MAD.md)

- [MOD - remainder of division](instructions/MOD.md)

- [MUL - multiply](instructions/MUL.md)

- [MULH - multiply high](instructions/MULH.md)

- [PLN - plane {-TGLLP}](instructions/PLN.md)

- [POW - power](instructions/POW.md)

- [RNDD - round down](instructions/RNDD.md)

- [RNDE - round to even](instructions/RNDE.md)

- [RNDU - round up](instructions/RNDU.md)

- [RNDZ - round to zero](instructions/RNDZ.md)

- [RSQRT - inverse square root](instructions/RSQRT.md)

- [SAD2 - two-wide sum of absolute {-TGLLP}](instructions/SAD2.md)

- [SAD2ADD - two-wide sum of absolute differences and addition {-TGLLP}](instructions/SAD2ADD.md)

- [SIN - sine](instructions/SIN.md)

- [SQRT - Rinverse square root](instructions/SQRT.md)

- [SQRTM - ieee square root {-ICLLP}](instructions/SQRTM.md)

- [SUBB - subtraction with borrow](instructions/SUBB.md)


Comparison Instructions 
-----------------------

## 

- [CMP - comparison](instructions/CMP.md)


Control Flow Instructions 
-------------------------

## 

- [CALL - subroutine call](instructions/CALL.md)

- [FADDR - function address](instructions/FADDR.md)

- [FCALL - function call](instructions/FCALL.md)

- [FRET - function return](instructions/FRET.md)

- [GOTO - SIMD goto](instructions/GOTO.md)

- [IFCALL - indirect function call](instructions/IFCALL.md)

- [JMP - jump](instructions/JMP.md)

- [LABEL - label declaration](instructions/LABEL.md)

- [RET - Ffunction return](instructions/RET.md)

- [SUBROUTINE - subroutine](instructions/SUBROUTINE.md)

- [SWITCHJMP - switch jump table](instructions/SWITCHJMP.md)


Data Movement Instructions 
--------------------------

## 

- [MIN_MAX - minimum or maximum](instructions/MIN_MAX.md)

- [MOV - move](instructions/MOV.md)

- [MOVS - move special](instructions/MOVS.md)

- [SEL - select](instructions/SEL.md)

- [SETP - set predicate value](instructions/SETP.md)


Logic and Shift Instructions 
----------------------------

## 

- [AND - bitwise and](instructions/AND.md)

- [ASR - arithmetic shift right](instructions/ASR.md)

- [BFE - bit field extract](instructions/BFE.md)

- [BFI - bit field insert](instructions/BFI.md)

- [BFREV - bit field reverse](instructions/BFREV.md)

- [CBIT - count bits set](instructions/CBIT.md)

- [FBH - find first bit from MSB side](instructions/FBH.md)

- [FBL - find first bit from LSB side](instructions/FBL.md)

- [NOT - bitwise not](instructions/NOT.md)

- [OR - bitwise](instructions/OR.md)

- [ROL - rotate left {ICLLP+}](instructions/ROL.md)

- [ROR - rotate right {ICLLP+}](instructions/ROR.md)

- [SHL - shift left](instructions/SHL.md)

- [SHR - shift right](instructions/SHR.md)

- [XOR - bitwise xor](instructions/XOR.md)


Media Extensions 
----------------

## 

- [AVS - adaptive video scaling](instructions/AVS.md)

- [VME_FBR - VME fractional and bidirectional refinement](instructions/VME_FBR.md)

- [VME_IDM - VME distortion mesh](instructions/VME_IDM.md)

- [VME_IME - VME integer motion estimation](instructions/VME_IME.md)

- [VME_SIC - VME skip and intra check](instructions/VME_SIC.md)


Miscellaneous Instructions 
--------------------------

## 

- [FILE - file name](instructions/FILE.md)

- [LIFETIME - lifetime start or end of a variable](instructions/LIFETIME.md)

- [LOC - line number](instructions/LOC.md)

- [RAW_SEND - raw message send](instructions/RAW_SEND.md)

- [RAW_SENDS - raw message split send](instructions/RAW_SENDS.md)


Sample Instructions 
-------------------

## 

- [SAMPLE_UNORM - sampler UNORM](instructions/SAMPLE_UNORM.md)


Surface-based Memory Access Instructions 
----------------------------------------

## 

- [DWORD_ATOMIC - dword untyped atomic messages](instructions/DWORD_ATOMIC.md)

- [GATHER - byte scattered read {DEPRECATED}](instructions/GATHER.md)

- [GATHER4_SCALED - scaled untyped surface read](instructions/GATHER4_SCALED.md)

- [GATHER4_TYPED - typed surface read](instructions/GATHER4_TYPED.md)

- [GATHER_SCALED - byte scaled read](instructions/GATHER_SCALED.md)

- [MEDIA_LD - media load](instructions/MEDIA_LD.md)

- [MEDIA_ST - media store](instructions/MEDIA_ST.md)

- [OWORD_LD - oword load](instructions/OWORD_LD.md)

- [OWORD_LD_UNALIGNED - unaligned oword load](instructions/OWORD_LD_UNALIGNED.md)

- [OWORD_ST - oword store](instructions/OWORD_ST.md)

- [SCATTER - byte scatter write {DEPRECATED}](instructions/SCATTER.md)

- [SCATTER4_SCALED - scaled untyped surface write](instructions/SCATTER4_SCALED.md)

- [SCATTER4_TYPED - typed surface write](instructions/SCATTER4_TYPED.md)

- [SCATTER_SCALED - byte scaled write](instructions/SCATTER_SCALED.md)


SVM - Shared Virtual Memory Access 
----------------------------------

## 

- [SVM_BLOCK_LD - SMV Block Load](instructions/SVM_BLOCK_LD.md)

- [SVM_GATHER4_SCALED - SVM gather4 with scaling pitch](instructions/SVM_GATHER4_SCALED.md)

- [SVM - Shared Virtual Memory Access - Generic](instructions/SVM.md)

- [SVM_ATOMIC - SVM atomic operations](instructions/SVM_ATOMIC.md)

- [SVM_BLOCK_ST - SMV Block Store](instructions/SVM_BLOCK_ST.md)

- [SVM_SCATTER - SMV scatter](instructions/SVM_SCATTER.md)

- [SVM_GATHER - SMV gather](instructions/SVM_GATHER.md)

- [SVM_SCATTER4_SCALED - SVM scatter4 with scaling pitch](instructions/SVM_SCATTER4_SCALED.md)


Synchronization Instructions 
----------------------------

## 

- [BARRIER - thread group barrier](instructions/BARRIER.md)

- [CACHE_FLUSH - textural cache flush](instructions/CACHE_FLUSH.md)

- [FENCE - memory fence](instructions/FENCE.md)

- [SBARRIER - split-phase barrier](instructions/SBARRIER.md)

- [WAIT - wait for thread dependencies](instructions/WAIT.md)

- [YIELD - yield to another thread](instructions/YIELD.md)


