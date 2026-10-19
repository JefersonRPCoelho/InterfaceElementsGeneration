# Automated tests: Google Test suites, the test driver of the report, and the random stress test, all run by CTest.
#
#     ctest                    all tests (the unit tests take about a second, the stress test about half a minute)
#     ctest -L unit            only the Google Test suites
#     ctest -L stress          only the stress test
#
# The tests whose name starts with DISABLED_ describe known defects that are still open (test_known_defects.cpp).

# The tests rely on the assertions of the library: some defects show up only as an assertion, and the death tests
# expect them. So NDEBUG is removed from the flags of this directory, whatever the build type, and the core is compiled
# again here (InterfaceElementsLibChecked) instead of using InterfaceElementsLib.
foreach (flags CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_MINSIZEREL)
    string(REGEX REPLACE "[-/]DNDEBUG" "" ${flags} "${${flags}}")
endforeach ()

# Google Test: an installed package is used if there is one; otherwise, the pinned release is downloaded when the
# project is configured.
find_package(GTest CONFIG QUIET)
if (NOT GTest_FOUND)
    include(FetchContent)
    if (POLICY CMP0135)
        cmake_policy(SET CMP0135 NEW)
    endif ()
    FetchContent_Declare(googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
            URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
    )
    # Use the same C runtime as the project on Windows, and do not install or build Google Mock.
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif ()
include(GoogleTest)

# The core with the assertions enabled.
add_library(InterfaceElementsLibChecked STATIC ${INTERFACE_ELEMENTS_CORE_SOURCES})
target_include_directories(InterfaceElementsLibChecked PUBLIC ${INTERFACE_ELEMENTS_CORE_INCLUDE_DIRS})

# Code shared by the tests and the driver: test meshes, topology checks (V1 to V7) and the builder inspector.
add_library(InterfaceElementsTestSupport STATIC
        support/MeshValidation.cpp
        support/TestMeshes.cpp
)
target_include_directories(InterfaceElementsTestSupport PUBLIC support)
target_link_libraries(InterfaceElementsTestSupport PUBLIC InterfaceElementsLibChecked)

# Google Test suites. Each test runs as a separate CTest test (in its own process), so an assertion in the library
# fails only that test. The timeout turns an infinite loop into a failure.
add_executable(InterfaceElementsTests
        test_ibhm.cpp
        test_vertex_count.cpp
        test_base_mesh.cpp
        test_invalid_input.cpp
        test_known_defects.cpp
)
target_link_libraries(InterfaceElementsTests PRIVATE InterfaceElementsTestSupport GTest::gtest_main)
gtest_discover_tests(InterfaceElementsTests PROPERTIES LABELS unit TIMEOUT 60)

# Test driver of the report (doc/report/tools/driver.cpp), used by the stress test and to generate the figures.
add_executable(driver ${PROJECT_SOURCE_DIR}/doc/report/tools/driver.cpp)
target_link_libraries(driver PRIVATE InterfaceElementsTestSupport)

# Random stress test (doc/report/tools/stress.py), with the seeds of the report.
#   stress_supported  seed 22, without the configuration of defect I1: must pass.
#   stress_all        seed 21, all configurations: disabled while I1 is open (141 of the 400 cases abort).
find_package(Python3 COMPONENTS Interpreter)
if (Python3_FOUND)
    set(STRESS ${PROJECT_SOURCE_DIR}/doc/report/tools/stress.py)
    add_test(NAME stress_supported
            COMMAND ${Python3_EXECUTABLE} -I ${STRESS} $<TARGET_FILE:driver> 22 400
            ${CMAKE_CURRENT_BINARY_DIR}/stress_supported --skip-border-multi)
    add_test(NAME stress_all
            COMMAND ${Python3_EXECUTABLE} -I ${STRESS} $<TARGET_FILE:driver> 21 400
            ${CMAKE_CURRENT_BINARY_DIR}/stress_all)
    set_tests_properties(stress_supported stress_all PROPERTIES LABELS stress TIMEOUT 1800)
    set_tests_properties(stress_all PROPERTIES DISABLED TRUE)
else ()
    message(STATUS "Python 3 not found: the stress test is not registered")
endif ()
