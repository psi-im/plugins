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
if( OMEMO_C_INCLUDE_DIR AND OMEMO_C_LIBRARY )
    # in cache already
    set(OMEMO_C_FIND_QUIETLY TRUE)
endif()

if( UNIX AND NOT( APPLE OR CYGWIN ) )
    find_package( PkgConfig QUIET )
    pkg_check_modules( OMEMO_C QUIET omemo-c )
    if( PC_OMEMO_C_FOUND )
        set( OMEMO_C_DEFINITIONS ${PC_OMEMO_C_CFLAGS} ${PC_OMEMO_C_CFLAGS_OTHER} )
    endif()
endif()

set( OMEMO_C_ROOT "" CACHE STRING "Path to omemo-c library" )

find_path(
    OMEMO_C_INCLUDE_DIR signal_protocol.h
    HINTS
    ${OMEMO_C_ROOT}/include
    ${PC_OMEMO_C_INCLUDEDIR}
    ${PC_OMEMO_C_INCLUDE_DIRS}
    PATH_SUFFIXES
    ""
    omemo
)

set(OMEMO_C_NAMES
    omemo-c
    libomemo-c
)
find_library(
    OMEMO_C_LIBRARY omemo-c
    NAMES ${OMEMO_C_NAMES}
    HINTS
    ${PC_OMEMO_C_LIBDIR}
    ${PC_OMEMO_C_LIBRARY_DIRS}
    ${OMEMO_C_ROOT}/lib
    ${OMEMO_C_ROOT}/bin
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    OMEMO_C
    DEFAULT_MSG
    OMEMO_C_LIBRARY
    OMEMO_C_INCLUDE_DIR
)

if( OMEMO_C_FOUND )
    set( OMEMO_C_LIBRARIES ${OMEMO_C_LIBRARY} )
    set( OMEMO_C_INCLUDE_DIRS ${OMEMO_C_INCLUDE_DIR} )
    add_library(omemo-c UNKNOWN IMPORTED ${OMEMO_C_LIBRARY})
    set_property(TARGET omemo-c PROPERTY
        IMPORTED_LOCATION "${OMEMO_C_LIBRARY}")
    add_library(omemo-c::omemo-c ALIAS omemo-c)
    target_include_directories(omemo-c INTERFACE "${OMEMO_C_INCLUDE_DIR}")
endif()

mark_as_advanced( OMEMO_C_INCLUDE_DIR OMEMO_C_LIBRARY )
