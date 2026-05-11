cmake_minimum_required(VERSION 3.16)
project(SimuladorSO VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Habilita o processamento automático das macros do Qt
set(CMAKE_AUTOMOC ON) 
set(CMAKE_AUTORCC ON) 

# Encontra as bibliotecas do Qt6 (Se usar Qt5, mude de Qt6 para Qt5 aqui e abaixo)
find_package(Qt5 REQUIRED COMPONENTS Widgets Core Gui)

include_directories(
    include/
)

add_executable(SimuladorSO
    src/main.cpp
    src/TCB.cpp
)

target_link_libraries(SimuladorSO PRIVATE Qt5::Widgets Qt5::Core Qt5::Gui)