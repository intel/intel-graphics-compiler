#include "iga.h"
#include "igad.h"
#include "iga.hpp"
// IGA headers
#include "../Backend/EncoderOpts.hpp"
#include "../Backend/GED/Decoder.hpp"
#include "../Backend/GED/Encoder.hpp"
#include "../Backend/GED/GEDUtil.hpp"
#include "../Backend/Native/Interface.hpp"
#include "../ErrorHandler.hpp"
#include "../Frontend/Formatter.hpp"
#include "../Frontend/KernelParser.hpp"
#include "../IR/DUAnalysis.hpp"
#include "../IR/IRChecker.hpp"
#include "../strings.hpp"
#include "../version.hpp"

// external dependencies
#include <cstring>
#include <map>
#include <vector>
#include <ostream>
#include <sstream>


using namespace iga;

//
// BINARY COMPATIBILITY:
// We can add stuff to the assemble/diasseemble options without forcing users
// to recompile. E.g. the user compiles with an older header and has:
//   struct opts {
//       uint32_t cb; // = sizeof(opts)
//       uint32_t opt;
//   };
// #define INIT_OPTS(A) {sizeof(struct opts), A}
//
// Later, we internally add new options (features) and make the structure
//   struct opts {
//       uint32_t cb; // = sizeof(opts)
//       uint32_t opt;
//       uint32_t newOpt;
//       uint32_t _reserved; // to pad out the structure on 64-bit compilers
//   };
// #define INIT_OPTS(A1,A2) {sizeof(struct opts), A1, A2, 0}
//
// WARNING: ensure you pad the structure to 64 bit alignment.
// When IGA copies in the user structure it is careful to honor the user's
// cb field to determine how much of the structure to read in.  We use a memcpy
// to copy the structure in safely.
//
//   void iga_api(struct opts *opts)
//   {
//     struct opts optsInternal = INIT_OPTS(default1, default2);
//     memcpy(&optsInternal, opts, opts->cb);
//     use(optsInternal);
//   }
//
// The above strategy allows us to add to the options as long as the new
// option has a sensible default.


const char *iga_status_to_string(const iga_status_t st) {
    switch (st) {
    case IGA_SUCCESS:              return "succeeded";
    case IGA_ERROR:                return "unknown error";
    case IGA_INVALID_ARG:          return "invalid argument";
    case IGA_OUT_OF_MEM:           return "out of memory";
    case IGA_DECODE_ERROR:         return "decode error";
    case IGA_ENCODE_ERROR:         return "encode error";
    case IGA_PARSE_ERROR:          return "parse error";
    case IGA_VERSION_ERROR:        return "version mismatch";
    case IGA_INVALID_OBJECT:       return "invalid object";
    case IGA_INVALID_STATE:        return "invalid state";
    case IGA_UNSUPPORTED_PLATFORM: return "unsupported platform";
    default:                       return "invalid error code";
    }
}

class IGAContext {
public:
    static bool convertPlatform(iga_gen_t gen, iga::Platform &platf) {
        switch (gen) {
        case IGA_GEN7:      platf = iga::Platform::GEN7;      break;
        case IGA_GEN7p5:    platf = iga::Platform::GEN7P5;    break;
        case IGA_GEN8:      platf = iga::Platform::GEN8;      break;
        case IGA_GEN8lp:    platf = iga::Platform::GEN8LP;    break;
        case IGA_GEN9:      platf = iga::Platform::GEN9;      break;
        case IGA_GEN9lp:    platf = iga::Platform::GEN9LP;    break;
        case IGA_GEN9p5:    platf = iga::Platform::GEN9P5;    break;
        case IGA_GEN10:     platf = iga::Platform::GEN10;     break;
        default:            platf = iga::Platform::INVALID; return false;
        }
        return true;
    }

    static const Model &convertPlatform(iga_gen_t gen) {
        iga::Platform platform;
        if (!convertPlatform(gen, platform)) {
            throw FatalError();
        }
        return *Model::LookupModel(platform);
    }
private:
    // set to a magic constant when the object is valid (live)
    // this helps detection such as people passing the wrong context
    // e.g. iga_release_context(&ctx) instead of iga_release_context(ctx)
    uint64_t                        m_validToken;
    // the options the context was created with
    iga_context_options_t           m_opts;
    // the model corresponding to the gen options
    const Model&                    m_model;
    // a cached copy of the assembled bits
    // we free this upon destruction
    void                           *m_assemble_bits;
    // a cached copy of the last disassembled text
    // we free this upon destruction
    char                           *m_disassemble_text;
    // a reusable empty string to return on errors
    char                            m_empty_string[4];

    // diagnostics from the last compile
    bool                            m_errorsValid, m_warningsValid;
    std::vector<iga_diagnostic_t>   m_errors, m_warnings;
public:
    static void clearDiagnostics(std::vector<iga_diagnostic_t> &api_ds) {
        for (auto &d : api_ds) {
            free((void*)d.message);
            memset(&d, 0xDE, sizeof(d));
        }
        api_ds.clear();
    }


  static iga_status_t translateDiagnosticList(
      const std::vector<iga::Diagnostic> &ds,
      std::vector<iga_diagnostic_t> &api_ds)
  {
      for (const auto &d : ds) {
            const char *str = strdup(d.message.c_str());
            if (!str) {
                return IGA_OUT_OF_MEM;
            }

            if (d.at.col != 0 && d.at.line != 0) {
                // col and line
                iga_diagnostic_t temp = {d.at.line, d.at.col, d.at.offset, d.at.extent, str};
                api_ds.push_back(temp);
            } else {
                // pc (offset is in bytes)
                iga_diagnostic_t temp = {0, 0, d.at.offset, d.at.extent, str};
                api_ds.push_back(temp);
            }
        }
        return IGA_SUCCESS;
    }

    iga_status_t translateDiagnostics(const iga::ErrorHandler &err) {
        clearDiagnostics(m_errors);
        clearDiagnostics(m_warnings);
        m_warningsValid = m_errorsValid = false;

        iga_status_t s1 = translateDiagnosticList(err.getErrors(), m_errors);
        if (s1 != IGA_SUCCESS) {
            clearDiagnostics(m_errors);
            return s1;
        }

        iga_status_t s2 = translateDiagnosticList(err.getWarnings(), m_warnings);
        if (s2 != IGA_SUCCESS) {
            clearDiagnostics(m_warnings);
            clearDiagnostics(m_errors);
            return s2;
        }

        m_warningsValid = m_errorsValid = true;
        return IGA_SUCCESS;
    }

    static const uint64_t VALID_COOKIE = 0xFEDCBA9876543210ull;

public:
    IGAContext(iga_context_options_t opts, iga::Platform platf)
        : m_validToken(VALID_COOKIE)
        , m_opts(opts)
        , m_model(convertPlatform(opts.gen))
        , m_assemble_bits(nullptr)
        , m_disassemble_text(nullptr)
        , m_errorsValid(false)
        , m_warningsValid(false)
    {
        memset(m_empty_string, 0, sizeof(m_empty_string));
    }


    ~IGAContext() {
        m_validToken = 0xDEADDEADDEADDEADull;
        clearDiagnostics(m_warnings);
        clearDiagnostics(m_errors);

        if (m_disassemble_text) {
            free(m_disassemble_text);
            m_disassemble_text = nullptr;
        }
        if (m_assemble_bits) {
            free(m_assemble_bits);
            m_assemble_bits = nullptr;
        }
    }


