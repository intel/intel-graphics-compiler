/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "InstDiff.hpp"
#include "Backend/Native/InstEncoder.hpp"
#include "Backend/Native/Interface.hpp"
#include "ColoredIO.hpp"
#include "Frontend/Formatter.hpp"
#include "bits.hpp"
#include "strings.hpp"

#include <algorithm>
#include <bitset>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

using namespace iga;

static std::string disassembleInst(Platform platform, bool useNativeDecoder,
                                   uint32_t fmtOpts, size_t fromPc,
                                   const void *bits) {
  ErrorHandler eh;
  std::stringstream ss;
  const Model *model = Model::LookupModel(platform);
  if (!model) {
    return "invalid model???";
  }
  FormatOpts fopts(*model);
  fopts.addApiOpts(fmtOpts);
  fopts.setSWSBEncodingMode(Model::LookupModel(platform)->getSWSBEncodeMode());
  FormatInstruction(eh, ss, fopts, fromPc, bits, useNativeDecoder);

  return ss.str();
}
static std::string fmtPc(const MInst *mi, size_t pc) {
  std::stringstream ssPc;
  ssPc << "[";
  if (mi) {
    ssPc << "0x" << std::setfill('0') << std::setw(3) << std::uppercase
         << std::hex << pc;
  } else {
    ssPc << std::setw(3) << "EOF";
  }
  ssPc << "] ";
  return ssPc.str();
}
static std::string fmtHexValue(uint64_t val) { return fmtHex(val); }
static std::string fmtBitRange(size_t off, size_t len) {
  std::stringstream ssOff; // [...:..]
  if (len != 1) {
    ssOff << "[" << (off + len - 1) << ":" << off << "]";
  } else {
    ssOff << "[" << off << "]";
  }
  return ssOff.str();
}
static std::string fmtBitRange(const Fragment &f) {
  return fmtBitRange(f.offset, f.length);
}
static std::string fmtSize(const Fragment &f) {
  std::stringstream ssSize;
  ssSize << "(" << f.length << ")";
  return ssSize.str();
}

static const char *UNMAPPED_FIELD_NAME = "<UNMAPPED>";
static const size_t FIELD_PC_WIDTH = 7;         // [0x200]
static const size_t FIELD_NAME_WIDTH = 20;      // Src.Type
static const size_t FIELD_OFFSET_WIDTH = 10;    // [34:32]
static const size_t FIELD_SIZE_WIDTH = 5;       // (32)
static const size_t FIELD_VALUE_INT_WIDTH = 14; // 0x1232
static const size_t FIELD_VALUE_STR_WIDTH = 20; // :d

static FragmentList
decodeFieldsWithWarnings(const Model &model, std::ostream &os, Loc loc,
                         const MInst *mi, bool &success,
                         const char *leftOrRight = nullptr) {
  FragmentList fields;
  ErrorHandler errHandler;
  native::DecodeFields(loc, model, (const void *)mi, fields, errHandler);
  for (const auto &e : errHandler.getErrors()) {
    std::stringstream ss;
    ss << "PC" << e.at.offset << ". " << e.message << "\n";
    emitYellowText(os, ss.str());
    success = false;
  }
  auto findFieldsContaining = [&](size_t bit_ix) {
    std::vector<Fragment *> matching;
    for (auto &fi : fields) {
      if (bit_ix >= (size_t)fi.first.offset &&
          bit_ix < (size_t)fi.first.offset + (size_t)fi.first.length) {
        matching.push_back(&fi.first);
      }
    }
    return matching;
  };

  std::stringstream warningStream;
  size_t iLen = mi->isCompact() ? 8 : 16;
  // check for field overlap
  for (size_t bitIx = 0; bitIx < 8 * iLen;) {
    // find all the fields that overlap at this bit index
    auto matching = findFieldsContaining(bitIx);
    // no fields map here or multiple matching, issue missing field warning
    if (matching.size() == 1) {
      // exactly one field maps this index, step forward
      bitIx++;
    } else {
      // find the end of this missing/overlapped segment
      size_t endIx = bitIx + 1;
      for (; endIx < 8 * iLen; endIx++) {
        auto m = findFieldsContaining(endIx);
        if (m != matching) {
          break;
        }
      }
      if (!matching.empty()) {
        // TODO: remove this: do it at a higher level when we
        // are emitting the columns
        if (leftOrRight) {
          warningStream << "in " << leftOrRight << " ";
        }
        warningStream << "BITS";
        warningStream << fmtBitRange(bitIx, endIx - bitIx) << ": ";
        warningStream << "PC:" << loc.offset
                      << ": multiple fields map this range\n";
        success = false;
      } else {
        // insert an ERROR field here since nothing maps there
        Fragment errorField =
            Fragment(UNMAPPED_FIELD_NAME, (int)bitIx, (int)(endIx - bitIx));
        FragmentList::iterator itr = fields.begin();
        for (; itr != fields.end() && itr->first.offset < errorField.offset;
             itr++)
          ;
        fields.insert(itr, std::pair<Fragment, std::string>(errorField, ""));
        success = false;
      }
      bitIx = endIx; // sanity restarts (or some new error)
    }
  }
  if (!warningStream.str().empty()) {
    success = false;
    emitRedText(os, warningStream.str());
  }

  return fields;
}

