if(NOT DEFINED PLUGIN_CMAKE OR NOT EXISTS "${PLUGIN_CMAKE}")
    message(FATAL_ERROR "OTR plugin CMakeLists.txt not found: ${PLUGIN_CMAKE}")
endif()

file(READ "${PLUGIN_CMAKE}" plugin_cmake)

# The normal plugin deliberately disables qca-otr's optional libotr oracle
# tests, so the option name itself may contain LIBOTR. Reject only constructs
# that would actually discover or link legacy runtime libraries.
set(forbidden_runtime_references
    "find_package(LIBOTR"
    "find_package(LIBGCRYPT"
    "find_package(LIBGPGERROR"
    "find_package(LIBTIDY"
    "pkg_check_modules(LIBOTR"
    "pkg_check_modules(GCRYPT"
    "pkg_check_modules(GPGERROR"
    "pkg_check_modules(TIDY"
    "PkgConfig::LIBOTR"
    "PkgConfig::GCRYPT"
    "PkgConfig::GPGERROR"
    "PkgConfig::TIDY"
    "\${LIBOTR_"
    "\${LIBGCRYPT_"
    "\${LIBGPGERROR_"
    "\${LIBTIDY_"
)

foreach(forbidden IN LISTS forbidden_runtime_references)
    string(FIND "${plugin_cmake}" "${forbidden}" position)
    if(NOT position EQUAL -1)
        message(FATAL_ERROR "Normal OTR plugin build still references forbidden runtime dependency: ${forbidden}")
    endif()
endforeach()
