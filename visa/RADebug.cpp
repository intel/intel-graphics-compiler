/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GraphColor.h"
#include "RADebug.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/Path.h>
#include "common/LLVMWarningsPop.hpp"

// All debugging tools related implementation should go here to keep actual
// code clutter free.

using namespace vISA;

void vISA::dumpLiveRanges(GlobalRA &gra, AllIntervals &sortedIntervals) {
  // Emit Python file to draw chart.
  auto &kernel = gra.kernel;
  std::ofstream of;
  std::string fname;
  if (kernel.getOptions()->getOptionCstr(VISA_AsmFileName))
    fname =
        std::string(kernel.getOptions()->getOptionCstr(VISA_AsmFileName)) + "-";
  fname += "iter-" + std::to_string(gra.getIterNo()) + std::string(".py");
  of.open(fname, std::ofstream::out);
  of << "import matplotlib.pyplot as plt"
     << "\n";
  of << "import numpy as np"
     << "\n";
  of << "fig,ax = plt.subplots()"
     << "\n";
  unsigned int x = 10;

  // draw line from (x1, y2) -> (x2, y2)
  // lbl is either "label" or "marker". Latter is used to mark def/use
  // locations. name contains variable name to write out
  auto plot = [&of](int x1, int y1, int x2, int y2, std::string lbl,
                    std::string name) {
    {
      std::string var = "x1 = [";
      var += std::to_string(x1);
      var += std::string(", ");
      var += std::to_string(x2);
      var += std::string("]\n");

      of << var;
    }

    {
      std::string var = std::string("y1");
      var += std::string(" = [");
      var += std::to_string(y1);
      var += std::string(", ");
      var += std::to_string(y2);
      var += std::string("]\n");

      of << var;
    }

    if (lbl == "label")
      of << "th1 = ax.text(*[" << x1 << "," << y1 + 1 << "], \"" << name
         << "\", fontsize=8,rotation=90, rotation_mode='anchor')"
         << "\n";

    of << "plt.plot(x1,y1, " << lbl << " = \"" << name << "\"";
    of << ")\n\n";
  };

  VarReferences refs(kernel);
  for (auto& interval : sortedIntervals) {
    auto start = interval.interval.start;
    auto end = interval.interval.end;
    int startLexId = 0, endLexId = 0;
    if (!start || !end)
      continue;

    // Live-intervals are dumped top-down. So plot
    // them in 4th quadrant.
    startLexId = -(int)start->getLexicalId();
    endLexId = -(int)end->getLexicalId();

    plot(x, startLexId, x, endLexId, "label", interval.dcl->getName());

    // Add def/use markers
    // Def indicated by '+' sign on chart
    // Use indicated by a circle on chart
    // Note: Indirect refs are not indicated on chart as they're
    // not computed by VarReferences class.
    auto defs = refs.getDefs(interval.dcl);
    auto uses = refs.getUses(interval.dcl);

    if (defs) {
      for (auto &def : *defs) {
        auto defInst = std::get<0>(def);
        int lexId = defInst->getLexicalId();
        if (lexId >= startLexId && lexId <= endLexId)
          plot(x, -(int)lexId, x, -(int)lexId, "marker", "+");
      }
    }

    if (uses) {
      for (auto &use : *uses) {
        auto useInst = std::get<0>(use);
        int lexId = useInst->getLexicalId();
        if (lexId >= startLexId && lexId <= endLexId)
          plot(x, -(int)lexId, x, -(int)lexId, "marker", ".");
      }
    }
    ++x;
  }

  // Demarcate loops by drawing a box around them
  std::vector<Loop *> allLoops;
  for (auto loop : kernel.fg.getLoops().getTopLoops()) {
    std::stack<Loop *> nested;
    nested.push(loop);
    while (nested.size() > 0) {
      auto top = nested.top();
      allLoops.push_back(loop);
      nested.pop();
      for (auto child : top->immNested) {
        nested.push(child);
      }
    }
  }

  for (auto loop : allLoops) {
    unsigned int minLexId = 0xffffffff, maxLexId = 0;
    for (auto bb : loop->getBBs()) {
      for (auto inst : bb->getInstList()) {
        if (inst->getLexicalId() != UNDEFINED_VAL &&
            inst->getLexicalId() != 0) {
          minLexId = std::min(minLexId, inst->getLexicalId());
          maxLexId = std::max(maxLexId, inst->getLexicalId());
        }
      }
    }

    plot(0, -(int)minLexId, 0, -(int)maxLexId, "label",
         std::string("L") + std::to_string(loop->id));
    plot(x + 10, -(int)minLexId, x + 10, -(int)maxLexId, "label",
         std::string("L") + std::to_string(loop->id));
    plot(0, -(int)minLexId, x + 10, -(int)minLexId, "label",
         std::string("L") + std::to_string(loop->id));
    plot(0, -(int)maxLexId, x + 10, -(int)maxLexId, "label",
         std::string("L") + std::to_string(loop->id));
  }

  of << "plt.show()"
     << "\n";
  of.close();
}

void Interference::interferenceVerificationForSplit() const {

  std::cout << "\n\n **** Interference Verification Table ****\n";
  for (unsigned i = 0; i < maxId; i++) {
    std::cout << "(" << i << ") ";
    // lrs[i]->dump();
    for (unsigned j = 0; j < maxId; j++) {
      if (interfereBetween(i, j)) {
        if (!interfereBetween(
                gra.getSplittedDeclare(lrs[i]->getDcl())->getRegVar()->getId(),
                j) &&
            (gra.getSplittedDeclare(lrs[i]->getDcl()) != lrs[j]->getDcl())) {
          std::cout << "\t";
          lrs[j]->getVar()->emit(std::cout);
        }
      }
    }
    std::cout << "\n";
  }
}

