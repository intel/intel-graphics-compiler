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
#include <string>
#include <sstream>
#include <ostream>

#include "IRToString.hpp"

using namespace iga;




std::string iga::ToSyntax(MathFC sf)
{
    switch (sf) {
    case MathFC::COS:   return "cos";
    case MathFC::EXP:   return "exp";
    case MathFC::FDIV:  return "fdiv";
    case MathFC::IDIV:  return "idiv";
    case MathFC::IQOT:  return "iqot";
    case MathFC::IREM:  return "irem";
    case MathFC::INV:   return "inv";
    case MathFC::INVM:  return "invm";
    case MathFC::LOG:   return "log";
    case MathFC::POW:   return "pow";
    case MathFC::RSQT:  return "rsqt";
    case MathFC::RSQTM: return "rsqtm";
    case MathFC::SIN:   return "sin";
    case MathFC::SQT:   return "sqt";
    default:
        return fmtHex(static_cast<uint32_t>(sf)) + "?";
    }
}
template <> MathFC iga::FromSyntax<MathFC>(std::string syn)
{
    for (auto sf : ALL_MathFCs) {
        if (syn == ToSyntax(sf))
            return sf;
    }
    return MathFC::INVALID;
}


std::string iga::ToSyntax(SFID sfid)
{
    switch (sfid) {
    case SFID::CRE:  return "cre";
    case SFID::DC0:  return "dc0";
    case SFID::DC1:  return "dc1";
    case SFID::DC2:  return "dc2";
    case SFID::DCRO: return "dcro";
    case SFID::GTWY: return "gtwy";
    case SFID::NULL_: return "null";
    case SFID::RC:   return "rc";
    case SFID::PIXI: return "pixi";
    case SFID::SMPL: return "smpl";
    case SFID::TS:   return "ts";
    case SFID::URB:  return "urb";
    case SFID::VME:  return "vme";
    default:
        return fmtHex(static_cast<uint32_t>(sfid)) + "?";
    }
}
template <> SFID iga::FromSyntax<SFID>(std::string syn)
{
    for (SFID sf : ALL_SFIDS) {
        if (syn == ToSyntax(sf))
            return sf;
    }
    return SFID::INVALID;
}




std::string iga::ToSyntax(SyncFC sfc)
{
    switch (sfc) {
    case SyncFC::NOP:   return "nop";
    case SyncFC::ALLRD: return "allrd";
    case SyncFC::ALLWR: return "allwr";
    case SyncFC::BAR:   return "bar";
    case SyncFC::HOST:  return "host";
    default:
        return fmtHex(static_cast<uint32_t>(sfc)) + "?";
    }
}
template <> SyncFC iga::FromSyntax<SyncFC>(std::string syn)
{
    for (auto sf : ALL_SyncFCs) {
        if (syn == ToSyntax(sf))
            return sf;
    }
    return SyncFC::INVALID;
}