    bool valid() const {
        return m_validToken == VALID_COOKIE;
    }


    iga_status_t assemble(
        iga_assemble_options_t &aopts,
        const char *inp,
        void **bits,
        uint32_t *bitsLen)
    {
        iga::ErrorHandler errHandler;
        // compatibility for legacy fields
        bool used_legacy_fields = false;
        if (aopts._reserved0) { // used to be error_on_compact_fail
            aopts.encoder_opts |= IGA_ENCODER_OPT_ERROR_ON_COMPACT_FAIL;
            used_legacy_fields = true;
        }
        if (aopts._reserved1) { // used to be error_on_compact_fail
            aopts.encoder_opts |= IGA_ENCODER_OPT_AUTO_DEPENDENCIES;
            used_legacy_fields = true;
        }
        if (used_legacy_fields) {
            errHandler.reportWarning(Loc(1,1,0,0),
                "iga_assemble call uses deprecated options "
                " (error_on_compact_fail or autoset_deps); see newest "
                "iga.h header file for updated fields");
        }

        // 1. Parse the kernel text
        ParseOpts popts;
        popts.supportLegacyDirectives =
            (aopts.syntax_opts & IGA_SYNTAX_OPT_LEGACY_SYNTAX) != 0;
        Kernel *kernel = iga::ParseGenKernel(m_model, inp, errHandler, popts);
        if (kernel && !errHandler.hasErrors() && aopts.enabled_warnings) {
            // check semantics if we parsed without error && they haven't
            // disabled all checking (-Wnone)
            CheckSemantics(*kernel, errHandler, aopts.enabled_warnings);
        }
        if (errHandler.hasErrors()) {
            *bits = nullptr;
            *bitsLen = 0;
            iga_status_t st = translateDiagnostics(errHandler);
            if (st != IGA_SUCCESS)
                return st;
            return IGA_PARSE_ERROR;
        }

        if (errHandler.hasErrors()) {
            *bits = nullptr;
            *bitsLen = 0;
            iga_status_t st = translateDiagnostics(errHandler);
            if (st != IGA_SUCCESS) {
                return st;
            }
            delete kernel;
            return st == IGA_SUCCESS ? IGA_ENCODE_ERROR : st;
        }

        // 3. Encode the final IR into bits
        //
        // clobber the last assembly's bits
        if (m_assemble_bits) {
            free(m_assemble_bits);
            m_assemble_bits = nullptr;
        }
        EncoderOpts eopts(
            (aopts.encoder_opts & IGA_ENCODER_OPT_AUTO_COMPACT) != 0,
            (aopts.encoder_opts & IGA_ENCODER_OPT_ERROR_ON_COMPACT_FAIL) == 0,
            (aopts.encoder_opts & IGA_ENCODER_OPT_AUTO_DEPENDENCIES) != 0);
        if ((aopts.encoder_opts & IGA_ENCODER_OPT_USE_NATIVE) == 0) {
            Encoder enc(m_model, errHandler, eopts);
            enc.encodeKernel(
                *kernel,
                kernel->getMemManager(),
                *bits,
                *bitsLen);
        } else {
            int ibitsLen = 0;
            iga::native::Encode(
                m_model,
                eopts,
                errHandler,
                *kernel,
                *bits,
                ibitsLen);
            *bitsLen = (uint32_t)ibitsLen;
        }
        if (errHandler.hasErrors()) {
            // failed encoding
            delete kernel;
            iga_status_t st = translateDiagnostics(errHandler);
            return st == IGA_SUCCESS ? IGA_ENCODE_ERROR : st;
        }

        // 4. Copy out the result
        // encoding succeeded, copy the bits out
        m_assemble_bits = (void *)malloc(*bitsLen);
        if (!m_assemble_bits) {
            delete kernel;
            return IGA_OUT_OF_MEM;
        }
        MEMCPY(m_assemble_bits, *bits, *bitsLen);
        *bits = m_assemble_bits;
        delete kernel;
        return translateDiagnostics(errHandler);
    }

    FormatOpts formatterOpts(
        const iga_disassemble_options_t &dopts,
        const char *(*formatLabel)(int32_t, void *),
        void *formatLabelEnv)
    {
        FormatOpts fopts(
            m_model.platform,
            formatLabel,
            formatLabelEnv);
        fopts.numericLabels =
            (dopts.formatting_opts & IGA_FORMATTING_OPT_NUMERIC_LABELS) != 0;
        fopts.syntaxExtensions =
            (dopts.formatting_opts & IGA_FORMATTING_OPT_SYNTAX_EXTS) != 0;
        fopts.hexFloats =
            (dopts.formatting_opts & IGA_FORMATTING_OPT_PRINT_HEX_FLOATS) != 0;
        fopts.printInstPc =
            (dopts.formatting_opts & IGA_FORMATTING_OPT_PRINT_PC) != 0;
        fopts.printInstBits =
            (dopts.formatting_opts & IGA_FORMATTING_OPT_PRINT_BITS) != 0;
        fopts.printInstDeps =
            (dopts.formatting_opts & IGA_FORMATTING_OPT_PRINT_DEPS) != 0;
        return fopts;
    }

    void checkForLegacyFields(
        iga_disassemble_options_t &dopts,
        iga::ErrorHandler &errHandler)
    {
        // compatibility for legacy fields
        bool used_legacy_fields = false;
        if (dopts._reserved0) { // used to be hex_floats
            dopts.formatting_opts |= IGA_FORMATTING_OPT_PRINT_HEX_FLOATS;
            used_legacy_fields = true;
        }
        if (dopts._reserved1) { // used to be hex_floats
            dopts.formatting_opts |= IGA_FORMATTING_OPT_PRINT_PC;
            used_legacy_fields = true;
        }
        if (used_legacy_fields) {
            errHandler.reportWarning(Loc(1,1,0,0),
                "iga_disassemble* call uses deprecated options "
                " (hex_floats or print_pc); see newest "
                "iga.h header file for updated fields");
        }
    }

    iga_status_t disassemble(
        iga_disassemble_options_t &dopts,
        const void *bits,
        uint32_t bitsLen,
        const char *(*formatLbl)(int32_t, void *),
        void *formatLblEnv,
        char **output)
    {
        iga::Kernel *k = nullptr;
        iga::ErrorHandler errHandler;
        try {
            checkForLegacyFields(dopts, errHandler);

            iga::Decoder decoder(m_model, errHandler);
            k = (dopts.formatting_opts & IGA_FORMATTING_OPT_NUMERIC_LABELS) != 0 ?
                decoder.decodeKernelNumeric(bits, bitsLen) :
                decoder.decodeKernelBlocks(bits, bitsLen);
            if (!k) {
                // bail to cleanup
                throw iga::FatalError();
            }

            std::stringstream ss;
            FormatOpts fopts = formatterOpts(dopts, formatLbl, formatLblEnv);
            DepAnalysis la;
            if (dopts.formatting_opts & IGA_FORMATTING_OPT_PRINT_DEPS) {
                la = ComputeDepAnalysis(k);
                fopts.liveAnalysis = &la;
            }
            FormatKernel(errHandler, ss, fopts, *k, bits);

            // copy the text out
            if (m_disassemble_text) {
                // previous disassemble clobbers new disassemble
                free(m_disassemble_text);
            }
            size_t slen = (size_t)ss.tellp();
            m_disassemble_text = (char *)malloc(1 + slen);
            if (!m_disassemble_text) {
                // bail out
                delete k;
                return IGA_OUT_OF_MEM;
            }
            ss.read(m_disassemble_text, slen);
            m_disassemble_text[slen] = 0;
            if(output) {
                *output = m_disassemble_text;
            }
        } catch (const iga::FatalError &) {
            // reportFatal stops hard via an exception; diagnostics will be
            // translated below
        }
        if (k)
            delete k;

        iga_status_t st = translateDiagnostics(errHandler);
        if (errHandler.hasErrors()) {
            return IGA_DECODE_ERROR;
        }
        return st;
    }