static void decodeFieldHeaders(std::ostream &os) {
  os << std::setw(FIELD_PC_WIDTH) << std::left << "PC";
  os << " ";
  os << std::setw(FIELD_NAME_WIDTH) << std::left << "FIELD";
  os << " ";
  os << std::setw(FIELD_OFFSET_WIDTH) << " "; // [...:..]
  os << " ";
  os << std::setw(FIELD_SIZE_WIDTH) << " "; // (..)
  os << " ";
  os << std::setw(FIELD_VALUE_INT_WIDTH + 1 + FIELD_VALUE_STR_WIDTH)
     << std::right << "VALUE";
  os << "\n";
}

static bool decodeFieldsForInst(bool useNativeDecoder, uint32_t fmtOpts,
                                std::ostream &os, size_t pc, const Model &model,
                                const MInst *mi) {
  bool success = true;

  auto syntax = disassembleInst(model.platform, useNativeDecoder, fmtOpts, pc,
                                (const void *)mi);
  os << fmtPc(mi, pc) << " " << syntax << "\n";
  os.flush();

  FragmentList fields =
      decodeFieldsWithWarnings(model, os, Loc((uint32_t)pc), mi, success);

  auto fieldOverlaps = [&](const Fragment &f) {
    for (const auto &fv : fields) {
      const Fragment *fp = &fv.first;
      if (fp != &f && f.overlaps(*fp))
        return true;
    }
    return false;
  };

  for (const FragmentListElem &fv : fields) {
    const Fragment &f = fv.first;
    uint64_t val = mi->getFragment(f);
    std::stringstream ss;
    ss << std::setw(FIELD_PC_WIDTH) << std::left << " ";
    ss << " ";

    ss << std::setw(FIELD_NAME_WIDTH) << std::left << f.name;
    ss << " ";

    ss << std::setw(FIELD_OFFSET_WIDTH) << std::right << fmtBitRange(f);
    ss << " ";

    // e.g. (32)
    ss << std::right << std::setw(FIELD_SIZE_WIDTH) << fmtSize(f);
    ss << " ";

    // 0x1234
    ss << std::right << std::setw(FIELD_VALUE_INT_WIDTH) << fmtHexValue(val);
    ss << " ";

    // :q
    ss << std::right << std::setw(FIELD_VALUE_STR_WIDTH) << fv.second;

    bool overlaps = fieldOverlaps(f);
    if (overlaps)
      ss << "  [FIELD OVERLAPS]";
    ss << "\n";

    // TODO: need to also emit warnings on overlapped fields
    if (std::string(UNMAPPED_FIELD_NAME) == f.name) {
      success = false;
      emitRedText(os, ss.str());
    } else if (overlaps) {
      success = false;
      emitRedText(os, ss.str());
    } else {
      os << ss.str();
    }
  }
  return success;
}