bool Interference::linearScanVerify() const {
  std::cout << "--------------- " << kernel.getName() << " ----------------"
            << "\n";

  for (unsigned i = 0; i < maxId; i++) {
    G4_VarBase *phyReg_i = lrs[i]->getVar()->getPhyReg();
    if (!phyReg_i || !phyReg_i->isGreg() ||
        gra.isUndefinedDcl(lrs[i]->getDcl()) ||
        lrs[i]->getDcl()->getRegVar()->isNullReg()) {
      continue;
    }
    unsigned regOff_i = lrs[i]->getVar()->getPhyRegOff() *
                        lrs[i]->getVar()->getDeclare()->getElemSize();
    unsigned GRFStart_i =
        phyReg_i->asGreg()->getRegNum() * kernel.numEltPerGRF<Type_UB>() +
        regOff_i;
    unsigned elemsSize_i = lrs[i]->getVar()->getDeclare()->getTotalElems() *
                           lrs[i]->getVar()->getDeclare()->getElemSize();
    unsigned GRFEnd_i = GRFStart_i + elemsSize_i - 1;
    for (unsigned j = 0; j < maxId; j++) {
      if (interfereBetween(i, j)) {
        if (gra.isUndefinedDcl(lrs[j]->getDcl()) ||
            builder.kernel.fg.isPseudoDcl(lrs[j]->getDcl()) ||
            lrs[j]->getDcl()->getRegVar()->isNullReg()) {
          continue;
        }

        G4_VarBase *phyReg_j = lrs[j]->getVar()->getPhyReg();
        unsigned regOff_j = lrs[j]->getVar()->getPhyRegOff() *
                            lrs[j]->getVar()->getDeclare()->getElemSize();
        unsigned GRFStart_j =
            phyReg_j->asGreg()->getRegNum() * kernel.numEltPerGRF<Type_UB>() +
            regOff_j;
        unsigned elemsSize_j = lrs[j]->getVar()->getDeclare()->getTotalElems() *
                               lrs[j]->getVar()->getDeclare()->getElemSize();
        unsigned GRFEnd_j = GRFStart_j + elemsSize_j - 1;
        if (!(GRFEnd_i < GRFStart_j || GRFEnd_j < GRFStart_i)) {
          LSLiveRange *i_LSLR = gra.getLSLR(lrs[i]->getDcl());
          LSLiveRange *j_LSLR = gra.getLSLR(lrs[j]->getDcl());
          unsigned i_start = 0;
          unsigned i_end = 0;
          if (i_LSLR) // For the stack call or some other function which will
                      // add extra declares after allocation
          {
            i_LSLR->getFirstRef(i_start);
            i_LSLR->getLastRef(i_end);
          }

          unsigned j_start = 0;
          unsigned j_end = 0;
          if (j_LSLR) {
            j_LSLR->getFirstRef(j_start);
            j_LSLR->getLastRef(j_end);
          }

          std::cout << "(" << i << "," << j << ")"
                    << lrs[i]->getDcl()->getName() << "(" << GRFStart_i << ":"
                    << GRFEnd_i << ")[" << i_start << "," << i_end << "] vs "
                    << lrs[j]->getDcl()->getName() << "(" << GRFStart_j << ":"
                    << GRFEnd_j << ")[" << j_start << "," << j_end << "]"
                    << "\n";
        }
      }
    }
  }

  return true;
}

static G4_Declare *findDeclare(G4_Kernel &kernel, char *name) {
  for (G4_Declare *dcl : kernel.Declares) {
    if (!strcmp(dcl->getName(), name)) {
      return dcl;
    }
  }

  return nullptr;
}

// For RA debugging
// Add extra interference info into intf graph
void GraphColor::getExtraInterferenceInfo() {
  // Get the file name
  llvm::SmallString<32> intfName;
  const char *fileName =
      builder.getOptions()->getOptionCstr(vISA_ExtraIntfFile);
  if (!fileName) {
    // Without specific file name, assume the file name is the name of assembly
    // file + ".extraintf" extension and the file is put under the default
    // ShaderOverride path, i.e., c:\Intel\IGC\ShaderOverride on Windows or
    // /tmp/IntelIGC/ShaderOverride on Linux.
#if defined(_WIN32)
    llvm::sys::path::append(intfName, "C:", "Intel", "IGC", "ShaderOverride");
#else
    llvm::sys::path::append(intfName, "/", "tmp", "IntelIGC", "ShaderOVerride");
#endif
    vASSERT(m_options->getOptionCstr(VISA_AsmFileName));
    llvm::StringRef asmName = m_options->getOptionCstr(VISA_AsmFileName);
    llvm::sys::path::append(
        intfName, sanitizePathString(llvm::sys::path::stem(asmName).str()));
    intfName.append(".extraintf");
    fileName = intfName.c_str();
  }

  // open extra inteference info file
  FILE *intfInput = nullptr;
  if ((intfInput = fopen(fileName, "rb")) == nullptr) {
    // No file, just return;
    return;
  }

  // Read the declare variable inteference info from the file
  // The file is composed with the lines with following format in each line:
  // Keydeclare:vardeclare0,vardeclare1,vardeclare2,...
  // Which means the Keydeclare interferences with following vardeclares
  int c;
  char stringBuf[1024];
  char *buf_ptr = stringBuf;
  G4_Declare *keyDeclare = nullptr;
  G4_Declare *varDeclare = nullptr;
  while ((c = fgetc(intfInput)) != EOF) {
    if (c == '\n' || c == ',') {
      *buf_ptr = '\0'; // End of string

      // Find the declare according to the string name
      varDeclare = findDeclare(kernel, stringBuf);

      // Must have a KeyDeclare
      if (!keyDeclare) {
        vASSERT(false);
        return;
      }

      if (varDeclare) {
        int src0Id = keyDeclare->getRegVar()->getId();
        int src1Id = varDeclare->getRegVar()->getId();

        // Set inteference
        intf.checkAndSetIntf(src0Id, src1Id);
        VISA_DEBUG_VERBOSE(std::cout << keyDeclare->getName() << ":"
                                     << varDeclare->getName() << "\n");
      }

      // End of line, the key declare set to null
      if (c == '\n') {
        keyDeclare = nullptr;
      }
      buf_ptr = stringBuf;
      continue;
    }

    // Jump over space
    if (c == '\t' || c == ' ' || c == '\r') {
      continue;
    }

    //':' is the end of key declare name
    if (c == ':') {
      *buf_ptr = '\0';
      keyDeclare = findDeclare(kernel, stringBuf);
      if (!keyDeclare) {
        vASSERT(false);
        return;
      }
      buf_ptr = stringBuf;
      continue;
    }

    *buf_ptr = c;
    buf_ptr++;
  }

  fclose(intfInput);
}

void Interference::dumpInterference() const {

  std::cout << "\n\n **** Interference Table ****\n";
  for (unsigned i = 0; i < maxId; i++) {
    std::cout << "(" << i << ") ";
    lrs[i]->dump();
    std::cout << "\n";
    for (unsigned j = 0; j < maxId; j++) {
      if (interfereBetween(i, j)) {
        std::cout << "\t";
        lrs[j]->getVar()->emit(std::cout);
      }
    }
    std::cout << "\n\n";
  }
}

void Interference::dumpVarInterference() const {

  std::cout << "\n\n **** Var Interference Table ****\n";
  for (G4_Declare *decl : gra.kernel.Declares) {
    if (decl->getRegVar()->isRegAllocPartaker()) {
      unsigned i = decl->getRegVar()->getId();
      // std::cout << "(" << i << ") ";
      lrs[i]->dump();
      std::cout << "\n";
      for (G4_Declare *decl : gra.kernel.Declares) {
        if (decl->getRegVar()->isRegAllocPartaker()) {
          unsigned j = decl->getRegVar()->getId();
          if (interfereBetween(i, j)) {
            std::cout << "\t";
            lrs[j]->getVar()->emit(std::cout);
          }
        }
      }
      std::cout << "\n\n";
    }
  }
}

