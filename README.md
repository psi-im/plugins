# Psi IM plugins

## Description

This repository contains all Psi IM plugins which developers prefer to maintain them together with upstream.

* generic/	- plugins for all platforms (Linux, Windows, macOS, *BSD, Haiku, etc.)
* unix/		- plugins for unix-based systems (Linux, *BSD)
* dev/		- plugins in development stage (may be incomplete or unstable)
* deprecated/	- plugins which don't work and not supported anymore (stored for historical reasons)

## Installation

Normally you don't need compile these plugins yourself: they are distributed together with main program (in special packages for your Linux distro, in installer or archive for Windows, in app bundle for macOS, etc.). But if you need to build plugins yourself see instructions below.

All plugins may be built separately using qmake or cmake or they may be built together using cmake.

The most fast and simple way to build all plugins from master branch:

```shell
git clone https://github.com/psi-im/psi.git
git clone https://github.com/psi-im/plugins.git
cd plugins
mkdir builddir
cd builddir
cmake ..
# If necessary install all missed build dependencies until previous command is executed without errors.
make -j4
make install DESTDIR="../installdir"
# If necessary replace "../installdir" from command above to any path you need
# or copy them manually from "../installdir".

```

Some available configuration options (for this type of build):

* 1
* 2
* 3
