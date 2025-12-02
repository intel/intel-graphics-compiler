/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*
 * The IGA - Intel Gen Assembler external C API
 */
#ifndef IGA_H
#define IGA_H

#include <stddef.h>
#include <stdint.h>
#ifdef _WIN32
#include <crtdefs.h>
#endif

#ifdef _WIN32
#ifdef IGA_BUILDING_DLL
#define IGA_API __declspec(dllexport)
/* define this symbol if you wish to statically link against iga32.dll */
#elif IGA_DLL_IMPORT
#define IGA_API __declspec(dllimport)
#else
#define IGA_API
#endif
#else
#if __GNUC__ >= 4
#define IGA_API __attribute__((visibility("default")))
#else
#define IGA_API
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* opaque pointer to context type */
typedef void *iga_context_t;

typedef enum {
  IGA_SUCCESS = 0,        /* the operation completed successfully
                           * (iga_disassemble and iga_assemble may
                           * still have logged warnings) */
  IGA_ERROR = 1,          /* general error */
  IGA_INVALID_ARG = 2,    /* something wrong with an arugment */
  IGA_OUT_OF_MEM = 3,     /* failed to allocate */
  IGA_DECODE_ERROR = 4,   /* error during decode phase */
  IGA_ENCODE_ERROR = 5,   /* error during the encode phase */
  IGA_PARSE_ERROR = 6,    /* error duing the parse phase (syntax) */
  IGA_VERSION_ERROR = 7,  /* this header file is newer than the binary
                           * called */
  IGA_INVALID_OBJECT = 8, /* attempt to use an destroyed object
                           * (e.g. iga_context_t) */
  IGA_INVALID_STATE = 9,  /* e.g. call to iga_get_errors before disassembly*/
  IGA_UNSUPPORTED_PLATFORM = 10, /* platform not supported with this binary */
  IGA_DIFF_FAILURE = 11          /* used by -Xifs for diffing instructions */
} iga_status_t;

/*
 * Converts an IGA status string code to a human-readable string.
 *
 * RETURNS: IGA_SUCCESS
 */
IGA_API const char *iga_status_to_string(iga_status_t st);

/*
 * Returns a NUL-terminated version string corresponding to the version of IGA.
 */
IGA_API const char *iga_version_string();

/* The encoding for GEN version enumerates follows this pattern */
#define GEN_VER(MAJ, MIN) (((MAJ) << 16) | (MIN))

/* The encoding for XE version enumerates follows this pattern
 * All XE_VER() must be larger than GEN_VER().  XE_VER(0,*) is illegal. TGL is
 * XE_VER(1,0)
 */
#define XE_VER(MAJ, MIN) (((MAJ) << 24) | (MIN))

/* Represents the specific version of Gen being assembled or disassembled */
typedef enum {
  IGA_GEN_INVALID = 0,
  IGA_GEN7 = GEN_VER(7, 0),
  IGA_GEN7p5 = GEN_VER(7, 5),
  IGA_GEN8 = GEN_VER(8, 0),
  IGA_GEN8lp = GEN_VER(8, 1),
  IGA_GEN9 = GEN_VER(9, 0),
  IGA_GEN9lp = GEN_VER(9, 1),
  IGA_GEN9p5 = GEN_VER(9, 5),
  IGA_GEN10 = GEN_VER(10, 0),
  IGA_GEN11 = GEN_VER(11, 0),
  //
  // XE versions
  IGA_XE = XE_VER(1, 0), // TGL
  IGA_XE_HP = XE_VER(1, 1),
  IGA_XE_HPG = XE_VER(1, 2),
  IGA_XE_HPC = XE_VER(1, 4)
  ,
  IGA_XE2 = XE_VER(2, 0)
  ,
  IGA_XE3 = XE_VER(3, 0)
  , IGA_XE3P_XPC = XE_VER(3, 3)

  // DEPRECATED
  // Preserve the old values to maintain the binary compatibility
  // The value should not be used anymore
  ,
  IGA_GEN12p1 = GEN_VER(12, 1) // IGA_XE
} iga_gen_t;

/*****************************************************************************/
/*                     Platform Query Functions                              */
/*****************************************************************************/

