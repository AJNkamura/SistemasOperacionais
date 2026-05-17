QT += core gui widgets

CONFIG += c++17
CONFIG += sdk_no_version_check

TARGET = SimuladorEscalonamento
TEMPLATE = app

# Todos os arquivos juntos na mesma pasta, sem caminhos complicados
HEADERS += \
    Escalonador.h \
    Interface.h \
    TCB.h

SOURCES += \
    main.cpp \
    Escalonador.cpp \
    Interface.cpp \
    TCB.cpp