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

//////////////////////////////////////////////////////////////////////////
// Creates an enumerated list of ops via macro callback.
// This keeps the op table stable as we add and remove elements
//
// Formally, this is transposing the Expression Problem:
//  + adding new variants is easy (1 line)
//  - adding new attributes/properties requires potentially
//    modifying multiple variants (macro cleverness with defaults reduces
//    some of that sting)
#ifndef DEFINE_SEND_OP
#error "DEFINE_SEND_OP(ENUM, MNMEONIC, ATTRS) not defined"
// #define DEFINE_SEND_OP(E, M, A)
#endif

#ifndef ATTRS_NONE
// in case of recursive additions
#define ATTRS_NONE          iga::SendOpInfo::Attr::NONE
#define ATTRS_SCALARADDR    iga::SendOpInfo::Attr::IS_SCALAR_ADDR
#define ATTRS_CMASK         iga::SendOpInfo::Attr::HAS_CMASK
#define ATTRS_ATOMIC_UNR \
    iga::SendOpInfo::Attr::GROUP_ATOMIC | iga::SendOpInfo::Attr::ATOMIC_UNARY
#define ATTRS_ATOMIC_BIN \
    iga::SendOpInfo::Attr::GROUP_ATOMIC | iga::SendOpInfo::Attr::ATOMIC_BINARY
#define ATTRS_ATOMIC_TER \
    iga::SendOpInfo::Attr::GROUP_ATOMIC | iga::SendOpInfo::Attr::ATOMIC_TERNARY
#endif

#ifndef DEFINE_LOAD_OP
// users can override this if needed
#define DEFINE_LOAD_OP(E, M, A) \
    DEFINE_SEND_OP(E, M, (A)|iga::SendOpInfo::Attr::GROUP_LOAD)
#endif
#ifndef DEFINE_STORE_OP
#define DEFINE_STORE_OP(E, M, A) \
    DEFINE_SEND_OP(E, M, (A)|iga::SendOpInfo::Attr::GROUP_STORE)
#endif
#ifndef DEFINE_ATOMIC_UNARY_OP
#define DEFINE_ATOMIC_UNARY_OP(E, M) \
    DEFINE_SEND_OP(E, M, ATTRS_ATOMIC_UNR)
#endif
#ifndef DEFINE_ATOMIC_BINARY_OP
#define DEFINE_ATOMIC_BINARY_OP(E, M) \
    DEFINE_SEND_OP(E, M, ATTRS_ATOMIC_BIN)
#endif
#ifndef DEFINE_ATOMIC_TERNARY_OP
#define DEFINE_ATOMIC_TERNARY_OP(E, M) \
    DEFINE_SEND_OP(E, M, ATTRS_ATOMIC_TER)
#endif
#ifndef DEFINE_OTHER_OP
#define DEFINE_OTHER_OP(E, M, A) \
    DEFINE_SEND_OP(E, M, (A)|iga::SendOpInfo::Attr::GROUP_OTHER)
#endif


DEFINE_LOAD_OP(LOAD,          "load",            ATTRS_NONE)
DEFINE_LOAD_OP(LOAD_STRIDED,  "load_strided",    ATTRS_SCALARADDR) // AKA load_block
DEFINE_LOAD_OP(LOAD_QUAD,     "load_quad",       ATTRS_CMASK) // AKA load_cmask
DEFINE_LOAD_OP(LOAD_STATUS,   "load_status",     ATTRS_NONE)

DEFINE_STORE_OP(STORE,          "store",          ATTRS_NONE)
DEFINE_STORE_OP(STORE_STRIDED,  "store_strided",  ATTRS_SCALARADDR) // AKA store_block
DEFINE_STORE_OP(STORE_QUAD,     "store_quad",     ATTRS_CMASK) // AKA store_cmask

DEFINE_ATOMIC_UNARY_OP(ATOMIC_LOAD, "atomic_load")
DEFINE_ATOMIC_BINARY_OP(ATOMIC_STORE, "atomic_store")
//
DEFINE_ATOMIC_BINARY_OP(ATOMIC_AND, "atomic_and")
DEFINE_ATOMIC_BINARY_OP(ATOMIC_OR,  "atomic_or")
DEFINE_ATOMIC_BINARY_OP(ATOMIC_XOR, "atomic_xor")
//
DEFINE_ATOMIC_BINARY_OP(ATOMIC_IINC,  "atomic_iinc")
DEFINE_ATOMIC_BINARY_OP(ATOMIC_IDEC,  "atomic_idec")
DEFINE_ATOMIC_BINARY_OP(ATOMIC_IPDEC, "atomic_ipdec")
DEFINE_ATOMIC_BINARY_OP(ATOMIC_IADD,  "atomic_iadd")
DEFINE_ATOMIC_BINARY_OP(ATOMIC_ISUB,  "atomic_isub")
DEFINE_ATOMIC_BINARY_OP(ATOMIC_IRSUB, "atomic_irsub")
DEFINE_ATOMIC_TERNARY_OP(ATOMIC_ICAS,  "atomic_icas")

DEFINE_ATOMIC_BINARY_OP(ATOMIC_SMIN,  "atomic_smin")
DEFINE_ATOMIC_BINARY_OP(ATOMIC_SMAX,  "atomic_smax")
//
DEFINE_ATOMIC_BINARY_OP(ATOMIC_UMIN,  "atomic_umin")
DEFINE_ATOMIC_BINARY_OP(ATOMIC_UMAX,  "atomic_umax")
//
DEFINE_ATOMIC_BINARY_OP(ATOMIC_FADD,  "atomic_fadd")
DEFINE_ATOMIC_BINARY_OP(ATOMIC_FSUB,  "atomic_fsub")
DEFINE_ATOMIC_BINARY_OP(ATOMIC_FMIN,  "atomic_fmin")
DEFINE_ATOMIC_BINARY_OP(ATOMIC_FMAX,  "atomic_fmax")
DEFINE_ATOMIC_TERNARY_OP(ATOMIC_FCAS,  "atomic_fcas")
//
//
DEFINE_OTHER_OP(READ_STATE,    "read_state",   ATTRS_NONE)
//
DEFINE_OTHER_OP(FENCE,         "fence",        ATTRS_NONE)
//
DEFINE_OTHER_OP(BARRIER,       "barrier",      ATTRS_NONE)
DEFINE_OTHER_OP(MONITOR,       "monitor",      ATTRS_NONE)
DEFINE_OTHER_OP(UNMONITOR,     "unmonitor",    ATTRS_NONE)
DEFINE_OTHER_OP(WAIT,          "wait",         ATTRS_NONE)
DEFINE_OTHER_OP(SIGNAL_EVENT,  "signal_event", ATTRS_NONE)
DEFINE_OTHER_OP(EOT,           "eot",          ATTRS_NONE)
//
//
// TODO: a domain expert should break this into better ops
// TODO: all sampler loads should go into the load category
DEFINE_OTHER_OP(SAMPLER_LOAD,  "sampler_load",  ATTRS_NONE)
DEFINE_OTHER_OP(SAMPLER_FLUSH, "sampler_flush", ATTRS_NONE)
//
// TODO: a domain expert should break this into better ops
// TODO: all loads should be moved up to IS_LOAD and reads IS_STORE
DEFINE_OTHER_OP(RENDER_WRITE,  "render_write", ATTRS_NONE)
DEFINE_OTHER_OP(RENDER_READ,   "render_read",  ATTRS_NONE)

#undef ATTRS_NONE
#undef ATTRS_SCALARADDR
