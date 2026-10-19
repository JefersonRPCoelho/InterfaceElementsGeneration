cmake_minimum_required(VERSION 3.16)
set(PROJECT InterfaceElementsPrototype)

project(${PROJECT} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Sources and include directories of the core: the IBHM and the interface element builder. The core has no external
# dependency. The lists are also used by the tests, which compile the core again with the assertions enabled.
set(INTERFACE_ELEMENTS_CORE_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/core/TopologyDataStructures/IBHM.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/InterfaceElementsBuilder/IBHMInterfaceElementBuilder.cpp
)
set(INTERFACE_ELEMENTS_CORE_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/core/TopologyDataStructures
        ${CMAKE_CURRENT_SOURCE_DIR}/core/InterfaceElementsBuilder
)

add_library(InterfaceElementsLib ${INTERFACE_ELEMENTS_CORE_SOURCES})
target_include_directories(InterfaceElementsLib PUBLIC ${INTERFACE_ELEMENTS_CORE_INCLUDE_DIRS})

# Prototype executable. It also compiles the legacy CHE experiment, used by main.cpp.
add_executable(${PROJECT_NAME}
        main.cpp
        core/TopologyDataStructures/CHE.cpp
        core/InterfaceElementsBuilder/CHEInterfaceElementBuilder.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE InterfaceElementsLib)

# Automated tests (Google Test, run by CTest). See tests/CMakeLists.txt.
option(INTERFACE_ELEMENTS_BUILD_TESTS "Build the automated tests" ON)
if (INTERFACE_ELEMENTS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()
