#=============================================================================
# Copyright 2016-2018 Psi+ Project, Vitaly Tonkacheyev
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================
if( SIGNAL_PROTOCOL_C_INCLUDE_DIR AND SIGNAL_PROTOCOL_C_LIBRARY )
    # in cache already
    set(SIGNAL_PROTOCOL_C_FIND_QUIETLY TRUE)
endif( SIGNAL_PROTOCOL_C_INCLUDE_DIR AND SIGNAL_PROTOCOL_C_LIBRARY )

if( UNIX AND NOT( APPLE OR CYGWIN ) )
    find_package( PkgConfig QUIET )
    pkg_check_modules( PC_SIGNAL_PROTOCOL_C QUIET libsignal-protocol-c )
    if( PC_SIGNAL_PROTOCOL_C_FOUND )
        set( SIGNAL_PROTOCOL_C_DEFINITIONS ${PC_SIGNAL_PROTOCOL_C_CFLAGS} ${PC_SIGNAL_PROTOCOL_C_CFLAGS_OTHER} )
    endif( PC_SIGNAL_PROTOCOL_C_FOUND )
endif( UNIX AND NOT( APPLE OR CYGWIN ) )

set( SIGNAL_PROTOCOL_C_ROOT "" CACHE STRING "Path to signal-protocol-c library" )

find_path(
    SIGNAL_PROTOCOL_C_INCLUDE_DIR signal_protocol.h
    HINTS
    ${PC_SIGNAL_PROTOCOL_C_INCLUDEDIR}
    ${PC_SIGNAL_PROTOCOL_C_INCLUDE_DIRS}
    ${SIGNAL_PROTOCOL_C_ROOT}/include
    PATH_SUFFIXES
    signal
)

find_library(
    SIGNAL_PROTOCOL_C_LIBRARY signal-protocol-c
    HINTS
    ${PC_SIGNAL_PROTOCOL_C_LIBDIR}
    ${PC_SIGNAL_PROTOCOL_C_LIBRARY_DIRS}
    ${SIGNAL_PROTOCOL_C_ROOT}/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    SignalProtocolC
    DEFAULT_MSG
    SIGNAL_PROTOCOL_C_LIBRARY
    SIGNAL_PROTOCOL_C_INCLUDE_DIR
)

mark_as_advanced( SIGNAL_PROTOCOL_C_INCLUDE_DIR SIGNAL_PROTOCOL_C_LIBRARY )
