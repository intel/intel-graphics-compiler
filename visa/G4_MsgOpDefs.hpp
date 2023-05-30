/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// A file containing the table of supported g4::MsgOp's for simple
// C Preprpocessor meta programming.  Other files will #include this to
// create case statements, tables, whatnot.  The typical use case will
// involve multiple #includes.  Unlike other headers this file is expected
// to be #included multiple times.
//
// Near the top of the use file where needed one should #include it with no
// special processing.
// ... other headers
// #include "G4_MsgOpDefs.hpp"
// ... other headers
// ...
// This will cause the file to define various types needed by the table callback.
//
// Later in the file one can define various preprocessor callbacks.
//
// #define DEFINE_G4_MSGOP(SYMBOL, SYNTAX, ENCODING, ATTRS) SYNTAX,
// const char *opNames[] {
// #include "G4_MsgOpDefs.hpp" // expands to all the syntax symbols
// // undefs DEFINE_G4_MSGOP
// };
//
// The callback can be used multiple times.
//
// #define DEFINE_G4_MSGOP_LSC_LOAD(SYMBOL, SYNTAX, ENCODING, ATTRS) ENCODING,
// const int justLscLoadEncodings[] {
// #include "G4_MsgOpDefs.hpp"
// // expands to all the encodings of LSC_LOAD; drops the rest
// // the #include undefs DEFINE_G4_MSGOP_LSC_LOAD
// };

#ifndef G4_MSGOP_TYPES_DEFINED
#define G4_MSGOP_TYPES_DEFINED
// These are various bits used in symbolic definitions of message operations
// That enable one to create packed enumation definitions
// Messages have a "group" and "attributes".
//
// Groups roughly correspond to SFIDs messages are defined on.
// Attributes hold generic predicates corresponding to messages such as
// if a message has a data channel mask.
// Since, messages might target different SFIDs on different platforms
// extra scrutiny might be needed as messages get moved around.
static const int MSGOP_GROUP_LSC_LOAD     = 0x001;
static const int MSGOP_GROUP_LSC_STORE    = (MSGOP_GROUP_LSC_LOAD << 1);
static const int MSGOP_GROUP_LSC_ATOMIC   = (MSGOP_GROUP_LSC_STORE << 1);
static const int MSGOP_GROUP_LSC_OTHER    = (MSGOP_GROUP_LSC_ATOMIC << 1);
static const int MSGOP_GROUP_GTWY         = (MSGOP_GROUP_LSC_OTHER << 1);
static const int MSGOP_GROUP_SMPL_NORMAL  = (MSGOP_GROUP_GTWY << 1);
static const int MSGOP_GROUP_SMPL_GATHER  = (MSGOP_GROUP_SMPL_NORMAL << 1);
static const int MSGOP_GROUP_SMPL_OTHER   = (MSGOP_GROUP_SMPL_GATHER << 1);
static const int MSGOP_GROUP_RENDER       = (MSGOP_GROUP_SMPL_OTHER << 1);
static const int MSGOP_GROUP_RTA          = (MSGOP_GROUP_RENDER << 1);
static const int MSGOP_GROUP_BTD          = (MSGOP_GROUP_RTA << 1);
static_assert((MSGOP_GROUP_BTD & ~0xFFF) == 0, "group field overflowed");

// e.g. load_quad, store_quad, ...
static const int MSGOP_ATTRS_EMPTY = 0x0000;
static const int MSGOP_ATTRS_HAS_CMASK = 0x0001;
static const int MSGOP_ATTRS_ATOMIC_UNARY = MSGOP_ATTRS_HAS_CMASK << 1;
static const int MSGOP_ATTRS_ATOMIC_BINARY = MSGOP_ATTRS_ATOMIC_UNARY << 1;
static const int MSGOP_ATTRS_ATOMIC_TERNARY = MSGOP_ATTRS_ATOMIC_BINARY << 1;
// other attributes here (NOTE: can overlap based on group)
#endif // G4_MSGOP_TYPES_DEFINED