void GlobalRA::reportUndefinedUses(LivenessAnalysis &liveAnalysis, G4_BB *bb,
                                   G4_INST *inst, G4_Declare *referencedDcl,
                                   std::set<G4_Declare *> &defs,
                                   Gen4_Operand_Number opndNum) {
  // Get topmost dcl
  while (referencedDcl->getAliasDeclare()) {
    referencedDcl = referencedDcl->getAliasDeclare();
  }

  if (referencedDcl->getAddressed() == true) {
    // Don't run analysis for addressed opnds.
    // Specifically, we don't analyze following,
    //
    // A0 = &V1
    // r[A0] = 0 <-- V1 indirectly defined
    // ... = V1 <-- Use-before-def warning for V1 skipped due to indirect def
    //
    return;
  }

  if (referencedDcl->getRegVar()->isRegAllocPartaker()) {
    [[maybe_unused]] const char *opndName = "";

    if (opndNum == Opnd_pred) {
      opndName = "predicate";
    } else if (opndNum == Opnd_src0) {
      opndName = "src0";
    } else if (opndNum == Opnd_src1) {
      opndName = "src1";
    } else if (opndNum == Opnd_src2) {
      opndName = "src2";
    }

    unsigned id = referencedDcl->getRegVar()->getId();
    if (liveAnalysis.def_in[bb->getId()].test(id) == false &&
        defs.find(referencedDcl) == defs.end()) {
      // Def not found for use so report it
      VISA_DEBUG_VERBOSE({
        std::cout << "Def not found for use " << referencedDcl->getName()
                  << " (" << opndName << ") at CISA offset "
                  << inst->getVISAId() << ", src line " << inst->getLineNo()
                  << ":\n";
        inst->emit(std::cout);
        std::cout << "\n\n";
      });
    }
  }
}

void VerifyAugmentation::verifyAlign(G4_Declare *dcl) {
  // Verify that dcl with Default32Bit align mask are 2GRF aligned
  auto it = masks.find(dcl);
  if (it == masks.end())
    return;

  if (gra->use4GRFAlign) {
    auto augMask = std::get<1>((*it).second);
    if (augMask == AugmentationMasks::Default64Bit) {
      auto assignment = dcl->getRegVar()->getPhyReg();
      if (assignment && assignment->isGreg()) {
        auto phyRegNum = assignment->asGreg()->getRegNum();
        if (phyRegNum % 4 != 0) {
          printf("Dcl %s is Default64Bit but assignment is not 4GRF aligned "
                 "(gra.getAugAlign() = %d)\n",
                 dcl->getName(), gra->getAugAlign(dcl));
        }
      }
    } else if (augMask == AugmentationMasks::Default32Bit) {
      auto assignment = dcl->getRegVar()->getPhyReg();
      if (assignment && assignment->isGreg()) {
        auto phyRegNum = assignment->asGreg()->getRegNum();
        auto augMask = std::get<1>((*it).second);
        if (phyRegNum % 2 != 0 && augMask == AugmentationMasks::Default32Bit) {
          printf("Dcl %s is Default32Bit but assignment is not Even aligned "
                 "(gra.getAugAlign() = %d)\n",
                 dcl->getName(), gra->getAugAlign(dcl));
        }
      }
    }
  } else {
    if (dcl->getByteSize() >=
            kernel->numEltPerGRF<Type_UD>() * TypeSize(Type_UD) &&
        dcl->getByteSize() <=
            2 * kernel->numEltPerGRF<Type_UD>() * TypeSize(Type_UD) &&
        kernel->getSimdSize() > kernel->numEltPerGRF<Type_UD>()) {
      auto assignment = dcl->getRegVar()->getPhyReg();
      if (assignment && assignment->isGreg()) {
        auto phyRegNum = assignment->asGreg()->getRegNum();
        auto augMask = std::get<1>((*it).second);
        if (phyRegNum % 2 != 0 && augMask == AugmentationMasks::Default32Bit) {
          printf("Dcl %s is Default32Bit but assignment is not Even aligned\n",
                 dcl->getName());
        }
      }
    }
  }
}

void VerifyAugmentation::dump(const char *dclName) {
  std::string dclStr = dclName;
  for (auto &m : masks) {
    std::string first = m.first->getName();
    if (first == dclStr) {
      printf("%s, %d, %s\n", dclName, m.first->getRegVar()->getId(),
             getStr(std::get<1>(m.second)));
    }
  }
}

void VerifyAugmentation::labelBBs() {
  std::string prev = "X:";
  unsigned id = 0;
  for (auto bb : kernel->fg) {
    if (bbLabels.find(bb) == bbLabels.end())
      bbLabels[bb] = prev;
    else
      prev = bbLabels[bb];

    if (bb->back()->opcode() == G4_opcode::G4_if) {
      auto TBB = bb->Succs.front();
      auto FBB = bb->Succs.back();

      bool hasEndif = false;
      for (auto inst : *FBB) {
        if (inst->opcode() == G4_opcode::G4_endif) {
          hasEndif = true;
          break;
        }
      }

      bbLabels[TBB] = prev + "T" + std::to_string(id) + ":";

      if (!hasEndif) {
        // else
        bbLabels[FBB] = prev + "F" + std::to_string(id) + ":";
      } else {
        // endif block
        bbLabels[FBB] = prev;
      }

      prev = prev + "T" + std::to_string(id) + ":";

      id++;
    } else if (bb->back()->opcode() == G4_opcode::G4_else) {
      auto succBB = bb->Succs.front();
      auto lbl = prev;
      lbl.pop_back();
      while (lbl.back() != ':') {
        lbl.pop_back();
      }

      bbLabels[succBB] = lbl;
    } else if (bb->back()->opcode() == G4_opcode::G4_endif) {
    }
  }

  for (auto bb : kernel->fg) {
    printf("BB%d -> %s\n", bb->getId(), bbLabels[bb].data());
  }
}

unsigned VerifyAugmentation::getGRFBaseOffset(const G4_Declare *dcl) const {
  unsigned regNum = dcl->getRegVar()->getPhyReg()->asGreg()->getRegNum();
  unsigned regOff = dcl->getRegVar()->getPhyRegOff();
  auto type = dcl->getElemType();
  return (regNum * kernel->numEltPerGRF<Type_UB>()) + (regOff * TypeSize(type));
}

