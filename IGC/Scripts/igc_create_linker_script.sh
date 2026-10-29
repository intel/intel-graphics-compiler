#!/bin/bash

# Collect exported symbols from objects and libraries and write a linker
# version script next to this script. IGC_SYMBOL_NM reads LLVM bitcode that
# objdump cannot read.
#
# Usage: igc_create_linker_script.sh <script> <bif> <vulkan> [objects...]
# A library argument equal to "null" is skipped. The BIF library contributes
# global data objects; every other library and each *.o argument contributes
# functions.

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" \
    >/dev/null 2>&1 && pwd) || exit 1
readonly SCRIPT_DIR

required_args=3
usage='<script> <bif> <vulkan> [objects...]'

if [[ $# -lt $required_args ]]; then
    printf 'Usage: %s %s\n' "${BASH_SOURCE[0]}" "$usage" >&2
    exit 2
fi

readonly LINKER_SCRIPT=$1
readonly BIF_LIBRARY=$2
shift 2
readonly VULKAN_FE_LIBRARY=$1
shift
readonly NM_TOOL=${IGC_SYMBOL_NM:-}
readonly OUTPUT_FILE="${SCRIPT_DIR}/${LINKER_SCRIPT}"

# Global data objects from the BIF library.
readonly OBJDUMP_OBJECTS_FILTER='/ O / && / g / && !/hidden/'

# Non-hidden global functions, then lower- and upper-case weak symbols. Each
# filter is a separate pass over the table so the order is kept without
# buffering; a line matching several filters is repeated.
readonly -a OBJDUMP_FUNCTION_FILTERS=(
    '/ F / && / g / && !/hidden/'
    '/ w / && !/hidden/'
    '/ W / && !/hidden/'
)
readonly OBJDUMP_PRINT_NAME='{ print $NF }'

# Marker symbol of a GCC slim-LTO object. Its real symbols live in LTO
# sections that objdump does not list, so the table is incomplete.
readonly OBJDUMP_GCC_SLIM_LTO_MARKER='$NF == "__gnu_lto_slim" { found = 1; exit }
    END { exit !found }'

# llvm-nm lines are "<value> <type> <name>". "types" matches the type.
readonly NM_SYMBOL_AWK='NF == 3 && $2 ~ types { print $3 }'

# Symbol lists can be large, so tool output and symbols stay in files and are
# never held in shell variables. The work directory is next to the output so
# the finished script is moved into place with a rename.
work_dir=
table_file=
error_file=
symbols_file=
staged_file=
script_file=

remove_work_dir() {
    if [[ -n "$work_dir" ]]; then
        rm -rf -- "$work_dir"
    fi
}

cleanup_and_exit() {
    exit "$1"
}

fail() {
    printf 'igc_create_linker_script.sh: error: %s\n' "$*" >&2
    cleanup_and_exit 1
}

trap remove_work_dir EXIT
trap 'cleanup_and_exit 130' INT
trap 'cleanup_and_exit 143' TERM

# Print the first line of a file, or "unknown error" if it is empty.
first_line_or_unknown() {
    local line=

    IFS= read -r line < "$1" || :
    printf '%s' "${line:-unknown error}"
}

# Fill "symbols_file" from the objdump table, one filter pass at a time.
filter_objdump_output() {
    local kind=$1
    local file=$2
    local filter
    local -a filters=("${OBJDUMP_FUNCTION_FILTERS[@]}")

    if [[ "$kind" == objects ]]; then
        filters=("$OBJDUMP_OBJECTS_FILTER")
    fi
    : > "$symbols_file" || fail "could not write '$symbols_file'"
    for filter in "${filters[@]}"; do
        awk "$filter $OBJDUMP_PRINT_NAME" "$table_file" >> "$symbols_file" ||
            fail "awk failed while filtering objdump output for '$file'"
    done
}

# Fill "symbols_file" from llvm-nm. It does not report bitcode visibility;
# hidden symbols remain local in the final link.
read_symbols_with_nm() {
    local kind=$1
    local file=$2
    local types='^[TWV]$'

    [[ "$kind" == objects ]] && types='^[DBR]$'

    if ! "$NM_TOOL" --defined-only "$file" \
            > "$table_file" 2> "$error_file"; then
        fail "'$NM_TOOL' cannot read '$file':" \
            "$(first_line_or_unknown "$error_file")"
    fi
    awk -v types="$types" "$NM_SYMBOL_AWK" "$table_file" > "$symbols_file" ||
        fail "awk failed while filtering '$NM_TOOL' output for '$file'"
}

# Read one input, using llvm-nm only if objdump fails. Stderr goes to a
# separate file so successful objdump warnings cannot be mistaken for
# symbol-table entries.
read_symbols() {
    local kind=$1
    local file=$2

    if objdump -t "$file" > "$table_file" 2> "$error_file"; then
        if awk "$OBJDUMP_GCC_SLIM_LTO_MARKER" "$table_file"; then
            fail "'$file' is a GCC slim-LTO object; objdump cannot list" \
                'its symbols. Build it without -flto or with' \
                '-ffat-lto-objects.'
        fi
        filter_objdump_output "$kind" "$file"
        return 0
    fi

    if [[ -z "$NM_TOOL" ]]; then
        fail "objdump cannot read '$file':" \
            "$(first_line_or_unknown "$error_file"); no llvm-nm is set." \
            'Set IGC_OPTION__SYMBOL_NM to an llvm-nm executable.'
    fi
    read_symbols_with_nm "$kind" "$file"
}

# Format this input's symbols and append them to the staged script.
append_symbols() {
    [[ -s "$symbols_file" ]] || return 0
    awk '{ printf "\t\t%s;\n", $0 }' "$symbols_file" >> "$staged_file" ||
        fail "could not write '$staged_file'"
}

export_symbols() {
    read_symbols "$1" "$2"
    append_symbols || fail "could not append symbols from '$2'"
}

work_dir=$(mktemp -d "${OUTPUT_FILE%/*}/.igc-linker-script.XXXXXX") ||
    fail "could not create a work directory next to '$OUTPUT_FILE'"
table_file="${work_dir}/table"
error_file="${work_dir}/error"
symbols_file="${work_dir}/symbols"
staged_file="${work_dir}/staged"
script_file="${work_dir}/script"

: > "$staged_file" ||
    fail "could not create '$staged_file'"

if [[ "$BIF_LIBRARY" != null ]]; then
    export_symbols objects "$BIF_LIBRARY"
fi


if [[ "$VULKAN_FE_LIBRARY" != null ]]; then
    export_symbols functions "$VULKAN_FE_LIBRARY"
fi

for object_file in "$@"; do
    [[ "$object_file" == *.o ]] || continue
    export_symbols functions "$object_file"
done

# An empty "global:" section is a syntax error in a version script.
printf '{\n' > "$script_file" ||
    fail "could not create '$script_file'"
if [[ -s "$staged_file" ]]; then
    printf '\tglobal:\n' >> "$script_file" ||
        fail "could not write '$script_file'"
    cat -- "$staged_file" >> "$script_file" ||
        fail "could not write '$script_file'"
fi
printf '\tlocal: *;\n};\n' >> "$script_file" ||
    fail "could not finish '$script_file'"
[[ ! -d "$OUTPUT_FILE" ]] || fail "'$OUTPUT_FILE' is a directory"
mv -f -- "$script_file" "$OUTPUT_FILE" ||
    fail "could not write '$OUTPUT_FILE'"