    iga_status_t disassembleInstruction(
        iga_disassemble_options_t &dopts,
        const void *bits,
        const char *(*formatLabel)(int32_t, void *),
        void *formatLabelEnv,
        char **output)
    {
        if (output)
            *output = &m_empty_string[0];
        // infer the length based on
        uint32_t bitsLen = ((((const uint8_t *)bits)[3]) & 0x20) ? 8 : 16;

        iga::ErrorHandler errHandler;
        try {
            checkForLegacyFields(dopts, errHandler);

            iga::Decoder decoder(m_model, errHandler);
            iga::Kernel *k = decoder.decodeKernelNumeric(bits, bitsLen);
            if (!k) {  // bail to cleanup
                throw iga::FatalError();
            }
            const iga::BlockList &bl = k->getBlockList();
            const Instruction *inst = bl.front()->getInstList().front();
            if (m_disassemble_text) {
                // previous disassemble clobbers new disassemble
                free(m_disassemble_text);
            }

            std::stringstream ss;
            FormatOpts fopts = formatterOpts(dopts,formatLabel,formatLabelEnv);
            FormatInstruction(errHandler, ss, fopts, *inst);

            size_t slen = (size_t)ss.tellp();
            m_disassemble_text = (char *)malloc(1 + slen);
            if (!m_disassemble_text) {
                delete k;
                return IGA_OUT_OF_MEM;
            }
            ss.read(m_disassemble_text, slen);
            m_disassemble_text[slen] = 0;
            if(output)
            {
                *output = m_disassemble_text;
            }
            if (k)
                delete k;
        } catch (const iga::FatalError &) {
            // reportFatal stops hard via an exception; diagnostics will be
            // translated below
        }

        iga_status_t st = translateDiagnostics(errHandler);
        if (errHandler.hasErrors()) {
            return IGA_DECODE_ERROR;
        }
        return st;
    }


    iga_status_t getErrors(
        const iga_diagnostic_t **ds, uint32_t *ds_len) const
    {
        if (!m_errorsValid) {
            *ds = nullptr;
            *ds_len = 0;
            return IGA_INVALID_STATE;
        }
        *ds_len = (uint32_t)m_errors.size();
        *ds = *ds_len ? &m_errors[0] : nullptr;
        return IGA_SUCCESS;
    }


    iga_status_t getWarnings(
        const iga_diagnostic_t **ds, uint32_t *ds_len) const
    {
        if (!m_warningsValid) {
            *ds = nullptr;
            *ds_len = 0;
            return IGA_INVALID_STATE;
        }
        *ds_len = (uint32_t)m_warnings.size();
        *ds = *ds_len ? &m_warnings[0] : nullptr;
        return IGA_SUCCESS;
    }
};


#define RETURN_INVALID_ARG_ON_NULL(X) \
    do { if (!(X)) return IGA_INVALID_ARG; } while (0)

#define CAST_CONTEXT(ID,C) \
    IGAContext *ID = (IGAContext *)(C); \
    if (!ID->valid()) { \
        return IGA_INVALID_OBJECT; \
    }


const char *iga_version_string()
{
    return IGA_VERSION_STRING " (" __DATE__ ")";
}


iga_status_t  iga_context_create(
    const iga_context_options_t *opts,
    iga_context_t *ctx)
{
    RETURN_INVALID_ARG_ON_NULL(ctx);
    RETURN_INVALID_ARG_ON_NULL(opts);


    iga_context_options_t coptsInternal =
        IGA_CONTEXT_OPTIONS_INIT(IGA_GEN_INVALID);
    if (opts->cb > sizeof(iga_context_options_t)) {
        return IGA_VERSION_ERROR;
    }
    MEMCPY(&coptsInternal, opts, opts->cb);

    iga::Platform platf;
    if (!IGAContext::convertPlatform(opts->gen, platf)) {
        return IGA_UNSUPPORTED_PLATFORM;
    }

    IGAContext *ctx_obj = nullptr;
    try {
        ctx_obj = new IGAContext(*opts, platf);
    } catch (std::bad_alloc &) {
        return IGA_OUT_OF_MEM;
    }

    *ctx = (iga_context_t *)ctx_obj;

    return IGA_SUCCESS;
}
iga_status_t  iga_create_context(
    const iga_context_options_t *opts,
    iga_context_t *ctx)
{
    return iga_context_create(opts, ctx);
}

iga_status_t  iga_context_release(iga_context_t ctx)
{
    RETURN_INVALID_ARG_ON_NULL(ctx);

    CAST_CONTEXT(ctx_obj, ctx);
    delete ctx_obj;

    return IGA_SUCCESS;
}
iga_status_t  iga_release_context(iga_context_t ctx)
{
    return iga_context_release(ctx);
}

iga_status_t  iga_context_assemble(
    iga_context_t ctx,
    const iga_assemble_options_t *aopts,
    const char *kernel_text,
    void **output,
    uint32_t *output_size)
{
    RETURN_INVALID_ARG_ON_NULL(ctx);
    RETURN_INVALID_ARG_ON_NULL(aopts);
    RETURN_INVALID_ARG_ON_NULL(kernel_text);
    RETURN_INVALID_ARG_ON_NULL(output);
    RETURN_INVALID_ARG_ON_NULL(output_size);
    // see note at the top of the file about binary compatibility
    if (aopts->cb > sizeof(*aopts)) {
        return IGA_VERSION_ERROR;
    }
    iga_assemble_options_t aoptsInternal = IGA_ASSEMBLE_OPTIONS_INIT();
    MEMCPY(&aoptsInternal, aopts, aopts->cb);

    CAST_CONTEXT(ctx_obj, ctx);
    return ctx_obj->assemble(
        aoptsInternal,
        kernel_text,
        output,
        output_size);
}
IGA_API iga_status_t  iga_assemble(
    iga_context_t ctx,
    const iga_assemble_options_t *opts,
    const char *kernel_text,
    void **output,
    uint32_t *output_size)
{
    return iga_context_assemble(ctx, opts, kernel_text, output, output_size);
}

