#include "../IR/Kernel.hpp"
#include "iga.h"

// entry point for binary encoding of a IGA IR kernel
class KernelEncoder
{
    void* buf = nullptr;
    uint32_t binarySize = 0;
    iga::Kernel* kernel = nullptr;
    bool autoCompact = false;

public:
    KernelEncoder(iga::Kernel* k, bool compact)
        : kernel(k)
        , autoCompact(compact) { }

    iga_status_t encode();
    void* getBinary() const { return buf; }
    uint32_t getBinarySize() const { return binarySize; }
};
