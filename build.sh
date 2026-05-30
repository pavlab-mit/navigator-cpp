#!/bin/bash

INVOCATION_ABS_DIR=$(pwd)
BUILD_TYPE="None"
BUILD_UNIT_TESTS="OFF"
BUILD_EXAMPLES="OFF"
CMD_LINE_ARGS=""

#-------------------------------------------------------------------
#  Part 1: Check for and handle command-line arguments
#-------------------------------------------------------------------
for ARGI; do
    if [ "${ARGI}" = "--help" ] || [ "${ARGI}" = "-h" ]; then
        printf "%s [SWITCHES]                       \n" $0
        printf "Switches:                           \n"
        printf "  --help, -h                        \n"
        printf "  --debug,   -d                     \n"
        printf "  --release, -r                     \n"
        printf "  --unit_tests                      \n"
        printf "  --examples                        \n"
        printf "Notes:                              \n"
        printf " (1) All other command line args will be passed as args    \n"
        printf "     to \"make\" when it is eventually invoked.            \n"
        printf " (2) For example -j2 will utilize a 2nd core in the build  \n"
        printf "     if your machine has two cores. -j4 etc for quad core. \n"
        exit 0
    elif [ "${ARGI}" = "--debug" ] || [ "${ARGI}" = "-d" ]; then
        BUILD_TYPE="Debug"
    elif [ "${ARGI}" = "--release" ] || [ "${ARGI}" = "-r" ]; then
        BUILD_TYPE="Release"
    elif [ "${ARGI}" = "--unit_tests" ]; then
        BUILD_UNIT_TESTS="ON"
    elif [ "${ARGI}" = "--examples" ]; then
        BUILD_EXAMPLES="ON"
    else
        CMD_LINE_ARGS="${CMD_LINE_ARGS} ${ARGI}"
    fi
done

#-------------------------------------------------------------------
#  Part 2: Detect OS and calculate default -j value
#-------------------------------------------------------------------
if [[ "$OSTYPE" == "darwin"* ]]; then
    NUM_CORES=$(sysctl -n hw.ncpu)
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    NUM_CORES=$(nproc)
else
    NUM_CORES=1
fi

if [ "$NUM_CORES" -gt 1 ]; then
    DEFAULT_J=$((NUM_CORES - 1))
else
    DEFAULT_J=1
fi

if [[ ! "$CMD_LINE_ARGS" =~ "-j" ]]; then
    CMD_LINE_ARGS="${CMD_LINE_ARGS} -j${DEFAULT_J}"
fi

#-------------------------------------------------------------------
#  Part 3: Build
#-------------------------------------------------------------------
mkdir -p build
cd build || exit 1

cmake -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
      -DBUILD_UNIT_TESTS="${BUILD_UNIT_TESTS}" \
      -DBUILD_EXAMPLES="${BUILD_EXAMPLES}" \
      ../

make ${CMD_LINE_ARGS}

cd "${INVOCATION_ABS_DIR}" || exit 1
