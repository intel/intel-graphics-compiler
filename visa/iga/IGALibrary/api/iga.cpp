/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "iga.h"
#include "igad.h"
#include "iga.hpp"
// IGA headers
#include "../Backend/GED/Interface.hpp"
#include "../Backend/Native/Interface.hpp"
#include "../ErrorHandler.hpp"
#include "../Frontend/Formatter.hpp"
#include "../Frontend/KernelParser.hpp"
#include "../IR/DUAnalysis.hpp"
#include "../IR/Checker/IRChecker.hpp"
#include "../Models/Models.hpp"
#include "../strings.hpp"
#include "../version.hpp"
#include "common/secure_string.h"
#include "common/secure_mem.h"

// external dependencies
#include <algorithm>
#include <cstring>
#include <map>
#include <ostream>
#include <sstream>
#include <unordered_map>
#include <vector>


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


const char *iga_status_to_string(iga_status_t st) {
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
    case IGA_DIFF_FAILURE:         return "differences encountered";
    default:                       return "invalid error code";
    }
}

// suppress -Wmissing-declarations
iga::Platform ToPlatform(iga_gen_t gen);

// Conversion to an internal platform
// we could just re-interpret the bits but this checks for garbage
// (validates the enum)
// This is not static so that it can be used by other compilation units.
iga::Platform ToPlatform(iga_gen_t gen)
{
    // for binary compatibilty we accept the enum values from pre Xe-renaming
    // platforms (e.g IGA_GEN12p1 is GEN_VER(12,1), but we now name XE_VER(1,0)
    switch (gen) {
        case iga_gen_t::IGA_GEN12p1:  gen = iga_gen_t::IGA_XE;     break;
        default:
            break;
    }

    const auto *m = iga::Model::LookupModel(iga::Platform(gen));
    return m ? m->platform : iga::Platform::INVALID;
}

iga_status_t iga_platforms_list(
    size_t gens_length_bytes,
    iga_gen_t *gens,
    size_t *gens_length_bytes_required)
{
    if (gens_length_bytes != 0 && gens == nullptr)
        return IGA_INVALID_ARG;

    const size_t MAX_SPACE_NEEDED = ALL_MODELS_LEN*sizeof(iga_gen_t);
    if (gens_length_bytes_required)
        *gens_length_bytes_required = MAX_SPACE_NEEDED;
    if (gens) {
        for (size_t i = 0;
            i < std::min(gens_length_bytes,MAX_SPACE_NEEDED)/sizeof(iga_gen_t);
            i++)
        {
            gens[i] = static_cast<iga_gen_t>(ALL_MODELS[i]->platform);
        }
    }
    return IGA_SUCCESS;
}

// We run into a minor annoyance here.  We must return IGA memory that
// that never gets cleaned up by the user.  Thus we use module global
// memory.  On DLL attach this initializer will run and on DLL detach it
// should be cleaned up.
//
//
// c.f. iga_platform_names and iga_platform_symbol_suffix
struct PlatformNameMap {
    std::unordered_map<iga::Platform,std::vector<std::string>> names;
    std::unordered_map<iga::Platform,std::string> exts;

    PlatformNameMap() {
        for (size_t i = 0; i < ALL_MODELS_LEN; i++) {
            const Model &me = *ALL_MODELS[i];
            std::vector<std::string> nmlist;
            for (const auto &mn : me.names)
            {
                std::string str = mn.str();
                if (str.empty())
                    break;
                nmlist.push_back(str);
            }
            names[me.platform] = nmlist;
            exts[me.platform] = me.extension.str();
        }
    }
};
//
static const PlatformNameMap s_names;

iga_status_t iga_platform_symbol_suffix(
    iga_gen_t gen,
    const char **suffix)
{
    if (suffix == nullptr)
        return IGA_INVALID_ARG;
    auto itr = s_names.exts.find(ToPlatform(gen));
    if (itr == s_names.exts.end()) {
        *suffix = nullptr;
        return IGA_INVALID_ARG;
    }
    *suffix = itr->second.c_str();

    return IGA_SUCCESS;
}

