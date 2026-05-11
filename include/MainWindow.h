#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include "SimCore.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void btnCarregarClicado();
    void btnAvancarClicado();
    void btnRetrocederClicado();
    void btnPlayPauseClicado();
    void btnExportarClicado();
    void tickAutomatico();

private:
    CoreSimulacao core;
    QTimer *timerPlay;

    // Elementos da UI
    QGraphicsView *viewGantt;
    QGraphicsScene *cenaGantt;
    QTableWidget *tabelaTarefas;
    QPushButton *btnPlay;

    void atualizarUI();
    void desenharGantt();
};

#endif