iga_status_t  iga_context_disassemble(
    iga_context_t ctx,
    const iga_disassemble_options_t *dopts,
    const void *input,
    uint32_t input_size,
    const char * (*fmt_label_name)(int32_t, void *),
    void *fmt_label_ctx,
    char **kernel_text)
{
    RETURN_INVALID_ARG_ON_NULL(ctx);
    RETURN_INVALID_ARG_ON_NULL(dopts);
    if (input == nullptr && input_size != 0)
        return IGA_INVALID_ARG;
    RETURN_INVALID_ARG_ON_NULL(kernel_text);
    if (dopts->cb > sizeof(*dopts)) {
        return IGA_VERSION_ERROR;
    }
    iga_disassemble_options_t doptsInternal = IGA_DISASSEMBLE_OPTIONS_INIT();
    MEMCPY(&doptsInternal, dopts, dopts->cb);

    CAST_CONTEXT(ctx_obj, ctx);
    return ctx_obj->disassemble(
        doptsInternal,
        input,
        input_size,
        fmt_label_name,
        fmt_label_ctx,
        kernel_text);
}
iga_status_t  iga_disassemble(
    iga_context_t ctx,
    const iga_disassemble_options_t *dopts,
    const void *input,
    uint32_t input_size,
    const char * (*fmt_label_name)(int32_t, void *),
    void *fmt_label_ctx,
    char **kernel_text)
{
    return iga_context_disassemble(
        ctx, dopts, input, input_size, fmt_label_name, fmt_label_ctx, kernel_text);
}

iga_status_t  iga_disassemble_instruction(
    iga_context_t ctx,
    const iga_disassemble_options_t *dopts,
    const void *input,
    const char * (*fmt_label_name)(int32_t, void *),
    void *fmt_label_ctx,
    char **kernel_text)
{
    return iga_context_disassemble_instruction(
        ctx, dopts, input, fmt_label_name, fmt_label_ctx, kernel_text);
}
iga_status_t  iga_context_disassemble_instruction(
    iga_context_t ctx,
    const iga_disassemble_options_t *dopts,
    const void *input,
    const char * (*fmt_label_name)(int32_t, void *),
    void *fmt_label_ctx,
    char **kernel_text)
{
    RETURN_INVALID_ARG_ON_NULL(ctx);
    RETURN_INVALID_ARG_ON_NULL(dopts);
    RETURN_INVALID_ARG_ON_NULL(input);
    RETURN_INVALID_ARG_ON_NULL(kernel_text);
    if (dopts->cb > sizeof(*dopts)) {
        return IGA_VERSION_ERROR;
    }
    iga_disassemble_options_t doptsInternal =
        IGA_DISASSEMBLE_OPTIONS_INIT_NUMERIC_LABELS();
    MEMCPY(&doptsInternal, dopts, dopts->cb);

    CAST_CONTEXT(ctx_obj, ctx);
    return ctx_obj->disassembleInstruction(
        doptsInternal,
        input,
        fmt_label_name,
        fmt_label_ctx,
        kernel_text);
}


iga_status_t iga_context_get_errors(
    iga_context_t ctx,
    const iga_diagnostic_t **ds,
    uint32_t *ds_len)
{
    RETURN_INVALID_ARG_ON_NULL(ctx);
    RETURN_INVALID_ARG_ON_NULL(ds);
    RETURN_INVALID_ARG_ON_NULL(ds_len);

    CAST_CONTEXT(ctx_obj, ctx);
    return ctx_obj->getErrors(ds,ds_len);
}
iga_status_t iga_get_errors(
    iga_context_t ctx,
    const iga_diagnostic_t **ds,
    uint32_t *ds_len)
{
    return iga_context_get_errors(ctx, ds, ds_len);
}

iga_status_t iga_context_get_warnings(
    iga_context_t ctx,
    const iga_diagnostic_t **ds,
    uint32_t *ds_len)
{
    RETURN_INVALID_ARG_ON_NULL(ctx);
    RETURN_INVALID_ARG_ON_NULL(ds);
    RETURN_INVALID_ARG_ON_NULL(ds_len);

    CAST_CONTEXT(ctx_obj, ctx);
    return ctx_obj->getWarnings(ds,ds_len);
}
iga_status_t iga_get_warnings(
    iga_context_t ctx,
    const iga_diagnostic_t **ds,
    uint32_t *ds_len)
{
    return iga_context_get_warnings(ctx, ds, ds_len);
}

iga_status_t iga_diagnostic_get_message(
    const iga_diagnostic_t *d,
    const char **message)
{
    RETURN_INVALID_ARG_ON_NULL(d);
    RETURN_INVALID_ARG_ON_NULL(message);

    *message = d->message;

    return IGA_SUCCESS;
}


iga_status_t iga_diagnostic_get_offset(
    const iga_diagnostic_t *d,
    uint32_t *offset)
{
    RETURN_INVALID_ARG_ON_NULL(d);
    RETURN_INVALID_ARG_ON_NULL(offset);

    *offset = d->offset;

    return IGA_SUCCESS;
}

// static const uint32_t IGA_DIAGNOSTIC_BINARY_MASK = 0x80000000;
// #define IGA_DIAGNOSTIC_IS_BINARY(D) \
//    ((D)->column & IGA_DIAGNOSTIC_BINARY_MASK)
// for now we use line == col == 0 to mean binary
#define IGA_DIAGNOSTIC_IS_BINARY(D) \
    ((D)->column == 0 && (D)->line == 0)


iga_status_t iga_diagnostic_get_type(
    const iga_diagnostic_t *d,
    iga_diagnostic_type_t *dt)
{
    RETURN_INVALID_ARG_ON_NULL(d);
    RETURN_INVALID_ARG_ON_NULL(dt);

    *dt = IGA_DIAGNOSTIC_IS_BINARY(d) ?
        IGA_DIAGNOSTIC_BINARY :
        IGA_DIAGNOSTIC_TEXT;

    return IGA_SUCCESS;
}


iga_status_t iga_diagnostic_get_text_line(
    const iga_diagnostic_t *d,
    uint32_t *line)
{
    RETURN_INVALID_ARG_ON_NULL(d);
    RETURN_INVALID_ARG_ON_NULL(line);

    if (IGA_DIAGNOSTIC_IS_BINARY(d))
        return IGA_INVALID_ARG;

    *line = d->line;

    return IGA_SUCCESS;
}


iga_status_t iga_diagnostic_get_text_column(
    const iga_diagnostic_t *d,
    uint32_t *col)
{
    RETURN_INVALID_ARG_ON_NULL(d);
    RETURN_INVALID_ARG_ON_NULL(col);

    if (IGA_DIAGNOSTIC_IS_BINARY(d))
        return IGA_INVALID_ARG;
    *col = d->column; // & ~IGA_DIAGNOSTIC_BINARY_MASK;

    return IGA_SUCCESS;
}


