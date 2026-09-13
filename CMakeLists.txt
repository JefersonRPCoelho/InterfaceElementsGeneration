cmake_minimum_required(VERSION 3.10)
set(PROJECT InterfaceElementsPrototype)

project(${PROJECT} LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Widgets OpenGL QUIET)

if (Qt5_FOUND)
    message(STATUS "Qt5 found!")
    set(QT_VERSION_MAJOR 5)
    set(QT_WIDGETS_LIB Qt5::Widgets)
    set(QT_OPENGL_LIB Qt5::OpenGL opengl32)
else ()
#    set(Qt_DIR "/opt/homebrew/opt/qt/lib/cmake/Qt6")  # For Qt6
    find_package(Qt6 COMPONENTS Widgets OpenGLWidgets QUIET)
    if (Qt6_FOUND)
        message(STATUS "Qt6 found!")
        set(QT_VERSION_MAJOR 6)
        set(QT_WIDGETS_LIB Qt6::Widgets)
        set(QT_OPENGL_LIB Qt6::OpenGLWidgets)
    else ()
        message(FATAL_ERROR "No qt found!")
    endif ()
endif ()

include(FetchContent)

FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG v2.13.6
)

FetchContent_MakeAvailable(pybind11)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    set(APP_ICON_RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/UI/Resources/icon.rc")
endif()

add_executable(${PROJECT_NAME}
        main.cpp
        ${APP_ICON_RESOURCE}
        core/UI/Resources/resources.qrc
        core/UI/MainWindow.cpp
        core/UI/Canvas.cpp
        core/UI/Utilities.cpp

        core/TopologyDataStructures/CHE.cpp
        core/TopologyDataStructures/IBHM.cpp
        core/InterfaceElementsBuilder/CHEInterfaceElementBuilder.cpp
        core/InterfaceElementsBuilder/IBHMInterfaceElementBuilder.cpp
)

target_include_directories(${PROJECT_NAME}
        PRIVATE
        core/TopologyDataStructures
        core/InterfaceElementsBuilder
)

add_library(InterfaceElementsLib
        core/TopologyDataStructures/IBHM.cpp
        core/InterfaceElementsBuilder/IBHMInterfaceElementBuilder.cpp
)

target_include_directories(InterfaceElementsLib
        PRIVATE
        core/TopologyDataStructures
        core/InterfaceElementsBuilder
)

pybind11_add_module(pyInterfaceElementsLib
        bindings/InterfaceElementsWrapper.cpp
)

target_link_libraries(pyInterfaceElementsLib
        PRIVATE
        InterfaceElementsLib
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${QT_WIDGETS_LIB} ${QT_OPENGL_LIB})