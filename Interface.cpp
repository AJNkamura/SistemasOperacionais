#include "Interface.h"
#include "Escalonador.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QImage>
#include <QPainter>
#include <QStatusBar>
#include <QHeaderView>

Interface::Interface(QWidget *parent) : QMainWindow(parent) {
    //Constroi interface
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // Painel Esquerdo - Gráfico e Botões
    QVBoxLayout *leftLayout = new QVBoxLayout();
    
    cenaGantt = new QGraphicsScene(this);
    viewGantt = new QGraphicsView(cenaGantt);
    leftLayout->addWidget(viewGantt);

    QHBoxLayout *botoesLayout = new QHBoxLayout();
    QPushButton *btnCarregar = new QPushButton("Carregar TXT");
    QPushButton *btnRetroceder = new QPushButton("<< Retroceder");
    btnPlay = new QPushButton("Play Automático");
    QPushButton *btnAvancar = new QPushButton("Avançar Passo >>");
    QPushButton *btnExportar = new QPushButton("Exportar Imagem");

    botoesLayout->addWidget(btnCarregar);
    botoesLayout->addWidget(btnRetroceder);
    botoesLayout->addWidget(btnPlay);
    botoesLayout->addWidget(btnAvancar);
    botoesLayout->addWidget(btnExportar);
    leftLayout->addLayout(botoesLayout);

    // Painel Direito - Tabela de Tarefas
    QVBoxLayout *rightLayout = new QVBoxLayout(); 
    
    tabelaTarefas = new QTableWidget(0, 5);
    tabelaTarefas->setHorizontalHeaderLabels({"ID", "Estado", "Prio", "Ingresso", "Tempo Rest."});
    
    tabelaTarefas->setFixedWidth(380);
    tabelaTarefas->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tabelaTarefas->setEditTriggers(QAbstractItemView::DoubleClicked); 
    
    btnAplicarEdicao = new QPushButton("Aplicar mudança na Tabela"); 
    
    rightLayout->addWidget(tabelaTarefas);
    rightLayout->addWidget(btnAplicarEdicao);

    mainLayout->addLayout(leftLayout);
    mainLayout->addLayout(rightLayout);
    resize(1100, 600); 

    // Eventos
    connect(btnCarregar, &QPushButton::clicked, this, &Interface::btnCarregarClicado);
    connect(btnRetroceder, &QPushButton::clicked, this, &Interface::btnRetrocederClicado);
    connect(btnAvancar, &QPushButton::clicked, this, &Interface::btnAvancarClicado);
    connect(btnPlay, &QPushButton::clicked, this, &Interface::btnPlayPauseClicado);
    connect(btnExportar, &QPushButton::clicked, this, &Interface::btnExportarClicado);
    connect(btnAplicarEdicao, &QPushButton::clicked, this, [this]() { btnEdicao(); });

    timerPlay = new QTimer(this);
    connect(timerPlay, &QTimer::timeout, this, &Interface::tickAutomatico);
}

void Interface::btnCarregarClicado() {
    QString fileName = QFileDialog::getOpenFileName(this, "Abrir Configuração", "", "Text Files (*.txt)");
    if (!fileName.isEmpty()) {
        core.carregarArquivo(fileName.toStdString());
        atualizarUI();
    }
}

void Interface::btnAvancarClicado() {
    core.avancarTick();
    atualizarUI();
}

void Interface::btnRetrocederClicado() {
    core.retrocederTick();
    atualizarUI();
}

void Interface::btnPlayPauseClicado() {
    if (timerPlay->isActive()) {
        timerPlay->stop();
        btnPlay->setText("Play Automático");
    } else {
        timerPlay->start(300); 
        btnPlay->setText("Pausar");
    }
}

void Interface::tickAutomatico() {
    if (core.isFinalizado()) {
        btnPlayPauseClicado(); 
    } else {
        btnAvancarClicado();
    }
}