/*
 * Returns a list of the platforms enumeration values supported by this
 * implementation of IGA into a user-provided buffer.  The buffer parameter
 * maybe omitted.
 *
 * PARAMETERS:
 *  gens_length_bytes            the length of the gens buffer in bytes;
 *                               can be 0 if gens is NULL
 *  gens                         the user-provided buffer (can be NULL);
 *                               if non-NULL, the output will contain the list
 *                               of iga_gen_t platforms that this IGA supports
 *  gens_length_bytes_required   the required length
 *
 * RETURNS:
 *   IGA_INVALID_ARGUMENT  if gens_length_bytes != 0 && gens == nullptr
 *   IGA_SUCCESS           otherwise
 */
IGA_API iga_status_t iga_platforms_list(size_t gens_length_bytes,
                                        iga_gen_t *gens,
                                        size_t *gens_length_bytes_required);

/*
 * Returns the platform suffix name of a given platform.
 *
 * PARAMETERS:
 *  gen                   the platform to query
 *  suffix                an output parameter assigned the result;
 *                        NULL is assigned if the 'gen' passed in is invalid
 *                        or unsupported
 *
 * RETURNS:
 *   IGA_INVALID_ARGUMENT  if symbol is NULL or 'gen' is invalid or not
 *                         supported by this platform
 *   IGA_SUCCESS           otherwise
 */
IGA_API iga_status_t iga_platform_symbol_suffix(iga_gen_t gen,
                                                const char **suffix);

/*
 * Returns the names for a given platform.  E.g. IGA_GEN9 returns "skl".
 *
 * PARAMETERS:
 *  gen                   the platform to query
 *  names_bytes           the length in bytes of the names array passed in
 *                        0 iff 'names' is nullptr
 *  names                 the array to return the platform names in
 *  names_bytes_needed    the required length of 'names' in bytes
 *
 * RETURNS:
 *   IGA_INVALID_ARGUMENT  if gen is an invalid platform
 *                         supported by this platform
 *   IGA_INVALID_ARGUMENT  if names_bytes != 0 && names == nullptr
 *   IGA_SUCCESS           otherwise
 */
IGA_API iga_status_t iga_platform_names(iga_gen_t gen, size_t names_bytes,
                                        const char **names,
                                        size_t *names_bytes_needed);

/*****************************************************************************/
/*                  Context Creation Functions                               */
/*****************************************************************************/

/*
 * Context creation options
 */
typedef struct {
  size_t cb; /* set to sizeof(iga_context_options_t) */
  iga_gen_t gen;
} iga_context_options_t;

/*
 * This macro initializes options to the context.
 *  iga_gen_t   P    the gen platform to use
 */
#define IGA_CONTEXT_OPTIONS_INIT(P)                                            \
  { sizeof(iga_context_options_t), (P) }

/*
 * Creates a context, which is needed to assemble and disassemble kernels.
 * A context manages internal dynamically allocated memory containing
 * the diagnostics.
 *
 * PARAMETERS:
 *  opts         the options to this iga context
 *  ctx          points to the newly created context
 *
 * RETURNS:
 *  IGA_SUCCESS           upon successful context creation
 *  IGA_INVALID_ARG       if an argument is NULL
 *  IGA_INVALID_OBJECT    if ctx has already been destroyed
 *  IGA_OUT_OF_MEM        upon internal allocation failure
 *  IGA_INVALID_PLATFORM  if the platform passed is unsupported
 */
IGA_API iga_status_t iga_context_create(const iga_context_options_t *opts,
                                        iga_context_t *ctx);
/* deprecated: covers to iga_context_create */
IGA_API iga_status_t iga_create_context(const iga_context_options_t *opts,
                                        iga_context_t *ctx);

/*
 * Releases a context previously created via 'iga_context_create'.
 *
 * RETURNS:
 *  IGA_SUCCESS         upon successful context release
 *  IGA_INVALID_ARG     if an argument is NULL
 *  IGA_INVALID_OBJECT  if ctx has already been destroyed
 */
IGA_API iga_status_t iga_context_release(iga_context_t ctx);
/* deprecated: covers to iga_context_release */
IGA_API iga_status_t iga_release_context(iga_context_t ctx);

/*****************************************************************************/
/*                  Assembly Functions                                       */
/*****************************************************************************/