iga_status_t iga_diagnostic_get_text_extent(
    const iga_diagnostic_t *d,
    uint32_t *ext)
{
    RETURN_INVALID_ARG_ON_NULL(d);
    RETURN_INVALID_ARG_ON_NULL(ext);

    if (IGA_DIAGNOSTIC_IS_BINARY(d))
        return IGA_INVALID_ARG;
    *ext = d->extent;

    return IGA_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//                         IGA OPSPEC API                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define IGA_COPY_OUT(DST, DST_LEN_PTR, SRC, SRC_LEN) \
    do { \
        if ((DST) != nullptr) { \
            size_t _CPLEN = *(DST_LEN_PTR) < (SRC_LEN) ? *(DST_LEN_PTR) : (SRC_LEN); \
            MEMCPY((DST), (SRC), _CPLEN * sizeof(*(DST))); \
        } \
        *(DST_LEN_PTR) = (SRC_LEN); \
    } while (0)

// same as above, but for a string
#define IGA_COPY_OUT_STR(DST, DST_LEN_PTR, SRC) \
    do { \
        size_t SRC_LEN = strlen(SRC) + 1; \
        if ((DST) != nullptr) { \
            size_t _CPLEN = *(DST_LEN_PTR) < (SRC_LEN) ? *(DST_LEN_PTR) : (SRC_LEN); \
            MEMCPY((DST), (SRC), _CPLEN * sizeof(*(DST))); \
            DST[_CPLEN - 1] = 0; \
        } \
        *(DST_LEN_PTR) = (SRC_LEN); \
    } while (0)


// In the opaque pointer returned, we flip some top bits.
// Should the user clobber their stack or accidentially send this pointer
// of to be written, this will hopefully trap (assuming we are in user space)
// immediately rather than corrupting our internal data structures.
// x86 takes either 0x80000000 or 0xC000000 up to 0xFFFFFFFF as system space
//
// TODO: another method would be to store this as a relative address...
// relative to something near the instspec....
// That would translate to fairly small integer that should be in the
// no access range (near 0)
static iga_opspec_t opspec_to_handle(const OpSpec *i) {
    const uintptr_t TOP_BIT = (sizeof(void *) == 4) ?
        0xC0000000 : 0x8000000000000000;
    return (iga_opspec_t)((uintptr_t)i ^ TOP_BIT);
}
static const OpSpec *opspec_from_handle(iga_opspec_t os) {
    const uintptr_t TOP_BIT = (sizeof(void *) == 4) ?
        0xC0000000 : 0x8000000000000000;
    return (const OpSpec *)((uintptr_t)os ^ TOP_BIT);
}


iga_status_t iga_opspec_enumerate(
    iga_gen_t gen,
    iga_opspec_t *ops_arr,
    size_t *ops_arr_len)
{
    RETURN_INVALID_ARG_ON_NULL(ops_arr_len);

    iga::Platform plat;
    if (!IGAContext::convertPlatform(gen, plat)) {
        return IGA_INVALID_ARG;
    }
    const Model *m = Model::LookupModel(plat);
    if (!m) {
        return IGA_INVALID_ARG;
    }
    std::vector<iga_opspec_t> ops;
    ops.reserve(128);
    for (const OpSpec *os : m->ops()) {
        ops.emplace_back(opspec_to_handle(os));
    }

    IGA_COPY_OUT(ops_arr, ops_arr_len, ops.data(), ops.size());
    return IGA_SUCCESS;
}


iga_status_t iga_opspec_from_op(
    iga_gen_t gen,
    uint32_t opcode,
    iga_opspec_t *op)
{
    RETURN_INVALID_ARG_ON_NULL(op);
    iga::Platform plat;
    if (!IGAContext::convertPlatform(gen, plat)) {
        return IGA_INVALID_ARG;
    }
    const Model *m = Model::LookupModel(plat);
    if (!m) {
        return IGA_INVALID_ARG;
    }
    const OpSpec *os = &m->lookupOpSpec(static_cast<iga::Op>(opcode));
    *op = opspec_to_handle(os);
    return IGA_SUCCESS;
}


iga_status_t iga_opspec_mnemonic(
    iga_opspec_t op,
    char *mnemonic,
    size_t *mnemonic_len)
{
    RETURN_INVALID_ARG_ON_NULL(op);
    RETURN_INVALID_ARG_ON_NULL(mnemonic_len);
    IGA_COPY_OUT_STR(mnemonic, mnemonic_len, opspec_from_handle(op)->mnemonic);
    return IGA_SUCCESS;
}


iga_status_t iga_opspec_name(
    iga_opspec_t op,
    char *name,
    size_t *name_len)
{
    RETURN_INVALID_ARG_ON_NULL(op);
    RETURN_INVALID_ARG_ON_NULL(name_len);
    IGA_COPY_OUT_STR(name, name_len, opspec_from_handle(op)->name);
    return IGA_SUCCESS;
}


iga_status_t iga_opspec_description(
    iga_opspec_t op,
    char *desc,
    size_t *desc_len)
{
    RETURN_INVALID_ARG_ON_NULL(op);
    RETURN_INVALID_ARG_ON_NULL(desc_len);
    IGA_COPY_OUT_STR(desc, desc_len, opspec_from_handle(op)->description);
    return IGA_SUCCESS;
}

iga_status_t iga_opspec_op(
    iga_opspec_t op,
    uint32_t *opcode)
{
    RETURN_INVALID_ARG_ON_NULL(op);
    RETURN_INVALID_ARG_ON_NULL(opcode);
    *opcode = static_cast<uint32_t>(opspec_from_handle(op)->op);
    return IGA_SUCCESS;
}


iga_status_t iga_opspec_op_encoding(
    iga_opspec_t op,
    uint32_t *opcode)
{
    RETURN_INVALID_ARG_ON_NULL(op);
    RETURN_INVALID_ARG_ON_NULL(opcode);
    *opcode = static_cast<uint32_t>(opspec_from_handle(op)->code);
    return IGA_SUCCESS;
}


iga_status_t  iga_get_interface(iga_functions_t *funcs)
{
    RETURN_INVALID_ARG_ON_NULL(funcs);

    funcs->iga_version_string = iga_version_string;
    funcs->iga_status_to_string = iga_status_to_string;

    funcs->iga_context_create = &iga_context_create;
    funcs->iga_context_release = &iga_context_release;
    funcs->iga_context_assemble = &iga_context_assemble;
    funcs->iga_context_disassemble = &iga_context_disassemble;
    funcs->iga_context_disassemble_instruction =
        &iga_context_disassemble_instruction;
    funcs->iga_context_get_errors = &iga_context_get_errors;
    funcs->iga_context_get_warnings = &iga_context_get_warnings;

    funcs->iga_diagnostic_get_message = &iga_diagnostic_get_message;
    funcs->iga_diagnostic_get_offset = &iga_diagnostic_get_offset;
    funcs->iga_diagnostic_get_type = &iga_diagnostic_get_type;
    funcs->iga_diagnostic_get_text_line = &iga_diagnostic_get_text_line;
    funcs->iga_diagnostic_get_text_column = &iga_diagnostic_get_text_column;
    funcs->iga_diagnostic_get_text_extent = &iga_diagnostic_get_text_extent;

    funcs->iga_opspec_enumerate = &iga_opspec_enumerate;
    funcs->iga_opspec_mnemonic = &iga_opspec_mnemonic;
    funcs->iga_opspec_name = &iga_opspec_name;
    funcs->iga_opspec_description = &iga_opspec_description;
    funcs->iga_opspec_op = &iga_opspec_op;

    return IGA_SUCCESS;
}



///////////////////////////////////////////////////////////////////////////////
//
// KernelView binary interface
//
///////////////////////////////////////////////////////////////////////////////

class KernelViewImpl {
private:
    KernelViewImpl(const KernelViewImpl& k)
        : m_model(*iga::Model::LookupModel(k.m_model.platform)) { }
    KernelViewImpl& operator =(const KernelViewImpl &k) { return *this; }
public:
    const iga::Model                       &m_model;
    iga::Kernel                            *m_kernel;
    iga::ErrorHandler                       m_errHandler;
    std::map<uint32_t,iga::Instruction*>    m_instsByPc;
    std::map<uint32_t, Block*>              m_blockToPcMap;

    KernelViewImpl(
        iga::Platform platf,
        const void *bytes,
        size_t bytesLength)
        : m_model(*iga::Model::LookupModel(platf))
        , m_kernel(nullptr)

    {
        iga::Decoder decoder(*Model::LookupModel(platf), m_errHandler);
        m_kernel = decoder.decodeKernelBlocks(bytes, bytesLength);

        int32_t pc = 0;
        for (iga::Block *b : m_kernel->getBlockList()) {
            m_blockToPcMap[b->getOffset()] = b;
            for (iga::Instruction *inst : b->getInstList()) {
                pc = inst->getPC();
                m_instsByPc[pc] = inst;
            }
        }
    }

    ~KernelViewImpl() {
        if (m_kernel) {
            delete m_kernel;
        }
    }


    const iga::Instruction *getInstruction(int32_t pc) const {
        auto itr = m_instsByPc.find(pc);
        if (itr == m_instsByPc.end()) {
            return nullptr;
        }
        return itr->second;
    }


    const Block *getBlock(int32_t pc) const {
        auto itr = m_blockToPcMap.find(pc);
        if (itr == m_blockToPcMap.end())
        {
            return nullptr;
        }
        return itr->second;
    }
};


kv_t *kv_create(
    iga_gen_t gen_platf,
    const void *bytes,
    size_t bytes_len,
    iga_status_t *status,
    char *errbuf,
    size_t errbuf_cap)
{
    if (errbuf && errbuf_cap > 0)
        *errbuf = 0;

    iga::Platform platf;
    if (!IGAContext::convertPlatform(gen_platf, platf)) {
        if (status)
            *status = IGA_INVALID_ARG;
        if (errbuf) {
            formatTo(errbuf, errbuf_cap, "%s", "iga api: invalid platform");
        }
        return nullptr;
    }

    KernelViewImpl *kvImpl = nullptr;
    try {
        kvImpl = new (std::nothrow)KernelViewImpl(
            platf, bytes, bytes_len);
        if (!kvImpl) {
            if (errbuf)
                formatTo(errbuf, errbuf_cap, "%s", "failed to allocate");
            if (status)
                *status = IGA_OUT_OF_MEM;
            return nullptr;
        }
    } catch (const iga::FatalError &fe) {
        if (errbuf) {
            const char *msg = fe.what();
            formatTo(errbuf, errbuf_cap, "decoding error: %s", msg);
        }
        if (status)
            *status = IGA_DECODE_ERROR;
        if (kvImpl)
        {
            delete kvImpl;
        }
        return nullptr;
    }

    // copy out the errors and warnings
    if (kvImpl) {
        std::stringstream ss;
        if (kvImpl->m_errHandler.hasErrors()) {
            for (auto d : kvImpl->m_errHandler.getErrors()) {
                ss << "ERROR: " << d.at.offset << ". " << d.message << "\n";
            }
        }
        for (auto d : kvImpl->m_errHandler.getWarnings()) {
            ss << "WARNING: " << d.at.offset << ". " << d.message << "\n";
        }
        (void)copyOut(errbuf, errbuf_cap, ss);
    }

    if (kvImpl->m_errHandler.hasErrors()) {
/*
        if (kvImpl) {
            // free the KernelViewImpl since we are failing
            delete kvImpl;
            kvImpl = nullptr;
        }
*/
        // copy out the error status
        if (status)
            *status = IGA_DECODE_ERROR;
    } else {
        // copy out the error status
        if (status)
            *status = IGA_SUCCESS;
    }

    return (kv_t *)kvImpl;
}


void kv_delete(kv_t *kv)
{
    if (kv)
        delete ((KernelViewImpl *)kv);
}


int32_t kv_get_inst_size(const kv_t *kv, int32_t pc)
{
    if (!kv)
        return 0;

    const iga::Instruction *inst = ((KernelViewImpl *)kv)->getInstruction(pc);
    if (!inst) {
        return 0;
    }
    return inst->hasInstOpt(iga::InstOpt::COMPACTED) ? 8 : 16;
}


uint32_t kv_get_inst_targets(
    const kv_t *kv,
    int32_t pc,
    int32_t *pcs)
{
    if (!kv)
        return 0;

    const Instruction *inst = ((KernelViewImpl *)kv)->getInstruction(pc);
    if (!inst || inst->getOp() == Op::ILLEGAL) {
        return 0;
    }

    if (!inst->getOpSpec().isBranching()) {
        return 0;
    }

    uint32_t nSrcs = 0;

    if (inst->getSourceCount() > 0) {
        const Operand &op = inst->getSource(SourceIndex::SRC0);
        if (op.getKind() == Operand::Kind::LABEL) {
            if (pcs)
                pcs[nSrcs] = inst->getJIP()->getOffset();
            nSrcs++;
        }
    }

    if (inst->getSourceCount() > 1) {
        const Operand &op = inst->getSource(SourceIndex::SRC1);
        if (op.getKind() == Operand::Kind::LABEL) {
            if (pcs)
                pcs[nSrcs] = inst->getUIP()->getOffset();
            nSrcs++;
        }
    }

    return nSrcs;
}


size_t kv_get_inst_syntax(
    const kv_t *kv,
    int32_t pc,
    char *sbuf,
    size_t sbuf_cap,
    const char *(*labeler)(int32_t, void *),
    void *labeler_env)
{
    if (!kv) {
        if (sbuf && sbuf_cap > 0)
            *sbuf = 0;
        return 0;
    }

    KernelViewImpl *kvImpl = (KernelViewImpl *)kv;
    const Instruction *inst = kvImpl->getInstruction(pc);
    if (!inst) {
        if (sbuf && sbuf_cap > 0)
            *sbuf = 0;
        return 0;
    }

    std::stringstream ss;
    FormatOpts fopts(kvImpl->m_model.platform, labeler, labeler_env);
    FormatInstruction(
        kvImpl->m_errHandler,
        ss,
        fopts,
        *inst);

    return copyOut(sbuf, sbuf_cap, ss);
}


size_t kv_get_default_label_name(
    int32_t pc,
    char *sbuf,
    size_t sbuf_cap)
{
    if (!sbuf || sbuf_cap == 0)
    {
        return 0;
    }
    std::stringstream strm;
    GetDefaultLabelName(strm, pc);
    return copyOut(sbuf, sbuf_cap, strm);
}


uint32_t kv_is_inst_target(const kv_t *kv, int32_t pc)
{
    if (!kv)
        return 0;
    return ((KernelViewImpl *)kv)->getBlock(pc) == nullptr ? 0 : 1;
}


int32_t kv_get_opgroup(const kv_t *kv, int32_t pc)
{
    if (!kv)
        return (int32_t)kv_opgroup_t::KV_OPGROUP_INVALID;

    KernelViewImpl *kvImpl = (KernelViewImpl *)kv;
    const Instruction *inst = kvImpl->getInstruction(pc);
    if (!inst) {
        return (int32_t)kv_opgroup_t::KV_OPGROUP_INVALID;
    }
    switch (inst->getOp()) {
    case Op::IF:    return (int32_t)kv_opgroup_t::KV_OPGROUP_IF;
    case Op::ENDIF: return (int32_t)kv_opgroup_t::KV_OPGROUP_ENDIF;
    case Op::ELSE:  return (int32_t)kv_opgroup_t::KV_OPGROUP_ELSE;
    case Op::WHILE: return (int32_t)kv_opgroup_t::KV_OPGROUP_WHILE;
    case Op::SEND:
        if (inst->hasInstOpt(InstOpt::EOT)) {
            return (int32_t)kv_opgroup_t::KV_OPGROUP_SEND_EOT;
        }
    default: return (int32_t)kv_opgroup_t::KV_OPGROUP_OTHER;
    }
}


uint32_t kv_get_send_descs(
    const kv_t *kv, int32_t pc, uint32_t *ex_desc, uint32_t *desc)
{
    if (!kv || !ex_desc || !desc)
        return 0;
    const Instruction *inst = ((KernelViewImpl *)kv)->getInstruction(pc);
    if (!inst || !inst->getOpSpec().isSendOrSendsFamily() || inst->getOp() == Op::ILLEGAL) {
        *ex_desc = *desc = KV_INVALID_SEND_DESC;
        return 0;
    }

    uint32_t n = 0;
    if (inst->getExtMsgDescriptor().type == SendDescArg::IMM) {
        n++;
        *ex_desc = inst->getExtMsgDescriptor().imm;
    } else {
        *ex_desc = KV_INVALID_SEND_DESC;
    }
    if (inst->getMsgDescriptor().type == SendDescArg::IMM) {
        n++;
        *desc = inst->getMsgDescriptor().imm;
    } else {
        *desc = KV_INVALID_SEND_DESC;
    }
    return n;
}

/******************** KernelView analysis APIs ********************************/
const Instruction *getInstruction(const kv_t *kv, int32_t pc)
{
    if (!kv) {
        return nullptr;
    }
    const Instruction *inst = ((KernelViewImpl *)kv)->getInstruction(pc);
    return inst;
}

int32_t kv_get_message_type(const kv_t *kv, int32_t pc)
{
    if (!kv) {
        return static_cast<int32_t>(SFMessageType::INVALID);
    }

    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || !inst->getOpSpec().isSendOrSendsFamily()) {
        return static_cast<int32_t>(SFMessageType::INVALID);
    }

    auto exDesc = inst->getExtMsgDescriptor();
    auto desc = inst->getMsgDescriptor();

    if (exDesc.type != SendDescArg::IMM || desc.type != SendDescArg::IMM)
        return static_cast<int32_t>(SFMessageType::NON_IMM);

    Platform p = ((KernelViewImpl *)kv)->m_model.platform;

    SFMessageType msgType = getMessageType(p, inst->getOpSpec(), exDesc.imm, desc.imm);

    return static_cast<int32_t>(msgType);
}

