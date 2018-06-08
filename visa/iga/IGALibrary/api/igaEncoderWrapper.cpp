// IGA headers
#include "../Backend/GED/Encoder.hpp"
#include "igaEncoderWrapper.hpp"

iga_status_t KernelEncoder::encode()
{

    iga::ErrorHandler errHandler;
    iga::Encoder enc(kernel->getModel(),
        errHandler, iga::EncoderOpts(autoCompact, true));
    enc.encodeKernel(
        *kernel,
        kernel->getMemManager(),
        buf,
        binarySize);
#ifdef _DEBUG
    if (errHandler.hasErrors()) {
        // failed encode
        for (auto &e : errHandler.getErrors()) {
            std::cerr <<
                "line " << e.at.line <<
                ", col " << e.at.col << ": " <<
                e.message << "\n";
        }
        return IGA_ERROR;
    }
    if (errHandler.hasWarnings()) {
        for (auto &w : errHandler.getWarnings()) {
            std::cerr <<
                "line " << w.at.line <<
                ", col " << w.at.col << ": " <<
                w.message << "\n";
        }
        // fallthrough to IGA_SUCCESS
    }
#endif // _DEBUG
    return IGA_SUCCESS;
}