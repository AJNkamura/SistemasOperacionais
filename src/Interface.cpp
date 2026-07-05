#include "Interface.h"
#include "Escalonador.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QLabel>
#include <QImage>
#include <QPainter>
#include <QStatusBar>
#include <QHeaderView>
#include <QScrollArea>
#include <map>

Interface::Interface(QWidget *parent) : QMainWindow(parent) {
    //Constroi interface
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // Painel Esquerdo - Gráfico e Botões
    QVBoxLayout *leftLayout = new QVBoxLayout();
    cenaGantt = new QGraphicsScene(this);
    viewGantt = new QGraphicsView(cenaGantt);
    viewGantt->setAlignment(Qt::AlignTop | Qt::AlignLeft); 
    leftLayout->addWidget(viewGantt);
    // Botões da parte de baixo
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
    
    // Legenda para o Gantt abaixo da lista de tarefas
    QLabel* textLegenda = new QLabel();
    textLegenda->setTextFormat(Qt::RichText);
    textLegenda->setWordWrap(true);
    textLegenda->setFixedHeight(125);
    textLegenda->setStyleSheet("background-color: #fcfcfc; border: 1px solid #ccc; font-size: 13px; padding: 6px; border-radius: 4px;");
    textLegenda->setText(
        "<b style='color: #333;'>Legenda de Eventos e Gráficos:</b><br>"
        "▶️ <b>Ingresso:</b> Tarefa chegou | 📍 <b>Término:</b> Fim da execução | 🎲 <b>Sorteio:</b> Desempate<br>"
        "⬇️ <b>Mutex Lock (P):</b> Obteve recurso | ⬆️ <b>Mutex Unlock (V):</b> Liberou recurso<br>"
        "⛔ <b>Mutex Block:</b> Suspensa (ocupado) | 💿 <b>I/O (E/S):</b> Suspensa por disco/rede<br>"
        "<hr style='margin: 4px 0; border: none; border-top: 1px solid #ddd;'>"
        "<b>Blocos com ' + ' :</b> Tarefa Suspensa aguardando Mutex | <b>Blocos com ' x ' :</b> Tarefa Suspensa em E/S"
    );
    
    btnAplicarEdicao = new QPushButton("Aplicar mudança na Tabela"); 

    rightLayout->addWidget(tabelaTarefas);
    rightLayout->addWidget(textLegenda);
    rightLayout->addWidget(btnAplicarEdicao);

    mainLayout->addLayout(leftLayout);
    mainLayout->addLayout(rightLayout);
    resize(1200, 700); 

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
        QTableWidgetItem *itemPrio     = new QTableWidgetItem(QString::number(t.prioridade_estatica));
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
 
        // Pinta o fundo da linha conforme o estado 
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

    int max_ticks = core.clock_global; 
    int largura_total = max_ticks * T_WIDTH;
    int altura_total = core.qtde_cpus * CPU_HEIGHT;

    int altura_cpus = core.qtde_cpus * CPU_HEIGHT;
    int offset_suspensas = altura_cpus + 40; 
    int altura_total_gantt = offset_suspensas + (core.lista_tarefas.size() * 30);

    //desenha cada tick
    for (int t = 0; t <= max_ticks; t++) {
        int x = t * T_WIDTH;
        
        QPen penGrid(Qt::lightGray, 1, Qt::DashLine);
        cenaGantt->addLine(x, 0, x, altura_total, penGrid);
        
        QGraphicsTextItem* numTick = cenaGantt->addText(QString::number(t));
        numTick->setPos(x - 5, -20);
        numTick->setDefaultTextColor(Qt::darkGray);
    }

    //desenha cada cpu
    for (int c = 0; c < core.qtde_cpus; ++c) {
        QGraphicsTextItem* cpuLabel = cenaGantt->addText(QString("CPU %1").arg(c + 1));
        cpuLabel->setPos(LABEL_X, c * CPU_HEIGHT + (CPU_HEIGHT / 4));
        cpuLabel->setDefaultTextColor(Qt::black);
        
        QPen penRow(Qt::gray, 1, Qt::SolidLine);
        cenaGantt->addLine(0, c * CPU_HEIGHT, largura_total, c * CPU_HEIGHT, penRow);
    }

    //label para parte de eventos
    cenaGantt->addLine(0, altura_cpus, largura_total, altura_cpus, QPen(Qt::black, 2)); 
    QGraphicsTextItem* labelSusp = cenaGantt->addText("Tarefas Suspensas ( Padrão 'X' = E/S  |  Padrão '+' = Mutex ):");
    labelSusp->setPos(LABEL_X, altura_cpus + 5);
    labelSusp->setDefaultTextColor(Qt::darkRed);

    //desenha cada tarefa
    for (size_t i = 0; i < core.lista_tarefas.size(); ++i) {
        int y_row = offset_suspensas + (i * 30);
        QGraphicsTextItem* tLabel = cenaGantt->addText(QString("T%1").arg(core.lista_tarefas[i].id));
        tLabel->setPos(LABEL_X, y_row);
        tLabel->setDefaultTextColor(Qt::darkGray);
        cenaGantt->addLine(0, y_row + 25, largura_total, y_row + 25, QPen(Qt::lightGray, 1, Qt::DashLine));
    }

    cenaGantt->addLine(0, altura_total, largura_total, altura_total, QPen(Qt::black, 2)); 
    cenaGantt->addLine(0, 0, 0, altura_total, QPen(Qt::black, 2));                                                        

    //método para desenhar a cor da tarefa
    auto getCor = [](const std::vector<TCB>& tarefas, int tid) -> QString {
        for (const auto& t : tarefas) {
            if (t.id == tid) return QString("#") + QString::fromStdString(t.cor_hex);
        }
        return "#DDDDDD";
    };

    //método para desenhar os ícones de eventos
    auto drawBadge = [&](QString icones, double cx, double cy) {
        if (icones.isEmpty()) return;
        QGraphicsTextItem* txt = cenaGantt->addText(icones);
        
        QFont f = txt->font();
        f.setPointSize(9); 
        txt->setFont(f);
        
        double w = 24.0; 
        double h = 14.0; 
        double rx = cx - (w / 2.0);
        double ry = cy - (h / 2.0);
        
        // retângulo branco com bordas arredondadas
        QGraphicsPathItem* bg = new QGraphicsPathItem();
        QPainterPath path;
        path.addRoundedRect(rx, ry, w, h, h / 2.0, h / 2.0);
        bg->setPath(path);
        bg->setBrush(QBrush(Qt::white));
        bg->setPen(QPen(Qt::NoPen));
        bg->setZValue(14);
        cenaGantt->addItem(bg);

        QRectF b = txt->boundingRect();
        txt->setPos(cx - (b.width() / 2.0), cy - (b.height() / 2.0) + 1.5);
        txt->setZValue(15); //um tequinho pra baixo do label TXX
    };

    // renderização dos ícones de ingresso
    std::map<int, std::vector<int>> chegadas_por_tick;
    for (const auto& t : core.lista_tarefas) {
        if (t.tempo_ingresso <= core.clock_global) {
            chegadas_por_tick[t.tempo_ingresso].push_back(t.id);
        }
    }

    for (const auto& par : chegadas_por_tick) {
        int tick = par.first;
        int x = tick * T_WIDTH;
        
        QString textoChegada = "▶️ ";
        for (size_t i = 0; i < par.second.size(); ++i) {
            textoChegada += QString("T%1").arg(par.second[i]);
            if (i < par.second.size() - 1) textoChegada += ", ";
        }
        
        QGraphicsTextItem* iconeChegada = cenaGantt->addText(textoChegada);
        iconeChegada->setPos(x, -40);
        iconeChegada->setDefaultTextColor(Qt::darkBlue);
        iconeChegada->setZValue(10);
    }

    // desenhar as tarefas conforme o histórico de fotos
    for (const auto& snap : core.historico) {
        if (snap.clock_global == 0) continue; 
        // o bloco se posiciona exatamente a partir de seu respectivo clock
        int x = (snap.clock_global - 1) * T_WIDTH; 

        //blocos das CPUs
        for (int c = 0; c < core.qtde_cpus && c < (int)snap.ids_quem_rodou.size(); ++c) {
            int tid = snap.ids_quem_rodou[c];
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
                rect->setZValue(0); 

                QGraphicsTextItem* texto = cenaGantt->addText(QString("T%1").arg(tid));
                texto->setPos(x + 2, c * CPU_HEIGHT -1);
                texto->setDefaultTextColor((estado_na_foto == Estado::SUSPENSA) ? Qt::white : Qt::black);
                texto->setZValue(1); 

                QString icones = "";
                if (snap.eventos_ocorridos.count(tid)) {
                    icones += QString::fromStdString(snap.eventos_ocorridos.at(tid));
                }
                
                if (std::find(snap.ids_sorteio.begin(), snap.ids_sorteio.end(), tid) != snap.ids_sorteio.end()) {
                    bool inicio_de_bloco = true;
                    if (snap.clock_global > 0) {
                        for (const auto& prev_snap : core.historico) {
                            if (prev_snap.clock_global == snap.clock_global - 1) {
                                for (int prev_tid : prev_snap.ids_quem_rodou) {
                                    if (prev_tid == tid) { inicio_de_bloco = false; break; }
                                }
                            }
                        }
                    }
                    if (inicio_de_bloco) icones += "🎲";
                }

                if (t_restante_na_foto == 0) icones += "📍";

                // Desenha todos os ícones juntos dentro do circulo branco abaixo da label
                drawBadge(icones, x + T_WIDTH / 2.0, c * CPU_HEIGHT + 28);
            }
        }

        // tarefas suspensas
        for (size_t i = 0; i < snap.lista_tarefas.size(); ++i) {
            const TCB& t = snap.lista_tarefas[i];
            if (t.estado == Estado::SUSPENSA) {
                int y_row = offset_suspensas + (i * 30);
                QString corStr = getCor(snap.lista_tarefas, t.id);
                QColor corFundo(corStr);

                // define o padrão gráfico do Qt baseado no tipo de bloqueio
                QBrush brush(corFundo);
                if (t.io_restante > 0) {
                    brush.setStyle(Qt::DiagCrossPattern); // Padrão X para E/S (IO)
                } else if (t.mutex_esperado != -1) {
                    brush.setStyle(Qt::CrossPattern); // Padrão + para Mutex
                }

                QGraphicsRectItem* rect = cenaGantt->addRect(x, y_row, T_WIDTH, 20);
                rect->setBrush(brush);
                rect->setPen(QPen(Qt::black));
                rect->setZValue(0);

                // desenha o badge no centro do bloco suspenso (se o evento ocorreu agora)
                if (snap.eventos_ocorridos.count(t.id) && std::find(snap.ids_quem_rodou.begin(), snap.ids_quem_rodou.end(), t.id) == snap.ids_quem_rodou.end()) {
                    QString icones = QString::fromStdString(snap.eventos_ocorridos.at(t.id));
                    drawBadge(icones, x + T_WIDTH / 2.0, y_row + 10);
                }
            }
        }
    }
    
    //Ponteiro vermelho da linha do tempo
    int x_linha = core.clock_global * T_WIDTH;
    QGraphicsLineItem* linhaPointer = cenaGantt->addLine(x_linha, 0, x_linha, core.qtde_cpus * CPU_HEIGHT, QPen(Qt::red, 2));
    linhaPointer->setZValue(20); 
    int largura_final = std::max(largura_total, x_linha);
    cenaGantt->setSceneRect(cenaGantt->itemsBoundingRect().adjusted(-50, -50, 100, 100));
}

void Interface::btnEdicao() { 
    //método para edição dos atributos de cada tarefa a partir da interface em tempo de execução
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
                t.prioridade_estatica = novaPrio;
                t.prioridade_dinamica = novaPrio;
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
    // método para exportar o diagrama de Gantt
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