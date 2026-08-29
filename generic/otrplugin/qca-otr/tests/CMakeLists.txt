find_package(Qt${QT_DEFAULT_MAJOR_VERSION} REQUIRED COMPONENTS Test)

add_executable(qca-otr-crypto-test
    cryptotest.cpp
)
target_link_libraries(qca-otr-crypto-test
    PRIVATE
        QcaOtr::QcaOtr
        Qt${QT_DEFAULT_MAJOR_VERSION}::Test
)
add_test(NAME qca-otr-crypto COMMAND qca-otr-crypto-test)

add_executable(qca-otr-wire-test
    wiretest.cpp
)
target_link_libraries(qca-otr-wire-test
    PRIVATE
        QcaOtr::QcaOtr
        Qt${QT_DEFAULT_MAJOR_VERSION}::Test
)
add_test(NAME qca-otr-wire COMMAND qca-otr-wire-test)

add_executable(qca-otr-ake-test
    aketest.cpp
)
target_link_libraries(qca-otr-ake-test
    PRIVATE
        QcaOtr::QcaOtr
        Qt${QT_DEFAULT_MAJOR_VERSION}::Test
)
add_test(NAME qca-otr-ake COMMAND qca-otr-ake-test)

add_executable(qca-otr-ake-session-test
    akesessiontest.cpp
)
target_link_libraries(qca-otr-ake-session-test
    PRIVATE
        QcaOtr::QcaOtr
        Qt${QT_DEFAULT_MAJOR_VERSION}::Test
)
add_test(NAME qca-otr-ake-session COMMAND qca-otr-ake-session-test)

if(QCA_OTR_BUILD_LIBOTR_INTEROP_TESTS)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBOTR REQUIRED IMPORTED_TARGET libotr)
    pkg_check_modules(GCRYPT REQUIRED IMPORTED_TARGET libgcrypt)

    add_executable(qca-otr-libotr-interop-test
        libotrinteroptest.cpp
    )
    target_link_libraries(qca-otr-libotr-interop-test
        PRIVATE
            QcaOtr::QcaOtr
            Qt${QT_DEFAULT_MAJOR_VERSION}::Test
            PkgConfig::LIBOTR
            PkgConfig::GCRYPT
    )
    add_test(NAME qca-otr-libotr-interop COMMAND qca-otr-libotr-interop-test)
endif()