iga_status_t iga::DecodeFields(Platform p, int /* verbosity */,
                               bool useNativeDecoder, uint32_t fmtOpts,
                               std::ostream &os, const uint8_t *bits,
                               size_t bitsLen) {
  const Model *model = Model::LookupModel(p);
  if (model == nullptr) {
    return IGA_UNSUPPORTED_PLATFORM;
  } else if (!iga::native::IsDecodeSupported(*model, DecoderOpts())) {
    return IGA_UNSUPPORTED_PLATFORM;
  }

  decodeFieldHeaders(os);
  bool success = true;

  size_t pc = 0;
  while (pc < bitsLen) {
    if (bitsLen - pc < 4) {
      emitYellowText(os, "WARNING: extra padding at end of kernel\n");
      break;
    }
    const MInst *mi = (const MInst *)&bits[pc];
    size_t iLen = mi->isCompact() ? 8 : 16;
    if (bitsLen - pc < iLen) {
      emitYellowText(os, "WARNING: extra padding at end of kernel\n");
      break;
    }

    success &=
        decodeFieldsForInst(useNativeDecoder, fmtOpts, os, pc, *model, mi);

    pc += iLen;
  }

  return success ? IGA_SUCCESS : IGA_ERROR;
}

iga_status_t iga::DiffFields(Platform p, int verbosity, bool useNativeDecoder,
                             uint32_t fmtOpts, std::ostream &os,
                             const char *source1, const uint8_t *bits1,
                             size_t bitsLen1, const char *source2,
                             const uint8_t *bits2, size_t bitsLen2) {
  return DiffFieldsFromPCs(p, verbosity, useNativeDecoder, fmtOpts, os, source1,
                           0, bits1, bitsLen1, source2, 0, bits2, bitsLen2);
}