bool VerifyAugmentation::interfereBetween(G4_Declare *dcl1, G4_Declare *dcl2) {
  bool interferes = true;
  unsigned v1 = dcl1->getRegVar()->getId();
  unsigned v2 = dcl2->getRegVar()->getId();
  bool v1Partaker = dcl1->getRegVar()->isRegAllocPartaker();
  bool v2Partaker = dcl2->getRegVar()->isRegAllocPartaker();

  if (v1Partaker && v2Partaker) {
    auto interferes = intf->interfereBetween(v1, v2);
    if (!interferes) {
      if (dcl1->getIsPartialDcl()) {
        interferes |= intf->interfereBetween(
            gra->getSplittedDeclare(dcl1)->getRegVar()->getId(), v2);
        if (dcl2->getIsPartialDcl()) {
          interferes |= intf->interfereBetween(
              v1, gra->getSplittedDeclare(dcl2)->getRegVar()->getId());
          interferes |= intf->interfereBetween(
              gra->getSplittedDeclare(dcl1)->getRegVar()->getId(),
              gra->getSplittedDeclare(dcl2)->getRegVar()->getId());
        }
      } else if (dcl2->getIsPartialDcl()) {
        interferes |= intf->interfereBetween(
            v1, gra->getSplittedDeclare(dcl2)->getRegVar()->getId());
      }
    }
    return interferes;
  } else if (!v1Partaker && v2Partaker) {
    // v1 is assigned by LRA
    unsigned startGRF = dcl1->getRegVar()->getPhyReg()->asGreg()->getRegNum();
    unsigned numGRFs = dcl1->getNumRows();

    for (unsigned grf = startGRF; grf != (startGRF + numGRFs); grf++) {
      for (unsigned var = 0; var != numVars; var++) {
        if (lrs[var] &&
            lrs[var]->getPhyReg() ==
                kernel->fg.builder->phyregpool.getGreg(grf) &&
            std::string(lrs[var]->getVar()->getName()) ==
                "r" + std::to_string(grf)) {
          if (!intf->interfereBetween(var, v2)) {
            interferes = false;
          }
        }
      }
    }
  } else if (v1Partaker && !v2Partaker) {
    return interfereBetween(dcl2, dcl1);
  } else if (!v1Partaker && !v2Partaker) {
    // both assigned by LRA
    if (dcl1->getRegFile() == G4_RegFileKind::G4_GRF &&
        dcl2->getRegFile() == G4_RegFileKind::G4_GRF) {
      auto lr1 = gra->getLocalLR(dcl1);
      auto lr2 = gra->getLocalLR(dcl2);

      if (lr1->getAssigned() && lr2->getAssigned()) {
        auto preg1Start = getGRFBaseOffset(dcl1);
        auto preg2Start = getGRFBaseOffset(dcl2);
        auto preg1End = preg1Start + dcl1->getByteSize();
        auto preg2End = preg2Start + dcl2->getByteSize();

        if (preg2Start >= preg1Start && preg2Start < preg1End) {
          return false;
        } else if (preg1Start >= preg2Start && preg1Start < preg2End) {
          return false;
        }
      }
    }

    interferes = true;
  }

  return interferes;
}

void VerifyAugmentation::verify() {
  std::cout << "Start verification for kernel: "
            << kernel->getOptions()->getOptionCstr(VISA_AsmFileName) << "\n";

  for (auto dcl : kernel->Declares) {
    if (dcl->getIsSplittedDcl()) {
      auto &tup = masks[dcl];
      std::cout << dcl->getName() << "(" << getStr(std::get<1>(tup))
                << ") is split"
                << "\n";
      for (const G4_Declare *subDcl : gra->getSubDclList(dcl)) {
        auto &tupSub = masks[subDcl];
        std::cout << "\t" << subDcl->getName() << " ("
                  << getStr(std::get<1>(tupSub)) << ")"
                  << "\n";
      }
    }
  }

  std::cout << "\n"
            << "\n"
            << "\n";

  auto overlapDcl = [this](G4_Declare *dcl1, G4_Declare *dcl2) {
    if (dcl1->getRegFile() == G4_RegFileKind::G4_GRF &&
        dcl2->getRegFile() == G4_RegFileKind::G4_GRF) {
      auto preg1Start = getGRFBaseOffset(dcl1);
      auto preg2Start = getGRFBaseOffset(dcl2);
      auto preg1End = preg1Start + dcl1->getByteSize();
      auto preg2End = preg2Start + dcl2->getByteSize();

      if (preg2Start >= preg1Start && preg2Start < preg1End) {
        return true;
      } else if (preg1Start >= preg2Start && preg1Start < preg2End) {
        return true;
      }
    }
    return false;
  };

  std::list<QueueEntry> active;
  for (auto &segment : sortedLiveRanges) {
    auto *dcl = segment.dcl;
    const auto &startEnd = segment.interval;
    auto &tup = masks[dcl];
    unsigned startIdx = startEnd.start->getLexicalId();
    auto dclMask = std::get<1>(tup);

    auto getMaskStr = [](AugmentationMasks m) {
      std::string str = "Undetermined";
      if (m == AugmentationMasks::Default16Bit)
        str = "Default16Bit";
      else if (m == AugmentationMasks::Default32Bit)
        str = "Default32Bit";
      else if (m == AugmentationMasks::Default64Bit)
        str = "Default64Bit";
      else if (m == AugmentationMasks::NonDefault)
        str = "NonDefault";
      else if (m == AugmentationMasks::DefaultPredicateMask)
        str = "DefaultPredicateMask";

      return str;
    };

    std::cout << dcl->getName() << " - " << getMaskStr(dclMask);
    auto augAlign = gra->getAugAlign(dcl);
    std::cout << " (align = " << augAlign << "GRF)";
    std::cout << "\n";

    if (callDclMap.count(dcl))
      continue;

    verifyAlign(dcl);

    for (auto it = active.begin(); it != active.end();) {
      auto &curSegment = (*it).interval;
      if (startIdx >= curSegment.end->getLexicalId()) {
        it = active.erase(it);
        continue;
      }
      it++;
    }

    for (auto &curSegment : active) {
      auto activeDcl = curSegment.dcl;
      auto &tupActive = masks[activeDcl];
      auto aDclMask = std::get<1>(tupActive);

      if (dclMask != aDclMask) {
        bool interfere = interfereBetween(activeDcl, dcl);

        if (activeDcl->getIsPartialDcl() || dcl->getIsPartialDcl())
          continue;

        if (!interfere) {
          std::cout << dcl->getRegVar()->getName() << "(" << getStr(dclMask)
                    << ") and " << activeDcl->getRegVar()->getName() << "("
                    << getStr(aDclMask)
                    << ") are overlapping with incompatible emask but not "
                       "masked as interfering"
                    << "\n";
        }

        if (overlapDcl(activeDcl, dcl)) {
          if (!interfere) {
            std::cout << dcl->getRegVar()->getName() << "(" << getStr(dclMask)
                      << ") and " << activeDcl->getName() << "("
                      << getStr(aDclMask)
                      << ") use overlapping physical assignments but not "
                         "marked as interfering"
                      << "\n";
          }
        }
      }
    }

    active.push_back(segment);
  }

  std::cout << "\nProgram has "
            << (intf->numVarsWithWeakEdges() == 0 ? "no" : "")
            << " vars with weak edges\n";

  std::cout << "End verification for kernel: "
            << kernel->getOptions()->getOptionCstr(VISA_AsmFileName) << "\n"
            << "\n"
            << "\n";
}

void VerifyAugmentation::populateBBLexId() {
  for (auto bb : kernel->fg) {
    if (bb->size() > 0)
      BBLexId.push_back(std::make_tuple(bb, bb->front()->getLexicalId(),
                                        bb->back()->getLexicalId()));
  }
}

