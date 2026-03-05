cmake_minimum_required(VERSION 3.10)

project(InterfaceElementsPrototype LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(InterfaceElementsPrototype
        main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/TopologyDataStructures/CHE.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/TopologyDataStructures/IBHM.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/InterfaceElementsBuilder/CHEInterfaceElementBuilder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/InterfaceElementsBuilder/IBHMInterfaceElementBuilder.cpp
)

target_include_directories(InterfaceElementsPrototype
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/TopologyDataStructures
        ${CMAKE_CURRENT_SOURCE_DIR}/InterfaceElementsBuilder
)
