# IGA JSON

IGA's `-Xprint-json` emits kernel output in a `.json` format for consumption
via various tools.  Other `-Xprint-*` options may affect the output.
For instance, `-Xprint-ldst` emits load/store syntax where supported
within the JSON, and `-Xprint-defs` includes dependency information on
instructions.

**WARNING** Parsing JSON is not supported at the moment.

**WARNING** This is a partial specification; some keys/fields definitions may be absent.

**WARNING** This format is volatile and may change without notice.

Many fields are omitted if not present (rather than set to `null` or something).


# Example
Consider the following (nonsensical) instruction sequence.
Assume XeHPC (PVC) for this example.

            nop
    begin:
    (f1.0)   add (32|M0)          r10.0<1>:f   r20.0<1;1,0>:f  r30.7<0;1,0>:f {A@1}
            mul (16|M0) (nz)f0.0 r30.0<1>:f   r10.4<0;1,0>:f  3.141:f {F@1}
    (W&f0.0) jmpi begin
    (W)     send.gtwy (1|M0)  null  r112  null:0  0x0  0x02000010  {EOT,A@1}


JSON output can be created from a binary.

     iga64 file.asm12p72 > file.krn12p72
     iga64 file.krn12p72 -Xprint-json -Xprint-defs

The following commands should create something akin to the following output.

    {
      "version":"2.0",
      "platform":"xehpc",
      "elems":[
        {"kind":"L", "id":0, "symbol":"L0000"},
        {"kind":"I", "id":0, "op":"nop", "es":1, "eo":0,
          "srcs":[], "liveTotals":{"r":200,"acc":0,"s":0,"f":4,"a":0}},
        {"kind":"L", "id":1, "symbol":"L0016", "preds":[1], "succs":[1,2]},
        {"kind":"I", "id":1, "pred":{"inv":true, "func":"", "defs":[]}, "op":"add", "es":32, "eo":0, "freg":{"rn":"f","r":1,"sr":0},
          "dst":{"kind":"RD", "reg":{"rn":"r","r":10,"sr":0}, "rgn":{"h":1}, "type":"f", "uses":[2]},
          "srcs":[
            {"kind":"RD", "reg":{"rn":"r","r":20,"sr":0}, "rgn":{"v":1,"w":1,"h":0}, "type":"f", "defs":[]},
            {"kind":"RD", "reg":{"rn":"r","r":30,"sr":7}, "rgn":{"v":0,"w":1,"h":0}, "type":"f", "defs":[2]}
          ], "regDist":"A@1", "sbid":{"id":1, "func":".dst"}, "liveTotals":{"r":200,"acc":0,"s":0,"f":4,"a":0}},
        {"kind":"I", "id":2, "op":"mul", "es":16, "eo":0, "fm":{"cond":"ne"}, "freg":{"rn":"f","r":0,"sr":0},
          "dst":{"kind":"RD", "reg":{"rn":"r","r":30,"sr":0}, "rgn":{"h":1}, "type":"f", "uses":[1]},
          "srcs":[
            {"kind":"RD", "reg":{"rn":"r","r":10,"sr":4}, "rgn":{"v":0,"w":1,"h":0}, "type":"f", "defs":[1]},
            {"kind":"IM", "value":"3.141", "type":"f"}
          ], "regDist":"F@1", "liveTotals":{"r":196,"acc":0,"s":0,"f":4,"a":0}},
        {"kind":"I", "id":3, "pred":{"func":"", "defs":[2]}, "wren":true, "op":"jmpi", "es":1, "eo":0, "freg":{"rn":"f","r":0,"sr":0},
          "srcs":[
            {"kind":"LB", "target":"L0016"}
          ], "liveTotals":{"r":200,"acc":0,"s":0,"f":5,"a":0}},
        {"kind":"L", "id":2, "symbol":"L0064", "preds":[1], "succs":[3]},
        {"kind":"I", "id":4, "wren":true, "op":"send", "subop":"gtwy", "es":1, "eo":0,
          "dst":{"kind":"DA", "reg":{"rn":"null","r":0,"sr":0}, "len":0},
          "srcs":[
            {"kind":"AD", "surf":{"atype":"invalid", "offset":0}, "scale":1, "addr":{"reg":{"rn":"r","r":112,"sr":0}, "len":1}, "offset":0},
            {"kind":"DA", "reg":{"rn":"null","r":0,"sr":0}, "len":0},
            {"kind":"IM", "value":"0"},
            {"kind":"IM", "value":"2000010"}
          ], "regDist":"A@1", "opts":["Atomic","EOT"], "liveTotals":{"r":64,"acc":0,"s":0,"f":0,"a":0}, "comment":"end of thread"},
        {"kind":"L", "id":3, "symbol":"L0080", "preds":[2]}
      ]
    }