/*
 * This structure contains options to the 'iga_assemble' call.
 */
typedef struct {
  uint32_t cb;               /* sizeof(iga_context_options_t)    */
  uint32_t enabled_warnings; /* bitset of IGA_WARNINGS*          */
  uint32_t encoder_opts;     /* bitset of IGA_ENCODER_OPT*       */
  uint32_t syntax_opts;      /* bitset of IGA_SYNTAX_OPT*        */
  /* the following must be 0 or a warning is raised */
  uint32_t _reserved0; /* use IGA_ENCODER_OPT_ERRONCOMPACT */
  uint32_t _reserved1; /* use IGA_ENCODER_OPT_AUTODEP      */
  /*
   * ... future fields (ensure total size is a multiple of 8;
   * add "reserved" if needed) ...
   * do not reuse _reserved0 and _reserved1 since those were
   * historically something else
   */

  /* number of sbid used for auto dependency setting. This value is effective
   * only when IGA_ENCODER_OPT_AUTO_DEPENDENCIES is given */
  uint32_t sbid_count;

  /*
   * Force the swsb_encode_mode.  If not given (SWSBInvalidMode),
   * then encode mode will be derived from platform.
   */
  uint32_t swsb_encode_mode; /* use iga::SWSB_ENCODE_MODE */
} iga_assemble_options_t;

/* detects screwups where someone adds a field and the compiler pads
 * the structure out implicitly */
static_assert(sizeof(iga_assemble_options_t) == 8 * 4,
              "wrong size for iga_assemble_options");

/*
 * encoding options
 */
/*
 * Automatically compact instructions without explicit compaction annotations.
 * E.g. with this enabled:
 *    op (...) ... {Compacted}   // will compact
 *    op (...) ... { }           // try and compact iff IGA_ENCOPTS_AUTOCOMPACT
 *    op (...) ... {Uncompacted} // will not attempt to compact
 */
#define IGA_ENCODER_OPT_AUTO_COMPACT 0x00000001u
/* auto set instruction dependencies */
#define IGA_ENCODER_OPT_AUTO_DEPENDENCIES 0x00000002u
/* treat failure to compact an instruction with a {Compacted} annotation
 * as a hard error rather than just raising a warning */
#define IGA_ENCODER_OPT_ERROR_ON_COMPACT_FAIL 0x00000004u
/* enable experimental native encoder */
#define IGA_ENCODER_OPT_USE_NATIVE 0x00000008u
/* forcely NoCompact to all instructions even if {Compacted} is set on the
   instruction This option will overried IGA_ENCODER_OPT_AUTO_COMPACT */
#define IGA_ENCODER_OPT_FORCE_NO_COMPACT 0x00000010u
/* auto set instruction dependencies with sbid counter. This option takes
   effect only when IGA_ENCODER_OPT_AUTO_DEPENDENCIES is set */
#define IGA_ENCODER_OPT_AUTO_SBID_COUNTER 0x00000020u
/*
 * options for the parsing phase
 */
/* nominal support for certain IsaAsm directives and syntax */
#define IGA_SYNTAX_OPT_LEGACY_SYNTAX 0x00000001u
/* enables syntax extensions */
#define IGA_SYNTAX_OPT_EXTENSIONS 0x00000002u

/*
 * extra assemble warnings (things to check)
 * (for iga_assemble_options_t::enabled_warnings)
 */
/* individual warnings */
#define IGA_WARNINGS_REGIONS 0x00000001u  /* -Wregions */
#define IGA_WARNINGS_TYPES 0x00000002u    /* -Wtypes */
#define IGA_WARNINGS_SCHED 0x00000004u    /* -Wscheduling */
#define IGA_WARNINGS_NORMFORM 0x00000008u /* -Wnormal-form */
/* useful predefined sets */
#define IGA_WARNINGS_NONE 0x00000000u /* -Wnone */
#define IGA_WARNINGS_ALL                                                       \
  (IGA_WARNINGS_REGIONS | IGA_WARNINGS_TYPES | IGA_WARNINGS_SCHED |            \
   IGA_WARNINGS_NORMFORM) /* -Wall */
