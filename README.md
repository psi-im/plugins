# Officially supported Psi plugins

## Description

This repository contains all Psi plugins which their developers prefer to maintain them together with upstream.

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
# If necessary install all missed build dependencies until previous command is
# executed without errors.
make -j4
make install DESTDIR="../installdir"
# If necessary replace "../installdir" from command above to any path you need
# or copy them manually from "../installdir".

```

Some available configuration options:

* -DBUILD_PLUGINS="${plugins}"	- set list of plugins to build or to not build
* -DBUILD_DEV_PLUGINS="ON"	- enable build of some plugins from dev/ (disabled by default)
* -DPLUGINS_PATH="psi/plugins"	- set suffix for installation path to plugins
* -DPLUGINS_ROOT_DIR="${path}"	- set path to directory with `plugins.cmake` file

Option `-DPLUGINS_ROOT_DIR` should be used only when:

* Psi sources are located in non-standard way, for example, you have cloned git repo of Psi not as in the simple example above, but completely to another place.
* You have not installed Psi development files into your system or version of Psi development files in your system is not compatible with current version of plugins.

Examples:

```shell
# Build only few plugins:
cmake .. -DBUILD_PLUGINS="omemoplugin;openpgpplugin;otrplugin"

# Build all plugins except few ones:
cmake .. -DBUILD_PLUGINS="-chessplugin;-otrplugin"

# Build all plugins when git repo of Psi is placed in far far directory:
cmake .. -DPLUGINS_ROOT_DIR=/home/user/very/stange/path/to/psi/plugins
```


