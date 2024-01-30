#!/bin/bash

#Uses objdump to obtain global functions from object files - 2nd script argument
#Creates a linker script in script's directory, with 1st argument as a name, sets every symbol as local, as the ones obtained as global

readonly SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
readonly NEWLINE=$'\n'

readonly LINKER_SCRIPT=$1
readonly BIF_LIBRARY=$2

shift

formatAndWriteSymbols() {
   if [[ -n "${1}" ]]; then
        formattedSymbols=$(echo "${1}" | awk '{print $NF}')
        formattedSymbols=$(echo -e "${formattedSymbols}" | sed ':a;N;$!ba;s/\n/\n\t\t/g')
        formattedSymbols="${formattedSymbols//${NEWLINE}/;${NEWLINE}}"
        echo -e "\t\t$formattedSymbols;" >> ${SCRIPT_DIR}/${LINKER_SCRIPT}
   fi
}

#-----------------------------------------------------------------------------------------

echo -e "{\n\t global:" > ${SCRIPT_DIR}/${LINKER_SCRIPT}

if [[ -n "$BIF_LIBRARY" ]]; then

    symbolsBIF=$(objdump -t $BIF_LIBRARY | grep " O " | grep " g " | grep -v hidden)
    formatAndWriteSymbols "$symbolsBIF"

fi

for obj_file in "$@"; do

    if [[ $obj_file == *.o ]]; then

        symbols=$(objdump -t $obj_file | grep " F " | grep " g " | grep -v hidden)
        formatAndWriteSymbols "$symbols"

    fi

done

echo -e "\tlocal: *;" >> ${SCRIPT_DIR}/${LINKER_SCRIPT}
echo -e "};" >> ${SCRIPT_DIR}/${LINKER_SCRIPT}
