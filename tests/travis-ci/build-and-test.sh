#!/bin/sh

# Author:  Boris Pek
# Version: N/A
# License: Public Domain

set -e
set -x

if [ ! -d "../psi" ]
then
    git clone https://github.com/psi-im/psi.git
    mv psi ../
fi

if [ "${TARGET}" = "linux64" ]
then
    ./tests/travis-ci/build-in-ubuntu.sh
elif [ "${TARGET}" = "macos64" ]
then
    ./tests/travis-ci/build-in-macos.sh
elif [ "${TARGET}" = "windows64" ]
then
    ./tests/travis-ci/build-for-windows.sh
else
    echo "Unknown target!"
    exit 1
fi

cd ./builddir/psi/plugins/
ls -alp *
du -shc *

file libextendedoptionsplugin.* \
     libomemoplugin.* \
     libopenpgpplugin.* \
     libotrplugin.*