// DEFINE_G4_MSGOP(SYMBOL, SYNTAX, ENCODING, GROUP, ATTRS)
// The top-level macro callback.  The more specific variants cover to this.
// A user can define this to capture all messages or define lower level
// macros to filter based on group.
//
// * SYMBOL a symbol to use in an enum (e.g. LOAD)
// * SYNTAX a string to use in output (e.g. "load")
// * ENCODING a an integer encoding for for this operation (typically Desc[5:0])
// * GROUP a group ordinal value grouping this message; a bit set of 12b (0xXXX)
// * ATTRS a bitset of attributes (at least a byte)
#ifndef DEFINE_G4_MSGOP
#define DEFINE_G4_MSGOP(SYMBOL, SYNTAX, ENCODING, GROUP, ATTRS)
#endif

#ifndef DEFINE_G4_MSGOP_LSC_LOAD
#define DEFINE_G4_MSGOP_LSC_LOAD(SYMBOL, SYNTAX, ENCODING, ATTRS) \
  DEFINE_G4_MSGOP(SYMBOL, SYNTAX, ENCODING, MSGOP_GROUP_LSC_LOAD, ATTRS)
#endif // DEFINE_G4_MSGOP_LOAD_GROUP

#ifndef DEFINE_G4_MSGOP_LSC_STORE
#define DEFINE_G4_MSGOP_LSC_STORE(SYMBOL, SYNTAX, ENCODING, ATTRS) \
  DEFINE_G4_MSGOP(SYMBOL, SYNTAX, ENCODING, MSGOP_GROUP_LSC_STORE, ATTRS)
#endif // DEFINE_G4_MSGOP_LSC_STORE

#ifndef DEFINE_G4_MSGOP_LSC_ATOMIC
#define DEFINE_G4_MSGOP_LSC_ATOMIC(SYMBOL, SYNTAX, ENCODING, ATTRS) \
  DEFINE_G4_MSGOP(SYMBOL, SYNTAX, ENCODING, MSGOP_GROUP_LSC_ATOMIC, ATTRS)
#endif // DEFINE_G4_MSGOP_LSC_ATOMIC

#ifndef DEFINE_G4_MSGOP_LSC_OTHER
#define DEFINE_G4_MSGOP_LSC_OTHER(SYMBOL, SYNTAX, ENCODING, ATTRS) \
  DEFINE_G4_MSGOP(SYMBOL, SYNTAX, ENCODING, MSGOP_GROUP_LSC_OTHER, ATTRS)
#endif // DEFINE_G4_MSGOP_LSC_OTHER

#ifndef DEFINE_G4_MSGOP_GTWY
#define DEFINE_G4_MSGOP_GTWY(SYMBOL, SYNTAX, ENCODING, ATTRS) \
  DEFINE_G4_MSGOP(SYMBOL, SYNTAX, ENCODING, MSGOP_GROUP_GTWY, ATTRS)
#endif // DEFINE_G4_MSGOP_GTWY

#ifndef DEFINE_G4_MSGOP_SMPL_NORMAL
#define DEFINE_G4_MSGOP_SMPL_NORMAL(SYMBOL, SYNTAX, ENCODING, ATTRS) \
  DEFINE_G4_MSGOP(SYMBOL, SYNTAX, ENCODING, MSGOP_GROUP_SMPL_NORMAL, ATTRS)
#endif // DEFINE_G4_MSGOP_SMPL_NORMAL

#ifndef DEFINE_G4_MSGOP_SMPL_GATHER
#define DEFINE_G4_MSGOP_SMPL_GATHER(SYMBOL, SYNTAX, ENCODING, ATTRS) \
  DEFINE_G4_MSGOP(SYMBOL, SYNTAX, ENCODING, MSGOP_GROUP_SMPL_GATHER, ATTRS)
#endif // DEFINE_G4_MSGOP_SMPL_GATHER

#ifndef DEFINE_G4_MSGOP_SMPL_OTHER
#define DEFINE_G4_MSGOP_SMPL_OTHER(SYMBOL, SYNTAX, ENCODING, ATTRS) \
  DEFINE_G4_MSGOP(SYMBOL, SYNTAX, ENCODING, MSGOP_GROUP_SMPL_OTHER, ATTRS)
