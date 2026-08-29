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
