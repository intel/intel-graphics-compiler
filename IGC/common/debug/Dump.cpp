/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/debug/Dump.hpp"

#include "common/debug/TeeOutputStream.hpp"

#include "AdaptorCommon/customApi.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "common/igc_regkeys.hpp"

#include <iStdLib/utility.h>

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Value.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/FileSystem.h>
#include "common/LLVMWarningsPop.hpp"
#include <optional>

#include <stdarg.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <algorithm>
#include "Probe/Assertion.h"

using namespace IGC;
using namespace IGC::Debug;
using namespace std;

namespace IGC {
namespace Debug {

char ModuleFlushDumpPass::ID = 0;
char FunctionFlushDumpPass::ID = 0;

const char *GetShaderTypeAcronym(ShaderType shaderType) {
  switch (shaderType) {
  case ShaderType::OPENCL_SHADER:
    return "OCL";
  case ShaderType::PIXEL_SHADER:
    return "PS";
  case ShaderType::DOMAIN_SHADER:
    return "DS";
  case ShaderType::HULL_SHADER:
    return "HS";
  case ShaderType::VERTEX_SHADER:
    return "VS";
  case ShaderType::GEOMETRY_SHADER:
    return "GS";
  case ShaderType::COMPUTE_SHADER:
    return "CS";
    break;
  case ShaderType::RAYTRACING_SHADER:
    return "RT";
    break;
  case ShaderType::UNKNOWN:
  default:
    IGC_ASSERT_MESSAGE(0, "Unknown Shader Type");
    return "UNKNOWN";
  }
}

/*************************************************************************************************\
 *  Generic
 */

DumpName::DumpName(std::string const &dumpName) : m_dumpName(dumpName) {}

DumpName::DumpName() {}

DumpName DumpName::ShaderName(std::string const &name) const {
  if (name == "") {
    return *this;
  }
  IGC_ASSERT_MESSAGE(name.find(' ') == std::string::npos, "Shader name must not contain spaces");
  DumpName copy(*this);
  copy.m_shaderName = name;
  return copy;
}

DumpName DumpName::Type(ShaderType type) const {
  DumpName copy(*this);
  copy.m_type = type;
  return copy;
}

DumpName DumpName::PSPhase(PixelShaderPhaseType phase) const {
  DumpName copy(*this);
  copy.m_psPhase = phase;
  return copy;
}

DumpName DumpName::Retry(unsigned retryId) const {
  DumpName copy(*this);
  copy.m_retryId = retryId;
  return copy;
}

DumpName DumpName::Extension(std::string const &extension) const {
  IGC_ASSERT_MESSAGE((extension.size() == 0) || (extension.at(0) != '.'),
                     "Extension shouldn't start with a '.', and shouldn't be empty");
  IGC_ASSERT_MESSAGE(extension.find(' ') == std::string::npos, "Extension must not contain spaces");
  DumpName copy(*this);
  copy.m_extension = extension;
  return copy;
}

DumpName DumpName::StagedInfo(void const *context) const {
  DumpName copy(*this);
  if (!context) {
    return copy;
  }

  IGC::CodeGenContext const *ctx = static_cast<IGC::CodeGenContext const *>(context);
  if (!IsStage2RestSIMDs(ctx->m_StagingCtx) && ctx->m_CgFlag != FLAG_CG_ALL_SIMDS) {
    copy.m_cgFlag = ctx->m_CgFlag;
  } else if (IsStage2RestSIMDs(ctx->m_StagingCtx)) {
    copy.m_cgFlag = FLAG_CG_ALL_SIMDS;
  }
  return copy;
}

DumpName DumpName::SIMDSize(SIMDMode width) const {
  DumpName copy(*this);
  copy.m_simdWidth = width;
  return copy;
}

DumpName DumpName::DispatchMode(ShaderDispatchMode mode) const {
  DumpName copy(*this);
  switch (mode) {
  case ShaderDispatchMode::SINGLE_PATCH:
  case ShaderDispatchMode::DUAL_PATCH:
  case ShaderDispatchMode::EIGHT_PATCH:
  case ShaderDispatchMode::QUAD_SIMD8_DYNAMIC:
    copy.m_ShaderMode = mode;
    break;
  default:
    break;
  }
  return copy;
}

DumpName DumpName::Hash(ShaderHash hash) const {
  DumpName copy(*this);
  copy.m_hash = hash;
  return copy;
}

DumpName DumpName::PostFix(std::string const &postfixStr) const {
  DumpName copy(*this);
  copy.m_postfixStr = postfixStr;
  return copy;
}

DumpName DumpName::Pass(std::string const &name, std::optional<unsigned int> index) const {
  std::string newName = name;
  // remove spaces
  newName.erase(
      remove_if(newName.begin(), newName.end(), [](char c) { return isspace(static_cast<unsigned char>(c)); }),
      newName.end());
  std::replace_if(newName.begin(), newName.end(), [](const char s) { return s == '/' || s == '\\'; }, '_');
  IGC_ASSERT_MESSAGE(newName.find(' ') == std::string::npos, "Pass name must not contain spaces");
  DumpName copy(*this);
  CPassDescriptor pd = {std::move(newName), index};
  copy.m_pass = pd;
  return copy;
}

std::unordered_map<QWORD, unsigned int> shaderHashMap;
std::mutex DumpName::hashMapLock;
unsigned int DumpName::shaderNum = 1;

std::string DumpName::AbsolutePath(OutputFolderName folder) const {
  std::stringstream ss;
  bool underscore = false; /// Determines whether the next segment prints a leading underscore

  ss << folder;

  //    m_dumpName = IGC::Debug::GetShaderOutputName();

  if (m_dumpName.has_value() && m_dumpName.value() != "") {
    ss << m_dumpName.value();
    underscore = true;
  }
  if (m_shaderName.has_value()) {
    ss << (underscore ? "_" : "") << m_shaderName.value();
    underscore = true;
  }
  if (m_type.has_value()) {
    ss << (underscore ? "_" : "");
    ss << GetShaderTypeAcronym(m_type.value());
    underscore = true;
  }
  if (m_psPhase.has_value()) {
    if (m_psPhase.value() != PixelShaderPhaseType::PSPHASE_LEGACY) {
      ss << (underscore ? "_" : "");
      switch (m_psPhase.value()) {
      case PixelShaderPhaseType::PSPHASE_COARSE:
        ss << "CPS";
        break;
      case PixelShaderPhaseType::PSPHASE_PIXEL:
        ss << "PS";
        break;
      default:
        IGC_ASSERT(0);
        break;
      }
    }
  }
  if (m_hash.has_value()) {

    if (m_type.has_value() && IGC_IS_FLAG_ENABLED(EnableShaderNumbering)) {
      hashMapLock.lock();
      bool increment = shaderHashMap.insert({m_hash->asmHash, shaderNum}).second;
      // Need to serialize access to the shaderNum counter in case different threads need to dump the same shader at
      // once.
      if (increment)
        shaderNum++;
      ss << "_" << shaderHashMap[m_hash->asmHash] << "_";
      hashMapLock.unlock();
    }

    if (m_hash->asmHash != 0) {
      ss << (underscore ? "_" : "") << "asm" << std::hex << std::setfill('0')
         << std::setw(sizeof(m_hash->asmHash) * CHAR_BIT / 4) << m_hash->asmHash << std::dec << std::setfill(' ');
    }

    if (m_hash->nosHash != 0) {
      ss << "_" << "nos" << std::hex << std::setfill('0') << std::setw(sizeof(m_hash->nosHash) * CHAR_BIT / 4)
         << m_hash->nosHash << std::dec << std::setfill(' ');
    }

    if (m_hash->psoHash != 0) {
      ss << "_" << "pso" << std::hex << std::setfill('0') << std::setw(sizeof(m_hash->psoHash) * CHAR_BIT / 4)
         << m_hash->psoHash << std::dec << std::setfill(' ');
    }

    if (m_hash->perShaderPsoHash != 0) {
      ss << "_" << "spec" << std::hex << std::setfill('0') << std::setw(sizeof(m_hash->perShaderPsoHash) * CHAR_BIT / 4)
         << m_hash->perShaderPsoHash << std::dec << std::setfill(' ');
    }

    underscore = true;
  }

  if (m_retryId.has_value() && m_retryId.value()) {
    ss << "_" << m_retryId.value();
  }

  if (m_cgFlag.has_value()) {
    const auto cgfv = m_cgFlag.value();

    IGC_ASSERT(cgfv < CG_FLAG_size);
    ss << (underscore ? "_" : "");
    ss << CG_FLAG_STR[cgfv];
    underscore = true;
  }

  if (m_pass.has_value()) {
    if (m_pass->m_index.has_value()) {
      ss << (underscore ? "_" : "") << std::setfill('0') << std::setw(4) << m_pass->m_index.value()
         << std::setfill(' ');
      underscore = true;
    }
    ss << (underscore ? "_" : "") << m_pass->m_name;
    underscore = true;
  }

  if (m_simdWidth.has_value()) {
    ss << (underscore ? "_" : "") << "simd" << numLanes(m_simdWidth.value());
    underscore = true;
  }

  if (m_ShaderMode.has_value()) {
    if (m_ShaderMode.value() == ShaderDispatchMode::SINGLE_PATCH) {
      ss << (underscore ? "_" : "") << "SinglePatch";
      underscore = true;
    }
    if (m_ShaderMode.value() == ShaderDispatchMode::DUAL_PATCH) {
      ss << (underscore ? "_" : "") << "DualPatch";
      underscore = true;
    }
    if (m_ShaderMode.value() == ShaderDispatchMode::EIGHT_PATCH) {
      ss << (underscore ? "_" : "") << "EightPatch";
      underscore = true;
    }
    if (m_ShaderMode.value() == ShaderDispatchMode::QUAD_SIMD8_DYNAMIC) {
      ss << (underscore ? "_" : "") << "QuadSIMD8Dynamic";
      underscore = true;
    }
  }

  if (m_postfixStr.has_value() && !m_postfixStr.value().empty()) {
    std::string s = m_postfixStr.value();
    // sanitize the function name
    std::replace(s.begin(), s.end(), (char)1, '_');
    std::replace(s.begin(), s.end(), '/', '_');
    std::replace(s.begin(), s.end(), '\\', '_');
    std::replace(s.begin(), s.end(), ':', '_');
    std::replace(s.begin(), s.end(), '*', '_');
    std::replace(s.begin(), s.end(), '?', '_');
    std::replace(s.begin(), s.end(), '\"', '_');
    std::replace(s.begin(), s.end(), '<', '_');
    std::replace(s.begin(), s.end(), '>', '_');
    std::replace(s.begin(), s.end(), '|', '_');

    ss << "_" << s;
  }

  if (m_extension.has_value()) {
    ss << "." << m_extension.value();
  }
  return ss.str();
}

std::string DumpName::GetKernelDumpName() const {
  std::string kernelDumpName = "";
  if (m_postfixStr.has_value() && !m_postfixStr.value().empty()) {
    kernelDumpName = m_postfixStr.value();
  }
  return kernelDumpName;
}

std::string DumpName::str() const { return AbsolutePath(IGC::Debug::GetShaderOutputFolder()); }

std::string DumpName::overridePath() const { return AbsolutePath(IGC::Debug::GetShaderOverridePath()); }

std::string DumpName::RelativePath() const { return AbsolutePath(""); }

bool DumpName::allow() const { return doesRegexMatch(RelativePath(), IGC_GET_REGKEYSTRING(ShaderDumpRegexFilter)); }

namespace {
bool isText(DumpType type) {
  switch (type) {
  case DumpType::NOS_TEXT:
    return true;
  case DumpType::CIS_TEXT:
    return true;
  case DumpType::COS_TEXT:
    return true;
  case DumpType::ASM_TEXT:
    return true;
  case DumpType::ASM_BC:
    return false;
  case DumpType::TRANSLATED_IR_TEXT:
    return true;
  case DumpType::TRANSLATED_IR_BC:
    return false;
  case DumpType::PASS_IR_TEXT:
    return true;
  case DumpType::PASS_IR_BC:
    return false;
  case DumpType::OptIR_TEXT:
    return true;
  case DumpType::OptIR_BC:
    return false;
  case DumpType::VISA_TEXT:
    return true;
  case DumpType::VISA_BC:
    return false;
  case DumpType::GENX_ISA_TEXT:
    return true;
  case DumpType::GENX_ISA_BC:
    return false;
  case DumpType::LLVM_OPT_STAT_TEXT:
    return true;
  case DumpType::TIME_STATS_TEXT:
    return true;
  case DumpType::TIME_STATS_CSV:
    return true;
  case DumpType::DBG_MSG_TEXT:
    return true;
  default:
    IGC_ASSERT_MESSAGE(0, "unreachable");
    return true;
  }
}

const char *commentPrefix(DumpType type) {
  IGC_ASSERT_MESSAGE(isText(type), "Only text types are supported");
  switch (type) {
  case DumpType::NOS_TEXT:
    return "// ";
  case DumpType::CIS_TEXT:
    return "// ";
  case DumpType::COS_TEXT:
    return "// ";
  case DumpType::ASM_TEXT:
    return "// ";
  case DumpType::TRANSLATED_IR_TEXT:
    return "; ";
  case DumpType::PASS_IR_TEXT:
    return "; ";
  case DumpType::OptIR_TEXT:
    return "; ";
  case DumpType::VISA_TEXT:
    return "/// ";
  case DumpType::GENX_ISA_TEXT:
    return "/// ";
  case DumpType::LLVM_OPT_STAT_TEXT:
    return "";
  case DumpType::TIME_STATS_TEXT:
    return "";
  case DumpType::DBG_MSG_TEXT:
    return "";
  case DumpType::TIME_STATS_CSV:
    IGC_ASSERT_MESSAGE(0, "CSV doesn't have comments");
    return "";
  default:
    IGC_ASSERT_MESSAGE(0, "unreachable");
    return "#";
  }
}

bool isConsolePrintable(DumpType type) { return isText(type) && type != DumpType::TIME_STATS_CSV; }
} // namespace

namespace {
/// Writes a prefix to the underlying stream upon first write. Forwards to the underlying
/// stream on every subsequent write.
class PrefixStream : public llvm::raw_ostream {
public:
  PrefixStream(std::string const &prefix, Colors color, llvm::raw_ostream *pUnder, bool deleteStream)
      : llvm::raw_ostream(true /* unbuffered */), m_prefix(prefix), m_prefixColor(color), m_pUnder(pUnder),
        m_deleteStream(deleteStream), m_isFirstWrite(true) {}