int32_t kv_get_message_sfid(const kv_t *kv, int32_t pc)
{
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst) {
        return static_cast<int32_t>(SFID::ERR);
    }

    auto exDesc = inst->getExtMsgDescriptor();
    auto desc = inst->getMsgDescriptor();

    if (exDesc.type != SendDescArg::IMM || desc.type != SendDescArg::IMM)
        return static_cast<int32_t>(SFID::NON_IMM);

    Platform p = ((KernelViewImpl *)kv)->m_model.platform;

    SFID sfid = getSFID(p, inst->getOpSpec(), exDesc.imm, desc.imm);

    return static_cast<int32_t>(sfid);
}

uint32_t kv_get_message_len(
    const kv_t *kv, int32_t pc, uint32_t* mLen, uint32_t* emLen, uint32_t* rLen)
{
    if (!mLen || !emLen || !rLen)
        return 0;

    const Instruction *inst = getInstruction(kv, pc);
    if (!inst) {
        return 0;
    }

    auto exDesc = inst->getExtMsgDescriptor();
    auto desc = inst->getMsgDescriptor();

    // TODO: do I check for immediates?
    //if (exDesc.type != SendDescArg::IMM || desc.type != SendDescArg::IMM)
    //    return 0;

    Platform p = ((KernelViewImpl *)kv)->m_model.platform;

    return getMessageLengths(p, inst->getOpSpec(), exDesc.imm, desc.imm, mLen, emLen, rLen);
}