iga_status_t iga_platform_names(
    iga_gen_t gen,
    size_t names_bytes,
    const char **names,
    size_t *names_bytes_needed)
{
    if (names_bytes != 0 && names == nullptr)
        return IGA_INVALID_ARG;

    auto itr = s_names.names.find(ToPlatform(gen));
    if (itr == s_names.names.end()) {
        return IGA_INVALID_ARG;
    }
    const auto &pltNames = itr->second;
    if (names_bytes_needed)
        *names_bytes_needed = pltNames.size()*sizeof(const char*);
    const int n_copy = std::min<int>(
        (int)pltNames.size(), (int)names_bytes/sizeof(const char *));
    for (int ni = 0; ni < n_copy; ni++)
        names[ni] = pltNames[ni].c_str();

    return IGA_SUCCESS;
}

class IGAContext {
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
                iga_diagnostic_t temp = {
                    d.at.line, d.at.col, (uint32_t)d.at.offset, d.at.extent, str};
                api_ds.push_back(temp);
            } else {
                // pc (offset is in bytes)
                iga_diagnostic_t temp = {0, 0, (uint32_t)d.at.offset, d.at.extent, str};
                api_ds.push_back(temp);
            }
        }
        return IGA_SUCCESS;
    }

    iga_status_t translateDiagnostics(const iga::ErrorHandler &eh) {
        clearDiagnostics(m_errors);
        clearDiagnostics(m_warnings);
        m_warningsValid = m_errorsValid = false;

        iga_status_t s1 = translateDiagnosticList(eh.getErrors(), m_errors);
        if (s1 != IGA_SUCCESS) {
            clearDiagnostics(m_errors);
            return s1;
        }

        iga_status_t s2 = translateDiagnosticList(eh.getWarnings(), m_warnings);
        if (s2 != IGA_SUCCESS) {
            clearDiagnostics(m_warnings);
            clearDiagnostics(m_errors);
            return s2;
        }

        m_warningsValid = m_errorsValid = true;
        return IGA_SUCCESS;
    }

    static const Model &convertPlatform(iga_gen_t gen) {
        const Model *m = Model::LookupModel(ToPlatform(gen));
        if (!m) {
            throw FatalError();
        }
        return *m;
    }

    static const uint64_t VALID_COOKIE = 0xFEDCBA9876543210ull;

