// Ponto de entrada da aplicação Qt.
// Cria o QApplication (necessário para qualquer app Qt) e exibe a janela principal.
#include <QApplication>
#include "MainWindow.h"
 
int main(int argc, char *argv[]) {       
    QApplication app(argc, argv);        
    MainWindow window;
    window.setWindowTitle("Simulador de Escalonamento SO");
    window.show();
    return app.exec();                   // Inicia o loop de eventos Qt
}
