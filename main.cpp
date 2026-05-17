#include <QApplication>
#include "include/Interface.h" // Aponta para a pasta correta

int main(int argc, char *argv[]) {       
    QApplication app(argc, argv);        
    Escalonador window; 
    window.setWindowTitle("Simulador de Escalonamento SO");
    window.show();
    return app.exec(); 
}