set -e

# loads the variables
#   PLATFORM = Linux | Cygwin | ...
#   BIT_SIZE = 32 | 64
#   ARCH_DIR = x86 | x64
function warning_message()
{
    printf "\033[1;31m$1\033[0m\n" 1>&2
}
function error_message()
{
    printf "\033[1;33m$1\033[0m\n" 1>&2
}

#INFER PLATFORM
# windows/cygwin environment variable
if [ "$PROCESSOR_ARCHITECTURE" == "x86" ]; then
    ARCH_DIR=x86
    BIT_SIZE=32
elif [ "$PROCESSOR_ARCHITECTURE" == "AMD64" ]; then
    ARCH_DIR=x64
    BIT_SIZE=64
# linux variables
elif [ "$HOSTTYPE" == "x86_32" ]; then
    ARCH_DIR=x86
    BIT_SIZE=32
elif [ "$HOSTTYPE" == "x86_64" ]; then
    ARCH_DIR=x64
    BIT_SIZE=64
elif [ `uname -m` == "x86_32" ]; then
    ARCH_DIR=x86
    BIT_SIZE=32
elif [ `uname -m` == "x86_64" ]; then
    ARCH_DIR=x64
    BIT_SIZE=64
else
    error_message "ERROR cannot determine processor architecture"
    error_message 'must have $PROCESSOR_ARCHITECTURE (as x86 or AMD64) or $HOSTTYPE (x86_32,x86_64) defined'
    exit 1
fi
UNAME_VALUE=`uname`
PLATFORM_NAME="UnknownUnix"
if [ "$UNAME_VALUE" == "Linux" ]; then
    PLATFORM_NAME="Linux"
elif echo "$UNAME_VALUE" | grep "CYGWIN" 1>/dev/null 2>/dev/null; then
    PLATFORM_NAME="Cygwin"
elif [ "$UNAME_VALUE" == "Darwin" ]; then
    PLATFORM_NAME="Darwin"
else
    error_message "Cannot infer PLATFORM from uname value: $UNAME_VALUE"
    exit 1
fi
PLATFORM=$PLATFORM_NAME$BIT_SIZE
