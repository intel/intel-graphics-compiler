#!/bin/bash

if [ ! -f scripts/load_environment.sh ]; then
    printf "\033[1;33m%s\033[0m\n" "$0 should be run from project root" 1>&2
fi
source scripts/load_environment.sh

function fatal_bad_arg()
{
    if [ "$#" -gt 0 ]; then
        error_message "$1"
    fi
    echo "usage: build_all.sh [-j] [-o OUTDIR]"
    echo "-j          executes a parallel build"
    echo "-o OUTDIR   overrides the default output directory"
    echo "            OUTDIR must be a subdirectory of the cmake dir"
    exit 1
}

if [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
    fatal_bad_arg
fi

BUILD_MODE=-b
TARGET_DIR_SPECIFIED=0
LAST_IS_DASH_O=0
for arg in $*; do
    if [ $LAST_IS_DASH_O -eq  1 ]; then
        TARGET_DIR="$arg"
        TARGET_DIR_SPECIFIED=1
        LAST_IS_DASH_O=1
    elif [ "$arg" == "-o" ]; then
        LAST_IS_DASH_O=1
    elif [ "$arg" == "-j" ]; then
        BUILD_MODE=-B
    else
        fatal_bad_arg "$arg: invalid option"
    fi
done

if [ $LAST_IS_DASH_O -eq  1 ]; then
    fatal_bad_arg "-o expects directory to follow"
fi


if [ $TARGET_DIR_SPECIFIED -ne 0 ]; then
    echo "################ GENERATING DEBUG BUILD ################"
    sh scripts/generate_makefiles.sh "$BUILD_MODE" -o "$OUTDIR" Debug
    echo "################ GENERATING RELEASE BUILD ################"
    sh scripts/generate_makefiles.sh "$BUILD_MODE" -o "$OUTDIR" Release
else
    echo "################ GENERATING DEBUG BUILD ################"
    sh scripts/generate_makefiles.sh "$BUILD_MODE" Debug
    echo "################ GENERATING RELEASE BUILD ################"
    sh scripts/generate_makefiles.sh "$BUILD_MODE" Release
fi
