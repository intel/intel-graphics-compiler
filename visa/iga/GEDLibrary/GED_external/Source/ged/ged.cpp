#ifndef GED_BUILDING_EXECUTABLE
#error must enable GED_BUILD_EXECUTABLE to build the GED standalone executable (disassembler)
#endif

#ifndef GED_DISASSEMBLY
#error must enable GED_DISASSEMBLY to build the GED standalone executable (disassembler)
#endif

#include <cstdio>
#include <iostream>
#include <iomanip>
#include "common/ged_base.h"
#include "common/ged_option_parser.h"
#include "xcoder/ged_disassembler.h"

using std::cout;
using std::cerr;
using std::endl;
using std::hex;
using std::setfill;
using std::setw;


static void DefineOptionParser(OptionParser& opt)
{
    opt.DefineOption(
        'h',        // short name (one character) - must exist
        "help",     // long name (more than one character) - optional, passing an empty string means there is no long name
        "Print this help message.", // description
        false,      // required
        false,      // accepts multiple instances
        false,      // takes an argument
        NULL        // default value (if takes an argument) - NULL means no default
    );

    opt.DefineOption(
        'v', "version",
        "Print version information.",
        false, false, false, NULL);

    opt.DefineOption(
        'l', "list-models",
        "Print the list of supported models.",
        false, false, false, NULL);

    opt.DefineOption(
        'm', "model",
        "Specify the model by which to disassemble the given sequence of instructions.",
        false, false, true, NULL);

    opt.DefineOption(
        'f', "file",
        "Input from file. Specify the name of the file containing the GEN instructions to disassemble.",
        false, false, true, NULL);

    set<string> requireOneOf;
    requireOneOf.insert("list-models");
    requireOneOf.insert("file");
    requireOneOf.insert("help");
    requireOneOf.insert("version");
    opt.RequireOneOf(requireOneOf);
}


static inline void PrintUsage(const OptionParser& opt)
{
    string usage;
    opt.Usage(usage);
    cout << usage << endl;
}


static inline void PrintVersion()
{
    GEDVERSION(cout);
}


static inline void PrintModels()
{
    cout << "Supported GEN models:" << endl;
    GEDASSERT(numOfSupportedModels > 0);
    for (unsigned int i = 0; i < numOfSupportedModels; ++i)
    {
        cout << modelNames[i] << endl;
    }
}


static inline bool UsageOnly(const OptionParser& opt)
{
    if (opt.OptionSpecified("help"))
    {
        PrintUsage(opt);
        return true;
    }
    if (opt.OptionSpecified("version"))
    {
        PrintVersion();
        return true;
    }
    if (opt.OptionSpecified("list-models"))
    {
        PrintModels();
        return true;
    }
    return false;
}


static inline bool GetModel(const OptionParser& opt, /* GED_MODEL */ unsigned int& model)
{
    if (!opt.OptionSpecified("model"))
    {
        cerr << "A GEN model must be specified for disassembly." << endl;
        PrintModels();
        return false;
    }

    const set<string>& models = opt.GetOptionValues("model");
    GEDASSERT(1 == models.size());
    if (!GetModelByName(*(models.begin()), model))
    {
        cerr << *(models.begin()) << " is not a valid GEN model." << endl;
        PrintModels();
        return false;
    }

    return true;
}


static inline bool GetFile(const OptionParser& opt, string& filename)
{
    if (!opt.OptionSpecified("file"))
    {
        cerr << "An input file containing the instruction sequence must be specified." << endl;
        return false;
    }
    const set<string>& files = opt.GetOptionValues("file");
    GEDASSERT(1 == files.size());
    filename = *(files.begin());
    GEDASSERT(!filename.empty());
    return true;
}


static bool ParseOptions(int argc, const char* argv[], /* GED_MODEL */ unsigned int& model, string& filename)
{
    static const string gedDesc =
        "ged:\nThis application disassembles a given sequence of GEN instructions.";
    OptionParser opt("ged", gedDesc); // option parser
    DefineOptionParser(opt); // add the supported options to the parser
    opt.ParseOptions(argc, argv); // parse the command line arguments
    if (UsageOnly(opt)) return true; // quick check to see if we only need to print the help message or list of models

    // For disassembly, a GEN model must be specified.
    if (!GetModel(opt, model)) return false;

    // Currently the instruction sequence input is only supported in a file.
    if (!GetFile(opt, filename)) return false;

    return true;
}


static void Disassemble(const /* GED_MODEL */ unsigned int model, const string& filename)
{
    FILE* binFile = NULL;
# ifdef TARGET_WINDOWS
    // No need to check the return value because if fopen_s fails, binFile will remain NULL and the error will be caught by the check
    // below.
    fopen_s(&binFile, filename.c_str(), "rb");
# else // not TARGET_WINDOWS
    binFile = fopen(filename.c_str(), "rb");
# endif // not TARGET_WINDOWS
    if (NULL == binFile)
    {
        cerr << "Failed to open file '" << filename << "'." << endl;
        exit(GED_RETURN_VALUE_FILE_OPEN_FAILED);
    }
    fseek(binFile, 0, SEEK_END);
    size_t fileSize = (size_t)ftell(binFile);
    unsigned char* const pBuffer = new unsigned char[fileSize];
    fseek(binFile, 0, SEEK_SET);
# ifdef TARGET_WINDOWS
    if (fileSize != fread_s(pBuffer, fileSize, sizeof(unsigned char), fileSize, binFile))
# else // not TARGET_WINDOWS
    if (fileSize != fread(pBuffer, sizeof(unsigned char), fileSize, binFile))
# endif // not TARGET_WINDOWS
    {
        cerr << "Failed to read file '" << filename << "'." << endl;
        exit(GED_RETURN_VALUE_FILE_READ_FAILED);
    }

    const unsigned char* ptr = pBuffer;
    const unsigned char* limit = ptr + fileSize;

    char dummy[sizeof(GEDDisassembler)];
    GEDDisassembler* ins = reinterpret_cast<GEDDisassembler*>(dummy);

    for (; ptr < limit; ptr += ins->GetInstructionSize())
    {
        GED_RETURN_VALUE status = GED_RETURN_VALUE_INVALID_FIELD;
        status = ins->Decode(model, ptr, GED_NATIVE_INS_SIZE);
        if (GED_RETURN_VALUE_SUCCESS != status)
        {
            cout << "BAD INSTRUCTION: 0x" << setfill('0') << hex << setw(16) << ((const uint64_t*)ptr)[1] <<
                                                                    setw(16) << ((const uint64_t*)ptr)[0] << endl;
        }
        else
        {
            cout << ins->Disassemble() << endl;
        }
    }

    delete[] pBuffer;
    fclose(binFile);
}


int main(int argc, const char* argv[])
{
    /* GED_MODEL */ unsigned int model;
    string filename;
    if (!ParseOptions(argc, argv, model, filename)) return GED_ERROR_TYPE_BAD_OPTIONS; // invalid options
    if (filename.empty()) return GED_ERROR_TYPE_SUCCESS; // no input
    Disassemble(model, filename);
    return GED_ERROR_TYPE_SUCCESS;
}