## Format

The top level object (a `Listing`) will contain the following fields/members.

  * `version` holds a version string
  * `platform` holds the platform this listing applies to
  * `elems` holds a list of listing elements (both instructions and labels);
    we call these `ListingElement`s.

`ListingElement`s are either labels or instructions.  All listing elements
have a `kind` field with either `"L"` to indicate label or `"I"` to indicate an
instruction object.  Other fields within these elements vary based on this.

### Labels

A label might look like the following.

        {"kind":"L", "id":2, "value":"L0016", "preds":[2], "succs":[2,3]},

Labels have the following fields.

  * `kind` is `"L"` for labels
  * `id` is a unique integer name for the label (**NOTE**: these can overlap with instruction id's)
  * `pc` the program counter is present if `-Xprint-pcs` enabled
  * `symbol` is a string value for the label.
  * `preds` and `succs` are lists of ids indicating predecessor and successor blocks.


### Instructions

Instructions have the following fields.  Many are absent if not applicable
for a given instruction and a default value should be inferred.

  * `id` a unique identifier for that instruction (unique with respect to other instructions)
  * `pc` the program counter (only present if `-Xprint-pcs` enabled)
  * `pred` a predication object if present, which has an optional `inv:Bool` (defaults to false) and `func:String` subfields; for the latter the empty string `""` indicates sequential predication evaluation (the usual case); e.g. `"pred":{"inv":True,func:".all",defs:[3]}`; the object also has optional `defs`
  * `wren` a bool indicating if WrEn (write-enable/NoMask) is set
  * `op` a string containing the operation mnemonic
  * `subop` the sub-operation (e.g. SFID for send, function index for bfn instructions, ...); this field may be absent.
  * `es` execution size of the instruction
  * `eo` execution offset (`M0` would correspond to 0, `M16` would have 16); if absent 0 is assumed.
  * `fm` is the flag modifier function (if present); the object has a `"cond"` string subfield and may have a `uses` as well; e.g. `"fm":{"cond":"lt"}`; (*NOTE:* that `sel` does not write a flag)
  * `freg` the flag register used in predication and flag modification; will be a `Reg` typed JSON object (for later reference); (*NOTE:* that `sel` does not write a flag)
  * `dst` an optional destination operand `Operand`
  * `srcs` is an optional list of source `Operand`s
  * `implicit` is an object with `defs` and `uses` for implicitly written or read registers (e.g. `addc` writes `acc0` and `mach` reads it).
  * `regDist` the register distance string (e.g. `"@4"`)
  * `sbid` either `null` or any SBID set in the instruction options (e.g. `"$4.dst"`)
  * `opts` a list of extra instruction options not specified above
  * `comment` an extra per-instruction comments the disassembler deigns to generate



## `Operand` Objects
Operands are objects with varying formats.  All will contain a `kind` field
to distinguish the format.  The various kinds follow.

All operands have a `kind` field to distinguish variants.
Most operands have the following fields.

* `uses` contains a list of which instructions use a destination `-Xprint-defs` is enabled.
* `defs` contains a list of which instructions define sources used in the current. **NOTE** this is `null` if dataflow analysis is off (`-Xprint-defs` to enable).
* `type` the operand type (as a string); some operands lack a type (labels or send descriptors)

If dataflow analysis (`-Xprint-defs`) is enabled operands that are read
will have a `defs` field containing a list of which instructions define
sources used by the current instruction.
In operands that are writen `uses` will be included.



### Label Operands: `"kind":"LB"`
A label have a `target` key with a string indicating the label target.

    (W&f0.0) jmpi begin

Generates the object `{"kind":"LB", "target":"L0016"}`

### Immediate Operands: `"kind":"IM"`
An immediate source operand will have a `value` and possibly a `type` field.
Immediate send descriptors will lack a `type`.

The `value` is represented as a string.  It's up to the user to parse that
further based on type or whatnot.  Consider `-Xprint-hex-floats` if
hex floats are desired in the `value` field value.

### Register Direct Operands: `"kind":"RD"`
A direct register operand has a `Reg` object
* `reg` is the register reference (a `Reg` object)
* `rgn` is the operand's region (an `Rgn`)
* `type` contains the operand type (a string)

Send descriptor register operands will lack a `type`.

### Register Indirect Operands: `"kind":"RI"`
A register indirect operand has the following fields.
* `areg` is the address register (a `Reg` object)
* `aoff` is the address offset (an integer)
* `rgn` is the operand's region (an `Rgn`)
* `type` contains the operand type (a string)

For example consider the following instruction.

    add (4) r[a0.8+0x20]<1>:ud  r[a0.0-0x10]<1,0>:ud  0x1:uw

This generates JSON including.

    ...
    "dst":{"kind":"RI", "areg":{"rn":"a","r":0,"sr":8}, "aoff":32, "rgn":{"h":1}, "type":"ud"},
    "srcs":[
    {"kind":"RI", "areg":{"rn":"a","r":0,"sr":0}, "aoff":-16, "rgn":{"w":1,"h":0}, "type":"ud"},
    ...


### Register Indirect Operands: `"kind":"DA"`
Send and load/store instructions (c.f. `-Xprint-ldst`) include a data payload
operand kind which consists of a register `reg` and usually an integer
payload length `len`.  In certain cases `len` is absent such as
when the instruction encoding lacks this information.


### Register Indirect Operands: `"kind":"AD"`
Send and load/store instructions (c.f. `-Xprint-ldst`) may include
an address payload operand which can be fairly complicated.

* `areg` the register being sent (usually a GRF); this is the primary payload
  and usually consists of addresses or header information
* `alen` the count of registers being sent as payload
* `adefs` the definitions of the address registers sent

In addition some optional fields may be included.

* `stype` the surface address type: `"flat"`, `"bti"`, `"bss"`, `"ss"`, ...
* `sbase` for any message supporting a base register
* `soff` for state base or base address register; for a BTI/BSS/SS this is the a0.# register
* `sdefs` any dependencies for the any surface or base register


## `Reg` Objects
A bare register is a register name (`rn`) along with a register number (`r`) and subregister number (`sr`).
The register `f2.1` and `r12.4` would render as the following.

     {"rn":"f","r":2,"sr":1}
     {"rn":"r","r":12,"sr":4}

## `Rgn` Objects
`Rgn` objects all include a `h` field for the horizontal stride.
Sources may include `v` and `w` if those fields are present
(in some formats these are implicit fields).

## `SBID` Objects
The `sbid` field will be an object consisting of the following.
* `id` the name of the SBID being accessed
* `func` the function: `"src"` (readout complete event), `"dst"` (writeback complete event), and `""` (allocation/initailization)

For example the SBID of `$2.src` would have the JSON representation
of `{"id":2,"func":".src"}`.

Later versions may include `defs`/`uses` for SBIDs.