iga_status_t iga::DiffFieldsFromPCs(Platform p, int verbosity,
                                    bool useNativeDecoder, uint32_t fmtOpts,
                                    std::ostream &os, const char *source1,
                                    size_t startPc1, const uint8_t *bits1,
                                    size_t bitsLen1, const char *source2,
                                    size_t startPc2, const uint8_t *bits2,
                                    size_t bitsLen2) {
  const Model *model = Model::LookupModel(p);
  if (model == nullptr) {
    return IGA_UNSUPPORTED_PLATFORM;
  } else if (!iga::native::IsDecodeSupported(*model, DecoderOpts())) {
    return IGA_UNSUPPORTED_PLATFORM;
  }

  if (bitsLen1 == bitsLen2 && memcmp(bits1, bits2, bitsLen1) == 0 &&
      verbosity < 0) {
    // don't show any output if they match exactly
    //
    // SPECIFY: this short-circuit prevents us from testing disassembly
    // on each instruction.  Is that what we want here?
    return IGA_SUCCESS;
  }

  if (source1 == nullptr) {
    source1 = "VALUE1";
  }
  if (source2 == nullptr) {
    source2 = "VALUE2";
  }

  os << std::setw(FIELD_PC_WIDTH) << std::left << "PC";
  os << " ";
  os << std::setw(FIELD_NAME_WIDTH) << std::left << "FIELD";
  os << " ";
  os << std::setw(FIELD_OFFSET_WIDTH) << " "; // [...:..]
  os << " ";
  os << std::setw(FIELD_SIZE_WIDTH) << " "; // (..)
  os << " ";
  os << std::setw(FIELD_VALUE_INT_WIDTH + 1 + FIELD_VALUE_STR_WIDTH)
     << std::right << source1;
  os << " ";
  os << std::setw(FIELD_VALUE_INT_WIDTH + 1 + FIELD_VALUE_STR_WIDTH)
     << std::right << source2;
  os << "\n";

  bool decodeFailure = false, differencesFound = false;
  size_t pc1 = startPc1, pc2 = startPc2;

  while (pc1 - startPc1 < bitsLen1 || pc2 - startPc2 < bitsLen2) {
    auto getPc = [&](const char *which, const uint8_t *bits, size_t bitsLen,
                     size_t pc) {
      if (bitsLen - pc < 4) {
        std::stringstream ss;
        ss << which << ": extra padding at end of kernel";
        decodeFailure = true;
      }
      const iga::MInst *mi = (const iga::MInst *)&bits[pc];
      if ((mi->isCompact() && bitsLen - pc < 8) || bitsLen - pc < 16) {
        std::stringstream ss;
        ss << which << ": extra padding at end of kernel";
        decodeFailure = true;
      }
      return mi;
    };

    const iga::MInst *mi1 = getPc("kernel 1", bits1, bitsLen1, pc1 - startPc1),
                     *mi2 = getPc("kernel 2", bits2, bitsLen2, pc2 - startPc2);

    // if given -q, then we ignore differences
    if (verbosity >= 0 || *mi1 != *mi2) {
      os << fmtPc(mi1, pc1) << " "
         << disassembleInst(p, useNativeDecoder, fmtOpts, pc1,
                            (const void *)mi1)
         << "\n";
      os << fmtPc(mi2, pc2) << " "
         << disassembleInst(p, useNativeDecoder, fmtOpts, pc2,
                            (const void *)mi2)
         << "\n";

      FragmentList fields1;
      bool successL = true;
      if (mi1) {
        fields1 = decodeFieldsWithWarnings(*model, os, Loc((uint32_t)pc1), mi1,
                                           successL, "left");
      }
      decodeFailure |= !successL;

      bool successR = true;
      FragmentList fields2;
      if (mi2) {
        fields2 = decodeFieldsWithWarnings(*model, os, Loc((uint32_t)pc2), mi2,
                                           successR, "right");
      }
      decodeFailure |= !successR;

      // union the fields
      FieldSet allFields;
      for (const auto &f : fields1) {
        allFields.insert(&f.first);
      }
      for (const auto &f : fields2) {
        allFields.insert(&f.first);
      }

      for (const Fragment *f : allFields) {
        auto inList = [&](const FragmentList &flist) {
          auto fieldEq = [&](const FragmentListElem &p) {
            return p.first == *f;
          };
          return std::find_if(flist.begin(), flist.end(), fieldEq) !=
                 flist.end();
        };
        auto findStr = [&](const FragmentList &flist) {
          for (const auto &fp : flist) {
            if (fp.first == *f) {
              return std::string(fp.second);
            }
          }
          return std::string("");
        };

        bool inList1 = mi1 != nullptr && inList(fields1);
        uint64_t val1 = mi1 ? mi1->getFragment(*f) : 0;
        std::string valStr1 = findStr(fields1);

        bool inList2 = mi2 != nullptr && inList(fields2);
        uint64_t val2 = mi2 ? mi2->getFragment(*f) : 0;
        std::string valStr2 = findStr(fields2);

        std::string diffToken = "  ";
        if (inList1 && inList2) {
          if (val1 != val2) {
            diffToken = "~~"; // changed
            differencesFound = true;
          } else if (verbosity < 0) {
            // skip to the next field
            continue;
          } else {
            diffToken = "  ";
          }
        } else if (inList1) {
          diffToken = "--"; // removed
          differencesFound = true;
        } else if (inList2) {
          diffToken = "++"; // added
          differencesFound = true;
        } else {
          // unreachable???
          diffToken = "  ";
        }

        os << std::setw(FIELD_PC_WIDTH) << std::left << diffToken;

        // e.g. Src1.Imm32
        os << std::setw(FIELD_NAME_WIDTH) << std::left << f->name;
        os << " ";

        // e.g. [127:96]
        os << std::setw(FIELD_OFFSET_WIDTH) << std::left << fmtBitRange(*f);
        os << " ";

        // e.g. (32)
        os << std::setw(FIELD_SIZE_WIDTH) << std::left << fmtSize(*f);
        os << " ";

        // bit value
        auto fmtElem = [&](bool inList, uint64_t val, const std::string &str) {
          if (inList) {
            os << std::right << std::setw(FIELD_VALUE_INT_WIDTH) << fmtHex(val);
          } else {
            os << std::right << std::setw(FIELD_VALUE_INT_WIDTH) << " ";
          }
          os << " " << std::setw(FIELD_VALUE_STR_WIDTH) << str;
        };
        if (inList1 && val1 != val2) {
          os << Intensity::BRIGHT << Color::GREEN;
        }
        fmtElem(inList1, val1, valStr1);
        os << " ";

        if (inList2 && val1 != val2) {
          os << Intensity::BRIGHT << Color::RED;
        }
        fmtElem(inList2, val2, valStr2);

        if (val1 != val2) {
          os << Reset::RESET;
        }
        os << "\n";
      } // for
    }   // if verbose or different

    // on to the next instruction
    if (mi1) {
      pc1 += mi1->isCompact() ? 8 : 16;
    }
    if (mi2) {
      pc2 += mi2->isCompact() ? 8 : 16;
    }
  } // for all instructions in both streams (in parallel)

  return decodeFailure      ? IGA_DECODE_ERROR
         : differencesFound ? IGA_DIFF_FAILURE
                            : IGA_SUCCESS;
}

