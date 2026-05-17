#include <QApplication>
#include "Interface.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    Interface window;
    window.setWindowTitle("Simulador de Escalonamento Multiprocessado");
    window.show();

    return app.exec();
}