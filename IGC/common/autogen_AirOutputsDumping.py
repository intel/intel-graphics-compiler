#!/usr/bin/env python

import stat
import re
import os
import sys
import binascii
import json
import errno

"""

"""
__curr_dir_path__ = os.path.dirname(os.path.realpath(__file__))
__sourceFile__ = sys.argv[1]
__genFile__ = sys.argv[2]

__genDir__ = os.path.dirname(__genFile__)
if not os.path.exists(__genDir__):
    try:
        os.makedirs(__genDir__)
    except OSError as err:
        # In case of a race to create this directory...
        if err.errno != errno.EEXIST:
            raise

userDefinedStruct = []

def main():
    inputFilePath = __sourceFile__
    if not os.path.isfile(inputFilePath):
        return
    outputFilePath = __genFile__

    structStartRegexpStr = "^[ \t]*struct (\w+)(?: : (\w+))?";
    structStartRegexp    = re.compile(structStartRegexpStr)
    structEndRegexpStr = "^[ \t]*\};$";
    structEndRegexp    = re.compile(structEndRegexpStr)
    structLineRegexpStr = "^[ \t]*(?:(?:(?:(?:\w+[ \t]+)*|(?:\w+))(\*)[ \t]*)|(?:\w+[ \t]+)+)(\w+)[ \t]*(?:\[((?:\w|:)+)\])?[ \t]*=?[ \t]*\w*[ \t]*;";
    structLineRegexp    = re.compile(structLineRegexpStr)

    enums = {}
    currentEnum = []
    currentStructName = ""

   

    isInStruct = False
    
    if os.path.isfile(outputFilePath):
        os.chmod(outputFilePath, stat.S_IWRITE)
    with open(outputFilePath, 'w') as fout:

        fout.write("\n// Auto-generated\n")
        fout.write("#pragma once\n")
        fout.write("#include <iostream>\n")
        fout.write("#include <iomanip>\n")
        fout.write("#include <sstream>\n")
        fout.write("namespace AIR {\n")
        with open(inputFilePath, 'r') as fin:
            for line in fin:
                if isInStruct == False:
                    match = structStartRegexp.search(line)
                    if match:
                        currentStructName = match.group(1)
                        if currentStructName == "GraphicsCompilerOutputs":
                            continue;
                        isInStruct = True
                        structName = "AIR::" + currentStructName
                        fout.write("void DumpCompilerAIROutput(const " + structName + "& obj, std::stringstream& ss)\n{\n")
                        fout.write("    ss << \"" + structName + ": \" << std::endl ;\n")
                        if match.group(2) != None:
                            fout.write("    DumpCompilerAIROutput(static_cast<const " + match.group(2) + "&>(obj), ss);\n\n")
                else:
                    match = structEndRegexp.search(line)
                    if match:
                        enums[currentStructName] = currentEnum
                        userDefinedStruct.append(currentStructName)
                        currentEnum = []
                        currentStructName = ""
                        isInStruct = False
                        fout.write("}\n\n")
                    else:
                        match = structLineRegexp.search(line)
                        if match:
                            isArray = True if match.group(3) != None else False
                            lineCopy = line    
                            lineCopy = " ".join(lineCopy.split())       
                            dataMemberType = lineCopy.split(' ', 1)[0]
                            isUserDefinedStruct = True if dataMemberType in userDefinedStruct else False
                            if isUserDefinedStruct:
                                if not isArray:
                                    fout.write("    DumpCompilerAIROutput( obj."+ match.group(2) + ", ss);\n")
                                    continue
                                else:
                                    fout.write("    for(size_t n = 0; n < " +  match.group(3) + "; ++n)\n    {\n" )
                                    s = match.group(2)
                                    fout.write("        ss << \"" + s +"[\"<<" +  "n" + "<< \"]:\";\n")
                                    #fout.write("        DumpCompilerAIROutput<"+ dataMemberType + ">( obj."+ match.group(2) +"[n]"+ ", ss); \n    }\n")
                                    fout.write("        DumpCompilerAIROutput( obj."+ match.group(2) +"[n]"+ ", ss); \n    }\n")
                                    continue

                            isPointerVariable = True if match.group(1) == '*' else False
                           
                            isBitMask = True if match.group(2).find("Mask") >= 0 else False
                            isChar = True if line.find(" char") >= 0 else False
                            fout.write("    for(size_t n = 0; n < " +
                                match.group(3) + "; ++n)\n" +
                                "    {\n" if isArray else "")
                            
                            arrayUserDefined = False
                            if isArray:
                                arrayType = match.group(0)
                                arrayUserDefined = True if arrayType in userDefinedStruct else False    
                            if arrayUserDefined:
                                fout.write("        ss << \"    \" ;") 
                                fout.write("    DumpCompilerAIROutput(obj."+match.group(2)+"[n], ss);\n")
                            else:    
                                fout.write(
                                ("    " if isArray else "") +
                                "    ss << " + "\"    " + match.group(2) +
                                ("[\" << n << \"]=\" << " if isArray else "=\" << ") +
                                ("\"0x\" << std::setfill('0') << std::setw(8) << std::hex << " if isBitMask else "") +
                                ("(" if isPointerVariable else "") +
                                ("static_cast<int>(" if isChar else "") +
                                "obj." + match.group(2) + ("[n]" if isArray else "") +
                                (")" if isChar else "") +
                                (" == nullptr ? \"nullptr\" : \"non-nullptr\")" if isPointerVariable else "") +
                                (" << std::dec" if isBitMask else "") +
                                " << std::endl;\n")
                            fout.write("    }\n" if isArray else "")
        fout.write("}\n");


if __name__ == '__main__':
    main()