typedef int64_t Mapping;
typedef std::pair<PC, int> PCStats;
struct MappingStats {
  //    int64_t uniqueMisses = 0;

  // the specific misses (PC,num-fields-missed)
  std::vector<PCStats> misses;

  void orderMisses() {
    std::sort(misses.begin(), misses.end(),
              [](const PCStats &m1, const PCStats &m2) {
                return m1.second < m2.second;
              });
  }
};
struct CompactionStats {
  using CompactionMisses = std::map<Mapping, MappingStats>;

  int64_t hits = 0;
  int64_t misses = 0;
  int64_t noCompactSet = 0;
  int64_t noCompactForm = 0; // e.g. send
  // maps field that missed to
  //   all the values that missed
  //   each value that missed maps to:
  //       - # of times it missed
  //       - # of times this miss was the unique
  std::map<const CompactionMapping *, CompactionMisses> fieldMisses;
};

static bool listInstructionCompaction(bool useNativeDecoder, uint32_t fmtOpts,
                                      std::ostream &os,
                                      CompactionStats &cmpStats, const Model &m,
                                      size_t pc, const MInst *bits) {
  const int MAX_HAMMING_DIST = 6; // what we consider a "close"  mapping
  const int MAX_CLOSE_MAPPINGS = 6;

  MInst compactedInst;
  CompactionDebugInfo cdi;
  CompactionResult cr =
      iga::native::DebugCompaction(m, bits, &compactedInst, cdi);

  bool success = true;
  switch (cr) {
  case CompactionResult::CR_SUCCESS:
    cmpStats.hits++;
    os << Color::GREEN << Intensity::BRIGHT << "=> compaction hit "
       << Reset::RESET << "\n";
    success &= decodeFieldsForInst(useNativeDecoder, fmtOpts, os, pc, m,
                                   &compactedInst);
    break;
  case CompactionResult::CR_MISS: {
    cmpStats.misses++;
    os << Color::YELLOW << Intensity::BRIGHT << "=> compaction miss "
       << Reset::RESET << "\n";

    bool missesImm = false;
    for (const CompactionMapping *cf : cdi.fieldMisses) {
      missesImm |= cf->isSrcImmField();
    }

    for (size_t i = 0, len = cdi.fieldMisses.size(); i < len; i++) {
      Op op = cdi.fieldOps[i];
      const CompactionMapping &cf = *cdi.fieldMisses[i];
      uint64_t missedMapping = cdi.fieldMapping[i];

      auto &mms = cmpStats.fieldMisses[&cf];
      MappingStats &ms = mms[missedMapping];
      ms.misses.push_back(PCStats((PC)pc, (int)cdi.fieldMisses.size()));

      // emit the field that missed
      os << Color::WHITE << Intensity::BRIGHT << cf.index.name << Reset::RESET;
      os << ": ";
      for (int mIx = 0; mIx < (int)cf.numMappings; ++mIx) {
        if (mIx != 0) {
          os << "`";
        }
        const Field *mf = cf.mappings[mIx];
        os << mf->name;
      }
      os << " misses compaction table\n";

      // compute the total number of bits in mappings
      // and the number of hex digits needed to for output alignment
      int hexDigits = ((int)cf.countNumBitsMapped() + 4 - 1) / 4; // align up

      // emit the actual mapping we were looking for
      os << "       " << fmtHex(missedMapping, hexDigits);
      if (cf.mappings == nullptr) { // e.g. src1imm12, skip it
        os << "   [no explicit mappings] \n";
        continue;
      }
      os << ": ";
      cf.emitBinary(os, missedMapping);
      os << ": (";
      if (cf.format) {
        os << cf.format(op, missedMapping);
      }
      os << ") is the necessary table entry\n";

      // find the closest hits
      std::vector<size_t> closestIndices;
      for (size_t dist = 1; dist < MAX_HAMMING_DIST &&
                            closestIndices.size() < MAX_CLOSE_MAPPINGS;
           dist++) {
        // closest hits of distance `dist`
        for (size_t mIx = 0; mIx < cf.numValues; mIx++) {
          uint64_t val = cf.values[mIx];
          std::bitset<64> bs(val ^ missedMapping);
          if (bs.count() == dist) {
            closestIndices.push_back(mIx);
          }
        }
      } // for dist=1...8 or until we find nearest K

      // emit information on the closest mappings
      if (closestIndices.empty()) {
        os << "  no close mappings found\n";
      } else {
        os << "  closest mappings are:\n";
        for (auto cIx : closestIndices) {
          uint64_t closeMapping = cf.values[cIx];
          std::bitset<64> bs(closeMapping ^ missedMapping);
          os << "  #" << std::setw(2) << std::left << cIx;
          os << "  " << fmtHex(cf.values[cIx], hexDigits) << ": ";
          int bitOff = (int)cf.countNumBitsMapped();
          // walk through the fields listing who misses
          // (from high to low bits)
          std::vector<const Field *> missingFields;
          for (int mIx = 0; mIx < (int)cf.numMappings; ++mIx) {
            if (mIx != 0) {
              os << "`";
            }
            const Field *mf = cf.mappings[mIx];
            bitOff -= mf->length();
            auto missedVal = iga::getBits(missedMapping, bitOff, mf->length());
            auto closeVal = iga::getBits(closeMapping, bitOff, mf->length());
            if (missedVal != closeVal) {
              os << Color::RED << Intensity::BRIGHT;
              fmtBinaryDigits(os, closeVal, mf->length());
              os << Reset::RESET;
              missingFields.push_back(mf);
            } else {
              fmtBinaryDigits(os, closeVal, mf->length());
            }
          }
          os << ": ";

          commafyList(os, missingFields, [](std::ostream &os, const Field *f) {
            os << Color::RED << Intensity::BRIGHT;
            os << f->name;
            os << Reset::RESET;
          });
          if (missingFields.size() == 1) {
            os << " misses";
          } else {
            os << " miss";
          }
          os << "\n";
        }
      }
    }
    break;
  }
  case CompactionResult::CR_NO_COMPACT:
    cmpStats.noCompactSet++;
    os << Color::WHITE << "{NoCompact/Uncompacted} set" << Reset::RESET << "\n";
    break;
  case CompactionResult::CR_NO_FORMAT:
    cmpStats.noCompactForm++;
    emitRedText(os, "=> no compacted form\n");
    break;
  default:
    os << "INTERNAL ERROR: " << (int)cr << "\n";
    success = false;
  }
  return success;
}