//Atualiza a tabela com o estado atual das tarefas
void Interface::atualizarUI() {
    tabelaTarefas->setRowCount(core.lista_tarefas.size());
    for (size_t i = 0; i < core.lista_tarefas.size(); ++i) {
        TCB &t = core.lista_tarefas[i];

        QString strEstado;
        if (t.tempo_restante <= 0 && t.estado != Estado::NOVO) {
            strEstado = "Terminada";
        } else {
            switch (t.estado) {
                case Estado::NOVO:       strEstado = "Novo";       break;
                case Estado::PRONTA:     strEstado = "Pronta";     break;
                case Estado::EXECUTANDO: strEstado = "Executando"; break;
                case Estado::SUSPENSA:   strEstado = "Suspensa";   break;
                case Estado::TERMINADA:  strEstado = "Terminada";  break;
                default:                 strEstado = "?";          break;
            }
        }

        QTableWidgetItem *itemID       = new QTableWidgetItem(QString("T%1").arg(t.id));
        QTableWidgetItem *itemEstado   = new QTableWidgetItem(strEstado);
        QTableWidgetItem *itemPrio     = new QTableWidgetItem(QString::number(t.prioridade));
        QTableWidgetItem *itemIngresso = new QTableWidgetItem(QString::number(t.tempo_ingresso));
        QTableWidgetItem *itemRestante = new QTableWidgetItem(QString::number(t.tempo_restante));

        itemID->setForeground(QBrush(Qt::black));
        itemEstado->setForeground(QBrush(Qt::black));
        itemPrio->setForeground(QBrush(Qt::black));
        itemIngresso->setForeground(QBrush(Qt::black));
        itemRestante->setForeground(QBrush(Qt::black));

        tabelaTarefas->setItem(i, 0, itemID);
        tabelaTarefas->setItem(i, 1, itemEstado);
        tabelaTarefas->setItem(i, 2, itemPrio);
        tabelaTarefas->setItem(i, 3, itemIngresso);
        tabelaTarefas->setItem(i, 4, itemRestante);
 
        // Coloriza o fundo da linha conforme o estado 
        QColor bg = Qt::white;
        if (t.tempo_restante <= 0 && t.estado != Estado::NOVO) bg = QColor("#e0e0e0"); 
        else if (t.estado == Estado::EXECUTANDO) bg = QColor("#c8f0c8"); 
        else if (t.estado == Estado::SUSPENSA)  bg = QColor("#f0c8c8"); 
        for (int col = 0; col < 5; col++) {
            if (tabelaTarefas->item(i, col))
                tabelaTarefas->item(i, col)->setBackground(bg);
        }
    }

    desenharGantt();

    QString statusMsg = QString("Tick Atual: %1  |  Tarefas Concluídas: %2/%3  |  Tempo Total de CPU Ociosa: %4 ticks")
                            .arg(core.clock_global)
                            .arg(core.tarefas_concluidas)
                            .arg(core.lista_tarefas.size())
                            .arg(core.tempo_ocioso);
    
    statusBar()->showMessage(statusMsg);
}

