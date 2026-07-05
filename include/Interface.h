#ifndef INTERFACE_H
#define INTERFACE_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include "Escalonador.h"

class Escalonador;

class Interface : public QMainWindow {
    Q_OBJECT
public:
    Interface(QWidget *parent = nullptr);

private slots:
    void btnCarregarClicado();
    void btnAvancarClicado();
    void btnRetrocederClicado();
    void btnPlayPauseClicado();
    void btnExportarClicado();
    void tickAutomatico();

private:
    Escalonador core;
    QTimer *timerPlay;

    QGraphicsView *viewGantt;
    QGraphicsScene *cenaGantt;
    QTableWidget *tabelaTarefas;
    QPushButton *btnPlay;
    QPushButton* btnAplicarEdicao;

    void atualizarUI();
    void desenharGantt();
    void btnEdicao();
};

#endif