#define IGA_WARNINGS_DEFAULT                                                   \
  (IGA_WARNINGS_REGIONS | IGA_WARNINGS_SCHED) /* -Wdefault */

/* helpful initializers for assembly options with good defaults */
#define IGA_ASSEMBLE_OPTIONS_INIT()                                            \
  {                                                                            \
    sizeof(iga_assemble_options_t), IGA_WARNINGS_DEFAULT,                      \
        (IGA_ENCODER_OPT_ERROR_ON_COMPACT_FAIL), 0, /* syntax_opts = NONE */   \
        0,                                          /* reserved */             \
        0,                                          /* reserved */             \
        16,                                         /* sbid_count */           \
        0,                                          /* SWSBInvalidMode */      \
  }

/*
 * Assembles some text into bits.
 *
 * PARAMETERS:
 *  ctx           the iga context
 *  opts          the assemble options
 *  kernel_text   a NUL-terminated string containing the kernel text
 *                to assemble
 *  output        the output assembly binary; upon failure, this is
 *                assigned NULL; this memory should not be modified
 *                or deallocated externally
 *  output_size   the length of 'output' (in bytes); assigned 0 upon failure
 *
 * RETURNS:
 *  IGA_SUCCESS         upon successful assembly; 'iga_get_warnings' may
 *                      contain warning diagnostics even upon success
 *  IGA_INVALID_ARG     if an argument is NULL
 *  IGA_INVALID_OBJECT  if ctx has already been destroyed
 *  IGA_PARSE_ERROR     if the text is somehow malformed and IGA is unable
 *                      to parse it; use iga_get_errors for more info
 *  IGA_ENCODE_ERROR    if the encoder encounters some error
 *                      (use iga_get_errors for more info)
 *  IGA_ERROR           upon some other error
 */
IGA_API iga_status_t iga_context_assemble(iga_context_t ctx,
                                          const iga_assemble_options_t *opts,
                                          const char *kernel_text,
                                          void **output, uint32_t *output_size);
/* deprecated API covers to iga_contex* */
IGA_API iga_status_t iga_assemble(iga_context_t ctx,
                                  const iga_assemble_options_t *opts,
                                  const char *kernel_text, void **output,
                                  uint32_t *output_size);

/*****************************************************************************/
/*                  Disassembly Functions                                    */
/*****************************************************************************/

/*
 * This structure contains options to the 'iga_disassemble' call.
 */
typedef struct {
  uint32_t cb;              /* set to sizeof(iga_context_options_t) */
  uint32_t formatting_opts; /* options in formatting the syntax */
  uint32_t _reserved0;      /* use formatting_opts; set this to 0! */
  uint32_t _reserved1;      /* use formatting_opts; set this to 0! */
  uint32_t decoder_opts;    /* opts for the decoding phase */
  uint32_t base_pc_offset;  /* base pc offset to add to pc in disassembly string
                               output*/
  /* ... future fields (ensure total size is a multiple of 8;
   * add "reserved" if needed) ... */
} iga_disassemble_options_t;

static_assert(sizeof(iga_disassemble_options_t) == 6 * 4,
              "wrong size for iga_disassemble_options_t");

/* A default value for iga_disassemble_options_t */
#define IGA_DISASSEMBLE_OPTIONS_INIT()                                         \
  {                                                                            \
    sizeof(iga_disassemble_options_t), IGA_FORMATTING_OPTS_DEFAULT,            \
        0,                         /* _reserved0 */                            \
        0,                         /* _reserved1 */                            \
        IGA_DECODING_OPTS_DEFAULT, /* decoder_opts */                          \
  }

/* A default value for iga_disassemble_options_t that enables numeric labels */
#define IGA_DISASSEMBLE_OPTIONS_INIT_NUMERIC_LABELS()                          \
  {                                                                            \
    sizeof(iga_disassemble_options_t),                                         \
        IGA_FORMATTING_OPTS_DEFAULT | IGA_FORMATTING_OPT_NUMERIC_LABELS,       \
        0,                         /* _reserved0 */                            \
        0,                         /* _reserved1 */                            \
        IGA_DECODING_OPTS_DEFAULT, /* decoder_opts */                          \
  }

/*
 * options for the formatting phase (the syntax emitted / printed)
 */
