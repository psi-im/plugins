#!/bin/sh

# Author:  Boris Pek
# Version: N/A
# License: Public Domain

set -e
set -x

PATH="/usr/lib/mxe/usr/bin:${PATH}"

[ -z "${BUILD_DEV_PLUGINS}" ] && BUILD_DEV_PLUGINS="OFF"

BUILD_OPTIONS="-DCMAKE_BUILD_TYPE=Release \
               -DBUILD_DEV_PLUGINS=${BUILD_DEV_PLUGINS} \
               "

mkdir -p builddir
cd builddir

if [ "${TARGET}" = "windows64" ]
then
    export PREFIX="x86-64-w64-mingw32.shared"
else
    echo "Unknown target!"
    exit 1
fi

${PREFIX}-cmake .. ${BUILD_OPTIONS}
make -k -j $(nproc) VERBOSE=1

