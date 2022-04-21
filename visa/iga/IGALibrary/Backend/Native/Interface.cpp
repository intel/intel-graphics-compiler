/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Interface.hpp"
#include "InstEncoder.hpp"
#include "InstCompactor.hpp"
#include "../BitProcessor.hpp"
#include "../../strings.hpp"

#include <vector>


using namespace iga;


///////////////////////////////////////////////////////////////////////////
//
// BTS (Big Theory Statement)
//
// In this scheme there are two encoders:
//   * a "parent" encoder, which is concerned with how we iterate over
//     the kernel and order instructions etc...  Examples of this are a
//     a serial or a parallel encoder.  E.g. SerialEncoder
//
//   * A "child" or instruction encoder (InstEncoder), concerned with
//     encoding a single instruction at a time only.  It is blissfully
//     unaware of any clever parallelism or other tricks.
//
// The general encoding algorithm consists of two phases:
//
//   1. Encoding.  Initial encoding encodes most instruction fields
//      but may not be able to resolve backpatches since later instructions
//      may not have known sizes at the moment.
//
//   2. Backpatching.  After all instructions have a determined size,
//      we iterate through all backpatches which required this information.
//      Typically, this just involves patching a jump field or two.
//
// The parent encoding algorithm in this file calls into
//   InstEncoder::encodeInstruction<P>
// with whatever platform P that is being targeted.  This class takes the
// instruction and a pointer the output and returns the encoded size.
// It may also register backpatches for something that needs resolution
// later (labels).
//
// Specific encoders for various platform instances are instances of the
// InstEncoder::encodeInstruction<P> method.   Typically these will redirect
// into lower-level modules with specific information about where fields are
// located (e.g. see Native/PreG12 for older encodings).
//
///////////////////////////////////////////////////////////////////////////
static size_t encodeInst(
    InstEncoder &enc,
    const EncoderOpts &opts,
    int ix, // instruction's index in the output array
    Instruction *inst,
    MInst *bits)
{
    bool mustCompact = inst->hasInstOpt(InstOpt::COMPACTED);
    bool mustntCompact = inst->hasInstOpt(InstOpt::NOCOMPACT);

    CompactionDebugInfo *cbdi = nullptr;
    CompactionDebugInfo mustCompactDebugInfo;
    if (mustCompact || (opts.autoCompact && !mustntCompact)) {
        // if {Compacted} is on we will raise a warning or error, we hope
        // to give them good info on why this failed
        cbdi = &mustCompactDebugInfo;
    }

    // encode native
    enc.encodeInstruction(ix, *inst, bits);

    if (mustCompact || (opts.autoCompact && !mustntCompact)) {
        // attempt compaction
        InstCompactor ic(enc, enc.getModel());
        MathFC mfc = inst->is(Op::MATH) ? inst->getMathFc() : MathFC::INVALID;

        auto cr = ic.tryToCompact(&inst->getOpSpec(), mfc, *bits, bits, cbdi);
        switch (cr) {
        case CompactionResult::CR_MISS:
        case CompactionResult::CR_NO_FORMAT:
        {
            std::stringstream ss;
            ss << "unable to compact instruction with {Compact} option: ";
            if (cr == CompactionResult::CR_NO_FORMAT) {
                ss << "not a compactable format";
            } else {
                // "foo, bar, and baz all miss"
                // "foo and bar miss"
                size_t len = cbdi->fieldMisses.size();
                for (size_t i = 0; i < len; i++) {
                    const CompactionMapping *cIx = cbdi->fieldMisses[i];
                    if (i > 0) {
                        ss << ",";
                    }
                    if (i == len - 1 && len >= 3) {
                        ss << " and ";
                    } else {
                        ss << " ";
                    }
                    uint64_t mappedValue = cbdi->fieldMapping[i];
                    ss << "for index " << cIx->index.name << " lacks";
                    if (cIx->format) {
                        ss << " (";
                        ss << cIx->format(inst->getOp(), cbdi->fieldMapping[i]);
                        ss << "): ";
                    }
                    ss <<  "0x" << std::hex << mappedValue << ": ";
                    // convert the field mapping value to separated form
                    // e.g. 00`0110`...
                    // mapped fields are from high bits to low bits
                    cIx->emitBinary(ss, mappedValue);
                }
                // TODO: could emit the closest mappings if there's only
                // a couple with a low enough hamming distance
                // (see -Xdcmp for that logic)
            }
            if (opts.explicitCompactMissIsWarning) {
                enc.warningAtT(inst->getLoc(), ss.str());
            } else {
                enc.errorAtT(inst->getLoc(), ss.str());
            }
            break;
        }
        default: break; // CR_SUCCESS or CR_NO_COMPACT
        }
    } // not trying to compact

    if (bits->isCompact()) {
        return 8;
    } else {
        return 16;
    }
}

