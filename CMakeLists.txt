cmake_minimum_required(VERSION 3.16)
project(SimuladorSO VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Habilita o processamento automático das macros do Qt
set(CMAKE_AUTOMOC ON) 
set(CMAKE_AUTORCC ON) 

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Core Gui)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Core Gui)

add_executable(SimuladorSO
    src/main.cpp
    src/TCB.cpp
    src/Escalonador.cpp
    src/Interface.cpp
    include/Interface.h
    include/Escalonador.h
    include/TCB.h
)

target_include_directories(SimuladorSO PRIVATE include/)

set_target_properties(SimuladorSO PROPERTIES
    AUTOMOC_MOC_OPTIONS "-I${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(SimuladorSO PRIVATE 
    Qt${QT_VERSION_MAJOR}::Widgets 
    Qt${QT_VERSION_MAJOR}::Core 
    Qt${QT_VERSION_MAJOR}::Gui
)