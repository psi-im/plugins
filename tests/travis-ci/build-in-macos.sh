#!/bin/sh

# Author:  Boris Pek
# Version: N/A
# License: Public Domain

set -e
set -x

[ -z "${HOMEBREW}" ] && HOMEBREW="/usr/local"

PATH="${HOMEBREW}/bin:${PATH}"
PATH="${HOMEBREW}/opt/ccache/libexec:${PATH}"
CUR_DIR="$(dirname $(realpath -s ${0}))"
TOOLCHAIN_FILE="${CUR_DIR}/homebrew-toolchain.cmake"

[ -z "${BUILD_DEV_PLUGINS}" ] && BUILD_DEV_PLUGINS="OFF"

BUILD_OPTIONS="-DCMAKE_BUILD_TYPE=Release \
               -DBUILD_DEV_PLUGINS=${BUILD_DEV_PLUGINS}"

mkdir -p builddir
cd builddir

which nproc > /dev/null && JOBS=$(nproc) || JOBS=4

cmake .. -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" ${BUILD_OPTIONS}
make -k -j ${JOBS} VERBOSE=1

