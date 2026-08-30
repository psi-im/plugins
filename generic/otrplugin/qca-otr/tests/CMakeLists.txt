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

add_executable(qca-otr-data-test
    datatest.cpp
)
target_link_libraries(qca-otr-data-test
    PRIVATE
        QcaOtr::QcaOtr
        Qt${QT_DEFAULT_MAJOR_VERSION}::Test
)
add_test(NAME qca-otr-data COMMAND qca-otr-data-test)

add_executable(qca-otr-negotiation-test
    negotiationtest.cpp
)
target_link_libraries(qca-otr-negotiation-test
    PRIVATE
        QcaOtr::QcaOtr
        Qt${QT_DEFAULT_MAJOR_VERSION}::Test
)
add_test(NAME qca-otr-negotiation COMMAND qca-otr-negotiation-test)

add_executable(qca-otr-tlv-test
    tlvtest.cpp
)
target_link_libraries(qca-otr-tlv-test
    PRIVATE
        QcaOtr::QcaOtr
        Qt${QT_DEFAULT_MAJOR_VERSION}::Test
)
add_test(NAME qca-otr-tlv COMMAND qca-otr-tlv-test)

add_executable(qca-otr-transport-test
    transporttest.cpp
)
target_link_libraries(qca-otr-transport-test
    PRIVATE
        QcaOtr::QcaOtr
        Qt${QT_DEFAULT_MAJOR_VERSION}::Test
)
add_test(NAME qca-otr-transport COMMAND qca-otr-transport-test)

add_executable(qca-otr-session-test
    sessiontest.cpp
)
target_link_libraries(qca-otr-session-test
    PRIVATE
        QcaOtr::QcaOtr
        Qt${QT_DEFAULT_MAJOR_VERSION}::Test
)
add_test(NAME qca-otr-session COMMAND qca-otr-session-test)

add_executable(qca-otr-control-test
    controltest.cpp
)
target_link_libraries(qca-otr-control-test
    PRIVATE
        QcaOtr::QcaOtr
        Qt${QT_DEFAULT_MAJOR_VERSION}::Test
)
add_test(NAME qca-otr-control COMMAND qca-otr-control-test)

add_executable(qca-otr-symkey-test
    symkeytest.cpp
)
target_link_libraries(qca-otr-symkey-test
    PRIVATE
        QcaOtr::QcaOtr
        Qt${QT_DEFAULT_MAJOR_VERSION}::Test
)
add_test(NAME qca-otr-symkey COMMAND qca-otr-symkey-test)

add_executable(qca-otr-smp-test
    smptest.cpp
)
target_link_libraries(qca-otr-smp-test
    PRIVATE
        QcaOtr::QcaOtr
        Qt${QT_DEFAULT_MAJOR_VERSION}::Test
)
add_test(NAME qca-otr-smp COMMAND qca-otr-smp-test)

add_executable(qca-otr-smp-session-test
    smpsessiontest.cpp
)
target_link_libraries(qca-otr-smp-session-test
    PRIVATE
        QcaOtr::QcaOtr
        Qt${QT_DEFAULT_MAJOR_VERSION}::Test
)
add_test(NAME qca-otr-smp-session COMMAND qca-otr-smp-session-test)

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

    add_executable(qca-otr-fragment-interop-test
        fragmentinteroptest.cpp
    )
    target_link_libraries(qca-otr-fragment-interop-test
        PRIVATE
            QcaOtr::QcaOtr
            Qt${QT_DEFAULT_MAJOR_VERSION}::Test
            PkgConfig::LIBOTR
            PkgConfig::GCRYPT
    )
    add_test(NAME qca-otr-fragment-interop COMMAND qca-otr-fragment-interop-test)

    add_executable(qca-otr-session-interop-test
        sessioninteroptest.cpp
    )
    target_link_libraries(qca-otr-session-interop-test
        PRIVATE
            QcaOtr::QcaOtr
            Qt${QT_DEFAULT_MAJOR_VERSION}::Test
            PkgConfig::LIBOTR
            PkgConfig::GCRYPT
    )
    add_test(NAME qca-otr-session-interop COMMAND qca-otr-session-interop-test)

    add_executable(qca-otr-control-interop-test
        controlinteroptest.cpp
    )
    target_link_libraries(qca-otr-control-interop-test
        PRIVATE
            QcaOtr::QcaOtr
            Qt${QT_DEFAULT_MAJOR_VERSION}::Test
            PkgConfig::LIBOTR
            PkgConfig::GCRYPT
    )
    add_test(NAME qca-otr-control-interop COMMAND qca-otr-control-interop-test)
endif()
