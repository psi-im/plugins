find_package(Qt${QT_DEFAULT_MAJOR_VERSION} REQUIRED COMPONENTS Widgets Xml)

add_executable(qca-otr-psi-adapter-compile-test
    ../tests/psiadaptercompiletest.cpp
    ../../src/otrmessaging.cpp
    ../../src/otrinternal.cpp
    ../../src/htmlnormalizer.cpp
)
target_include_directories(qca-otr-psi-adapter-compile-test
    PRIVATE
        ../../src
)
target_link_libraries(qca-otr-psi-adapter-compile-test
    PRIVATE
        QcaOtr::QcaOtr
        Qt${QT_DEFAULT_MAJOR_VERSION}::Widgets
        Qt${QT_DEFAULT_MAJOR_VERSION}::Xml
)
add_test(NAME qca-otr-psi-adapter-compile COMMAND qca-otr-psi-adapter-compile-test)

add_test(
    NAME qca-otr-runtime-dependency-guard
    COMMAND ${CMAKE_COMMAND}
        -DPLUGIN_CMAKE=${CMAKE_CURRENT_LIST_DIR}/../../CMakeLists.txt
        -P ${CMAKE_CURRENT_LIST_DIR}/dependencyguard.cmake
)
