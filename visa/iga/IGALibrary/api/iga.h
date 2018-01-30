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
/*
 * The IGA - Intel Gen Assembler external C API
 */
#ifndef IGA_H
#define IGA_H

#include <stdint.h>
#include <stddef.h>
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
        #define IGA_API __attribute__ ((visibility ("default")))
    #else
#define IGA_API
    #endif
#endif

#ifdef __cplusplus
extern "C"  {
#endif

/* opaque pointer to context type */
typedef void *iga_context_t;

typedef enum {
    IGA_SUCCESS        = 0, /* the operation completed successfully
                             * (iga_disassemble and iga_assemble may
                             * still have logged warnings) */
    IGA_ERROR          = 1, /* general error */
    IGA_INVALID_ARG    = 2, /* something wrong with an arugment */
    IGA_OUT_OF_MEM     = 3, /* failed to allocate */
    IGA_DECODE_ERROR   = 4, /* error during decode phase */
    IGA_ENCODE_ERROR   = 5, /* error during the encode phase */
    IGA_PARSE_ERROR    = 6, /* error duing the parse phase (syntax) */
    IGA_VERSION_ERROR  = 7, /* this header file is newer than the binary
                             * called */
    IGA_INVALID_OBJECT = 8, /* attempt to use an destroyed object
                             * (e.g. iga_context_t) */
    IGA_INVALID_STATE  = 9, /* e.g. call to iga_get_errors before disassembly*/
    IGA_UNSUPPORTED_PLATFORM = 10 /* platform not supported with this binary */
} iga_status_t;

/*
 * Converts an IGA status string code to a human-readable string.
 *
 * RETURNS: IGA_SUCCESS
 */
IGA_API const char *iga_status_to_string(const iga_status_t st);

/*
* Returns a NUL-terminated version string corresponding to the version of
* IGA.
*/
IGA_API const char *iga_version_string();


/* The encoding for GEN version enumerates follows this pattern */
#define GEN_VER(MAJ,MIN) (((MAJ)<<16)|(MIN))

/* Represents the specific version of Gen being assembled or disassembled */
typedef enum {
    IGA_GEN_INVALID = 0
  , IGA_GEN7      = GEN_VER(7,0)
  , IGA_GEN7p5    = GEN_VER(7,5)
  , IGA_GEN8      = GEN_VER(8,0)
  , IGA_GEN8lp    = GEN_VER(8,1)
  , IGA_GEN9      = GEN_VER(9,0)
  , IGA_GEN9lp    = GEN_VER(9,1)
  , IGA_GEN9p5    = GEN_VER(9,5)
  , IGA_GEN10     = GEN_VER(10,0)
} iga_gen_t;


/*
 * Context options
 */
typedef struct {
    size_t        cb;   /* set to sizeof(iga_context_options_t) */
    iga_gen_t     gen;
} iga_context_options_t;
/* this is an ugly wart, cb should have been uint32_t *
 * TODO: change this in IGA 2.0 and break binary compatibility there*/

/*
 * This macro initializes options to the context.
 *  iga_gen_t   P    the gen platform to use
 */
#define IGA_CONTEXT_OPTIONS_INIT(P) \
    {sizeof(iga_context_options_t), (P)}


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
IGA_API iga_status_t  iga_context_create(
    const iga_context_options_t *opts,
    iga_context_t *ctx);
/* deprecated: covers to iga_context_create */
IGA_API iga_status_t  iga_create_context(
    const iga_context_options_t *opts,
    iga_context_t *ctx);


/*
 * Releases a context previously created via 'iga_context_create'.
 *
 * RETURNS:
 *  IGA_SUCCESS         upon successful context release
 *  IGA_INVALID_ARG     if an argument is NULL
 *  IGA_INVALID_OBJECT  if ctx has already been destroyed
 */
IGA_API iga_status_t  iga_context_release(iga_context_t ctx);
/* deprecated: covers to iga_context_release */
IGA_API iga_status_t  iga_release_context(iga_context_t ctx);

/*
 * This structure contains options to the 'iga_assemble' call.
 */
typedef struct {
    uint32_t     cb;                 /* sizeof(iga_context_options_t)    */
    uint32_t     enabled_warnings;   /* bitset of IGA_WARNINGS*          */
    uint32_t     encoder_opts;       /* bitset of IGA_ENCODER_OPT*       */
    uint32_t     syntax_opts;        /* bitset of IGA_SYNTAX_OPT*        */
     /* the following must be 0 or a warning is raised */
    uint32_t     _reserved0;         /* use IGA_ENCODER_OPT_ERRONCOMPACT */
    uint32_t     _reserved1;         /* use IGA_ENCODER_OPT_AUTODEP      */
    /*
     * ... future fields (ensure total size is a multiple of 8;
     * add "reserved" if needed) ...
     * do not reuse _reserved0 and _reserved1 since those were
     * historically something else
     */
} iga_assemble_options_t;

/* detects screwups where someone adds a field and the compiler pads
 * the structure out implicitly */
static_assert(sizeof(iga_assemble_options_t) == 6*4,
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
#define IGA_ENCODER_OPT_AUTO_COMPACT            0x00000001u
/* auto set instruction dependencies */
#define IGA_ENCODER_OPT_AUTO_DEPENDENCIES       0x00000002u
/* treat failure to compact an instruction with a {Compacted} annotation
* as a hard error rather than just raising a warning */
#define IGA_ENCODER_OPT_ERROR_ON_COMPACT_FAIL   0x00000004u
/* enable experimental native encoder */
#define IGA_ENCODER_OPT_USE_NATIVE              0x00000008u

/*
 * options for the parsing phase
 */
/* nominal support for certain IsaAsm directives and syntax */
#define IGA_SYNTAX_OPT_LEGACY_SYNTAX   0x00000001u
/* enables syntax extensions */
#define IGA_SYNTAX_OPT_EXTENSIONS      0x00000002u


/*
 * extra assemble warnings (things to check)
 * (for iga_assemble_options_t::enabled_warnings)
 */
/* individual warnings */
#define IGA_WARNINGS_REGIONS    0x00000001u  /* -Wregions */
#define IGA_WARNINGS_TYPES      0x00000002u  /* -Wtypes */
#define IGA_WARNINGS_SCHED      0x00000004u  /* -Wscheduling */
#define IGA_WARNINGS_NORMFORM   0x00000008u  /* -Wnormal-form */
/* useful predefined sets */
#define IGA_WARNINGS_NONE       0x00000000u  /* -Wnone */
#define IGA_WARNINGS_ALL \
    (IGA_WARNINGS_REGIONS|\
    IGA_WARNINGS_TYPES|\
    IGA_WARNINGS_SCHED|\
    IGA_WARNINGS_NORMFORM)  /* -Wall */
#define IGA_WARNINGS_DEFAULT \
    (IGA_WARNINGS_REGIONS|\
    IGA_WARNINGS_SCHED) /* -Wdefault */

/* helpful initializers for assembly options with good defaults */
#define IGA_ASSEMBLE_OPTIONS_INIT() \
    { \
      sizeof(iga_assemble_options_t), \
      IGA_WARNINGS_DEFAULT, \
      (IGA_ENCODER_OPT_ERROR_ON_COMPACT_FAIL), \
      0, /* syntax_opts = NONE */ \
      0, /* reserved */ \
      0  /* reserved */ \
    }
#define IGA_ASSEMBLE_OPTIONS_INIT_COMPACTION_WARNING() \
    { \
      sizeof(iga_assemble_options_t), \
      IGA_WARNINGS_DEFAULT, \
      (IGA_ENCODER_OPT_AUTO_COMPACT | IGA_ENCODER_OPT_AUTO_DEPENDENCIES), \
      0, /* syntax_opts = NONE */ \
      0, /* reserved */ \
      0  /* reserved */ \
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
IGA_API iga_status_t  iga_context_assemble(
    iga_context_t ctx,
    const iga_assemble_options_t *opts,
    const char *kernel_text,
    void **output,
    uint32_t *output_size);
/* deprecated API covers to iga_contex* */
IGA_API iga_status_t  iga_assemble(
    iga_context_t ctx,
    const iga_assemble_options_t *opts,
    const char *kernel_text,
    void **output,
    uint32_t *output_size);


/*
 * This structure contains options to the 'iga_disassemble' call.
 */
typedef struct {
    uint32_t     cb;   /* set to sizeof(iga_context_options_t) */
    uint32_t     formatting_opts; /* options in formatting the syntax */
    /* the following must be 0 or a warning is raised */
    uint32_t     _reserved0; /* use formatting_opts */
    uint32_t     _reserved1; /* use formatting_opts */
    /* ... future fields (ensure total size is a multiple of 8;
     * add "reserved" if needed) ... */
} iga_disassemble_options_t;

static_assert(sizeof(iga_disassemble_options_t) == 4*4,
    "wrong size for iga_disassemble_options_t");

/* A default value for iga_disassemble_options_t */
#define IGA_DISASSEMBLE_OPTIONS_INIT() \
    { sizeof(iga_disassemble_options_t), \
      IGA_FORMATTING_OPTS_DEFAULT, \
      0, /* _reserved0 */ \
      0  /* _reserved1 */ }

/* A default value for iga_disassemble_options_t that enables numeric labels */
#define IGA_DISASSEMBLE_OPTIONS_INIT_NUMERIC_LABELS() \
    {sizeof(iga_disassemble_options_t), \
    IGA_FORMATTING_OPTS_DEFAULT|IGA_FORMATTING_OPT_NUMERIC_LABELS, \
    0, /* _reserved0 */ \
    0  /* _reserved1 */ }

/*
 * options for the formatting phase (the syntax emitted / printed)
 */
/* does not use numeric labels */
#define IGA_FORMATTING_OPT_NUMERIC_LABELS   0x00000001u
/* enables certain syntax extensions */
#define IGA_FORMATTING_OPT_SYNTAX_EXTS      0x00000002u
/* force floats to be emitted in hexadecimal */
#define IGA_FORMATTING_OPT_PRINT_HEX_FLOATS 0x00000004u
/* print the instruction PC's */
#define IGA_FORMATTING_OPT_PRINT_PC         0x00000008u
/* print the instruction bits */
#define IGA_FORMATTING_OPT_PRINT_BITS       0x00000010u
/* print instruction dependencies */
#define IGA_FORMATTING_OPT_PRINT_DEPS       0x00000020u

/* just the default formatting opts */
#define IGA_FORMATTING_OPTS_DEFAULT \
    (0u)
/* a union of all formatter opts */
#define IGA_FORMATTING_OPTS_ALL = \
    (IGA_FORMATTING_OPT_NUMERIC_LABELS\
    |IGA_FORMATTING_OPT_SYNTAX_EXTS\
    |IGA_FORMATTING_OPT_PRINT_HEX_FLOATS\
    |IGA_FORMATTING_OPT_PRINT_PC\
    |IGA_FORMATTING_OPT_PRINT_BITS\
    |IGA_FORMATTING_OPT_PRINT_DEPS)

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
 *                  call using this context (freed or even reused)
 * RETURNS:
 *  IGA_SUCCESS         upon successful disassembly; 'iga_get_warnings' may
 *                      contain warning diagnostics even upon success
 *  IGA_INVALID_ARG     if an argument is NULL; 'input' may be NULL only
 *                      if 'input_size' is also 0
 *  IGA_INVALID_OBJECT  if ctx has already been destroyed
 *  IGA_DECODE_ERROR    upon failure to decode error; specific error messages
 *                      may be retrieved via 'iga_context_get_errors'
 */
IGA_API  iga_status_t  iga_context_disassemble(
    iga_context_t ctx,
    const iga_disassemble_options_t *opts,
    const void *input,
    uint32_t input_size,
    const char *(*fmt_label_name)(int32_t, void *),
    void *fmt_label_ctx,
    char **kernel_text);
/* deprecated API covers to iga_contex* */
IGA_API  iga_status_t  iga_disassemble(
    iga_context_t ctx,
    const iga_disassemble_options_t *opts,
    const void *input,
    uint32_t input_size,
    const char *(*fmt_label_name)(int32_t, void *),
    void *fmt_label_ctx,
    char **kernel_text);


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
 *                  call using this context (freed or even reused)
 *
 * RETURNS:
 *  IGA_SUCCESS         upon successful disassembly; 'iga_get_warnings' may
 *                      contain warning diagnostics even upon success
 *  IGA_INVALID_ARG     if an argument is NULL
 *  IGA_INVALID_OBJECT  if ctx has already been destroyed
 *  IGA_DECODE_ERROR    upon failure to decode error; specific error messages
 *                      may be retrieved via 'iga_context_get_errors'
 */
IGA_API  iga_status_t  iga_context_disassemble_instruction(
    iga_context_t ctx,
    const iga_disassemble_options_t *dopts,
    const void *input,
    const char *(*fmt_label_name)(int32_t, void *),
    void *fmt_label_ctx,
    char **kernel_text);
/* deprecated API covers to iga_contex* */
IGA_API  iga_status_t  iga_disassemble_instruction(
    iga_context_t ctx,
    const iga_disassemble_options_t *dopts,
    const void *input,
    const char *(*fmt_label_name)(int32_t, void *),
    void *fmt_label_ctx,
    char **kernel_text);

/*
 * A diagnostic message (e.g. error or warning)
 *
 * WARNING: This structure may be made opaque in future releases.
 *          Treat it as an opaque type and use the accessors to minimize
 *          any conversion time down the road.
 */
typedef struct {
    /* lines start at 1; 0 means no location information */
    uint32_t    line;
    /* columns start at 1; 0 means no location information */
    uint32_t    column;
    /* the input offset */
    uint32_t    offset;
    /* the length of the diagnostic (in characters) */
    uint32_t    extent;
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
IGA_API iga_status_t iga_context_get_errors(
    iga_context_t ctx,
    const iga_diagnostic_t **ds,
    uint32_t *ds_len);
IGA_API iga_status_t iga_get_errors(
    iga_context_t ctx,
    const iga_diagnostic_t **ds,
    uint32_t *ds_len);


/*
 * Symmetric to 'iga_context_get_errors'.  Various API calls can still succeed
 * even if there are warnings; hence, robust tools should always check this
 * as well.
 */
IGA_API iga_status_t iga_context_get_warnings(
    iga_context_t ctx,
    const iga_diagnostic_t **ds,
    uint32_t *ds_len);
IGA_API iga_status_t iga_get_warnings(
    iga_context_t ctx,
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
IGA_API iga_status_t iga_diagnostic_get_message(
    const iga_diagnostic_t *d,
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
IGA_API iga_status_t iga_diagnostic_get_offset(
    const iga_diagnostic_t *d,
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
IGA_API iga_status_t iga_diagnostic_get_type(
    const iga_diagnostic_t *d,
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
IGA_API iga_status_t iga_diagnostic_get_text_line(
    const iga_diagnostic_t *d,
    uint32_t *line);


/*
 * Similar to 'iga_diagnostic_text_get_line', but returns the column.
 * Columns are counted from 1.
 */
IGA_API iga_status_t iga_diagnostic_get_text_column(
    const iga_diagnostic_t *d,
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
IGA_API iga_status_t iga_diagnostic_get_text_extent(
    const iga_diagnostic_t *d,
    uint32_t *extent);


/*
 * An opaque type representing an operation (instruction) type.
 * Can be efficiently copied and passed by value.
 */
typedef struct opspec* iga_opspec_t;

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
IGA_API iga_status_t iga_opspec_enumerate(
    iga_gen_t gen,
    iga_opspec_t *ops_arr,
    size_t *ops_arr_len);


/*
 * Looks up an opspec based on an iga::Op value.
 *   'gen'          : gen plaform
 *   'op'           : the operation value to lookup; this is an iga::Op
 *   'os'           : the output value
 *
 * RETURNS:
 *  IGA_SUCCESS           upon success
 *  IGA_INVALID_ARG       if an argument invalid (invalid GEN or op or null os)
 */
IGA_API iga_status_t iga_opspec_from_op(
    iga_gen_t gen,
    /* iga::Op */ uint32_t op,
    iga_opspec_t *os);


/*
 * Lists the mnemonic name for a given op (e.g. "sends")
 * (has similar semantics regarding the parameters as 'iga_opspec_enum')
 *
 * RETURNS:
 *  IGA_SUCCESS           upon success
 *  IGA_INVALID_ARG       if a gen argument invalid
 */
IGA_API iga_status_t iga_opspec_mnemonic(
    const iga_opspec_t op,
    char *mnemonic,
    size_t *mnemonic_len);


/*
 * Lists the op name for a given op (e.g. "Split Send Message")
 * (has similar semantics regarding the parameters as 'iga_opspec_enum')
 *
 * RETURNS:
 *  IGA_SUCCESS           upon success
 *  IGA_INVALID_ARG       if a gen argument invalid
 */
IGA_API iga_status_t iga_opspec_name(
    const iga_opspec_t op,
    char *name,
    size_t *name_len);


/*
 * Lists the op desription for a given op
 * (has similar semantics regarding the parameters as 'iga_opspec_enum')
 *
 * RETURNS:
 *  IGA_SUCCESS           upon success
 *  IGA_INVALID_ARG       if a gen argument invalid
 */
IGA_API iga_status_t iga_opspec_description(
    const iga_opspec_t op,
    char *desc,
    size_t *desc_len);


/*
 * Returns iga::Op for this operation as a uint32_t
 *
 * RETURNS:
 *  IGA_SUCCESS           upon success
 *  IGA_INVALID_ARG       if an argument is NULL
 */
IGA_API iga_status_t iga_opspec_op(
    iga_opspec_t os,
    uint32_t *op);


/*
 * Returns iga::Op value opcode encoding.  This can vary for the same op
 * on platform to platform.
 *
 * RETURNS:
 *  IGA_SUCCESS           upon success
 *  IGA_INVALID_ARG       if an argument is NULL
 */
IGA_API iga_status_t iga_opspec_op_encoding(
    iga_opspec_t op,
    uint32_t *opcode);

/*************************************************************************
 *                                                                       *
 *                  The KernelView C interface                           *
 *                                                                       *
 *************************************************************************/


/*
 * This symbols defines the maximum number of PC targets that an instruction
 * may have.  It is typically used to statically allocate an array of target
 * PCs with the kv_get_inst_targets function.
 * E.g.
 *   uint32_t targetPCs[KV_MAX_TARGETS_PER_INSTRUCTION];
 *   uint32_t num = kv_get_inst_targets(kv, atPc, &targets[0]);
 *   for (int i = 0; i < num; i++) {
 *      processTarget(targetPCs[i]);
 *   }
 *
 */
#define KV_MAX_TARGETS_PER_INSTRUCTION 3
/*
 * This symbol represents an invalid PC.  0 is a valid PC (the beginnning
 * of the kernel).
 */
#define KV_INVALID_PC ((int32_t)0xFFFFFFFF)

/* incomplete type for a kernel view handle */
struct kv_t;


/*
 * Creates a kernel view.
 *   'plat' - the platform
 *   'bytes' - the kernel binary
 *   'bytes_len' - the length of 'bytes'
 *   'status' - the IGA status code
 *   'errbuf' - an optional buffer to emit errors or warnings (can pass nullptr)
 *   'errbuf_cap' - the capacity of errbuf.
 * RETURNS: a kernel view object for use in other kv_* functions.
 * Deallocate it with kv_delete.  If there is a decode error (or other errors), this
 * function returns an instance of Kernel Views and ERROR status. If user proceeds
 * to use the returned Kernel View we do not guarantee that all bits are correct
 */
IGA_API kv_t *kv_create(
    iga_gen_t plat,
    const void *bytes,
    size_t bytes_len,
    iga_status_t *status,
    char *errbuf,
    size_t errbuf_cap);

/* destroys a kernel view */
IGA_API void kv_delete(kv_t *);


/*
 * Returns the size of the instruction at 'pc'; returns 0 if the program
 * address is out of bounds.  This allows one to iterate a kernel using this
 * API.  For example:
 *
 *   uint32_t iLen;
 *   for (uint32_t pc = 0;
 *        (iLen = kv_get_inst_size(kv, pc)) != 0;
 *        pc += iLen)
 *   {
 *     ... process instruction
 *   }
 */
IGA_API int32_t kv_get_inst_size(const kv_t *kv, int32_t pc);


/*
 * This function returns the absolute PC targets of this instruction.
 * For branching instructions, it populates 'pcs' with the jump targets
 * of this instruction.  The number of PC's will always be less than or
 * equal to MAX_KV_TARGETS_COUNT.  The function returns the number of
 * target PCs populated in the 'pcs' argument.
 *
 * For non-branching instructions this returns 0 and does not touch 'pcs'.
 *
 * If 'pcs' is NULL, it is ignored.  The number of targets is still returned.
 */
IGA_API uint32_t kv_get_inst_targets(
    const kv_t *kv,
    int32_t pc,
    int32_t *pcs);


/*
 * This function returns the syntax for a given instruction.
 * The user passes the buffer 'sbuf' (along with its capacity) to hold
 * the output.
 *
 * The optional 'get_label_name' callback converts a PC into a label.
 * The caller can provide NULL and internal label names will be used.
 * The 'env' context parameter is passed to 'get_label_name'.
 * Memory returned by the callback is only read.  If the callback allocates,
 * then the caller of this function must cleanup.
 */
IGA_API size_t kv_get_inst_syntax(
    const kv_t *kv,
    int32_t pc,
    char *sbuf,
    size_t sbuf_cap,
    const char *(*get_label_name)(int32_t, void *),
    void *env);

/*
 * This function returns the default label name if custom labeler is not used.
 */
IGA_API size_t kv_get_default_label_name(
    int32_t pc,
    char *sbuf,
    size_t sbuf_cap);

/*
 * Returns non-zero iff this instruction is a branch target.
 * The caller can use this function to determine if it should emit a label
 * first.
 */
IGA_API uint32_t kv_is_inst_target(const kv_t *kv, int32_t pc);


/*
 * This enumeration allows one to determine if a given PC is for structured
 * control flow.  This is for tools that want to render an indentation for
 * readability.
 */
typedef enum {
    KV_OPGROUP_INVALID,   /* not a valid op (e.g. out of bounds, middle of instruction) */
    KV_OPGROUP_OTHER,     /* some other instruction */
    KV_OPGROUP_IF,        /* an 'if' op */
    KV_OPGROUP_ELSE,      /* an 'else' op */
    KV_OPGROUP_ENDIF,     /* an 'endif' op */
    KV_OPGROUP_WHILE,     /* a 'while' op */
    KV_OPGROUP_SEND_EOT,  /* a send message with the EOT bit set */
} kv_opgroup_t;


/*
 * This function returns the opcode group.  The result may be compared
 * to the integral value of the various kv_opcode_group enumerates.
 * (See enum kv_get_opgroup_t.)
 */
IGA_API int32_t kv_get_opgroup(const kv_t *kv, int32_t pc);


/*
 * Returns the send function descriptors.  The count of descriptors is
 * returned; hence, if the instruction is invalid or not a send or
 * send using two index registers, 0 is returned.
 * If one of the descriptors is not immediate, then 1 is returned
 * and that descriptor is set to KV_INVALID_SEND_DESC.
 *
 * Also returns 0 if any parameter is NULL (and parameters are untouched).
 */
IGA_API uint32_t kv_get_send_descs(
    const kv_t *kv,
    int32_t pc,
    uint32_t *ex_desc,
    uint32_t *desc);
/*
 * A symbol to indicate an invalid send descriptor value.
 */
#define KV_INVALID_SEND_DESC ((uint32_t)0xFFFFFFFFF)

 /* TODO: review necessity of this macro.
  * A symbol to indicate an invalid message length value.
  */
#define KV_INVALID_LEN ((uint32_t)0xFFFFFFFFF)

/*************************KV Analyze APIS**************************************/

/*
 * Returns message type for the following SFID:
 * Sampler, DP_CC, DP_DC0, DP_DC1, DP_DC2, DP_RC, DP_DCR0
 */
IGA_API int32_t kv_get_message_type(const kv_t *kv, int32_t pc);

/*
 * Returns message sfid.
 */
IGA_API int32_t kv_get_message_sfid(const kv_t *kv, int32_t pc);

/*
 * Sets message length, extended message length, and response length in units of registers.
 * The count of lengths successfully set is returned. If any of the parameters is NULL,
 * it returns 0. Invalid lengths are set to KV_INVALID_LEN.
 */
IGA_API uint32_t kv_get_message_len(const kv_t *kv, int32_t pc, uint32_t* mLen, uint32_t* emLen, uint32_t* rLen);

/*
 * Returns Execution size of the instruction
 * 0 - INVALID
 * 1 - EXEC_SIZE_1
 * 2 - EXEC_SIZE_2
 * 3 - EXEC_SIZE_4
 * 4 - EXEC_SIZE_8
 * 5 - EXEC_SIZE_16
 * 6 - EXEC_SIZE_32
 */
IGA_API uint32_t kv_get_execution_size(const kv_t *kv, int32_t pc);


/*
 * Returns number of sources this instruction has.
 */
IGA_API int32_t kv_get_number_sources(const kv_t *kv, int32_t pc);

/*
 * This function returns OPcode integer.  The value corresponds to
 * binary encoding value of the opcode.
 */
IGA_API uint32_t kv_get_opcode(const kv_t *kv, int32_t pc);

/*
 * This function returns if intruction has destination.
 */
IGA_API int32_t kv_get_has_destination(const kv_t *kv, int32_t pc);

/*
 * This function returns destination Register row
 */
IGA_API int32_t kv_get_destination_register(const kv_t *kv, int32_t pc);

/*
 * This function returns destination subRegister
 */
IGA_API int32_t kv_get_destination_sub_register(const kv_t *kv, int32_t pc);

/*
 * This function returns destination data type
 * i.e. F, HF, INT, etc
 */
IGA_API uint32_t kv_get_destination_data_type(const kv_t *kv, int32_t pc);

/*
 * This function returns destination register type
 * i.e. GRF, various ARF registers
 */
IGA_API uint32_t kv_get_destination_register_type(const kv_t *kv, int32_t pc);

/*
 * This function returns destination register KIND
 * DIRECT, INDIRECT, IMM, INDIR etc
 */
IGA_API uint32_t kv_get_destination_register_kind(const kv_t *kv, int32_t pc);

/*
 * This function returns source register line number for a given source.
 */
IGA_API int32_t kv_get_source_register(const kv_t *kv, int32_t pc, uint32_t sourceNumber);

/*
 * This function returns source subRegister for a given source.
 */
IGA_API int32_t kv_get_source_sub_register(const kv_t *kv, int32_t pc, uint32_t sourceNumber);

/*
 * This function returns source data type for a given source
 * i.e. F, HF, INT, etc
 */
IGA_API uint32_t kv_get_source_data_type(const kv_t *kv, int32_t pc, uint32_t sourceNumber);

/*
 * This function returns source register type for a given source.
 * i.e. GRF, various ARF registers
 */
IGA_API uint32_t kv_get_source_register_type(const kv_t *kv, int32_t pc, uint32_t sourceNumber);

/*
 * This function returns source register KIND for a given source
 * DIRECT, INDIRECT, IMM, INDIR etc
 */
IGA_API uint32_t kv_get_source_register_kind(const kv_t *kv, int32_t pc, uint32_t sourceNumber);

/*
 * This function returns whether source is a vector.
 */
IGA_API int32_t kv_is_source_vector(const kv_t *kv, int32_t pc, uint32_t sourceNumber);

/*
 * This function returns mask offset
 */
IGA_API uint32_t kv_get_channel_offset(const kv_t *kv, int32_t pc);

/*
 * This function returns mask control
 */
IGA_API uint32_t kv_get_mask_control(const kv_t *kv, int32_t pc);

/*
 * This function exposes destination region.
 */
IGA_API int32_t kv_get_destination_region(const kv_t *kv, int32_t pc, uint32_t *hz);

/*
 * This function exposes source operand region.
 */
IGA_API int32_t kv_get_source_region(const kv_t *kv, int32_t pc, uint32_t src_op, uint32_t *vt, uint32_t *wi, uint32_t *hz);


#ifdef __cplusplus
}
#endif

#endif
