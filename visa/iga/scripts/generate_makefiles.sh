#!/bin/bash

set -o igncr

if [ ! -f scripts/load_environment.sh ]; then
  printf "\033[1;33m%s\033[0m\n" "$0 should be run from project root" 1>&2
fi
source scripts/load_environment.sh

function fatal_usage()
{
  if [ "$#" -gt 0 ]; then
    echo "$1"
  fi
  echo "usage: generate_makefiles.sh [-bB] [-o OUTDIR] [Debug|Release]"
  echo "-b          executes make after cmake"
  echo "-B          executes a parallel make after cmake"
  echo "-o OUTDIR   overrides the default output directory"
  echo "            OUTDIR must be a subdirectory of the cmake dir"
  exit 1
}
if [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
  fatal_usage
fi

CXX_COMPILER="c++"
if [ ! -z "$CXX" ]; then
  CXX_COMPILER="$CXX"
fi
# just takes the 4.8.4 or 4.9.2
# CPP_VERSION=`$CXX_COMPILER --version | head -n 1 | awk '{ print $NF }'`
CXX_VERSION=`$CXX_COMPILER --version | head -n 1`

PARALLEL=
if [ "$1" == "-j" ]; then
  PARALLEL=-j
  shift
fi

EXECUTE_MAKE=0
if [ "$1" == "-B" ]; then
  EXECUTE_MAKE=1
  PARALLEL=-j
  shift
elif [ "$1" == "-b" ]; then
  EXECUTE_MAKE=1
  shift
fi

TARGET_DIR_SPECIFIED=0
if [ "$1" == "-o" ]; then
  shift
  if [ "$#" -eq 0 ] || [ -z "$1" ]; then
    fatal_usage "-o expects directory to follow"
  fi
  TARGET_DIR="$1"
  TARGET_DIR_SPECIFIED=1
  REL_PATH=".."
  shift
fi

BUILD_TYPE=Release
if [ "$#" -gt 0 ]; then
  if [ "$1" == "Debug" ]; then
    BUILD_TYPE=Debug
  elif [ "$1" == "Release" ]; then
    BUILD_TYPE=Release
  else
    fatal_usage "$1: expected Debug or Release"
  fi
  shift
fi



if [ $TARGET_DIR_SPECIFIED -eq 0 ]; then
  TARGET_DIR="builds/${PLATFORM}_external/${BUILD_TYPE}"
  REL_PATH="../../.."
fi

printf "platform:      \033[1;33m%s\033[0m\n" "$PLATFORM"
printf "gcc version:   \033[1;33m%s\033[0m\n" "$CXX_VERSION"
printf "output:        \033[1;33m%s\033[0m\n" "$TARGET_DIR"


echo "********************************************************************"
echo "* CREATING MAKEFILES in ${TARGET_DIR} (for ${BUILD_TYPE} configuration)"

rm -rf "${TARGET_DIR}"
mkdir -p "${TARGET_DIR}"
pushd "${TARGET_DIR}"

cmake \
  "-DCMAKE_BUILD_TYPE=$BUILD_TYPE" \
  -G "Unix Makefiles" \
  "${REL_PATH}"

echo "********************************************************************"
if [ $EXECUTE_MAKE -eq 1 ]; then
  echo "* EXECUTING MAKE ${PARALLEL}"
  make ${PARALLEL}
  echo "Done building to $TARGET_DIR"
else
  echo "* (cd ${TARGET_DIR} && make [-j]) to build"
fi
popd