/* does not use numeric labels */
#define IGA_FORMATTING_OPT_NUMERIC_LABELS 0x00000001u
/* enables certain syntax extensions */
#define IGA_FORMATTING_OPT_SYNTAX_EXTS 0x00000002u
/* force floats to be emitted in hexadecimal */
#define IGA_FORMATTING_OPT_PRINT_HEX_FLOATS 0x00000004u
/* print the instruction PC's */
#define IGA_FORMATTING_OPT_PRINT_PC 0x00000008u
/* print the instruction bits */
#define IGA_FORMATTING_OPT_PRINT_BITS 0x00000010u
/* print immediate instruction dependencies (mostly for IGA debugging) */
#define IGA_FORMATTING_OPT_PRINT_DEPS 0x00000020u
/* print load/store pseduo instructions where possible */
#define IGA_FORMATTING_OPT_PRINT_LDST 0x00000040u
/* print bfn symbolic instructions */
#define IGA_FORMATTING_OPT_PRINT_BFNEXPRS 0x00000080u
/* use ansi_span escapes when possible */
#define IGA_FORMATTING_OPT_PRINT_ANSI 0x00000100u
/* emit JSON instead of asm */
#define IGA_FORMATTING_OPT_PRINT_JSON 0x00000200u
/* emit instruction definitions from a simple dataflow analysis */
#define IGA_FORMATTING_OPT_PRINT_DEFS 0x00000400u
/* emit JSON old V1 version */
#define IGA_FORMATTING_OPT_PRINT_JSON_V1 0x00000800u

/* just the default formatting opts */
#define IGA_FORMATTING_OPTS_DEFAULT (0u)
/* a union of all formatter opts */
#define IGA_FORMATTING_OPTS_ALL                                                \
  = (IGA_FORMATTING_OPT_NUMERIC_LABELS | IGA_FORMATTING_OPT_SYNTAX_EXTS |      \
     IGA_FORMATTING_OPT_PRINT_HEX_FLOATS | IGA_FORMATTING_OPT_PRINT_PC |       \
     IGA_FORMATTING_OPT_PRINT_BITS | IGA_FORMATTING_OPT_PRINT_DEPS |           \
     IGA_FORMATTING_OPT_PRINT_DEFS)

/* uses the native decoder for decoding the kernel */
#define IGA_DECODING_OPT_NATIVE 0x00000001u
/* just the default decoding opts */
#define IGA_DECODING_OPTS_DEFAULT (0u)


/*
 * Disassembles kernel bits into a string.
 *
 * PARAMETERS:
 *  ctx             an iga context
 *  dopts           the disassemble options
 *  input           the instructions to disassemble
 *  input_size      the size of the 'input' in bytes
 *  fmt_label_name  optional callback to resolve a PC to specific label
 *                   - if the callback is NULL or the callback returns NULL,
 *                     then IGA generates the label names
 *                   - for instructions with multiple labels the caller
 *                     may reuse any buffers for each label
 *                   *** PCs are relative to the start of the disassembly  ***
 *                   *** for this API                                      ***
 *  fmt_label_ctx   A callback context (environment) forwarded to 'fmt_label'
 *                  E.g. a buffer to construct the label with
 *  kernel_text     a pointer containing the NUL-terminate disassembly text;
 *                  this memory is internally allocated and should not be freed
 *                  by the user; moreover, it may be invalidated upon the next
 *                  call using this context (freed or even reused); upon a
 *                  decoding error this may contain the partially decoded
 *                  kernel or the empty string if something catestrophic
 *                  happened, but the pointer returned will always be valid
 *                  (until the subsequent call)
 *
 * RETURNS:
 *  IGA_SUCCESS         upon successful disassembly; 'iga_get_warnings' may
 *                      contain warning diagnostics even upon success
 *  IGA_INVALID_ARG     if an argument is NULL; 'input' may be NULL only
 *                      if 'input_size' is also 0
 *  IGA_INVALID_OBJECT  if ctx has already been destroyed
 *  IGA_DECODE_ERROR    upon failure to decode error; specific error messages
 *                      may be retrieved via 'iga_context_get_errors'
 */
IGA_API iga_status_t iga_context_disassemble(
    iga_context_t ctx, const iga_disassemble_options_t *opts, const void *input,
    uint32_t input_size, const char *(*fmt_label_name)(int32_t, void *),
    void *fmt_label_ctx, char **kernel_text);