iga_status_t iga::DebugCompaction(Platform p, int /* verbosity */,
                                  bool useNativeDecoder, uint32_t fmtOpts,
                                  std::ostream &os, const uint8_t *bits,
                                  size_t bitsLen) {
  const Model *model = Model::LookupModel(p);
  if (model == nullptr) {
    return IGA_UNSUPPORTED_PLATFORM;
  } else if (!iga::native::IsDecodeSupported(*model, DecoderOpts())) {
    return IGA_UNSUPPORTED_PLATFORM;
  }

  CompactionStats cs;

  std::ios_base::fmtflags osFlags = os.flags();

  bool success = true;
  size_t pc = 0;
  while (pc < bitsLen) {
    if (bitsLen - pc < 4) {
      emitYellowText(os, "WARNING: extra padding at end of kernel\n");
      break;
    }
    const MInst *mi = (const MInst *)&bits[pc];
    size_t iLen = mi->isCompact() ? 8 : 16;
    if (bitsLen - pc < iLen) {
      emitYellowText(os, "WARNING: extra padding at end of kernel\n");
      break;
    }

    os << "============================================================\n";
    auto syntax =
        disassembleInst(p, useNativeDecoder, fmtOpts, pc, (const void *)mi);
    os << fmtPc(mi, pc) << " " << syntax << "\n";
    os.flush();
    success &= listInstructionCompaction(useNativeDecoder, fmtOpts, os, cs,
                                         *model, pc, mi);
    pc += iLen;
  }
  os << "\n";
  os << "\n";
  os << "*************************** SUMMARY ***************************\n";
  uint64_t totalInsts =
      cs.hits + cs.misses + cs.noCompactSet + cs.noCompactForm;
  auto emitSubset = [&](const char *name, int64_t val) {
    os << "  ";
    std::string title = name;
    title += ':';
    os << std::setw(24) << std::setfill(' ') << std::left << title;
    os << std::setw(12) << std::right << val;
    os << "  ";
    double pct = 100.0 * val / (double)totalInsts;
    os << std::setprecision(3) << std::setw(8) << std::right << pct << "%";
    os << "\n";
  };
  emitSubset("hits", cs.hits);
  emitSubset("misses", cs.misses);
  emitSubset("{Uncompacted} set", cs.noCompactSet);
  emitSubset("no compact form", cs.noCompactForm);
  os << std::setw(24) << "TOTAL:" << std::setw(10) << std::right << totalInsts
     << "\n";
  os << "***************************************************************\n";
  // show the various indices that missed
  int64_t totalMisses = 0;
  for (const auto &missEntry : cs.fieldMisses) {
    const CompactionMapping *cf = missEntry.first;
    os << std::setw(24) << cf->index.name;
    int64_t misses = 0;
    for (const auto& ms : missEntry.second) {
      misses += ms.second.misses.size();
    }
    totalMisses += misses;

    os << "  ";
    os << std::setw(10) << std::right << misses;
    os << "\n";
  }

  // now list the actual misses
  os << "***************************************************************\n";
  for (const auto &missEntry : cs.fieldMisses) {
    const CompactionMapping *cf = missEntry.first;
    os << cf->index.name << "\n";

    // order the misses by the total number
    std::vector<std::pair<Mapping, MappingStats>> orderedEntries;
    for (const auto& ms : missEntry.second) {
      orderedEntries.push_back(ms);
    }
    std::sort(orderedEntries.begin(), orderedEntries.end(),
              [](const std::pair<Mapping, MappingStats> &p1,
                 const std::pair<Mapping, MappingStats> &p2) {
                return p1.second.misses.size() > p2.second.misses.size();
              });

    for (const auto &ms : orderedEntries) {
      Mapping mVal = ms.first;
      const MappingStats &mStats = ms.second;

      os << "    ";
      cf->emitBinary(os, mVal); // e.g. 000`0010`0`00
      os << "  total misses:" << mStats.misses.size();
      size_t misses1 = 0, misses2 = 0, misses3plus = 0;
      for (const auto &missExample : mStats.misses) {
        if (missExample.second == 1) {
          misses1++;
        } else if (missExample.second == 1) {
          misses2++;
        } else if (missExample.second == 2) {
          misses2++;
        } else {
          misses3plus++;
        }
      }
      os << "  1m:" << misses1 << "  2m:" << misses2 << "  3+m:" << misses3plus;

      if (cf->format) {
        os << "  ";
        Op HACK = Op::ADD; // gives - instead of ~ for bitwise,
                           // but it's the best I can do
        os << cf->format(HACK, mVal);
      }
      os << "\n";

      // emit those that miss
      // mStats.orderMisses();
      for (const auto &missExample : mStats.misses) {
        PC missPc = missExample.first;
        int totalMissesForThisPc = missExample.second;
        os << "        misses " << totalMissesForThisPc << ": "
           << disassembleInst(p, useNativeDecoder, fmtOpts, missPc,
                              bits + missPc)
           << "\n";
      }
    }
  }

  os.flags(osFlags);

  return IGA_SUCCESS;
}