struct SerialEncoder : BitProcessor
{
    const EncoderOpts    &opts;
    const Model          &model;
    uint8_t              *instBufBase = nullptr;
    int                   instBufTotalBytes = 0; // valid number of bytes in the buffer to be returned

    InstEncoder           instEncoder;
    std::vector<MInst*>   encodedInsts; // pointers into where each instruction starts

    SerialEncoder(
        ErrorHandler &errHandler,
        const EncoderOpts &_opts,
        const Model &_model)
        : BitProcessor(errHandler), opts(_opts), model(_model)
        , instEncoder(_opts, *this, _model)
    {
    }

    void resolveBackpatches()
    {
        // resolve backpatches
        for (auto &bp : instEncoder.getBackpatches()) {
            instEncoder.resolveBackpatch(bp, encodedInsts[bp.state.instIndex]);
        }
    }

    void encodeKernel(Kernel &k)
    {
#ifndef IGA_DISABLE_ENCODER_EXCEPTIONS
        try {
#endif
            instEncoder.getBackpatches().clear();
            encodedInsts.clear();
            encodedInsts.reserve(k.getInstructionCount());

            // preallocate buffer to return
            size_t allocLen = k.getInstructionCount() * UNCOMPACTED_SIZE;
            if (allocLen == 0) // for empty kernel case, allocate something
                allocLen = sizeof(MInst);
            instBufBase = (uint8_t *)k.getMemManager().alloc(allocLen);
            if (!instBufBase) {
                fatalAtT(0, "failed to allocate memory for kernel binary");
                return;
            }

            // walk through the instructions encoding each one at a time
            uint8_t *instBufCurr = instBufBase;
            int instIx = 0;
            for (auto blk : k.getBlockList()) {
                blk->setPC((PC)(instBufCurr - instBufBase));
                for (auto i : blk->getInstList()) {
                    encodedInsts.push_back((MInst *)instBufCurr);
                    i->setPC((PC)(instBufCurr - instBufBase));
                    size_t iLen = encodeInst(
                        instEncoder,
                        opts,
                        instIx++,
                        i,
                        (MInst *)instBufCurr);
                    instBufCurr += iLen;
                }
            }
            instBufTotalBytes = (int)(instBufCurr - instBufBase);

            resolveBackpatches();
#ifndef IGA_DISABLE_ENCODER_EXCEPTIONS
        } catch (const iga::FatalError&) {
            // error is already reported
        }
#endif
    }
};


static void EncodeSerial(
    const Model &model,
    const EncoderOpts &opts,
    ErrorHandler &eh,
    Kernel &k,
    void *&bits,
    size_t &bitsLen)
{
    SerialEncoder se(eh, opts, model);
    se.encodeKernel(k);
    if (!eh.hasErrors()) {
        bits = se.instBufBase;
        bitsLen = (size_t)se.instBufTotalBytes;
    } else {
        bits = nullptr;
        bitsLen = 0;
    }
}

#if 0
// index each instruction
// keep a base pointer with the minimum compacted instruction
// each checks to see if their block is the current minimum when committing
// if so, copy out the block and bump that pointer

static const size_t CHUNK_SIZE = 8;
struct EncoderWorker
{
    const Model &model;
    const EncoderOpts &opts;

    std::vector<Backpatch> patches;
    volatile bool done = false;
    volatile bool &hasFatalError;

    std::vector<Instruction *> &instQueue;

    EncoderWorker(
        const Model &_model,
        const EncoderOpts &_opts,
        std::vector<Instruction *> &_instQueue,
        volatile bool &_hasFatalError
    )
        : model(_model)
        , opts(_opts)
        , instQueue(_instQueue)
        , hasFatalError(_hasFatalError)
    {
    }
    EncoderWorker(const EncoderWorker&) = delete;