bool VerifyAugmentation::isClobbered(LiveRange *lr, std::string &msg) {
  msg.clear();

  auto &tup = masks[lr->getDcl()];
  auto &segments = std::get<2>(tup);

  for (auto &curSegment : segments) {
    auto startLexId = curSegment.start->getLexicalId();
    auto endLexId = curSegment.end->getLexicalId();

    std::vector<std::pair<G4_INST *, G4_BB *>> insts;
    std::vector<std::tuple<INST_LIST_ITER, G4_BB *>> defs;
    std::vector<std::tuple<INST_LIST_ITER, G4_BB *>> uses;

    for (auto bb : kernel->fg) {
      if (bb->size() == 0)
        continue;

      if (bb->back()->getLexicalId() > endLexId &&
          bb->front()->getLexicalId() > endLexId)
        continue;

      if (bb->back()->getLexicalId() < startLexId &&
          bb->front()->getLexicalId() < startLexId)
        continue;

      // lr is active in current bb
      for (auto instIt = bb->begin(), end = bb->end(); instIt != end;
           instIt++) {
        auto inst = (*instIt);
        if (inst->isPseudoKill())
          continue;

        if (inst->getLexicalId() > startLexId &&
            inst->getLexicalId() <= endLexId) {
          insts.push_back(std::make_pair(inst, bb));
          auto dst = inst->getDst();
          if (dst && dst->isDstRegRegion()) {
            auto topdcl = dst->asDstRegRegion()->getTopDcl();
            if (topdcl == lr->getDcl())
              defs.push_back(std::make_tuple(instIt, bb));
          }

          for (unsigned i = 0, numSrc = inst->getNumSrc(); i != numSrc; i++) {
            auto src = inst->getSrc(i);
            if (src && src->isSrcRegRegion()) {
              auto topdcl = src->asSrcRegRegion()->getTopDcl();
              if (topdcl == lr->getDcl())
                uses.push_back(std::make_tuple(instIt, bb));
            }
          }
        }
      }
    }

    for (auto &use : uses) {
      auto &useStr = bbLabels[std::get<1>(use)];
      auto inst = *std::get<0>(use);
      vISA_ASSERT(useStr.size() > 0, "empty string found");
      std::list<std::tuple<G4_INST *, G4_BB *>> rd;

      for (unsigned i = 0, numSrc = inst->getNumSrc(); i != numSrc; i++) {
        auto src = inst->getSrc(i);
        if (src && src->isSrcRegRegion() &&
            src->asSrcRegRegion()->getTopDcl() == lr->getDcl()) {
          unsigned lb = 0, rb = 0;
          lb = lr->getPhyReg()->asGreg()->getRegNum() *
                   kernel->numEltPerGRF<Type_UB>() +
               (lr->getPhyRegOff() * lr->getDcl()->getElemSize());
          lb += src->getLeftBound();
          rb = lb + src->getRightBound() - src->getLeftBound();

          for (auto &otherInsts : insts) {
            if (otherInsts.first->getLexicalId() > inst->getLexicalId())
              break;

            auto oiDst = otherInsts.first->getDst();
            auto oiBB = otherInsts.second;
            if (oiDst && oiDst->isDstRegRegion() && oiDst->getTopDcl()) {
              unsigned oilb = 0, oirb = 0;
              auto oiLR = DclLRMap[oiDst->getTopDcl()];
              if (!oiLR->getPhyReg())
                continue;

              oilb = oiLR->getPhyReg()->asGreg()->getRegNum() *
                         kernel->numEltPerGRF<Type_UB>() +
                     (oiLR->getPhyRegOff() * oiLR->getDcl()->getElemSize());
              oilb += oiDst->getLeftBound();
              oirb = oilb + oiDst->getRightBound() - oiDst->getLeftBound();

              if (oilb <= (unsigned)rb && oirb >= (unsigned)lb) {
                rd.push_back(std::make_tuple(otherInsts.first, oiBB));
              }
            }
          }
        }
      }

      auto isComplementary = [](std::string &cur, std::string &other) {
        if (cur.size() < other.size())
          return false;

        if (cur.substr(0, other.size() - 1) ==
            other.substr(0, other.size() - 1)) {
          char lastAlphabet = cur.at(other.size() - 1);
          if (lastAlphabet == 'T' && other.back() == 'F')
            return true;
          if (lastAlphabet == 'F' && other.back() == 'T')
            return true;
        }

        return false;
      };

      auto isSameEM = [](G4_INST *inst1, G4_INST *inst2) {
        if (inst1->getMaskOption() == inst2->getMaskOption() &&
            inst1->getMaskOffset() == inst2->getMaskOffset())
          return true;
        return false;
      };

      if (rd.size() > 0) {
        printf("Current use str = %s for inst:\t", useStr.data());
        inst->emit(std::cout);
        printf("\t$%d\n", inst->getVISAId());
      }
      // process all reaching defs
      for (auto rid = rd.begin(); rid != rd.end();) {
        auto &reachingDef = (*rid);

        auto &str = bbLabels[std::get<1>(reachingDef)];

        // skip rd if it is from complementary branch
        if (isComplementary(str, useStr) &&
            isSameEM(inst, std::get<0>(reachingDef))) {
          rid = rd.erase(rid);
          continue;
        }
        rid++;
      }

      // keep rd that appears last in its BB
      for (auto rid = rd.begin(); rid != rd.end();) {
        auto ridBB = std::get<1>(*rid);
        for (auto rid1 = rd.begin(); rid1 != rd.end();) {
          if (*rid == *rid1) {
            rid1++;
            continue;
          }

          auto rid1BB = std::get<1>(*rid1);
          if (ridBB == rid1BB && std::get<0>(*rid)->getLexicalId() >
                                     std::get<0>(*rid1)->getLexicalId()) {
            rid1 = rd.erase(rid1);
            continue;
          }
          rid1++;
        }

        if (rid != rd.end())
          rid++;
      }

      if (rd.size() > 0) {
        bool printed = false;
        // display left overs in rd from different dcl
        for (auto &reachingDef : rd) {
          if (std::get<0>(reachingDef)->getDst()->getTopDcl() ==
              lr->getDcl()->getRootDeclare())
            continue;

          if (inst->getVISAId() == std::get<0>(reachingDef)->getVISAId())
            continue;

          if (!printed) {
            printf("\tLeft-over rd:\n");
            printed = true;
          }
          printf("\t");
          std::get<0>(reachingDef)->emit(std::cout);
          printf("\t$%d\n", std::get<0>(reachingDef)->getVISAId());
        }
      }
    }
  }
  return false;
}

void VerifyAugmentation::loadAugData(AllIntervals &s, const LiveRangeVec &l,
                                     const CALL_DECL_MAP& c, unsigned n,
                                     const Interference *i, GlobalRA &g) {
  reset();
  sortedLiveRanges = s;
  gra = &g;
  kernel = &gra->kernel;
  lrs = l;
  numVars = n;
  intf = i;
  callDclMap = c;

  for (unsigned i = 0; i != numVars; i++) {
    DclLRMap[lrs[i]->getDcl()] = lrs[i];
  }
  for (auto& entry : sortedLiveRanges) {
    auto dcl = entry.dcl;
    if (dcl->getRegFile() == G4_RegFileKind::G4_GRF ||
        dcl->getRegFile() == G4_RegFileKind::G4_INPUT) {
      LiveRange *lr = nullptr;
      auto it = DclLRMap.find(dcl);
      if (it != DclLRMap.end()) {
        lr = it->second;
      }
      if (masks.count(dcl) == 0) {
        std::unordered_set<Interval, CustomHash> segment;
        segment.insert(Interval(entry.interval.start, entry.interval.end));
        masks[dcl] = std::make_tuple(lr, gra->getAugmentationMask(dcl),
                                     segment);
      } else {
        auto &segmentSet = std::get<2>(masks[dcl]);
        Interval segment(entry.interval.start, entry.interval.end);
        segmentSet.insert(segment);
      }
    }
  }
}

