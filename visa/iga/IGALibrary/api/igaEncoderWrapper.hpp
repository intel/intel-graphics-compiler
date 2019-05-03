#include "../IR/Kernel.hpp"
#include "iga.h"

// entry point for binary encoding of a IGA IR kernel
class KernelEncoder
{
    void* m_buf = nullptr;
    uint32_t m_binarySize = 0;
    iga::Kernel* m_kernel = nullptr;
    bool m_autoCompact = false;

public:
    // @param compact: auto compact instructions if applicable
    // @param noCompactFisrtEightInst: Force NOCOMPACT the first 8 instructions in this encoding unit
    //     The first eight instructions must be in the same bb
    //     This can be set simulataneously with compact. The first 8 instructions will not be cmopacted
    //     even if it is compactable
    KernelEncoder(iga::Kernel* k, bool compact)
        : m_kernel(k)
        , m_autoCompact(compact)
    { }

    iga_status_t encode();
    void* getBinary() const { return m_buf; }
    uint32_t getBinarySize() const { return m_binarySize; }

    // patchImmValue - Decode the first instruction start from binary, and patch the imm field to given val
    // input type - the type of given immediate value
    // input val - the given immediate value
    // return - true on success, false if any error
    // This function is used by visa to patch the add or mov instructions' imm field for the indirect call
    // FIXME: Move this api to somewhere else that's more apporopriate
    static bool patchImmValue(const iga::Model& model, unsigned char* binary, iga::Type type, const iga::ImmVal &val);

};