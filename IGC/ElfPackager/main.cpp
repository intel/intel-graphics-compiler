/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/ScaledNumber.h>
#include "llvm/ADT/SetVector.h"
#include "llvm/Transforms/IPO/FunctionImport.h"
#include "llvmWrapper/Transforms/Utils/Cloning.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Bitcode/BitcodeWriterPass.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/CommandLine.h"

#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/IR/Module.h"
#include "llvmWrapper/ADT/STLExtras.h"

#include "common/LLVMWarningsPop.hpp"
#include "../GenISAIntrinsics/GenIntrinsics.h"

#include "ElfReader.h"
#include "ElfWriter.h"

#include "CLElfTypes.h"

#include "../AdaptorOCL/OCL/LoadBuffer.h"

#include <string>
#include <list>
#include <fstream>

using namespace std;
using namespace llvm;
using namespace CLElfLib;


static cl::opt<std::string>
    InputBCFilename(cl::Positional, cl::desc("<input .llvm file>"), cl::init("-"));
static cl::opt<std::string>
    InputFuncList("funcList", cl::desc("<input .llvm file>"));
static cl::opt<std::string>
    OutputPath(cl::Positional, cl::desc("<output .llvm file>"), cl::init("-"));
static cl::opt<bool>
    IncludeSizet("includeSizet", cl::desc("if the module has size_t"));

void MakeHeader(StringRef Func, SmallVector<char, 0> &headerVector, int index)
{
    //This is for creating the header file
    short Func_Size = (short)Func.size();
    headerVector.push_back((char)Func_Size);
    headerVector.push_back((char)(Func_Size >> 8));
    for (short func_iterate = 0; func_iterate < Func_Size; func_iterate++)
    {
        headerVector.push_back(Func[func_iterate]);
    }
    headerVector.push_back((char)index);
    headerVector.push_back((char)(index >> 8));
}

void CreateElfSection(CLElfLib::CElfWriter* pWriter, CLElfLib::SSectionNode sectionNode, std::string Name, char* pData, unsigned DataSize)
{
    // Create section
    sectionNode.Name = Name;
    sectionNode.pData = pData;
    sectionNode.DataSize = DataSize;
    sectionNode.Flags = 0;
    sectionNode.Type = SH_TYPE_PROG_BITS;

    // Add it to the file
    pWriter->AddSection(&sectionNode);
}