/* deprecated API covers to iga_context_* */
IGA_API iga_status_t iga_disassemble(
    iga_context_t ctx, const iga_disassemble_options_t *opts, const void *input,
    uint32_t input_size, const char *(*fmt_label_name)(int32_t, void *),
    void *fmt_label_ctx, char **kernel_text);

/*
 * Disassembles a single instruction.
 *
 *  ctx             an iga context
 *  dopts           the disassemble options
 *  input           the instructions to disassemble; the first 8-16 bytes
 *                  must be readable (16 if compaction control is set)
 *  fmt_label_name  optional callback to resolve a PC to specific label
 *                   - if the callback is NULL or the callback returns NULL,
 *                     then IGA generates the label names
 *                   - for instructions with multiple labels the caller
 *                     may reuse any buffers for each label
 *                   *** PCs are relative to the start of this instruction ***
 *                   *** for this API                                      ***
 *  fmt_label_ctx   A callback context (environment) forwarded to 'fmt_label'
 *                  E.g. a buffer to construct the label with
 *  kernel_text     a pointer containing the NUL-terminate disassembly text;
 *                  this memory is internally allocated and should not be freed
 *                  by the user; moreover, it may be invalidated upon the next
 *                  call using this context (freed or even reused); upon a
 *                  decoding error this may contain the partially decoded
 *                  kernel or the empty string if something catestrophic
 *                  happened, but the pointer returned will always be valid
 *                  (until the subsequent call)
 *
 * RETURNS:
 *  IGA_SUCCESS         upon successful disassembly; 'iga_get_warnings' may
 *                      contain warning diagnostics even upon success
 *  IGA_INVALID_ARG     if an argument is NULL
 *  IGA_INVALID_OBJECT  if ctx has already been destroyed
 *  IGA_DECODE_ERROR    upon failure to decode error; specific error messages
 *                      may be retrieved via 'iga_context_get_errors'
 */
IGA_API iga_status_t iga_context_disassemble_instruction(
    iga_context_t ctx, const iga_disassemble_options_t *dopts,
    const void *input, const char *(*fmt_label_name)(int32_t, void *),
    void *fmt_label_ctx, char **kernel_text);
/* deprecated API covers to iga_contex* */
IGA_API iga_status_t iga_disassemble_instruction(
    iga_context_t ctx, const iga_disassemble_options_t *dopts,
    const void *input, const char *(*fmt_label_name)(int32_t, void *),
    void *fmt_label_ctx, char **kernel_text);

/*****************************************************************************/
/*             Diagnostic Processing Functions                               */
/*****************************************************************************/

/*
 * A diagnostic message (e.g. error or warning)
 *
 * WARNING: This structure may be made opaque in future releases.
 *          Treat it as an opaque type and use the accessors to minimize
 *          any conversion time down the road.
 */
typedef struct {
  /* lines start at 1; 0 means no location information */
  uint32_t line;
  /* columns start at 1; 0 means no location information */
  uint32_t column;
  /* the input offset */
  uint32_t offset;
  /* the length of the diagnostic (in characters) */
  uint32_t extent;
  /* the diagnostic message (NUL terminated) */
  const char *message;
} iga_diagnostic_t;

/*
 * Gets the errors from previous 'iga_context_assemble' or
 * 'iga_context_disassmble'.  It is an error to call this before one of these
 * functions have been called.  Further invocations of this function return
 * the same values until one of the above functions is called again.  Memory
 * returned by this function should not be modified externally.  Moreover, it
 * should not be accessed after the next call to 'iga_context_assemble' or
 * 'iga_context_disassmble' or the call to 'iga_context_release'.
 *
 * PARAMETERS:
 *   ctx          the iga context used in the call to 'iga_context_assemble'
 *                or 'iga_context_disassmble'
 *   ds           assigned to an internally allocated array of diagnostic
 *                messages (the caller should *not* mutate or free this memory)
 *   ds_len       assigned the length of 'ds' upon success
 *
 * RETURNS:
 *  IGA_SUCCESS   upon getting the list of errors; note if the last call
 *                contained no errors, this still returns IGA_SUCCESS;
 *                'ds' is assigned a NULL pointer and 'ds_len' is assigned 0.
 *  IGA_INVALID_ARG     if an argument is NULL
 *  IGA_INVALID_OBJECT  if ctx has already been destroyed
 *  IGA_ERROR           if 'iga_assemble' or 'iga_disassmble' never called,
 *                      internal allocation fails, or the context is invalid
 *                      (after call to 'iga_context_release').
 */
