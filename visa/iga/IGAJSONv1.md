# IGA JSON

IGA's `-Xprint-jsonV1` emits kernel output in a `.json` format for consumption
via various tools.

**WARNING** This format is being deprecated and will soon be removed.  See IGAJSONv2 (now `-Xprint-json`) for the newer format.

**WARNING** Parsing JSON is not supported at the moment.

**WARNING** This is a partial specification; some keys/fields definitions may be absent.



# Example
Consider the following (nonsensical) instruction sequence.
Assume XeHPC (PVC) for this example.

    L0000:
             mov (8) r1<1>:d   0:d
    (W&f0.0) add (8) r1<1>:d  r[a0.4+0x4]<1;1,0>:d  r10.2<4;1,0>:b
             send.ugm (32|M0)  r10  r12  null:0  0x0  0x08803580 {$2}
    (W)      jmpi (1) L0000 {@4}


JSON output can be created from a binary.

     iga64 file.asm12p72 > file.krn12p72
     iga64 file.krn12p72 -Xprint-json

The following commands should create something akin to the following output.

    {
      "version":"1.0",  "platform":"xehpc",  "insts":[
        {"kind":"L","value":"L0000"},
        {"kind":"I", "id":1, "pred":null, "wren":false, "op":"mov", "subop":null, "es":8, "eo":0, "fm":null, "freg":null, "other":null,
          "dst":{"kind":"RD", "reg":{"rn":"r","r":1,"sr":0}, "sat":false, "rgn":{"Hz":1}, "type":"d","defs":[]},
          "srcs":[
            {"kind":"IM", "value":"0", "type":"d", "defs":[]}
          ], "regDist":null, "sbid":null, "opts":[], "comment":null},
        {"kind":"I", "id":2, "pred":{"inv":false, "func":"", "defs":[]}, "wren":true, "op":"add", "subop":null, "es":8, "eo":0, "fm":null, "freg":{"rn":"f","r":0,"sr":0}, "other":null,
          "dst":{"kind":"RD", "reg":{"rn":"r","r":1,"sr":0}, "sat":false, "rgn":{"Hz":1}, "type":"d","defs":[]},
          "srcs":[
            {"kind":"RI", "mods":"", "areg":{"rn":"a","r":0,"sr":4}, "aoff":4, "type":"d", "defs":[]},
            {"kind":"RD", "mods":"", "reg":{"rn":"r","r":10,"sr":2}, "rgn":{"Vt":4,"Wi":1,"Hz":0}, "type":"b", "defs":[]}
          ], "regDist":null, "sbid":null, "opts":[], "comment":null},
        {"kind":"I", "id":3, "pred":null, "wren":false, "op":"send", "subop":"ugm", "es":32, "eo":0, "fm":null, "freg":null, "other":null,
          "dst":{"kind":"DA", "reg":{"rn":"r","r":10,"sr":0}, "len":8},
          "srcs":[
            {"kind":"AD", "surf":{"type":"flat", "offset":0, "defs":[]}, "scale":1, "addr":{"reg":{"rn":"r","r":12,"sr":0}, "len":4, "defs":[]}, "offset":0},
            {"kind":"DA", "reg":{"rn":"null","r":0,"sr":0}, "len":0, "defs":[]},
            {"kind":"IM", "value":"0x0", "rgn":null, "type":null, "defs":[]},
            {"kind":"IM", "value":"0x8803580", "rgn":null, "type":null, "defs":[]}
          ], "regDist":null, "sbid":"$2", "opts":[], "comment":"gathering load"},
        {"kind":"I", "id":4, "pred":null, "wren":true, "op":"jmpi", "subop":null, "es":1, "eo":0, "fm":null, "freg":null, "other":null,
          "dst":null,
          "srcs":[
            {"kind":"LB", "target":"L0000", "type":null, "defs":[]}
          ], "regDist":"@4", "sbid":null, "opts":[], "comment":null},
        {"kind":"L","value":"L0064"}
      ]
    }


## Format

The top level object (a `Listing`) will contain the following fields/members.

  * `version` holds a version string
  * `platform` holds the platform this listing applies to
  * `insts` holds a list of listing elements (both instructions and labels);
    we call these `ListingElement`s.

`ListingElement`s are either labels or instructions.  All listing elements
have a `kind` field with either `"L"` to indicate label or `"I"` to indicate an
instruction object.

A label might look like the following.

        {"kind":"L","value":"L0000"},

It has a `value` with a string consisting of the label text.

An instruction has many more fields.  All instructions have the following fields.

  * `id` a unique identifier for that instruction (unique with respect to other instructions)
  * `pred` a predication object (can be `null` if no predication)
  * `wren` a bool indicating if WrEn (write-enable/NoMask) is set
  * `op` a string containing the operation mnemonic
  * `subop` either null or the sub-operation (e.g. SFID for send, function index for bfn instructions, ...)
  * `es` execution size (e.g. 1, 2, 4, 8, 16, or 32)
  * `eo` execution offset (`M0` would correspond to 0, `M16` would have 16)
  * `fm` flag modifier is set for instructions that write a flag
  * `freg` the flag register used in predication and flag modification; will be a `Reg` typed JSON object (for later reference) or `null` if absent
  * `other` may be `null` or contain information on liveness analysis (if `-Xprint-defs` was included)
     for implicit sources absent from syntax (e.g. `acc0`)
  * `dst` either `null` or a destination operand `Operand`
  * `srcs` will be a list of `Operand`
  * `regDist` the register distance string (e.g. `"@4"`)
  * `sbid` either `null` or any SBID set in the instruction options (e.g. `"$4.dst"`)
  * `opts` a list of extra instruction options not specified above
  * `comment` an extra per-instruction comments the disassembler might want to include (or `null`)

## `Reg` Objects
A bare register is a register name (`rn`) along with a register number (`r`) and subregister number (`sr`).
The register `f2.1` and `r12.4` would render as the following.

     {"rn":"f","r":2,"sr":1}
     {"rn":"r","r":12,"sr":4}


## `Operand` Objects
Operands are objects with varying formats.  All will contain a `kind` field
to distinguish the format.  The various kinds follow.
Most/all operands have the following fields.

* `kind` distinguishes the operand kind
* `defs` contains a list of which instructions define sources used in the current. **NOTE** this is `null` if dataflow analysis is off (`-Xprint-defs` to enable).
* `type` the operand type (as a string) or `null` if not set

If dataflow analysis is enabled most/all will have a `defs` field containing a
list of which instructions define sources used by the current instruction.

* `LB` a label will also have a `target` key with a string indicating the label target
* `IM` an immediate source operand will have a `value` and possibly a `type` field
* `RD` a register direct operand
* `RI` a register indirect operand

Where relevent these fields will contain the `Reg` object for the direct or
indirect register used and may include `Rgn` (region) objects for regions used.

`Rgn` objects all include a `Hz` field for the horizontal stride.
Sources may include `Vt` and `Wi` if those fields are present
(in some formats these are implicit fields).

Send instructions have additional operand kinds:
* `AD` address: an address type (possibly with surface info), an offset, payload size
* `DA` data: a register an payload, and
* `IM` immediate: for immediate send descriptors.
The `-Xprint-ldst` option might impact operand layout.