public:
    IGAContext(iga_context_options_t opts)
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
        uint32_t *bitsLen32)
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
        ParseOpts popts(m_model);
        popts.supportLegacyDirectives =
            (aopts.syntax_opts & IGA_SYNTAX_OPT_LEGACY_SYNTAX) != 0;
        Kernel *pKernel = iga::ParseGenKernel(m_model, inp, errHandler, popts);
        if (pKernel && !errHandler.hasErrors() && aopts.enabled_warnings) {
            // check semantics if we parsed without error && they haven't
            // disabled all checking (-Wnone)
            CheckSemantics(*pKernel, errHandler, aopts.enabled_warnings);
        }
        if (errHandler.hasErrors()) {
            *bits = nullptr;
            *bitsLen32 = 0;
            iga_status_t st = translateDiagnostics(errHandler);
            if (pKernel)
                delete pKernel;
            return (st != IGA_SUCCESS) ? st : IGA_PARSE_ERROR;
        } else if (pKernel == nullptr) {
            // parser returned nullptr for kernel, but with no errors
            // shouldn't be reachable; implies we have a missing diagnostic
            if (pKernel)
                delete pKernel;
            return IGA_ERROR;
        }

        // 3. Encode the final IR into bits
        //
        // clobber the last assembly's bits
        if (m_assemble_bits) {
            free(m_assemble_bits);
            m_assemble_bits = nullptr;
        }
        size_t bitsLen = 0;
        EncoderOpts eopts(
              (aopts.encoder_opts & IGA_ENCODER_OPT_AUTO_COMPACT) != 0,
              (aopts.encoder_opts & IGA_ENCODER_OPT_ERROR_ON_COMPACT_FAIL) == 0,
              (aopts.encoder_opts & IGA_ENCODER_OPT_FORCE_NO_COMPACT) != 0);
        eopts.autoDepSet = (aopts.encoder_opts & IGA_ENCODER_OPT_AUTO_DEPENDENCIES) != 0;
        eopts.sbidCount = aopts.sbid_count;
        eopts.swsbEncodeMode = aopts.swsb_encode_mode;

        if ((aopts.encoder_opts & IGA_ENCODER_OPT_USE_NATIVE) == 0) {
            if (!iga::ged::IsEncodeSupported(m_model, eopts)) {
                delete pKernel;
                return IGA_UNSUPPORTED_PLATFORM;
            }
            iga::ged::Encode(m_model, eopts, errHandler, *pKernel, *bits, bitsLen);
        } else {
            if (!iga::native::IsEncodeSupported(m_model, eopts)) {
                delete pKernel;
                return IGA_UNSUPPORTED_PLATFORM;
            }
            iga::native::Encode(
                m_model,
                eopts,
                errHandler,
                *pKernel,
                *bits,
                bitsLen);
        }
        *bitsLen32 = (uint32_t)bitsLen;
        if (errHandler.hasErrors()) {
            // failed encoding
            delete pKernel;
            iga_status_t st = translateDiagnostics(errHandler);
            return st == IGA_SUCCESS ? IGA_ENCODE_ERROR : st;
        }

        // 4. Copy out the result
        // encoding succeeded, copy the bits out
        m_assemble_bits = (void *)malloc(*bitsLen32);
        if (!m_assemble_bits) {
            delete pKernel;
            return IGA_OUT_OF_MEM;
        }
        memcpy_s(m_assemble_bits, *bitsLen32, * bits, *bitsLen32);
        *bits = m_assemble_bits;
        delete pKernel;
        return translateDiagnostics(errHandler);
    }

    FormatOpts formatterOpts(
        const iga_disassemble_options_t &dopts,
        const char *(*formatLabel)(int32_t, void *),
        void *formatLabelEnv,
        // swsb encoding mode, if not specified, the encoding mode will
        // be derived from platform by SWSB::getEncdoeMode
        SWSB_ENCODE_MODE swsbEnMod = SWSB_ENCODE_MODE::SWSBInvalidMode)
    {
        FormatOpts fopts(
            m_model,
            formatLabel,
            formatLabelEnv);
        fopts.addApiOpts(dopts.formatting_opts);
        if (swsbEnMod == SWSB_ENCODE_MODE::SWSBInvalidMode)
            fopts.setSWSBEncodingMode(m_model.getSWSBEncodeMode());
        else
            fopts.setSWSBEncodingMode(swsbEnMod);

        return fopts;
    }

    void checkForLegacyFields(
        iga_disassemble_options_t &dopts,
        iga::ErrorHandler &errHandler)
    {
        // crude compatibility for legacy fields
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

    iga_status_t disassembleKernel(
        iga::ErrorHandler &errHandler,
        iga_disassemble_options_t &dopts,
        const void *bits,
        uint32_t bitsLen,
        Kernel *&k)
    {
        k = nullptr;
        checkForLegacyFields(dopts, errHandler);
        DecoderOpts dopts2(
            (dopts.formatting_opts & IGA_FORMATTING_OPT_NUMERIC_LABELS) != 0);
        if ((dopts.decoder_opts & IGA_DECODING_OPT_NATIVE) == 0) {
            if (!iga::ged::IsDecodeSupported(m_model,dopts2)) {
                return IGA_UNSUPPORTED_PLATFORM;
            }
            k = iga::ged::Decode(
                m_model, dopts2, errHandler, bits, (size_t)bitsLen);
        } else {
            if (!iga::native::IsDecodeSupported(m_model,dopts2)) {
                return IGA_UNSUPPORTED_PLATFORM;
            }
            k = iga::native::Decode(
                m_model, dopts2, errHandler, bits, (size_t)bitsLen);
        }
        return k == nullptr ? IGA_DECODE_ERROR : IGA_SUCCESS;
    }

    iga_status_t disassemble(
        iga_disassemble_options_t &dopts,
        const void *bits,
        uint32_t bitsLen,
        const char *(*formatLbl)(int32_t, void *),
        void *formatLblEnv,
        char **output)
    {
        if (output)
            *output = &m_empty_string[0];

        iga::Kernel *k = nullptr;
        iga::ErrorHandler errHandler;
        iga_status_t st = IGA_ERROR;

        st = disassembleKernel(
            errHandler,
            dopts,
            bits,
            bitsLen,
            k);
        if (k != nullptr) {
            // we succeeded in decoding; now format the output to text
            std::stringstream ss;
            FormatOpts fopts = formatterOpts(dopts, formatLbl, formatLblEnv);
            DepAnalysis la;
            if (dopts.formatting_opts & IGA_FORMATTING_OPT_PRINT_DEFS) {
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

            delete k;
        } // k non-null

        st = translateDiagnostics(errHandler);
        if (errHandler.hasErrors()) {
            return IGA_DECODE_ERROR;
        }
        return st;
    }


    iga_status_t disassembleInstruction(
        iga_disassemble_options_t &dopts,
        const void *bits,
        const char *(*formatLbl)(int32_t, void *),
        void *formatLblEnv,
        char **output)
    {
        if (output)
            *output = &m_empty_string[0];

        // infer the length based on compaction bit
        size_t bitsLen = ((const MInst *)bits)->isCompact() ? 8 : 16;

        iga::Kernel *k = nullptr;
        iga::ErrorHandler errHandler;
        iga_status_t st = IGA_ERROR;

        // force decoding opts to use numeric labels
        dopts.formatting_opts |= IGA_FORMATTING_OPT_NUMERIC_LABELS;

        st = disassembleKernel(
            errHandler,
            dopts,
            bits,
            (uint32_t)bitsLen,
            k);
        if (k != nullptr) {
            if (m_disassemble_text) {
                // previous disassemble clobbers new disassemble
                free(m_disassemble_text);
            }

            const Instruction *firstInst = nullptr;
            const iga::BlockList &bl = k->getBlockList();
            for (const auto &b : bl) {
                if (!b->getInstList().empty()) {
                    firstInst = b->getInstList().front();
                    break;
                }
            }
            if (!firstInst) {
                delete k;
                return IGA_ERROR; // should be unreachable
            }
            std::stringstream ss;
            FormatOpts fopts = formatterOpts(dopts,formatLbl,formatLblEnv);
            FormatInstruction(errHandler, ss, fopts, *firstInst);

            size_t slen = (size_t)ss.tellp();
            m_disassemble_text = (char *)malloc(1 + slen);
            if (!m_disassemble_text) {
                delete k;
                return IGA_OUT_OF_MEM;
            }
            ss.read(m_disassemble_text, slen);
            m_disassemble_text[slen] = 0;
            if (output) {
                *output = m_disassemble_text;
            }

            delete k;
        } // k non-null

        st = translateDiagnostics(errHandler);
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
}; // class IGAContext


#define RETURN_INVALID_ARG_ON_NULL(X) \
    do { if (!(X)) return IGA_INVALID_ARG; } while (0)

#define CAST_CONTEXT(ID,C) \
    IGAContext *ID = (IGAContext *)(C); \
    if (!ID->valid()) { \
        return IGA_INVALID_OBJECT; \
    }


const char *iga_version_string()
{
    return IGA_VERSION_PREFIX_STRING IGA_VERSION_SUFFIX;
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
    memcpy_s(&coptsInternal, opts->cb, opts, opts->cb);

    iga::Platform p = ToPlatform(opts->gen);
    if (p == iga::Platform::INVALID) {
        return IGA_UNSUPPORTED_PLATFORM;
    }

    IGAContext *ctx_obj = nullptr;
    try {
        ctx_obj = new IGAContext(*opts);
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
    memcpy_s(&aoptsInternal, aopts->cb, aopts, aopts->cb);

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
    memcpy_s(&doptsInternal, dopts->cb, dopts, dopts->cb);

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
    memcpy_s(&doptsInternal, dopts->cb, dopts, dopts->cb);

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
/*  #define IGA_DIAGNOSTIC_IS_BINARY(D) \
    ((D)->column & IGA_DIAGNOSTIC_BINARY_MASK)
 for now we use line == col == 0 to mean binary
*/
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
            memcpy_s(                                                          \
                (DST), _CPLEN * sizeof(*(DST)), (SRC), _CPLEN * sizeof(*(DST))); \
        } \
        *(DST_LEN_PTR) = (SRC_LEN); \
    } while (0)

// same as above, but for a string
#define IGA_COPY_OUT_STR(DST, DST_LEN_PTR, SRC) \
    do { \
        size_t SRC_LEN = strlen(SRC) + 1; \
        if ((DST) != nullptr) { \
            size_t _CPLEN = *(DST_LEN_PTR) < (SRC_LEN) ? *(DST_LEN_PTR) : (SRC_LEN); \
            memcpy_s(                                                          \
                (DST), _CPLEN * sizeof(*(DST)), (SRC), _CPLEN * sizeof(*(DST))); \
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
// (or store as Platform x Op)
// That would translate to fairly small integer that should be in the
// no access range (near 0)
static iga_opspec_t opspec_to_handle(const OpSpec *os) {
    const uintptr_t TOP_BIT = (sizeof(void *) == 4) ?
        0xC0000000 : 0x8000000000000000;
    return (iga_opspec_t)((uintptr_t)os ^ TOP_BIT);
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

    const Model *m = Model::LookupModel(ToPlatform(gen));
    if (!m) {
        return IGA_UNSUPPORTED_PLATFORM;
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
    uint32_t op_enum,
    iga_opspec_t *op)
{
    RETURN_INVALID_ARG_ON_NULL(op);
    const Model *m = Model::LookupModel(ToPlatform(gen));
    if (!m) {
        return IGA_UNSUPPORTED_PLATFORM;
    }
    const OpSpec *os = &m->lookupOpSpec(static_cast<iga::Op>(op_enum));
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

    const OpSpec *os = opspec_from_handle(op);
    IGA_COPY_OUT_STR(mnemonic, mnemonic_len, os->mnemonic.str().c_str());
    return IGA_SUCCESS;
}


iga_status_t iga_opspec_name(
    iga_opspec_t op,
    char *name,
    size_t *name_len)
{
    RETURN_INVALID_ARG_ON_NULL(op);
    RETURN_INVALID_ARG_ON_NULL(name_len);

    const OpSpec *os = opspec_from_handle(op);
    IGA_COPY_OUT_STR(name, name_len, os->name.str().c_str());
    return IGA_SUCCESS;
}


iga_status_t iga_opspec_description(
    iga_opspec_t op,
    char *desc,
    size_t *desc_len)
{
    RETURN_INVALID_ARG_ON_NULL(op);
    RETURN_INVALID_ARG_ON_NULL(desc_len);

    IGA_COPY_OUT_STR(desc, desc_len, "<description unsupported>");
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

    *opcode = static_cast<uint32_t>(opspec_from_handle(op)->opcode);
    return IGA_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
// RETURNS ALL THE FUNCTIONS IN A TABLE
///////////////////////////////////////////////////////////////////////////////

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