    void run()
    {
        // continuously grab an instruction and run it
        std::vector<Instruction *> chunk;
        chunk.reserve(8);

        // shared condition variable for the lock so only one thread is
        // changing the lock at a time
        ErrorHandler eh;
        BitProcessor bp(eh);
        while (!done &&
            eh.hasFatalError())
        {
            // lock
            if (!instQueue.empty()) {
                Instruction *inst = nullptr;
                {
                    inst = instQueue.back();
                    instQueue.pop_back();
                }
                for (Instruction *inst : chunk) {
                    encodeInst(inst, bp);
                }
                chunk.clear();
            }
        }
    }
    void encodeInst(Instruction *i, BitProcessor &bp)
    {
        bp.setCurrInst(i);
        // TODO: call into the serial algorithm
    }
};

static void EncodeParallel(
    const Model &model,
    EncoderOpts &opts,
    ErrorHandler &eh,
    Kernel &k,
    void *&bits,
    int &bitsLen)
{
    std::vector<Instruction *> instQueue;
    instQueue.reserve(256);
    volatile bool fatalError;

    EncoderWorker worker(model, opts, instQueue, fatalError);
#ifndef IGA_DISABLE_ENCODER_EXCEPTIONS
    try {
#endif
        // would start the threads here
        for (auto blk : k.getBlockList()) {
            for (auto i : blk->getInstList()) {
                instQueue.emplace_back(i);
            }
        }
#ifndef IGA_DISABLE_ENCODER_EXCEPTIONS
    } catch (const iga::FatalError&) {
        // error is already reported
    }
    // TODO: kill any threads
#endif
}
#endif
bool iga::native::IsEncodeSupported(
    const Model &m,
    const EncoderOpts &)
{
    switch (m.platform)
    {
    case Platform::FUTURE:
    default:
        break;
    }
    return false;
}

void iga::native::Encode(
    const Model &model,
    const EncoderOpts &opts,
    ErrorHandler &eh,
    Kernel &k,
    void *&bits,
    size_t &bitsLen)
{
    IGA_ASSERT(IsEncodeSupported(model, opts), "platform not supported; "
        "caller should have checked via iga::native::IsEncodeSupported");
    //
    // EncodeParallel(model, opts, eh, k, bits, bitsLen);
    EncodeSerial(model, opts, eh, k, bits, bitsLen);
}

///////////////////////////////////////////////////////////////////////////////
// decoding interfaces
bool iga::native::IsDecodeSupported(
    const Model &m,
    const DecoderOpts &)
{
    switch (m.platform)
    {
    case Platform::FUTURE: break;
    default: break;
    }
    return false;
}


Kernel *iga::native::Decode(
    const Model &m,
    const DecoderOpts &dopts,
    ErrorHandler &eh,
    const void *bits,
    size_t bitsLen)
{
    IGA_ASSERT(IsDecodeSupported(m, dopts), "invalid platform for decode; "
        "caller should have checked via iga::native::IsDecodeSupported");
    eh.reportError(Loc(0), "feature unavailable");
    return nullptr;
}


void iga::native::DecodeFields(
    Loc loc,
    const Model &m,
    const void *bits,
    FragmentList &fields,
    ErrorHandler &eh)
{
    DecoderOpts dopts;
    IGA_ASSERT(IsDecodeSupported(m, dopts), "invalid platform for decode; "
        "caller should have checked via iga::native::IsDecodeSupported");
    //
    eh.reportError(loc, "feature unavailable");
}


CompactionResult iga::native::DebugCompaction(
    const Model &m,
    const void *inputBits,
    void *compactedOutput, // optional
    CompactionDebugInfo &info)
{
    DecoderOpts dopts;
    IGA_ASSERT(IsDecodeSupported(m, dopts), "invalid platform for decode; "
        "caller should have checked via iga::native::IsDecodeSupported");

    MInst *mi = (MInst *)inputBits;
    if (mi->isCompact()) {
        // already compact, no need to try and compact it
        // do copy it out though
        if (compactedOutput) {
            memcpy(compactedOutput, inputBits, 8);
        }
        return CompactionResult::CR_SUCCESS;
    }

    OpSpecMissInfo missInfo;
    const OpSpec &os = m.lookupOpSpecFromBits(inputBits, missInfo);
    if (!os.isValid()) {
        return CompactionResult::CR_NO_FORMAT;
    }
    ErrorHandler eh;
    BitProcessor bp(eh);
    InstCompactor ic(bp, m);
    MInst dummyOutput;
    MInst *outputBits = compactedOutput == nullptr ?
        &dummyOutput :
        (MInst *)compactedOutput;
    return ic.tryToCompact(&os, *mi, outputBits, &info);
}