IGA_API iga_status_t iga_context_get_errors(iga_context_t ctx,
                                            const iga_diagnostic_t **ds,
                                            uint32_t *ds_len);
IGA_API iga_status_t iga_get_errors(iga_context_t ctx,
                                    const iga_diagnostic_t **ds,
                                    uint32_t *ds_len);

/*
 * Symmetric to 'iga_context_get_errors'.  Various API calls can still succeed
 * even if there are warnings; hence, robust tools should always check this
 * as well.
 */
IGA_API iga_status_t iga_context_get_warnings(iga_context_t ctx,
                                              const iga_diagnostic_t **ds,
                                              uint32_t *ds_len);
IGA_API iga_status_t iga_get_warnings(iga_context_t ctx,
                                      const iga_diagnostic_t **ds,
                                      uint32_t *ds_len);

/*
 * Fetches a diagnostic's message.  The internal memory containing the message
 * should be copied out before further disassemble or assembly or context
 * destruction.  This diagnostic field is valid for both binary and text.
 *
 * RETURNS:
 *  IGA_SUCCESS           upon success
 *  IGA_INVALID_ARG       if an argument is NULL
 */
IGA_API iga_status_t iga_diagnostic_get_message(const iga_diagnostic_t *d,
                                                const char **message);

/*
 * Returns the starting file offset for the diagnostic in bytes (zero based).
 * This diagnostic field is valid for both binary and text.
 *
 * RETURNS:
 *  IGA_SUCCESS           upon success
 *  IGA_INVALID_ARG       if an argument is NULL or the diagnostic is a binary
 *                        location
 */
IGA_API iga_status_t iga_diagnostic_get_offset(const iga_diagnostic_t *d,
                                               uint32_t *offset);

/*
 * Diagnostics can refer to locations in binary files or text files.
 * The following enumeration indicates which.
 */
typedef enum {
  /* the location refers to a text location */
  IGA_DIAGNOSTIC_TEXT,
  /* the location refers to a binary location */
  IGA_DIAGNOSTIC_BINARY,
} iga_diagnostic_type_t;

/*
 * Fetches the type of diagnostic (binary or text).
 * For IGA_DIAGNOSTIC_TEXT iga_diagnostic_text_get_* should be used to access
 * further information.
 * For IGA_DIAGNOSTIC_BINARY 'iga_diagnostic_get_offset* should be used
 * access more the binary offset
 *
 * RETURNS:
 *  IGA_SUCCESS           upon success
 *  IGA_INVALID_ARG       if an argument is NULL or the diagnostic is a binary
 *                        location
 */
IGA_API iga_status_t iga_diagnostic_get_type(const iga_diagnostic_t *d,
                                             iga_diagnostic_type_t *dt);

/*
 * Fetches a diagnostic's line number (must be IGA_DIAGNOSTIC_TEXT).
 * Lines numbers are counted from 1.
 *
 * RETURNS:
 *  IGA_SUCCESS           upon success
 *  IGA_INVALID_ARG       if an argument is NULL or the diagnostic is a binary
 *                        location
 */
IGA_API iga_status_t iga_diagnostic_get_text_line(const iga_diagnostic_t *d,
                                                  uint32_t *line);

/*
 * Similar to 'iga_diagnostic_text_get_line', but returns the column.
 * Columns are counted from 1.
 */
IGA_API iga_status_t iga_diagnostic_get_text_column(const iga_diagnostic_t *d,
                                                    uint32_t *col);

/*
 * Fetches a binary diagnostic's byte offset (must be IGA_DIAGNOSTIC_TEXT).
 * The extent is the length of the region of source code.
 *
 * RETURNS:
 *  IGA_SUCCESS           upon success
 *  IGA_INVALID_ARG       if an argument is NULL or the diagnostic is a binary
 *                        location
 */
