<!---======================= begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ==========================-->

# Appendix: Debug Information

Debug information in vISA is emitted as structures mapping virtual
bytecode input to machine code output. There are several mapping tables
emitted and each is described below.

Debug Information Header
========================

Debug information header format is as follows:

```
    DebugFormatHeader
    {
      ud magic_number;
      uw numCompiledObjects;
      DebugInfoFormat debugInfo[numCompiledObjectsObjects];
    }
```

-   **magic_number**: A magic number used to identify debug information
    format used. This number is equal to 0xdeadd010 currently. Changes
    to debug information format will change this.
-   **numCompiledObjects**: Number of objects, ie kernels and stack call
    functions, having debug information in debug information stream.
-   **debugInfo:** Actual debug information for each compiled object.
    Compiled object refers to either a kernel or a stack call function.
    Debug information format per object is as follows:

```
    DebugInfoFormat
    {
      uw objectNameLen;
      ub objectName[objectNameLen];
      ud reloc_offset;
      MappingTable offsetMap;
      MappingTable indexMap;
      VarInfoMap vars;
      uw numSubs
      SubroutineInfo subInfo[numSubs];
      CallFrameInfo frameInfo;
    }
```

-   **nameLen:** Length of name of object.
-   **objectName:** Name of the object. Not null terminated.
-   **relocOffset:** Represents offset of first instruction in binary
    buffer. Value is 0 for kernels, non-zero for stack call functions.
-   **offsetMap:** Mapping for vISA instruction offset to Gen
    instruction offset. Base of vISA offset is first instruction of
    compilated object. Offsets expressed in bytes. This table is
    optional and may be absent when vISA file is not generated. When
    absent, offsetMap.numElements field is set to 0.
-   **indexMap:** Mapping for vISA instruction index from vISA input to
    Gen instruction offset. vISA instruction index starts at 0 for first
    vISA instruction and increases in unit step.
-   **vars:** Table mapping virtual variables with physical assigned
    storage location.
-   **numSubs:** Number of sub-routines that are part of this
    compilation object.
-   **subInfo:** Debugging information for each included sub-routine.
-   **frameInfo:** Debug information about call frame in presence of
    stack call callees.

```
    MappingTable
    {
      ud numElements;
      VISAMap data[numElements];
    }
```

-   **numElements:** Number of entries in mapping table.
-   **<data:**> Actual mapping from vISA instruction offset to Gen
    instruction.

```
    VISAMap
    {
      ud visaOffset/visaIndex;
      ud genOffset;
    }
```

-   **visaOffset/visaIndex:** Byte offset/vISA index in vISA input.
-   **genOffset:** Byte offset of corresponding Gen instruction in
    compiled code. If a vISA instruction leads to several Gen
    instruction, this points to first one. All Gen instructions between
    current genOffset and next mapping entry's genOffset are a result of
    lowering current vISA instruction. In case of scheduled code, Gen
    instructions resulting from a single vISA instruction could be
    scattered in generated code. In such cases, union of all such
    entries need to be considered to map to a single vISA instruction.
    Generated mapping is best effort and there could be some vISA
    instructions absent due to optimizations.

```
    VarInfoMap
    {
      ud numElements;
      VarInfo var[numElements];
    }
```

-   **numElements:** Number of elements in variable mapping table.
-   **var:** Actual mapping entry. Valid only when optimizations are
    disabled.

```
    VarInfo
    {
      uw nameLen;
      ub varName[nameLen];
      VarLiveIntervalsVISA lr;
    }
```

-   **nameLen:** Number of characters in name field.
-   **varName:** Non null terminated string of characters.
-   **lr:** Live-internal information in expressed in vISA index.

```
    VarLiveIntervalsVISA/VarIntervalsGenISA
    {
      uw numIntervals;
      IntervalVISA/GenISA intervals[numIntervals];
    }
```

-   **numIntervals:** Number of intervals having valid mapping.
-   **intervals:** Actual live-intervals.

```
    IntervalVISA/GenISA
    {
      uw start/ud start;
      uw end/ud end;
      ub virtualType;
      ub physicalType;
      union
      {
        Register
        {
          uw regnum;
          uw subRegNum;
        }
        Memory
        {
          ud isBaseOffBEFP : 1;
          d memoryOffset : 31;
        }
      }
    }
```

-   **start:** Variable is uw type for IntervalVISA and represents vISA
    index where current variable becomes live, ie has a valid value.
    Variable is ud type for IntervalGenISA type and represents Gen ISA
    start offset in bytes. Entire variable becomes live even if only a
    subset of elements are defined.
-   **end:** Variable is uw type for IntervalVISA and represents vISA
    index where current variable's value is no longer available.
    Variable is ud type for IntervalGenISA type and represents Gen ISA
    end offset in bytes. A variable is no longer live once all elements
    are no longer used.

-   **virtualType:** Can take one of following values:

|  Value          |  Interpretation |
| --- | --- |
|  0              |  Address vISA variable |
|  1              |  Flag vISA variable |
|  2              |  General vISA variable |