bool dump(const char *s, const LiveRangeVec &lrs, unsigned size) {
  // Utility function to dump lr from name.
  // Returns true if lr name found.
  std::string name = s;
  for (unsigned i = 0; i != size; i++) {
    auto lr = lrs[i];
    if (lr && name.compare(lr->getVar()->getName()) == 0) {
      lr->dump();
      return true;
    }
  }
  return false;
}

bool dump(const char *s, const G4_Kernel *kernel) {
  // Utility function to dump dcl for given variable name.
  // Returns true if variable found.
  std::string name = s;
  for (auto dcl : kernel->Declares) {
    if (name.compare(dcl->getName()) == 0) {
      dcl->dump();
      return true;
    }
  }
  return false;
}

bool Interference::dumpIntf(const char *s) const {
  // Utility function to dump intf for given variable based on name.
  // Returns true if variable found.
  std::cout << "\n\n **** Interference Table ****\n";
  for (unsigned i = 0; i < maxId; i++) {
    std::string name = lrs[i]->getVar()->getName();
    if (name.compare(s) == 0) {
      std::cout << "(" << i << ") ";
      lrs[i]->dump();
      std::cout << "\n";
      for (unsigned j = 0; j < maxId; j++) {
        if (interfereBetween(i, j)) {
          std::cout << "\t";
          lrs[j]->getVar()->emit(std::cout);
        }
      }
      std::cout << "\n";
      return true;
    }
  }
  return false;
}

// sortedIntervals comes from augmentation.
// This can be invoked either post RA where phy regs are assigned to dcls,
// or after assignColors with lrs and numLRs passed which makes this function
// use temp allocations from lrs. Doesnt handle sub-routines yet.
void RegChartDump::dumpRegChart(std::ostream &os, const LiveRangeVec &lrs,
                                unsigned numLRs) {
  constexpr unsigned N = 1024;
  vISA_ASSERT(N >= gra.kernel.getNumRegTotal(), "more GRFs than N");
  std::unordered_map<G4_INST *, std::bitset<N>> busyGRFPerInst;
  bool dumpHex = false;

  auto getPhyReg = [&](const G4_Declare *dcl) {
    auto preg = dcl->getRegVar()->getPhyReg();
    if (preg)
      return preg;

    for (unsigned i = 0; i != numLRs; i++) {
      const LiveRange *lr = lrs[i];
      if (lr->getDcl() == dcl) {
        preg = lr->getPhyReg();
        break;
      }
    }

    return preg;
  };

  for (auto& segment : sortedLiveIntervals) {
    auto *dcl = segment.dcl;
    if (dcl->getRegFile() != G4_RegFileKind::G4_GRF &&
        dcl->getRegFile() != G4_RegFileKind::G4_INPUT)
      continue;

    auto phyReg = getPhyReg(dcl);
    if (!phyReg)
      continue;

    if (!phyReg->isGreg())
      continue;

    auto GRFStart = phyReg->asGreg()->getRegNum();
    auto numRows = dcl->getNumRows();

    auto startInst = startEnd[dcl].first;
    auto endInst = startEnd[dcl].second;

    bool start = false;
    if (gra.kernel.fg.getIsStackCallFunc()) {
      start = (dcl->getRegFile() & G4_RegFileKind::G4_INPUT);
    } else {
      // Pre-defined %arg dcl uses G4_INPUT RegFileKind but it isn't live-in
      // to a kernel. So make an exception.
      if (!gra.kernel.fg.builder->isPreDefArg(dcl))
        start = (dcl->getRegFile() & G4_RegFileKind::G4_INPUT);
    }
    bool done = false;
    for (auto bb : gra.kernel.fg) {
      for (auto inst : *bb) {
        if (inst == startInst) {
          start = true;
          continue;
        }

        if (!start)
          continue;

        for (unsigned i = GRFStart; i != (GRFStart + numRows); i++) {
          busyGRFPerInst[inst].set(i, true);
        }

        if (inst == endInst || endInst == startInst) {
          done = true;
          break;
        }
      }

      if (done)
        break;
    }
  }

  // Now emit instructions with GRFs
  for (auto bb : gra.kernel.fg) {
    for (auto inst : *bb) {
      constexpr unsigned maxInstLen = 80;
      auto item = busyGRFPerInst[inst];
      std::stringstream ss;
      inst->emit(ss);
      auto len = ss.str().length();

      if (inst->isLabel()) {
        os << ss.str() << "\n";
        continue;
      }

      if (len <= maxInstLen) {
        os << ss.str();
        for (unsigned i = 0; i != maxInstLen - ss.str().length(); i++)
          os << " ";
      } else {
        auto tmpStr = ss.str();
        auto limitedStr = tmpStr.substr(0, maxInstLen);
        os << limitedStr;
      }

      os << "        ";

      if (!dumpHex) {
        // dump GRFs | - busy, * - free
        for (unsigned i = 0; i != gra.kernel.getNumRegTotal(); i++) {
          // emit in groups of 10 GRFs
          if (i > 0 && (i % 10) == 0)
            os << "  ";

          if (item[i] == true)
            os << "|"; // busy
          else
            os << "*"; // free
        }
      } else {
        for (unsigned i = 0; i != N; i += sizeof(unsigned short) * 8) {
          unsigned short busyGRFs = 0;
          for (unsigned j = 0; j != sizeof(unsigned short) * 8; j++) {
            auto offset = i + j;
            if (offset < N) {
              if (item[offset])
                busyGRFs |= (1 << j);
            }
          }
          printf("r%d:%4x      ", i, busyGRFs);
        }
      }
      os << "\n";
    }
    os << "\n";
  }
}

void RegChartDump::recordLiveIntervals(const AllIntervals &dcls) {
  sortedLiveIntervals = dcls;
  for (auto& segment : dcls) {
    auto *dcl = segment.dcl;
    auto start = segment.interval.start;
    auto end = segment.interval.end;
    startEnd.insert(std::make_pair(dcl, std::make_pair(start, end)));
  }
}