#endif // DEFINE_G4_MSGOP_SMPL_OTHER

#ifndef DEFINE_G4_MSGOP_RENDER
#define DEFINE_G4_MSGOP_RENDER(SYMBOL, SYNTAX, ENCODING, ATTRS) \
  DEFINE_G4_MSGOP(SYMBOL, SYNTAX, ENCODING, MSGOP_GROUP_RENDER, ATTRS)
#endif // DEFINE_G4_MSGOP_RENDER

#ifndef DEFINE_G4_MSGOP_RTA
#define DEFINE_G4_MSGOP_RTA(SYMBOL, SYNTAX, ENCODING, ATTRS) \
  DEFINE_G4_MSGOP(SYMBOL, SYNTAX, ENCODING, MSGOP_GROUP_RTA, ATTRS)
#endif // DEFINE_G4_MSGOP_RTA

#ifndef DEFINE_G4_MSGOP_BTD
#define DEFINE_G4_MSGOP_BTD(SYMBOL, SYNTAX, ENCODING, ATTRS) \
  DEFINE_G4_MSGOP(SYMBOL, SYNTAX, ENCODING, MSGOP_GROUP_BTD, ATTRS)
#endif // DEFINE_G4_MSGOP_BTD

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// The callback table
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// LSC loads
DEFINE_G4_MSGOP_LSC_LOAD(LOAD,            "load",           0x00,
                         MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_LSC_LOAD(LOAD_STRIDED,    "load_strided",   0x01,
                         MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_LSC_LOAD(LOAD_QUAD,       "load_quad",      0x02,
                         MSGOP_ATTRS_HAS_CMASK)
DEFINE_G4_MSGOP_LSC_LOAD(LOAD_STATUS,     "load_status",    0x1B,
                         MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_LSC_LOAD(LOAD_BLOCK2D,    "load_block2d",   0x03,
                         MSGOP_ATTRS_EMPTY)

///////////////////////////////////////////////////////////////////////////////
// LSC stores
DEFINE_G4_MSGOP_LSC_STORE(STORE,           "store",          0x04,
                          MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_LSC_STORE(STORE_STRIDED,   "store_strided",  0x05,
                          MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_LSC_STORE(STORE_QUAD,      "store_quad",     0x06,
                          MSGOP_ATTRS_HAS_CMASK)
DEFINE_G4_MSGOP_LSC_STORE(STORE_BLOCK2D,   "store_block2d", 0x07,
                          MSGOP_ATTRS_EMPTY)

///////////////////////////////////////////////////////////////////////////////
// LSC atomics
DEFINE_G4_MSGOP_LSC_ATOMIC(ATOMIC_IINC,   "atomic_iinc",   0x08,
                           MSGOP_ATTRS_ATOMIC_UNARY)
DEFINE_G4_MSGOP_LSC_ATOMIC(ATOMIC_IDEC,   "atomic_idec",   0x09,
                           MSGOP_ATTRS_ATOMIC_UNARY)
DEFINE_G4_MSGOP_LSC_ATOMIC(ATOMIC_LOAD,   "atomic_load",   0x0A,
                           MSGOP_ATTRS_ATOMIC_UNARY)
DEFINE_G4_MSGOP_LSC_ATOMIC(ATOMIC_STORE,  "atomic_store",  0x0B,
                           MSGOP_ATTRS_ATOMIC_BINARY)
DEFINE_G4_MSGOP_LSC_ATOMIC(ATOMIC_IADD,   "atomic_iadd",   0x0C,
                           MSGOP_ATTRS_ATOMIC_BINARY)
DEFINE_G4_MSGOP_LSC_ATOMIC(ATOMIC_ISUB,   "atomic_isub",   0x0D,
                           MSGOP_ATTRS_ATOMIC_BINARY)
//
DEFINE_G4_MSGOP_LSC_ATOMIC(ATOMIC_SMIN,   "atomic_smin",   0x0E,
                           MSGOP_ATTRS_ATOMIC_BINARY)
DEFINE_G4_MSGOP_LSC_ATOMIC(ATOMIC_SMAX,   "atomic_smax",   0x0F,
                           MSGOP_ATTRS_ATOMIC_BINARY)
DEFINE_G4_MSGOP_LSC_ATOMIC(ATOMIC_UMIN,   "atomic_umin",   0x10,
                           MSGOP_ATTRS_ATOMIC_BINARY)
DEFINE_G4_MSGOP_LSC_ATOMIC(ATOMIC_UMAX,   "atomic_umax",   0x11,
                           MSGOP_ATTRS_ATOMIC_BINARY)
DEFINE_G4_MSGOP_LSC_ATOMIC(ATOMIC_ICAS,   "atomic_icas",   0x12,
                           MSGOP_ATTRS_ATOMIC_TERNARY)
//
DEFINE_G4_MSGOP_LSC_ATOMIC(ATOMIC_FADD,   "atomic_fadd",   0x13,
                           MSGOP_ATTRS_ATOMIC_BINARY)
DEFINE_G4_MSGOP_LSC_ATOMIC(ATOMIC_FSUB,   "atomic_fsub",   0x14,
                           MSGOP_ATTRS_ATOMIC_BINARY)
DEFINE_G4_MSGOP_LSC_ATOMIC(ATOMIC_FMIN,   "atomic_fmin",   0x15,
                           MSGOP_ATTRS_ATOMIC_BINARY)
DEFINE_G4_MSGOP_LSC_ATOMIC(ATOMIC_FMAX,   "atomic_fmax",   0x16,
                           MSGOP_ATTRS_ATOMIC_BINARY)
DEFINE_G4_MSGOP_LSC_ATOMIC(ATOMIC_FCAS,   "atomic_fcas",   0x17,
                           MSGOP_ATTRS_ATOMIC_TERNARY)
//
DEFINE_G4_MSGOP_LSC_ATOMIC(ATOMIC_AND,    "atomic_and",    0x18,
                           MSGOP_ATTRS_ATOMIC_BINARY)
DEFINE_G4_MSGOP_LSC_ATOMIC(ATOMIC_OR,     "atomic_or",     0x19,
                           MSGOP_ATTRS_ATOMIC_BINARY)
DEFINE_G4_MSGOP_LSC_ATOMIC(ATOMIC_XOR,    "atomic_xor",    0x1A,
                           MSGOP_ATTRS_ATOMIC_BINARY)
//

///////////////////////////////////////////////////////////////////////////////
// LSC other
DEFINE_G4_MSGOP_LSC_OTHER(RSI,            "rsi",           0x1E,
                          MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_LSC_OTHER(FENCE,          "fence",         0x1F,
                          MSGOP_ATTRS_EMPTY)

///////////////////////////////////////////////////////////////////////////////
// gateway ops
DEFINE_G4_MSGOP_GTWY(EOT, "eot", 0x00, MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_GTWY(EOTR, "eotr", 0x0A, MSGOP_ATTRS_EMPTY)
//
DEFINE_G4_MSGOP_GTWY(BARRIER_SIGNAL, "barrier_signal", 0x04,
                     MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_GTWY(BARRIER_SIGNAL_NAMED, "barrier_signal_named", 0x05,
                     MSGOP_ATTRS_EMPTY)


///////////////////////////////////////////////////////////////////////////////
// sampler ops
DEFINE_G4_MSGOP_SMPL_NORMAL(SAMPLE,   "sample",   0x00, MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_SMPL_NORMAL(SAMPLE_B, "sample_b", 0x01, MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_SMPL_NORMAL(SAMPLE_L, "sample_l", 0x02, MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_SMPL_NORMAL(SAMPLE_C, "sample_c", 0x03, MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_SMPL_NORMAL(SAMPLE_D, "sample_d", 0x04, MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_SMPL_NORMAL(SAMPLE_B_C, "sample_b_c", 0x05,
                            MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_SMPL_NORMAL(SAMPLE_L_C, "sample_l_c", 0x06,
                            MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_SMPL_NORMAL(SAMPLE_KILLPIX, "sample_killpix", 0x0C,
                            MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_SMPL_NORMAL(SAMPLE_D_C, "sample_d_c", 0x11, MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_SMPL_NORMAL(SAMPLE_LZ, "sample_lz", 0x18, MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_SMPL_NORMAL(SAMPLE_C_LZ, "sample_c_lz", 0x19,
                            MSGOP_ATTRS_EMPTY)
////////////////////////////////
// gather 4 sampler ops
DEFINE_G4_MSGOP_SMPL_GATHER(GATHER4,   "gather4",   0x08, MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_SMPL_GATHER(GATHER4_C, "gather4_c", 0x10, MSGOP_ATTRS_EMPTY)

////////////////////////////////
// other sampler messages
DEFINE_G4_MSGOP_SMPL_OTHER(LD, "ld", 0x07, MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_SMPL_OTHER(LOD, "lod", 0x09, MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_SMPL_OTHER(RESINFO, "resinfo", 0x0A, MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_SMPL_OTHER(SAMPLE_INFO, "sample_info", 0x0B,
                           MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_SMPL_OTHER(LD_LZ, "ld_lz", 0x1A, MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_SMPL_OTHER(LD_2DMS_W, "ld_2dms_w", 0x1C, MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_SMPL_OTHER(LD_MCS, "ld_mcs", 0x1D, MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_SMPL_OTHER(SAMPLER_FLUSH, "sampler_flush", 0x1F,
                           MSGOP_ATTRS_EMPTY)
//
///////////////////////////////////////////////////////////////////////////////
// render target ops
DEFINE_G4_MSGOP_RENDER(READ, "read", 0x02, MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_RENDER(WRITE, "write", 0x06, MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_RENDER(DUAL_WRITE, "dual_write", 0x05, MSGOP_ATTRS_EMPTY)

///////////////////////////////////////////////////////////////////////////////
// btd ops and ray tracing
DEFINE_G4_MSGOP_RTA(TRACE_RAY, "trace_ray", 0x00, MSGOP_ATTRS_EMPTY)
//
DEFINE_G4_MSGOP_BTD(SPAWN, "spawn", 0x00, MSGOP_ATTRS_EMPTY)
DEFINE_G4_MSGOP_BTD(STACK_ID_RELEASE, "stack_id_release", 0x01,
                    MSGOP_ATTRS_EMPTY)

///////////////////////////////////////////////////////////////////////////////
// #undef anything that was defined, including by the #include'r so
// that the callback can be used again elsewhere.
#ifdef DEFINE_G4_MSGOP
#undef DEFINE_G4_MSGOP
#endif
#ifdef DEFINE_G4_MSGOP_LSC_LOAD
#undef DEFINE_G4_MSGOP_LSC_LOAD
#endif
#ifdef DEFINE_G4_MSGOP_LSC_STORE
#undef DEFINE_G4_MSGOP_LSC_STORE
#endif
#ifdef DEFINE_G4_MSGOP_LSC_ATOMIC
#undef DEFINE_G4_MSGOP_LSC_ATOMIC
#endif
#ifdef DEFINE_G4_MSGOP_LSC_OTHER
#undef DEFINE_G4_MSGOP_LSC_OTHER
#endif
#ifdef DEFINE_G4_MSGOP_GTWY
#undef DEFINE_G4_MSGOP_GTWY
#endif
#ifdef DEFINE_G4_MSGOP_SMPL_NORMAL
#undef DEFINE_G4_MSGOP_SMPL_NORMAL
#endif
#ifdef DEFINE_G4_MSGOP_SMPL_GATHER
#undef DEFINE_G4_MSGOP_SMPL_GATHER
#endif
#ifdef DEFINE_G4_MSGOP_SMPL_OTHER
#undef DEFINE_G4_MSGOP_SMPL_OTHER
#endif
#ifdef DEFINE_G4_MSGOP_RENDER
#undef DEFINE_G4_MSGOP_RENDER
#endif
#ifdef DEFINE_G4_MSGOP_RTA
#undef DEFINE_G4_MSGOP_RTA
#endif
#ifdef DEFINE_G4_MSGOP_BTD
#undef DEFINE_G4_MSGOP_BTD
#endif