-   **physicalType:** Can take one of following values:

|  Value          |  Interpretation |
| --- | --- |
|  0              |  Address vISA variable |
|  1              |  Flag vISA variable |
|  2              |  General vISA variable |
|  3              |  Memory |

-   **regNum:** Valid if physicalType is 0, 1, 2. Indicates physical
    register number holding vISA variable.
-   **subRegNum:** Valid if physicalType is 0, 1, 2. Indicates physical
    sub-register number holding vISA variable. Expressed in byte units.
-   **isBaseOffBEFP:** Valid if physicalType is 3. Size is 1 bit.
    Bit-field set to 0 if vISA variable offset is represented based off
    BE_FP (ie, back-end frame pointer). If 1, it represents absolute
    offset in scratch space.
-   **memoryOffset:** Bit-field with size 31 bits. Valid if physicalType
    is 3. Field holding signed memory offset in byte units.

```
    SubroutineInfo
    {
      uw nameLen;
      ub name[nameLen];
      ud start;
      ud end;
      VarLiveIntervalsGenISA retVal;
    }
```

-  **nameLen:** Number of characters in name field.
-  **name:** Name of sub-routine. Not null terminated.
-  **start:** Index of first vISA instruction in sub-routine.
-  **end:** Index of last vISA instruction in sub-routine.
-  **retval:** Live-interval information of variable holding return
   address. This can be used to determine call site from where
   control was transferred.

```
    CallFrameInfo
    {
      uw frameSizeInBytes;
      ub befpValid;
      VarLiveIntervalsGenISA befp;
      ub callerbefpValid;
      VarLiveIntervalsGenISA callerbefp;
      ub retAddrValid;
      VarLiveIntervalsGenISA retAddr;
      uw numCalleeSaveEntries;
      PhyRegSaveInfoPerIP calleeSaveEntry[numCalleeSaveEntries];
      uw numCallerSaveEntries;
      PhyRegSaveInfoPerIP callerSaveEntry[numCallerSaveEntries];
    }
```

-   **frameSizeInBytes:** Number of bytes used by current frame.
-   **befpValid:** Set to 1 if BE_FP pointer is valid. When back-end
    stack is not used such as for standalone kernel, this will be 0.
-   **befp:** Live-interval of BE_FP. Present only when befpValid is
    set to 1. Field absent and should not be read when befpValid is 0.
-   **callerbefpValid:** Indicates whether BE_FP of caller frame is
    valid. Can be used to virtually unwind stack.
-   **callerbefp:** Live-interval of caller's BE_FP. Valid only when
    callerbefpValid is 1.
-   **retAddrValid:** Set to 1 when current object is a stack call
    function. It indicates that current object may return to caller
    frame.
-   **retAddr:** Live-interval of variable holding return address of
    current frame. Valid only if retAddrValid is 1.
-   **numCalleeSaveEntries:** Number of entries in callee save area
    table. Useful during virtual stack unwinding.
-   **calleeSaveEntry:** Caller frame's variables allocated to callee
    save area will be written out to memory on current frame so current
    compiled object can reuse such registers. To retrieve value of such
    variables from caller frame allocated to callee save area, a
    debugger would need to read memory location on current frame where
    callee save physical registers are stored. This member provides
    relevant mapping between callee save physical register and its
    location on current frame. Callee save registers are stored in
    memory before first vISA instruction in a frame and restored just
    before fret vISA instruction.
-   **numCallerSaveEntries:** Number of entries in caller save area
    table. Useful during virtual stack unwinding.
-   **callerSaveEntry:** Any caller save registers live across a stack
    call site will be stored in memory of current frame before the
    function call and will be restored after returning from the call.
    Caller save registers will be reused by callee stack call function.
    In callee stack call function, if debugger wants to virtually unwind
    stack and query a caller frame variable that was allocated to a
    caller save register, debugger needs to lookup this table to get the
    location where the variable's value is available.

```
    PhyRegSaveInfoPerIP
    {
      ud genIPOffset;
      uw numEntries;
      RegInfoMapping data[numEntries];
    }
```

-   **genIPOffset:** Caller/callee save entry at this Gen IP.
-   **numEntries:** Number of mapping entries for caller/callee save
    area.
-   **<data:**> Holds mapping information for caller/callee save
    registers and their storage location in memory. This is useful for
    virtual stack unwinding.

```
    RegInfoMapping
    {
      uw srcRegOff;
      uw numBytes;
      ub dstInReg;
      Mapping dst;
    }
```

-   **srcRegOff:** Physical register for which this caller/callee save
    entry is emitted. Physical register is expressed in byte units and
    zero based off GRF file. So r0.0 is expressed as 0 and r1.0 is
    expressed as 32.
-   **numBytes:** Total number of bytes beginning srcRegOff for this
    caller/callee save entry.
-   **dstInReg:** Indicates whether physical register in srcRegOff is
    stored in a register or memory.
-   **dst:** Actual location where registers indicated by srcRegOff and
    numBytes are stored.
