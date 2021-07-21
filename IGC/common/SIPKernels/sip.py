# ========================== begin_copyright_notice ============================
#
# Copyright (C) 2017-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# =========================== end_copyright_notice =============================

import os
import os.path
import shutil
import sys

# This function just adds an extra zero when the hex number as char has only
# single digit
def hex0x0to0x00(inp):
    if inp == "0x0":
        return "0x00"
    if inp == "0x1":
        return "0x01"
    if inp == "0x2":
        return "0x02"
    if inp == "0x3":
        return "0x03"
    if inp == "0x4":
        return "0x04"
    if inp == "0x5":
        return "0x05"
    if inp == "0x6":
        return "0x06"
    if inp == "0x7":
        return "0x07"
    if inp == "0x8":
        return "0x08"
    if inp == "0x9":
        return "0x09"
    if inp == "0xa":
        return "0x0a"
    if inp == "0xb":
        return "0x0b"
    if inp == "0xc":
        return "0x0c"
    if inp == "0xd":
        return "0x0d"
    if inp == "0xe":
        return "0x0e"
    if inp == "0xf":
        return "0x0f"
    return inp



def main():
    sipBin = ""
    sipHeader = ""
    sipCharArrayName = ""
    if len(sys.argv) > 3 :
        sipBin = sys.argv[1]
        sipHeader = sys.argv[2]
        sipCharArrayName = sys.argv[3]
        if os.path.exists(sipBin):
            sipBinArray = open(sipBin,"rb").read()
            sipHeaderFile = open(sipHeader,"w+")
            print("Found "+sipBin)
            print("Converting "+str(sipBin)+" to "+str(sipHeader))
            sipHeaderFile.write("namespace SIP\n")
            sipHeaderFile.write("{\n")
            sipHeaderFile.write("    const unsigned char "+sipCharArrayName+"[] =\n")
            sipHeaderFile.write("    {\n")
            lenA = len(sipBinArray)
            for i in range(lenA):
                if i >0 and i % 16 == 0 :
                    sipHeaderFile.write("\n")
                sipHeaderFile.write(hex0x0to0x00(hex(sipBinArray[i])).lower())
                if i+1 < lenA:
                    sipHeaderFile.write(", ")
            sipHeaderFile.write("};\n}")
            print("Conversion  done.. \n")
            sipHeaderFile.close()
        else:
            print("Error : Unable to open "+sipInput)
    else:
        print("Usage sip.py sip.bin sip.h sipStruct")
    return

if __name__ == "__main__":
    main()