std::unique_ptr<IGCLLVM::Module> LocalCloneModule(
    const Module *M, ValueToValueMapTy &VMap,
    std::vector<GlobalValue*> &ExtractValues,
    std::map<const llvm::Function*, std::set<StringRef>> &FuncMap) {
    // First off, we need to create the new module.
    std::unique_ptr<IGCLLVM::Module> New =
        IGCLLVM::make_unique<IGCLLVM::Module>(M->getModuleIdentifier(), M->getContext());
    New->setDataLayout("");
    New->setTargetTriple("");
    New->setModuleInlineAsm(M->getModuleInlineAsm());


    auto mapGlobal = [&](const GlobalVariable *I)
    {
        if (VMap.find(&*I) == VMap.end())
        {
            GlobalVariable *GV = new GlobalVariable(*New,
                I->getType()->getPointerElementType(),
                I->isConstant(), I->getLinkage(),
                (Constant*) nullptr, I->getName(),
                (GlobalVariable*) nullptr,
                I->getThreadLocalMode(),
                I->getType()->getAddressSpace());

            GV->copyAttributesFrom(&*I);
            VMap[&*I] = GV;
            GlobalVariable *GV2 = cast<GlobalVariable>(VMap[&*I]);
            if (I->hasInitializer())
                GV2->setInitializer(MapValue(I->getInitializer(), VMap));
        }
    };

    auto mapFunction = [&](const Function *I)
    {

        Function *NF = cast<Function>(New.get()->getOrInsertFunction(
            I->getName(),
            I->getFunctionType(),
            I->getAttributes()));
        if(I->getName().startswith(GenISAIntrinsic::getGenIntrinsicPrefix()))
            NF->setAttributes(I->getAttributes());

        VMap[&*I] = NF;
        Function *F = cast<Function>(VMap[&*I]);

        for (auto UI = I->begin(), UE = I->end(); UI != UE; ++UI)
        {
            auto BB = &*UI;
            for (auto UI_BB = BB->begin(), UE_BB = BB->end();
                UI_BB != UE_BB; ++UI_BB)
            {
                auto inst = &*UI_BB;
                if (auto CI = dyn_cast<CallInst>(inst))
                {
                    auto val = IGCLLVM::getCalledValue(CI);
                    auto *I2 = dyn_cast<llvm::Function>(val->stripPointerCasts());

                    Function *NF2 = cast<Function>(New.get()->getOrInsertFunction(
                        I2->getName(),
                        I2->getFunctionType(),
                        I2->getAttributes()));

                    if(I->getName().startswith(GenISAIntrinsic::getGenIntrinsicPrefix()))
                        NF2->setAttributes(I->getAttributes());

                    VMap[&*I2] = NF2;
                    Function *F2 = cast<Function>(VMap[&*I2]);
                    F2->setLinkage(GlobalValue::ExternalLinkage);
                }
            }
        }

        auto funcGlobalSet = FuncMap[&*I];
        for (auto &global_Iterator : funcGlobalSet)
        {
            mapGlobal(M->getNamedGlobal(global_Iterator));
        }


        if (!I->isDeclaration()) {
            Function::arg_iterator DestI = F->arg_begin();
            for (Function::const_arg_iterator J = I->arg_begin(); J != I->arg_end();
                ++J) {
                DestI->setName(J->getName());
                VMap[&*J] = &*DestI++;
            }

            SmallVector<ReturnInst*, 8> Returns;  // Ignore returns cloned.
            IGCLLVM::CloneFunctionInto(F, &*I, VMap, /*ModuleLevelChanges=*/true, Returns);
        }

        if (I->hasPersonalityFn())
            F->setPersonalityFn(MapValue(I->getPersonalityFn(), VMap));
    };



    for (auto iterator : ExtractValues)
    {
        if (auto func = dyn_cast<Function>(iterator))
        {
            mapFunction(func);
        }
        else if (auto global = dyn_cast<GlobalVariable>(iterator))
        {
            mapGlobal(global);
        }
        else
        {
            continue;
        }
    }

    // And named metadata....
    for (Module::const_named_metadata_iterator I = M->named_metadata_begin(),
        E = M->named_metadata_end(); I != E; ++I) {
        const NamedMDNode &NMD = *I;
        NamedMDNode *NewNMD = New->getOrInsertNamedMetadata(NMD.getName());
        for (unsigned i = 0, e = NMD.getNumOperands(); i != e; ++i)
            NewNMD->addOperand(MapMetadata(NMD.getOperand(i), VMap));
    }

    return New;
}

