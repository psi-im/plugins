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
    export HOMEBREW="/usr/local"
    ./tests/travis-ci/build-in-macos.sh
fi

ls -alp ./builddir/plugins/*
du -shc ./builddir/plugins/*

