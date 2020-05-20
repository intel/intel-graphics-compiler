/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
//
/// genx::AlignmentInfo : alignment information
/// -------------------------------------------
/// 
/// AlignmentInfo is a cache of information on the alignment of instruction
/// values in a function. It does not persist between passes.
///
/// A pass that needs alignment information constructs an AlignmentInfo at 
/// the start of the pass, and then calls the ``get`` method each time it wants
/// alignment information for a particular instruction value. AlignmentInfo 
/// calculates it if it is not already in its cache, which probably involves 
/// also calculating the alignment of other instructions that the given one 
/// depends on.
///
/// This cacheing and lazy calculation is done instead of having a separate analysis
/// pass because alignment is needed for only a small subset of values in a function.
///
/// The alignment is returned as an *Alignment* object with three fields:
/// *ConstBits*, if ConstBits is not 0x7fffffff, alignment is a known bit-pattern,
/// otherwise *LogAlign* and *ExtraBits* (where 0 <= ExtraBits < (1 << LogAlign)),
/// stating that the value is known to be A << LogAlign | ExtraBits for some A.
///
/// For a vector value, the alignment information is for element 0.
///
/// The calculation uses a worklist algorithm that can cope with phi nodes and
/// loops. So, for example, a variable (used as an indirect region index) that
/// starts at 10 then is incremented by 8 inside a loop is correctly calculated
/// to be 8A+2 for some A.
///
//===----------------------------------------------------------------------===//

#ifndef GENXALIGNMENTINFO_H
#define GENXALIGNMENTINFO_H

#include "GenX.h"
#include "IgnoreRAUWValueMap.h"

namespace llvm {
  class raw_ostream;

namespace genx {

// Alignment : the alignment of a value
class Alignment {
  unsigned LogAlign;
  unsigned ExtraBits;
  unsigned ConstBits;
public:
  // No-arg constructor sets to uncomputed state.
  Alignment() { setUncomputed(); }
  // Constructor given LogAlign and ExtraBits fields.
  Alignment(unsigned LogAlign, unsigned ExtraBits)
  : LogAlign(LogAlign), ExtraBits(ExtraBits), ConstBits(0x7fffffff) {}
  // Constructor given literal value.
  Alignment(unsigned C);
  // Constructor given Constant.
  Alignment(Constant *C);
  // Copy-constructor
  Alignment(const Alignment& Rhs) {
    LogAlign = Rhs.LogAlign;
    ExtraBits = Rhs.ExtraBits;
    ConstBits = Rhs.ConstBits;
  }
  // Copy-operator
  Alignment& operator=(const Alignment &Rhs) {
    LogAlign = Rhs.LogAlign;
    ExtraBits = Rhs.ExtraBits;
    ConstBits = Rhs.ConstBits;
    return *this;
  }

  // Get an unknown alignment
  static Alignment getUnknown() { return Alignment(0, 0); }
  // Merge two Alignments
  Alignment merge(Alignment Other) const;
  // Add one Alignment with another Alignment
  Alignment add(Alignment Other) const;
  // Mul one Alignment with another Alignment
  Alignment mul(Alignment Other) const;

  // accessors
  bool isUncomputed() const { return LogAlign == 0xffffffff; }
  bool isUnknown() const { return LogAlign == 0 && ConstBits == 0x7fffffff; }
  bool isConstant() const { return !isUncomputed() && ConstBits != 0x7fffffff; }
  unsigned getLogAlign() const { assert(!isUncomputed()); return LogAlign; }
  unsigned getExtraBits() const { assert(!isUncomputed()); return ExtraBits; }
  int64_t getConstBits() const { assert(isConstant()); return ConstBits; }
  // comparison
  bool operator==(const Alignment &Rhs) const {
    return (LogAlign == Rhs.LogAlign &&
            ExtraBits == Rhs.ExtraBits &&
            ConstBits == Rhs.ConstBits);
  }
  // Debug dump/print
  void dump() const;
  void print(raw_ostream &OS) const;
private:
  void setUncomputed() {
    LogAlign = 0xffffffff;
    ExtraBits = 0;
    ConstBits = 0x7fffffff;
  }
};

// AlignmentInfo : cache of alignment of instructions in a function
class AlignmentInfo {
  ValueMap<const Value *, Alignment,
          IgnoreRAUWValueMapConfig<const Value *>> InstMap;
public:
  // AlignmentInfo constructor
  AlignmentInfo() {}
  // Clear the cache of value alignments
  void clear() { InstMap.clear(); }
  // get the alignment of a Value
  Alignment get(Value *V);
public:
  // return an Alignment for a value
  Alignment getFromInstMap(Value *V);
};

inline raw_ostream &operator<<(raw_ostream &OS, const Alignment &A) {
  A.print(OS);
  return OS;
}

} // end namespace genx
} // end namespace llvm

#endif /* GENXALIGNMENTINFO_H */