void Interface::desenharGantt() {
    cenaGantt->clear();
    int T_WIDTH = 30; 
    int CPU_HEIGHT = 50;
    const int LABEL_X = -65;

    int max_ticks = core.clock_global + 5; 
    int largura_total = max_ticks * T_WIDTH;
    int altura_total = core.qtde_cpus * CPU_HEIGHT;

    for (int t = 0; t <= max_ticks; t++) {
        int x = t * T_WIDTH;
        
        QPen penGrid(Qt::lightGray, 1, Qt::DashLine);
        cenaGantt->addLine(x, 0, x, altura_total, penGrid);
        
        QGraphicsTextItem* numTick = cenaGantt->addText(QString::number(t));
        numTick->setPos(x - 5, -20);
        numTick->setDefaultTextColor(Qt::darkGray);
    }

    for (int c = 0; c < core.qtde_cpus; ++c) {
        QGraphicsTextItem* cpuLabel = cenaGantt->addText(QString("CPU %1").arg(c + 1));
        cpuLabel->setPos(LABEL_X, c * CPU_HEIGHT + (CPU_HEIGHT / 4));
        cpuLabel->setDefaultTextColor(Qt::black);
        
        QPen penRow(Qt::gray, 1, Qt::SolidLine);
        cenaGantt->addLine(0, c * CPU_HEIGHT, largura_total, c * CPU_HEIGHT, penRow);
    }

    cenaGantt->addLine(0, altura_total, largura_total, altura_total, QPen(Qt::black, 2)); 
    cenaGantt->addLine(0, 0, 0, altura_total, QPen(Qt::black, 2));                                                        

    auto getCor = [](const std::vector<TCB>& tarefas, int tid) -> QString {
        for (const auto& t : tarefas) {
            if (t.id == tid) return QString("#") + QString::fromStdString(t.cor_hex);
        }
        return "#DDDDDD";
    };

    // Desenhar as tarefas conforme o histórico de fotos
    for (const auto& snap : core.historico) {
        //  o bloco se posiciona exatamente a partir de seu respectivo clock
        int x = snap.clock_global * T_WIDTH;
        
        for (int c = 0; c < core.qtde_cpus && c < (int)snap.ids_task_cpu.size(); ++c) {
            int tid = snap.ids_task_cpu[c];
            if (tid != -1) {
                Estado estado_na_foto = Estado::NOVO;
                int t_restante_na_foto = 1;
                
                for(const auto& t : snap.lista_tarefas) {
                    if(t.id == tid) { 
                        estado_na_foto = t.estado; 
                        t_restante_na_foto = t.tempo_restante;
                        break; 
                    }
                }

                QString corStr = (estado_na_foto == Estado::SUSPENSA) ? "#000000" : getCor(snap.lista_tarefas, tid);
                QColor corFundo(corStr);
                
                QGraphicsRectItem* rect = cenaGantt->addRect(x, c * CPU_HEIGHT, T_WIDTH, CPU_HEIGHT - 10);
                rect->setBrush(corFundo);
                rect->setPen(QPen(Qt::black));
                
                QGraphicsTextItem* texto = cenaGantt->addText(QString("T%1").arg(tid));
                texto->setPos(x + 2, c * CPU_HEIGHT + 2);
                texto->setDefaultTextColor((estado_na_foto == Estado::SUSPENSA) ? Qt::white : Qt::black);

                // Icones de entrada e saida
                for(const auto& t : snap.lista_tarefas) {
                    if(t.id == tid) {
                        if (snap.clock_global == t.tempo_ingresso) {
                            QGraphicsTextItem* iconeChegada = cenaGantt->addText("▶️");
                            iconeChegada->setPos(x + 10, c * CPU_HEIGHT - 15);
                        }
                        if (t_restante_na_foto == 0) {
                            QGraphicsTextItem* iconeFim = cenaGantt->addText("📍");
                            iconeFim->setPos(x + 10, c * CPU_HEIGHT + 15);
                        }
                    }
                }
            }
        }
    }
    
    //Ponteiro vermelho da linha do tempo
    int x_linha = core.clock_global * T_WIDTH;
    cenaGantt->addLine(x_linha, 0, x_linha, core.qtde_cpus * CPU_HEIGHT, QPen(Qt::red, 2));
}

void Interface::btnEdicao() {
    if (timerPlay->isActive()) return; 

    for (int i = 0; i < tabelaTarefas->rowCount(); ++i) {
        QString idStr = tabelaTarefas->item(i, 0)->text().replace("T", "");
        int idTarefa = idStr.toInt();

        int novaPrio = tabelaTarefas->item(i, 2)->text().toInt();
        int novoTempoRestante = tabelaTarefas->item(i, 4)->text().toInt();
        QString novoEstadoStr = tabelaTarefas->item(i, 1)->text().toLower();

        Estado novoEstado;
        if (novoEstadoStr == "pronta") novoEstado = Estado::PRONTA;
        else if (novoEstadoStr == "suspensa") novoEstado = Estado::SUSPENSA;
        else if (novoEstadoStr == "terminada") novoEstado = Estado::TERMINADA;
        else if (novoEstadoStr == "executando") novoEstado = Estado::EXECUTANDO;
        else novoEstado = Estado::NOVO;

        for (auto& t : core.lista_tarefas) {
            if (t.id == idTarefa) {
                t.prioridade = novaPrio;
                t.tempo_restante = novoTempoRestante;
                if (t.estado != novoEstado && t.estado != Estado::TERMINADA) {
                     t.estado = novoEstado;
                }
                break;
            }
        }
    }
    atualizarUI();
}

void Interface::btnExportarClicado() {
    cenaGantt->clearSelection();
    QRectF rect = cenaGantt->itemsBoundingRect();
    if (rect.isEmpty()) return;

    QImage image(rect.size().toSize(), QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    cenaGantt->render(&painter, QRectF(), rect);
    painter.end();
 
    QString path = QFileDialog::getSaveFileName(this, "Salvar Gantt", "resultado_gantt.png", "PNG (*.png)");
    if (!path.isEmpty()) {
        image.save(path);
    }
}