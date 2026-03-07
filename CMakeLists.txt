cmake_minimum_required(VERSION 3.10)
set(PROJECT InterfaceElementsPrototype)

project(${PROJECT} LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Widgets OpenGL REQUIRED QUIET)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    set(APP_ICON_RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/UI/Resources/icon.rc")
endif()

add_executable(${PROJECT_NAME}
        main.cpp
        ${APP_ICON_RESOURCE}
        ${CMAKE_CURRENT_SOURCE_DIR}/UI/Resources/resources.qrc
        ${CMAKE_CURRENT_SOURCE_DIR}/UI/MainWindow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/UI/Canvas.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/UI/Utilities.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/TopologyDataStructures/CHE.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/TopologyDataStructures/IBHM.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/InterfaceElementsBuilder/CHEInterfaceElementBuilder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/InterfaceElementsBuilder/IBHMInterfaceElementBuilder.cpp
)

target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/TopologyDataStructures
        ${CMAKE_CURRENT_SOURCE_DIR}/InterfaceElementsBuilder
)


target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets Qt5::OpenGL opengl32)