  virtual ~PrefixStream() {
    flush();
    if (m_deleteStream) {
      m_pUnder->flush();
      delete m_pUnder;
    }
  }
  PrefixStream(const PrefixStream &) = delete;
  PrefixStream &operator=(const PrefixStream &) = delete;

  virtual uint64_t current_pos() const override { return 0; }

  virtual raw_ostream &changeColor(enum Colors Color, bool Bold = false, bool BG = false) override {
    m_pUnder->changeColor(Color, Bold, BG);
    return *this;
  }

  virtual raw_ostream &resetColor() override {
    m_pUnder->resetColor();
    return *this;
  }

  virtual raw_ostream &reverseColor() override {
    m_pUnder->reverseColor();
    return *this;
  }

  virtual bool is_displayed() const override { return m_pUnder->is_displayed(); }

  virtual bool has_colors() const override { return m_pUnder->has_colors(); }

protected:
  virtual size_t preferred_buffer_size() const override { return 0; }

private:
  void write_impl(const char *Ptr, size_t Size) override {
    if (m_isFirstWrite) {
      ods().changeColor(m_prefixColor, true, false);
      *m_pUnder << m_prefix;
      ods().resetColor();
      m_isFirstWrite = false;
    }
    m_pUnder->write(Ptr, Size);
  }

private:
  const std::string m_prefix;
  const Colors m_prefixColor;
  llvm::raw_ostream *const m_pUnder;
  const bool m_deleteStream;
  bool m_isFirstWrite;
};
} // anonymous namespace

// Note: This constructor is not allowed to hold on to the dumpName reference after it finishes
// (In the case where Dump is on the heap and DumpName is on the stack, the reference would go bad
// when the calling function returns)
Dump::Dump(DumpName const &dumpName, DumpType type)
    : m_name(dumpName), m_pStream(std::make_unique<llvm::raw_string_ostream>(m_string)), // buffered stream!
      m_pStringStream(nullptr), m_type(type) {
  m_pStringStream = m_pStream.get();
  if (isConsolePrintable(m_type)) {
    if (IGC_IS_FLAG_ENABLED(PrintToConsole)) {
      bool dumpFlag = GetDebugFlag(DebugFlag::DUMP_TO_OUTS);
      SetDebugFlag(DebugFlag::DUMP_TO_OUTS, true);
      m_pStream->SetUnbuffered();
      m_pStream.reset(new TeeOutputStream(&ods(), false, m_pStream.release(), true));
      SetDebugFlag(DebugFlag::DUMP_TO_OUTS, dumpFlag);
    }

    std::stringstream ss;
    ss << commentPrefix(m_type) << "------------------------------------------------\n";
    ss << commentPrefix(m_type) << dumpName.RelativePath() << "\n";
    ss << commentPrefix(m_type) << "------------------------------------------------\n";
    m_pStream.reset(new PrefixStream(ss.str(), llvm::raw_ostream::Colors::YELLOW, m_pStream.release(), true));
  }
}

void Dump::flush() {
  m_pStringStream->flush();
  if (m_string.empty()) {
    return;
  }
  std::ios_base::openmode mode = std::ios_base::out;
  if (!isText(m_type))
    mode |= std::ios_base::binary;
  if (m_name.allow()) {
    const auto collisionMode = static_cast<FILENAME_COLLISION_MODE>(IGC_GET_FLAG_VALUE(ShaderDumpCollisionMode));
    if (collisionMode == FILENAME_COLLISION_MODE::OVERRIDE || !std::filesystem::exists(m_name.str())) {
      std::ofstream asmFile(m_name.str(), mode);
      asmFile << m_string;
    } else if (isText(m_type)) { // Do NOT append binary files
      std::ofstream asmFile(m_name.str(), mode | std::ios_base::app);
      if (collisionMode == FILENAME_COLLISION_MODE::APPEND)
        asmFile << commentPrefix(m_type) << "Warning: appending filename collides\n" << m_string;
      else
        asmFile << commentPrefix(m_type) << "Warning: skipping filename collides\n";
    }
  }
  m_string.clear();
}

Dump::~Dump() { flush(); }

llvm::raw_ostream &Dump::stream() const { return *m_pStream; }

void DumpLLVMIRText(llvm::Module *pModule, const DumpName &dumpName,
                    llvm::AssemblyAnnotationWriter *optionalAnnotationWriter /* = nullptr */) {
#if defined(IGC_DEBUG_VARIABLES)
  if (dumpName.allow()) {
    auto dump = Dump(dumpName, DumpType::PASS_IR_TEXT);

    IGC::Debug::DumpLock();
    pModule->print(dump.stream(), optionalAnnotationWriter);
    IGC::Debug::DumpUnlock();
  }
#endif
}

int PrintDebugMsgV(const Dump *dump, const char *fmt, va_list ap) {
#if defined(_DEBUG) || defined(_INTERNAL)
  if (dump) {
    char buf[512];
    int r;
    r = vsnprintf(buf, sizeof(buf), fmt, ap);
    buf[sizeof(buf) - 1] = '\0';
    dump->stream() << buf;
    return r;
  }
#endif
  return 0;
}

void PrintDebugMsg(const Dump *dump, const char *fmt, ...) {
#if defined(_DEBUG) || defined(_INTERNAL)
  va_list ap;
  va_start(ap, fmt);
  PrintDebugMsgV(dump, fmt, ap);
  va_end(ap);
#endif
}

ShaderHash ShaderHashOCL(const UINT *pShaderCode, size_t size) {
  ShaderHash hash;
  hash.asmHash = iSTD::Hash(reinterpret_cast<const DWORD *>(pShaderCode), int_cast<DWORD>(size));
  hash.nosHash = 0;
  return hash;
}

ShaderHash ShaderHashOGL(QWORD glslHash, QWORD nosHash) {
  ShaderHash shaderHash;
  shaderHash.asmHash = glslHash;
  shaderHash.nosHash = nosHash;
  return shaderHash;
}

DumpName GetDumpNameObj(const IGC::CShader *pProgram, const char *ext) {
  IGC::CodeGenContext *context = pProgram->GetContext();

  bool overrideHash = false;

  DumpName dumpName = IGC::Debug::DumpName(IGC::Debug::GetShaderOutputName())
                          .ShaderName(context->shaderName)
                          .Type(context->type)
                          .Hash(context->hash)
                          .StagedInfo(context);

  if (overrideHash)
    context->hash.asmHash = 0;

  IGC_ASSERT_MESSAGE(pProgram->entry, "ICE: entry should be setup  and valid!");
  std::string shaderName(pProgram->entry->getName().str());
  pProgram->getShaderFileName(shaderName);

  dumpName = dumpName.PostFix(shaderName);
  dumpName = dumpName.DispatchMode(pProgram->m_ShaderDispatchMode);
  dumpName =
      dumpName.SIMDSize(pProgram->m_State.m_dispatchSize).Retry(context->m_retryManager->GetRetryId()).Extension(ext);

  return dumpName;
}

DumpName GetLLDumpName(IGC::CodeGenContext *pContext, const char *dumpName) {
  auto name = DumpName(GetShaderOutputName())
                  .Hash(pContext->hash)
                  .Type(pContext->type)
                  .Pass(dumpName)
                  .Retry(pContext->m_retryManager->GetRetryId())
                  .Extension("ll");
  return name;
}

} // namespace Debug
} // namespace IGC
