#!/bin/sh

# Author:  Boris Pek
# Version: N/A
# License: Public Domain

set -e
set -x

PATH="${HOMEBREW}/bin:${PATH}"
CUR_DIR="$(dirname $(realpath -s ${0}))"
TOOLCHAIN_FILE="${CUR_DIR}/homebrew-toolchain.cmake"

[ -z "${BUILD_DEV_PLUGINS}" ] && BUILD_DEV_PLUGINS="OFF"

BUILD_OPTIONS="-DCMAKE_BUILD_TYPE=Release \
               -DBUILD_DEV_PLUGINS=${BUILD_DEV_PLUGINS} \
               .."

mkdir -p builddir
cd builddir

cmake ${BUILD_OPTIONS} \
      -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}"
make VERBOSE=1 -k -j4

