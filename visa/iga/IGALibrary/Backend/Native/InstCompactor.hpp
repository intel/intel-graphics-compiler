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
#ifndef IGA_BACKEND_NATIVE_INSTCOMPACTOR_HPP
#define IGA_BACKEND_NATIVE_INSTCOMPACTOR_HPP

#include "Field.hpp"
#include "MInst.hpp"
#include "InstEncoder.hpp"


namespace iga {
    class InstCompactor : public BitProcessor {
        const Model &model;

        const OpSpec *os = nullptr;
        Subfunction sfs;

        MInst compactedBits;
        MInst uncompactedBits;
        CompactionDebugInfo *compactionDebugInfo = nullptr;
        bool compactionMissed = false;

        // the compaction result (if compaction enabled)
        CompactionResult compactionResult = CompactionResult::CR_NO_COMPACT;

        CompactionResult tryToCompactImpl();
        CompactionResult tryToCompactImplFamilyGen12();
    public:
        InstCompactor(BitProcessor &_parent, const Model &_model)
            : BitProcessor(_parent)
            , model(_model)
        {
        }

        CompactionResult tryToCompact(
            const OpSpec *_os,
            Subfunction _sfs,
            MInst _uncompactedBits, // copy-in
            MInst *compactedBitsOutput, // output
            CompactionDebugInfo *cdbi)
        {
            os = _os;
            sfs = _sfs;
            uncompactedBits = _uncompactedBits;
            compactedBits.qw0 = compactedBits.qw1 = 0;
            compactionDebugInfo = cdbi;

            auto cr = tryToCompactImpl();
            if (cr == CompactionResult::CR_SUCCESS) {
                // only clobber their memory if we succeed
                *compactedBitsOutput = compactedBits;
            }
            return cr;
        }
        CompactionResult tryToCompact(
            const OpSpec *_os,
            MInst _uncompactedBits, // copy-in
            MInst *compactedBitsOutput, // output
            CompactionDebugInfo *cdbi)
        {
            return tryToCompact(
                _os,
                MathFC::INVALID,
                _uncompactedBits,
                compactedBitsOutput,
                cdbi);
        }

        ///////////////////////////////////////////////////////////////////////
        // used by child implementations
        const OpSpec &getOpSpec() const {return *os;}

        unsigned getSourceCount() const {
            return os->getSourceCount(sfs);
        }

        uint64_t getUncompactedField(const Field &f) const {
            return uncompactedBits.getField(f);
        }

        void setCompactedField(const Field &f, uint64_t val) {
            compactedBits.setField(f, val);
        }

        bool getCompactionMissed() const {return compactionMissed;}

        void setCompactionResult(CompactionResult cr) {compactionResult = cr;}

        void transferField(
            const Field &compactedField, const Field &nativeField)
        {
            IGA_ASSERT(compactedField.length() == nativeField.length(),
                "native and compaction field length mismatch");
            compactedBits.setField(
                compactedField,
                uncompactedBits.getField(nativeField));
        }

        // returns false upon failure (so we can exit compaction early)
        //
        // the immLo and immHi values tells us the range of bits that hold
        // immediate value bits; the bounds are [lo,hi): inclusive/exclusive
        bool compactIndex(const CompactionMapping &cm, int immLo, int immHi);

        bool compactIndex(const CompactionMapping &cm) {
            return compactIndex(cm, 0, 0);
        }

        void fail(const CompactionMapping &cm, uint64_t mappedValue) {
            if (compactionDebugInfo) {
                compactionDebugInfo->fieldOps.push_back(os->op);
                compactionDebugInfo->fieldMisses.push_back(&cm);
                compactionDebugInfo->fieldMapping.push_back(mappedValue);
            }
            compactionMissed = true;
        }
    }; // InstCompactor

    //
    void CompactGen12(Platform p, InstCompactor &ic);
};

#endif // IGA_BACKEND_NATIVE_INSTCOMPACTOR_HPP