IGA_API iga_status_t iga_diagnostic_get_text_extent(const iga_diagnostic_t *d,
                                                    uint32_t *extent);

/*****************************************************************************/
/*             Operation Enumeration Functions                               */
/*****************************************************************************/
/*
 * An opaque type representing an operation (instruction) type.
 * Can be efficiently copied and passed by value.
 */
typedef const struct opspec *iga_opspec_t;

/*
 * Enumerates the opcodes for a given platform.
 *   'gen'          : gen plaform to enumerate operations on
 *   'ops_arr'      : a pointer (can be nul) to an array to hold
 *                    all opcode spec.
 *   'ops_arr_len'  : the required length for 'ops_arr'
 * This API is the usual two-shot pattern, in which the caller invokes it
 * first time to get the length, the caller allocates enough space to hold
 * info based on the returned length, then the caller invokes it again.
 *
 * This function always returns the required length of the array
 * from argument 'ops_arr_len'.
 *
 * For example,
 *      size_t len;
 *      iga_opspec_enumerate(gen, nullptr, &len);
 *      iga_opspec_t *ops_arr =
 *          (iga_opspec_t*)malloc(sizeof(iga_opspec_t) * len);
 *      iga_opspec_enumerate(gen, ops_arr, &len);
 *
 * RETURNS:
 *  IGA_SUCCESS           upon success
 *  IGA_INVALID_ARG       if a gen argument invalid or ops_arr_len is null
 */
IGA_API iga_status_t iga_opspec_enumerate(iga_gen_t gen, iga_opspec_t *ops_arr,
                                          size_t *ops_arr_len);

/*
 * Looks up an opspec based on an iga::Op value.
 *   'gen'          gen plaform
 *   'op'           the operation value to lookup; this is an iga::Op;
 *                  an invalid op will succeed but return Op::INVALID
 *   'os'           the output value
 *
 * RETURNS:
 *  IGA_SUCCESS           upon success
 *  IGA_INVALID_ARG       if an argument is invalid
 *                        (invalid GEN or op or null os)
 */
IGA_API iga_status_t iga_opspec_from_op(iga_gen_t gen,
                                        /* iga::Op */ uint32_t op_enum,
                                        iga_opspec_t *os);

/*
 * Lists the mnemonic name for a given op (e.g. "sends")
 * (has similar semantics regarding the parameters as 'iga_opspec_enum')
 *
 * RETURNS:
 *  IGA_SUCCESS           upon success
 *  IGA_INVALID_ARG       if given an invalid argument
 */
IGA_API iga_status_t iga_opspec_mnemonic(const iga_opspec_t op, char *mnemonic,
                                         size_t *mnemonic_len);

/*
 * Lists the op name for a given op (e.g. "Split Send Message")
 * (has similar semantics regarding the parameters as 'iga_opspec_enum')
 *
 * RETURNS:
 *  IGA_SUCCESS           upon success
 *  IGA_INVALID_ARG       if a gen argument invalid
 */
IGA_API iga_status_t iga_opspec_name(const iga_opspec_t op, char *name,
                                     size_t *name_len);

/*
 * Lists the op desription for a given op
 * (has similar semantics regarding the parameters as 'iga_opspec_enum')
 *
 * RETURNS:
 *  IGA_SUCCESS           upon success
 *  IGA_INVALID_ARG       if a gen argument invalid
 */
IGA_API iga_status_t iga_opspec_description(iga_opspec_t op, char *desc,
                                            size_t *desc_len);

/*
 * Returns iga::Op for this operation as a uint32_t
 *
 * RETURNS:
 *  IGA_SUCCESS           upon success
 *  IGA_INVALID_ARG       if an argument is NULL
 */
IGA_API iga_status_t iga_opspec_op(iga_opspec_t os, uint32_t *op);

/*
 * Returns iga::Op value opcode encoding.  This can vary for the same op
 * on platform to platform.
 *
 * RETURNS:
 *  IGA_SUCCESS           upon success
 *  IGA_INVALID_ARG       if an argument is NULL
 */
IGA_API iga_status_t iga_opspec_op_encoding(iga_opspec_t op, uint32_t *opcode);

#ifdef __cplusplus
}
#endif

#endif