uint32_t kv_get_execution_size(const kv_t *kv, int32_t pc)
{
    if (!kv) {
        return static_cast<uint32_t>(ExecSize::INVALID);
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst) {
        return static_cast<uint32_t>(ExecSize::INVALID);
    }

    return static_cast<uint32_t>(inst->getExecSize());
}


int32_t kv_get_number_sources(const kv_t *kv, int32_t pc)
{
    if (!kv) {
        return -1;
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst) {
        return -1;
    }

    return inst->getSourceCount();
}

uint32_t kv_get_opcode(const kv_t *kv, int32_t pc)
{
    if (!kv) {
        return static_cast<uint32_t>(Op::INVALID);
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst) {
        return static_cast<uint32_t>(Op::INVALID);
    }
    return static_cast<uint32_t>(inst->getOpSpec().op);
    //    return static_cast<uint32_t>(inst->getOpSpec().code);
}

int32_t kv_get_has_destination(const kv_t *kv, int32_t pc)
{
    if (!kv) {
        return -1;
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || inst->getOp() == Op::ILLEGAL) {
        return -1;
    }

    const OpSpec& instSpec = inst->getOpSpec();
    return instSpec.supportsDestination() ? 1 : 0;
}
/*
* This function returns destination Register row
*/
int32_t kv_get_destination_register(const kv_t *kv, int32_t pc)
{
    if (!kv) {
        return -1;
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst) {
        return -1;
    }
    if (!inst->getOpSpec().supportsDestination()) {
        return -1;
    }
    const Operand &dst = inst->getDestination();
    if (dst.getKind() != Operand::Kind::DIRECT) {
        return -1;
    }
    return dst.getDirRegRef().regNum;
}

/*
* This function returns destination subRegister
*/
int32_t kv_get_destination_sub_register(const kv_t *kv, int32_t pc)
{
    if (!kv) {
        return -1;
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || inst->getOp() == Op::ILLEGAL) {
        return -1;
    }

    const OpSpec& instSpec = inst->getOpSpec();
    if (!instSpec.supportsDestination()) {
        return -1;
    }
    const Operand &dst = inst->getDestination();
    if (dst.getKind() != Operand::Kind::DIRECT) {
        return -1;
    }
    return dst.getDirRegRef().subRegNum;
}

/*
* This function returns destination data type
* i.e. F, HF, INT, etc
*/
uint32_t kv_get_destination_data_type(const kv_t *kv, int32_t pc)
{
    if (!kv) {
        return static_cast<uint32_t>(Type::INVALID);
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || inst->getOp() == Op::ILLEGAL) {
        return static_cast<uint32_t>(Type::INVALID);
    }
    return (int32_t)inst->getDestination().getType();
}

/*
* This function returns destination register type
* i.e. GRF, various ARF registers
*/
uint32_t kv_get_destination_register_type(const kv_t *kv, int32_t pc)
{
    if (!kv) {
        return static_cast<uint32_t>(RegName::INVALID);
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || inst->getOp() == Op::ILLEGAL) {
        return static_cast<uint32_t>(RegName::INVALID);
    }
    return static_cast<uint32_t>(inst->getDestination().getDirRegName());
}

/*
* This function returns destination register KIND
* DIRECT, INDIRECT, IMM, etc
*/
uint32_t kv_get_destination_register_kind(const kv_t *kv, int32_t pc)
{
    if (!kv) {
        return static_cast<uint32_t>(Kind::INVALID);
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || inst->getOp() == Op::ILLEGAL) {
        return static_cast<uint32_t>(Kind::INVALID);
    }
    return (uint32_t)inst->getDestination().getKind();
}