void DynPerfModel::run() {
  char LocalBuffer[1024];
  for (auto BB : Kernel.fg.getBBList()) {
    for (auto Inst : BB->getInstList()) {
      if (Inst->isLabel() || Inst->isPseudoKill())
        continue;

      if (Inst->isSpillIntrinsic())
        NumSpills++;
      if (Inst->isFillIntrinsic())
        NumFills++;

      auto InnerMostLoop = Kernel.fg.getLoops().getInnerMostLoop(BB);
      auto NestingLevel = InnerMostLoop ? InnerMostLoop->getNestingLevel() : 0;
      if (Inst->isFillIntrinsic()) {
        FillDynInst +=
            (unsigned int)std::pow<unsigned int>(10, NestingLevel) * 1;
      } else if (Inst->isSpillIntrinsic()) {
        SpillDynInst +=
            (unsigned int)std::pow<unsigned int>(10, NestingLevel) * 1;
      }
      TotalDynInst +=
          (unsigned int)std::pow<unsigned int>(10, NestingLevel) * 1;
    }
  }

  std::stack<Loop *> Loops;
  for (auto Loop : Kernel.fg.getLoops().getTopLoops()) {
    Loops.push(Loop);
  }
  while (Loops.size() > 0) {
    auto CurLoop = Loops.top();
    Loops.pop();
    unsigned int FillCount = 0;
    unsigned int SpillCount = 0;
    unsigned int TotalCount = 0;
    unsigned int LoopLevel = CurLoop->getNestingLevel();
    if (SpillFillPerNestingLevel.size() <= LoopLevel)
      SpillFillPerNestingLevel.resize(LoopLevel + 1);
    std::get<0>(SpillFillPerNestingLevel[LoopLevel]) += 1;
    for (auto BB : CurLoop->getBBs()) {
      for (auto Inst : BB->getInstList()) {
        if (Inst->isLabel() || Inst->isPseudoKill())
          continue;
        TotalCount++;
        if (Inst->isFillIntrinsic()) {
          FillCount++;
          std::get<2>(SpillFillPerNestingLevel[LoopLevel]) += 1;
        } else if (Inst->isSpillIntrinsic()) {
          SpillCount++;
          std::get<1>(SpillFillPerNestingLevel[LoopLevel]) += 1;
        }
      }
    }

    sprintf_s(LocalBuffer, sizeof(LocalBuffer),
              "Loop %d @ level %d: %d total, %d fill, %d spill, %d subroutine "
              "calls\n",
              CurLoop->id, LoopLevel, TotalCount, FillCount, SpillCount,
              CurLoop->subCalls);
    Buffer += std::string(LocalBuffer);

    for (auto Child : CurLoop->immNested)
      Loops.push(Child);
  };
}

void DynPerfModel::dump() {
  char LocalBuffer[1024];

  unsigned int InstCount = 0;
  for (auto BB : Kernel.fg.getBBList()) {
    for (auto Inst : BB->getInstList()) {
      if (!Inst->isLabel() && !Inst->isPseudoKill())
        InstCount++;
    }
  }
  auto AsmName = Kernel.getOptions()->getOptionCstr(VISA_AsmFileName);
  auto SpillSize = Kernel.fg.builder->getJitInfo()->stats.spillMemUsed;
  sprintf_s(LocalBuffer, sizeof(LocalBuffer),
            "Kernel name: %s\n # BBs : %d\n # Asm Insts: %d\n # RA Iters = "
            "%d\n Spill Size = %d\n # Spills: %d\n # Fills : %d\n",
            AsmName, (int)Kernel.fg.getBBList().size(), InstCount, NumRAIters,
            SpillSize, NumSpills, NumFills);
  Buffer += std::string(LocalBuffer);

  sprintf_s(LocalBuffer, sizeof(LocalBuffer),
            "Total dyn inst: %llu\nFill dyn inst: %llu\nSpill dyn inst: %llu\n",
            TotalDynInst, FillDynInst, SpillDynInst);
  Buffer += std::string(LocalBuffer);

  sprintf_s(LocalBuffer, sizeof(LocalBuffer),
            "Percent Fill/Total dyn inst: %f\n",
            (double)FillDynInst /
                ((double)TotalDynInst > 0 ? (double)TotalDynInst : 1) * 100.0f);
  Buffer += std::string(LocalBuffer);

  for (unsigned int I = 1; I != SpillFillPerNestingLevel.size() &&
                           SpillFillPerNestingLevel.size() > 0;
       ++I) {
    sprintf_s(LocalBuffer, sizeof(LocalBuffer), "LL%d(#%d): {S-%d, F-%d}, ", I,
              std::get<0>(SpillFillPerNestingLevel[I]),
              std::get<1>(SpillFillPerNestingLevel[I]),
              std::get<2>(SpillFillPerNestingLevel[I]));
    Buffer += std::string(LocalBuffer);
  }

  std::string FN = AsmName;
  FN += std::string("-dyn-perf.txt");
  std::ofstream OF;
  OF.open(FN, std::ofstream::out);
  OF << Buffer << "\n";
  OF.close();

  std::cout << Buffer << "\n";
}

SpillAnalysis::~SpillAnalysis() {
  if (Refs) {
    delete Refs;
    Refs = nullptr;
  }
}

void SpillAnalysis::Dump(std::ostream &OS) {
  auto &GRA = GC->getGRA();
  auto &Kernel = GRA.kernel;
  auto &Loops = Kernel.fg.getLoops();
  const auto &Spills = GC->getSpilledLiveRanges();
  std::unordered_map<G4_INST *, G4_BB *> InstBBMap;

  for (auto *BB : Kernel.fg.getBBList())
    for (auto *Inst : BB->getInstList())
      InstBBMap[Inst] = BB;

  OS << "Name, Dcl Byte Size, Spill Cost, Degree, #Defs, #Uses, Distance, "
        "#BBs, All BBs Where Live"
     << "\n";

  for (auto *Spill : Spills) {
    // dump - {Dcl size, Spill cost, Live BBs (loop annotation)}
    auto Dcl = Spill->getDcl();
    auto DclSizeBytes = Dcl->getByteSize();
    auto SpillCost = Spill->getSpillCost();
    auto Degree = DclDegree[Dcl];
    auto LiveBBs = GetLiveBBs(Dcl, InstBBMap);
    auto Distance = GetDistance(Dcl);
    auto NumDefs = Refs->getDefCount(Dcl);
    auto NumUses = Refs->getUseCount(Dcl);

    OS << Dcl->getName() << "," << DclSizeBytes << ", " << SpillCost << ", "
       << Degree << ", " << NumDefs << ", " << NumUses << ", " << Distance
       << ", " << LiveBBs.size() << ", ";

    for (auto *LiveBB : LiveBBs) {
      OS << "BB" << LiveBB->getId();
      auto *ClosestLoop = Loops.getInnerMostLoop(LiveBB);
      if (ClosestLoop) {
        OS << " [L" << ClosestLoop->id << "]";
      }
      OS << " ";
    }

    OS << "\n";
  }
}

unsigned int SpillAnalysis::GetDistance(G4_Declare *Dcl) {
  if (AugIntervals.count(Dcl) == 0) {
    // Construct distance in conventional way
    auto &Kernel = GC->getGRA().kernel;
    unsigned int Start = 0xffffffff, End = 0x0;

    auto *Defs = Refs->getDefs(Dcl);
    auto *Uses = Refs->getUses(Dcl);

    for (auto &Def : *Defs) {
      auto *DefInst = std::get<0>(Def);
      Start = std::min(Start, DefInst->getLexicalId());
      End = std::max(End, DefInst->getLexicalId());
    }

    for (auto &Use : *Uses) {
      auto *UseInst = std::get<0>(Use);
      Start = std::min(Start, UseInst->getLexicalId());
      End = std::max(End, UseInst->getLexicalId());
    }

    for (auto *BB : Kernel.fg.getBBList()) {
      if (LA->isLiveAtEntry(BB, Dcl->getRegVar()->getId()))
        Start = std::min(Start, BB->front()->getLexicalId());
      if (LA->isLiveAtExit(BB, Dcl->getRegVar()->getId()))
        End = std::max(End, BB->back()->getLexicalId());
    }

    return End - Start;
  }

  // Return augmentation distance when available
  auto Distance = AugIntervals[Dcl];
  return Distance.second->getLexicalId() - Distance.first->getLexicalId();
}

