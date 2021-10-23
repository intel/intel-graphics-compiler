/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "MessageDecoder.hpp"

#include <tuple>
#include <utility>


using namespace iga;

void MessageDecoder::decodePayloadSizes() {
    bool hasMLenRLenInDesc = true;
    bool hasXLenInExDesc = platform() < Platform::XE_HPG;
    auto plural = [](int x) {return x == 1 ? "" : "s";};
    if (hasMLenRLenInDesc) {
        decodeDescField("Mlen", 25, 4,
            [&] (std::stringstream &ss, uint32_t val) {
                ss << val << " address register" << plural(val) << " written";
            });
        decodeDescField("Rlen", 20, 5,
            [&] (std::stringstream &ss, uint32_t val) {
                ss << val << " register" << plural(val) << " read back";
            });
    }
    if (hasXLenInExDesc) {
        decodeDescField("Xlen", 32 + 6, 5,
            [&] (std::stringstream &ss, uint32_t val) {
                ss << val << " data register" << plural(val) << " written";
            });
    }
    if (platform() <= Platform::GEN11) {
        decodeDescField("SFID", 32 + 0, 4,
            [] (std::stringstream &ss, uint32_t val) {
                ss << val << " shared function ID";
            });
    }
}

///////////////////////////////////////////////////////////////////////////////
// shared LSC fields (gateway EOT uses this)
void MessageDecoder::addLscFenceFields(
    std::stringstream &sym, std::stringstream &descs)
{
    auto ft = getDescBits(12, 3);
    std::stringstream ftss;
    switch (ft) {
    case 0:
        sym << ".none";
        ftss << "no op";
        break;
    case 1:
        sym << ".evict";
        ftss << "evict (dirty lines invalidated and evicted)";
        break;
    case 2:
        sym << ".invalidate";
        ftss << "invalidate (all clean lines, but do not evict)";
        break;
    case 3:
        sym << ".discard";
        ftss << "discard (dirty and clean lines invalidated "
            "without eviction)";
        break;
    case 4:
        sym << ".clean";
        ftss <<
            "clean (dirty lines written out, "
            "but kept in clean state)";
        break;
    case 5:
        sym << ".flushl3";
        ftss << "flush L3 only"; // XeHPG only
        break;
    default:
        sym << ".0x" << std::hex << ft << "?";
        ftss << "invalid flush type";
        error(12, 3, "invalid flush type");
    }
    descs << " " << ftss.str();
    addField("FlushOp", 12, 3, ft, ftss.str());
    //
    addLscFenceScopeField(sym, descs);
}

void MessageDecoder::addLscFenceScopeField(
    std::stringstream &sym, std::stringstream &descs)
{
    descs << " scoped to";
    std::stringstream scss;
    auto sc = getDescBits(9, 3);
    switch (sc) {
    case 0: sym << ".group"; scss << "thread group"; break;
    case 1: sym << ".local"; scss << "slice"; break;
    case 2: sym << ".tile"; scss << "tile"; break;
    case 3: sym << ".gpu"; scss << "gpu"; break;
    case 4: sym << ".gpus"; scss << "all gpus"; break;
    case 5: sym << ".sysrel"; scss << "system release"; break;
    case 6: sym << ".sysacq"; scss << "system acquire"; break;
    default:
        sym << ".0x" << std::hex << sc << "?";
        scss << "invalid flush scope";
        error(9, 3, "invalid flush scope");
    }
    descs << " " << scss.str();
    //
    addField("FlushScope", 9, 3, sc, scss.str());
}


void MessageDecoder::addLscFencePortFields(
    std::stringstream &sym, std::stringstream &descs)
{
    auto ports = getDescBits(12, 4);
    sym << ".0x" << std::hex << std::uppercase << ports;
    std::stringstream ss;
    //
    if (ports == 0x0) {
        ss << "None";
        descs << " unfenced";
    } else {
        descs << " fencing ";
        for (int i = 0; i < 4; i++) {
            if (ports & (1 << i)) {
                if (ports & (((1 << i) - 1))) {
                    ss << "|";
                    descs << ", ";
                }
                static const char *SYMS[] {"SLM", "UGM", "UGML", "TGM"};
                ss << SYMS[i];
                descs << SYMS[i];
            }
        }
    }
    addField("FenceDataPorts", 12, 4, ports, ss.str());
}