/*
 * This function returns source register line number for a given source.
 */
int32_t kv_get_source_register(const kv_t *kv, int32_t pc, uint32_t sourceNumber)
{
    if (!kv) {
        return -1;
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || sourceNumber >= inst->getSourceCount()) {
        return -1;
    }
    const auto &src = inst->getSource((size_t)sourceNumber);
    if (src.getKind() != Operand::Kind::DIRECT) {
        return -1;
    }
    return (int32_t)src.getDirRegRef().regNum;
}

/*
 * This function returns source subRegister for a given source.
 */
int32_t kv_get_source_sub_register(const kv_t *kv, int32_t pc, uint32_t sourceNumber)
{
    if (!kv) {
        return -1;
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || inst->getOp() == Op::ILLEGAL ||
        sourceNumber >= inst->getSourceCount())
    {
        return -1;
    }
    const auto &src = inst->getSource((size_t)sourceNumber);
    if (src.getKind() != Operand::Kind::DIRECT) {
        return -1;
    }
    return (int32_t)src.getDirRegRef().subRegNum;
}

/*
 * This function returns source data type for a given source
 * i.e. F, HF, INT, etc
 */
uint32_t kv_get_source_data_type(const kv_t *kv, int32_t pc, uint32_t sourceNumber)
{
    if (!kv) {
        return static_cast<uint32_t>(Type::INVALID);
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || sourceNumber >= inst->getSourceCount()) {
        return static_cast<uint32_t>(Type::INVALID);
    }
    const auto &src = inst->getSource((size_t)sourceNumber);
    if (src.getKind() == Operand::Kind::INVALID) {
        return static_cast<uint32_t>(Type::INVALID);
    }
    return static_cast<uint32_t>(src.getType());
}

/*
 * This function returns source register type for a given source.
 * i.e. GRF, various ARF registers
 */
uint32_t kv_get_source_register_type(const kv_t *kv, int32_t pc, uint32_t sourceNumber)
{
    if (!kv) {
        return static_cast<uint32_t>(RegName::INVALID);
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || sourceNumber >= inst->getSourceCount()) {
        return static_cast<uint32_t>(RegName::INVALID);
    }
    const auto &src = inst->getSource((size_t)sourceNumber);
    if (src.getKind() != Operand::Kind::INVALID) {
        return -1;
    }
    return static_cast<uint32_t>(src.getDirRegName());
}

/*
 * This function returns source register KIND for a given source
 * DIRECT, INDIRECT, IMM, INDIR etc
 */
uint32_t kv_get_source_register_kind(const kv_t *kv, int32_t pc, uint32_t sourceNumber)
{
    if (!kv) {
        return static_cast<uint32_t>(Kind::INVALID);
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || sourceNumber >= inst->getSourceCount()) {
        return static_cast<uint32_t>(Kind::INVALID);
    }
    return static_cast<uint32_t>(inst->getSource((uint8_t)sourceNumber).getKind());
}

/*
 * Returns whether source is a vector.
 */
int32_t kv_is_source_vector(const kv_t *kv, int32_t pc, uint32_t sourceNumber)
{
    if (!kv) {
        return -1;
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || sourceNumber >= inst->getSourceCount()) {
        return -1;
    }

    auto src = inst->getSource((uint8_t)sourceNumber);
    if (src.getKind() != Operand::Kind::DIRECT ||
        src.getKind() != Operand::Kind::INDIRECT)
    {
        return -1;
    }

    auto rgn = src.getRegion();
    if (rgn == Region::SRC010 || rgn == Region::SRC0X0 || rgn == Region::SRCXX0) {
        return 0;
    }

    return 1;
}

uint32_t kv_get_channel_offset(const kv_t *kv, int32_t pc)
{
    if (!kv) {
        return static_cast<uint32_t>(ChannelOffset::M0);
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || inst->getOp() == Op::ILLEGAL) {
        return static_cast<uint32_t>(ChannelOffset::M0);
    }
    return (uint32_t)inst->getChannelOffset();
}

uint32_t kv_get_mask_control(const kv_t *kv, int32_t pc)
{
    if (!kv) {
        return static_cast<uint32_t>(MaskCtrl::NORMAL);
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || inst->getOp() == Op::ILLEGAL) {
        return static_cast<uint32_t>(MaskCtrl::NORMAL);
    }
    return (uint32_t)inst->getMaskCtrl();
}

/*
 * This function returns 0 if instruction's destination operand horizontal stride (DstRgnHz) is succesfully determined.
 * Otherwise returns -1.
 * DstRgnHz is returned by *hz parameter, as numeric numeric value (e.g. 1,2,4).
 */
int32_t kv_get_destination_region(const kv_t *kv, int32_t pc, uint32_t *hz)
{
    uint32_t DstRgnHz = static_cast<uint32_t>(Region::Horz::HZ_INVALID);
    if (!kv) {
        *hz = DstRgnHz;
        return -1;
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst || !inst->getOpSpec().supportsDestination()) {
        *hz = DstRgnHz;
        return -1;
    }
    const Operand &dst = inst->getDestination();
    DstRgnHz = static_cast<uint32_t>(dst.getRegion().getHz());
    *hz = DstRgnHz;
    return 0;
}

/*
 * This function returns 0 if any of instruction's src operand region components (Src RgnVt, RgnWi, RgnHz) are succesfully determined.
 * Otherwise returns -1.
 * Vt, Wi and Hz are returned by *vt, *wi and *hz parameter, as numeric numeric values (e.g. 1,2,4).
 */
int32_t kv_get_source_region(const kv_t *kv, int32_t pc, uint32_t src_op, uint32_t *vt, uint32_t *wi, uint32_t *hz)
{
    uint32_t SrcRgnVt = static_cast<uint32_t>(Region::Vert::VT_INVALID);
    uint32_t SrcRgnWi = static_cast<uint32_t>(Region::Width::WI_INVALID);
    uint32_t SrcRgnHz = static_cast<uint32_t>(Region::Horz::HZ_INVALID);
    const uint32_t c_maxSrcOperands = 3;
    if (!kv && src_op < c_maxSrcOperands) {
        *vt = SrcRgnVt;
        *wi = SrcRgnWi;
        *hz = SrcRgnHz;
        return -1;
    }
    const Instruction *inst = getInstruction(kv, pc);
    if (!inst ||
        !(inst->getOpSpec().getSourceCount() > src_op)) {
        *vt = SrcRgnVt;
        *wi = SrcRgnWi;
        *hz = SrcRgnHz;
        return -1;
    }
    const Operand &src = inst->getSource(src_op);
    if(!(src.getKind() == Operand::Kind::DIRECT) ||
       !(src.getDirRegName() == RegName::GRF_R)
       )
    {
        *vt = SrcRgnVt;
        *wi = SrcRgnWi;
        *hz = SrcRgnHz;
        return -1;
    }
    SrcRgnVt = static_cast<uint32_t>(src.getRegion().getVt());
    SrcRgnWi = static_cast<uint32_t>(src.getRegion().getWi());
    SrcRgnHz = static_cast<uint32_t>(src.getRegion().getHz());
    *vt = SrcRgnVt;
    *wi = SrcRgnWi;
    *hz = SrcRgnHz;
    return 0;
}