int main(int argc, char *argv[])
{
    LLVMContext Context;
    SMDiagnostic Err;
    std::map<std::string, std::vector<GlobalValue*>> Map;
    cl::ParseCommandLineOptions(argc, argv);
    CLElfLib::CElfWriter* pWriter = CLElfLib::CElfWriter::Create(EH_TYPE_NONE, EH_MACHINE_NONE, 0);
    CLElfLib::SSectionNode sectionNode;

    ErrorOr<std::unique_ptr<MemoryBuffer>> FileOrErr =
        MemoryBuffer::getFileOrSTDIN(InputBCFilename);
    std::unique_ptr<llvm::MemoryBuffer> genericBufferPtr(FileOrErr.get().release());
    Expected<std::unique_ptr<Module>> M = llvm::parseBitcodeFile(genericBufferPtr->getMemBufferRef(), Context);
    if (llvm::Error EC = M.takeError())
    {
        Err.print("Unable to Parse bitcode", errs());
        return -1;
    }

    auto &Bif_FunctionList = M.get()->getFunctionList();
    auto &GlobalList = M.get()->getGlobalList();
    std::vector<GlobalValue*> NotFound;
    std::vector<std::string> NotFound_names;

    if (InputFuncList.empty())
    {
        for (auto &iterator1 : Bif_FunctionList)
        {
            auto name = iterator1.getName();
            if (name.str().find("__builtin_IB_") == std::string::npos &&
                name.str().find("llvm.") == std::string::npos)
                Map[name.str()].push_back(&iterator1);
        }
    }
    else
    {
        std::vector<string> FunctionList;
        std::ifstream myfile;
        myfile.open(InputFuncList, std::ifstream::in);
        std::string line;

        if (myfile.is_open())
        {
            while (getline(myfile, line))
            {
                FunctionList.push_back(line);
            }
            myfile.close();
        }
        for (auto &iterator1 : Bif_FunctionList)
        {
            auto FuncName = iterator1.getName();
            bool found = false;
            if (FuncName.str().find("__builtin_IB_") == std::string::npos &&
                FuncName.str().find("llvm.") == std::string::npos)
            {
                for (auto iterator2 : FunctionList)
                {
                    if (FuncName.find(iterator2) != std::string::npos)
                    {
                        found = true;
                        Map[iterator2].push_back(&iterator1);
                        break;
                    }
                }
                if (!found)
                {
                    NotFound.push_back(&iterator1);
                    NotFound_names.push_back(iterator1.getName().str());
                }
            }
        }
    }

    std::map<const llvm::Function*, std::set<StringRef>> FuncToGlobalsMap;

    for (auto &iterator3 : GlobalList)
    {
        std::function<void(User*)> findUsers = [&](User* U)
        {
            if (auto inst = dyn_cast<Instruction>(U))
            {
                FuncToGlobalsMap[inst->getParent()->getParent()].insert(iterator3.getName());
                return;
            }
            else if (dyn_cast<GlobalValue>(U))
            {
                return;
            }
            else
            {
                for (auto Users2 : U->users())
                {
                    findUsers(Users2);
                }
                return;
            }
        };

        for (auto Users : iterator3.users())
        {
            findUsers(Users);
        }
        Map[iterator3.getName().str()].push_back(&iterator3);
    }

    Map["other"] = NotFound;

    //At this point we have the bins and the functions that go inside each bin

    std::map<std::string, std::string> ElfMap;

    // Need to construct the header file
    SmallVector<char, 0> headerVector;

    int counter;
    if (IncludeSizet)
        counter = 6;
    else
        counter = 2;

    for (auto map_iterator : Map)
    {
        std::vector<GlobalValue*> ExtractValue = map_iterator.second;
        llvm::ValueToValueMapTy Val;
        std::unique_ptr<IGCLLVM::Module> M1 = LocalCloneModule(M.get().get(),
            Val, ExtractValue, FuncToGlobalsMap);
        legacy::PassManager Passes;
        SmallVector<char, 0> Buffer;
        raw_svector_ostream OS(Buffer);
        Passes.add(createBitcodeWriterPass(OS, false));
        Passes.run(*M1.get());
        ElfMap[map_iterator.first] = OS.str().str();
    }

    for (auto map_iterator : Map)
    {
        std::vector<GlobalValue*> ExtractValue = map_iterator.second;
        for (size_t i = 0, e = ExtractValue.size(); i != e; ++i)
        {
            MakeHeader(ExtractValue[i]->getName(), headerVector, counter);
        }
        counter++;
    }


    //Adding the global Header file now that it is constructed
    raw_svector_ostream OS(headerVector);
    CreateElfSection(pWriter,
        sectionNode,
        "Header",
        const_cast<char*>(OS.str().data()),
        OS.str().size());

    if (IncludeSizet)
    {
        std::string sizetPath;
        #ifdef _WIN32
            sizetPath = InputBCFilename.substr(0, InputBCFilename.find_last_of("\\/"));
        #else
            sizetPath = InputBCFilename.substr(0, InputBCFilename.find_last_of("/"));
        #endif

        std::unique_ptr<Module> M_sizet_32 =
            getLazyIRFileModule(sizetPath + "//IGCsize_t_32.bc", Err, Context, true);
        if (!M_sizet_32) {
            Err.print("function-import-sizet_32", errs());
        }

        std::unique_ptr<Module> M_sizet_64 =
            getLazyIRFileModule(sizetPath + "//IGCsize_t_64.bc", Err, Context, true);
        if (!M_sizet_64) {
            Err.print("function-import-sizet_64", errs());
        }

        SmallVector<char, 0> headerVector_sizet_32;
        SmallVector<char, 0> headerVector_sizet_64;

        for (auto &iterator1 : M_sizet_32->getFunctionList())
        {
            auto Func = iterator1.getName();
            if (iterator1.isMaterializable())
                MakeHeader(Func, headerVector_sizet_32, 4);
        }
        for (auto &iterator1 : M_sizet_64->getFunctionList())
        {
            auto Func = iterator1.getName();
            if (iterator1.isMaterializable())
                MakeHeader(Func, headerVector_sizet_64, 5);
        }

        raw_svector_ostream OS_sizet_32(headerVector_sizet_32);
        raw_svector_ostream OS_sizet_64(headerVector_sizet_64);

        if (Error EC = M_sizet_32->materializeAll())
        {
            Err.print("Unable to Parse bitcode", errs());
        }

        if (Error EC = M_sizet_64->materializeAll())
        {
            Err.print("Unable to Parse bitcode", errs());
        }

        legacy::PassManager Passes1;
        SmallVector<char, 0> Buffer_sizet32;
        raw_svector_ostream OS_sizet32(Buffer_sizet32);
        Passes1.add(createBitcodeWriterPass(OS_sizet32, true));
        Passes1.run(*M_sizet_32.get());

        legacy::PassManager Passes2;
        SmallVector<char, 0> Buffer_sizet64;
        raw_svector_ostream OS_sizet64(Buffer_sizet64);
        Passes2.add(createBitcodeWriterPass(OS_sizet64, true));
        Passes2.run(*M_sizet_64.get());

        //Inserting the header files for size_t and the .bc files for size_t
        CreateElfSection(pWriter,
            sectionNode,
            "Header_sizet_32",
            const_cast<char*>(OS_sizet_32.str().data()),
            OS_sizet_32.str().size());

        CreateElfSection(pWriter,
            sectionNode,
            "Header_sizet_64",
            const_cast<char*>(OS_sizet_64.str().data()),
            OS_sizet_64.str().size());

        CreateElfSection(pWriter,
            sectionNode,
            "IGCsize_t_32",
            const_cast<char*>(OS_sizet32.str().data()),
            OS_sizet32.str().size());

        CreateElfSection(pWriter,
            sectionNode,
            "IGCsize_t_64",
            const_cast<char*>(OS_sizet64.str().data()),
            OS_sizet64.str().size());
    }

    //Now to add all of the sections in the file
    for (auto elf_iterator : ElfMap)
    {
        CreateElfSection(pWriter,
            sectionNode,
            elf_iterator.first,
            const_cast<char*>(elf_iterator.second.data()),
            elf_iterator.second.size());
    }

    // Resolve size of ELF blob
    size_t dataSize = 0;
    char* ElfBlob = NULL;
    pWriter->ResolveBinary(ElfBlob, dataSize);
    ElfBlob = new char[dataSize];
    if (ElfBlob == NULL)
    {
        return -1;
    }
    pWriter->ResolveBinary(ElfBlob, dataSize);

    // Write ELF file to disk
    std::ofstream ofs(OutputPath, std::ifstream::binary);
    ofs.write(ElfBlob, dataSize);
    delete[] ElfBlob;
    if (!ofs.good())
    {
        return -1;
    }
    ofs.close();
    CLElfLib::CElfWriter::Delete(pWriter);

  return 0;
}