void SpillAnalysis::LoadAugIntervals(AllIntervals &SortedIntervals,
                                     GlobalRA &GRA) {
  for (auto &Segment : SortedIntervals) {
    auto *LR = Segment.dcl;
    auto *Start = GRA.getLastStartInterval(LR);
    auto *End = GRA.getLastEndInterval(LR);
    AugIntervals[LR] = std::make_pair(Start, End);
  }
}

void SpillAnalysis::LoadDegree(G4_Declare *Dcl, unsigned int degree) {
  // This should be called after degree computation and before simplification.
  DclDegree[Dcl] = degree;
}

void SpillAnalysis::Clear() {
  if (Refs)
    delete Refs;

  Refs = nullptr;
  LA = nullptr;
  GC = nullptr;
  SM = nullptr;
  AugIntervals.clear();
  DclDegree.clear();
}

void SpillAnalysis::DumpHistogram(std::ostream &OS) {
  // Compute and dump histogram
  std::map<unsigned int, unsigned int> SpillSizeHistogram;
  for (auto Spill : GC->getSpilledLiveRanges()) {
    auto ByteSize = Spill->getDcl()->getByteSize();
    SpillSizeHistogram[ByteSize] += 1;
  }

  OS << "Spill Size Histogram For Iter#" << GC->getGRA().getIterNo() << " : "
     << "\n";
  for (auto &Item : SpillSizeHistogram) {
    OS << "# vars of " << Item.first << " bytes spilled: " << Item.second
       << "\n";
  }

  OS << "\n";
}

void SpillAnalysis::Do(LivenessAnalysis *L, GraphColor *C, SpillManagerGRF *S) {
  SetLivenessAnalysis(L);
  SetGraphColor(C);
  SetSpillManager(S);

  unsigned int LexId = 0;
  for (auto *BB : C->getGRA().kernel.fg.getBBList())
    for (auto *Inst : BB->getInstList())
      Inst->setLexicalId(LexId++);

  Refs = new VarReferences(C->getGRA().kernel);

  auto IterNo = C->getGRA().getIterNo();

  std::string FN =
      C->getGRA().kernel.getOptions()->getOptionCstr(VISA_AsmFileName);
  FN += std::string("-spill-iter-");
  FN += std::to_string(IterNo);
  FN += std::string(".csv");
  std::ofstream OF;
  OF.open(FN, std::ofstream::out);
  Dump(OF);
  OF.close();

  FN = C->getGRA().kernel.getOptions()->getOptionCstr(VISA_AsmFileName);
  FN += std::string("-misc-spill-data.txt");
  OF.open(FN, IterNo == 0 ? std::ofstream::out : std::ofstream::app);
  if (IterNo == 0) {
    ((vISA::Analysis *)&C->getGRA().kernel.fg.getLoops())->dump(OF);
  }
  DumpHistogram(OF);
  OF.close();
}

std::vector<G4_BB *>
SpillAnalysis::GetLiveBBs(G4_Declare *Dcl,
                          std::unordered_map<G4_INST *, G4_BB *> &InstBBMap) {
  // Return all BBs over which Dcl is live. This includes augmentation data.
  auto Order = [](const G4_BB *First, const G4_BB *Second) {
    return First->getId() < Second->getId();
  };
  std::set<G4_BB *, decltype(Order)> BBs(Order);
  auto &Kernel = GC->getGRA().kernel;

  VarReferences VarRefs(Kernel);
  auto *Defs = VarRefs.getDefs(Dcl);
  auto *Uses = VarRefs.getUses(Dcl);

  for (const auto &Def : *Defs) {
    auto *BB = std::get<1>(Def);
    BBs.insert(BB);
  }

  for (const auto &Use : *Uses) {
    auto *BB = std::get<1>(Use);
    BBs.insert(BB);
  }

  for (auto BB : Kernel.fg.getBBList()) {
    if (LA->isLiveAtEntry(BB, Dcl->getRegVar()->getId()) ||
        LA->isLiveAtExit(BB, Dcl->getRegVar()->getId())) {
      BBs.insert(BB);
    }
  }

  if (AugIntervals.count(Dcl)) {
    auto &Interval = AugIntervals[Dcl];
    auto AugBBs = GetIntervalBBs(Interval.first, Interval.second, InstBBMap);
    std::for_each(AugBBs.begin(), AugBBs.end(),
                  [&](G4_BB *BB) { BBs.insert(BB); });
  }

  std::vector<G4_BB *> RetBBs;
  std::for_each(BBs.begin(), BBs.end(),
                [&](G4_BB *BB) { RetBBs.push_back(BB); });

  return RetBBs;
}

std::vector<G4_BB *> vISA::SpillAnalysis::GetIntervalBBs(
    G4_INST *Start, G4_INST *End,
    std::unordered_map<G4_INST *, G4_BB *> &InstBBMap) {
  // Return vector of BBs given Start/End G4_INST*s
  std::vector<G4_BB *> BBs;
  auto &Kernel = GC->getGRA().kernel;
  bool Started = false;
  for (auto *BB : Kernel.fg.getBBList()) {
    bool BBAdded = false;
    for (auto *Inst : BB->getInstList()) {
      if (Inst == Start)
        Started = true;

      if (Started && !BBAdded) {
        BBs.push_back(BB);
        BBAdded = true;
      }

      if (Inst == End)
        return BBs;
    }
  }

  return BBs;
}

void Augmentation::verifyHomeLocation() {
  VarReferences refs(kernel);

  auto allRefsInFunc = [&](G4_Declare *dcl, FuncInfo *func) {
    auto *allDefs = refs.getDefs(dcl);
    if (allDefs) {
      for (auto &def : *allDefs) {
        auto *bb = std::get<1>(def);
        if (bb->getFuncInfo() != func)
          return false;
      }
    }

    auto *allUses = refs.getUses(dcl);
    if (allUses) {
      for (auto &def : *allUses) {
        auto *bb = std::get<1>(def);
        if (bb->getFuncInfo() != func)
          return false;
      }
    }

    return true;
  };

  for (auto &entry : sortedIntervals) {
    auto *dcl = entry.dcl;
    if (isSubroutineArg(dcl) || isSubroutineRetVal(dcl)) {
      vISA_ASSERT(!homeFunc[dcl->getDeclId()],
                  "not expecting arg/retval to have home func");
      continue;
    }

    auto *func = homeFunc[dcl->getDeclId()];
    if (allRefsInFunc(dcl, func))
      continue;
    vISA_ASSERT(false, "bad home func");
  }
}