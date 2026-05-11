#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QImage>
#include <QPainter>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    // CONSTRUÇÃO DA INTERFACE 
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // Painel Esquerdo (Gráfico e Botões)
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

    // Painel Direito (Tabela de Tarefas)
    tabelaTarefas = new QTableWidget(0, 5);
    tabelaTarefas->setHorizontalHeaderLabels({"ID", "Estado", "Prio", "Ingresso", "Tempo Rest."});
    tabelaTarefas->setFixedWidth(300);

    mainLayout->addLayout(leftLayout);
    mainLayout->addWidget(tabelaTarefas);
    resize(1000, 600);

    // Conectar os Sinais e Slots (Eventos)
    connect(btnCarregar, &QPushButton::clicked, this, &MainWindow::btnCarregarClicado);
    connect(btnRetroceder, &QPushButton::clicked, this, &MainWindow::btnRetrocederClicado);
    connect(btnAvancar, &QPushButton::clicked, this, &MainWindow::btnAvancarClicado);
    connect(btnPlay, &QPushButton::clicked, this, &MainWindow::btnPlayPauseClicado);
    connect(btnExportar, &QPushButton::clicked, this, &MainWindow::btnExportarClicado);

    timerPlay = new QTimer(this);
    connect(timerPlay, &QTimer::timeout, this, &MainWindow::tickAutomatico);
}

void MainWindow::btnCarregarClicado() {
    QString fileName = QFileDialog::getOpenFileName(this, "Abrir Configuração", "", "Text Files (*.txt)");
    if (!fileName.isEmpty()) {
        core.carregarArquivo(fileName.toStdString());
        atualizarUI();
    }
}

void MainWindow::btnAvancarClicado() {
    core.avancarTick();
    atualizarUI();
}

void MainWindow::btnRetrocederClicado() {
    core.retrocederTick();
    atualizarUI();
}

void MainWindow::btnPlayPauseClicado() {
    if (timerPlay->isActive()) {
        timerPlay->stop();
        btnPlay->setText("Play Automático");
    } else {
        timerPlay->start(300); // 300ms por tick
        btnPlay->setText("Pausar");
    }
}

void MainWindow::tickAutomatico() {
    if (core.isFinalizado()) {
        btnPlayPauseClicado(); // Pausa
    } else {
        btnAvancarClicado();
    }
}

void MainWindow::atualizarUI() {
    // 1. Atualiza a tabela com o estado atual das tarefas
    tabelaTarefas->setRowCount(core.lista_tarefas.size());
    for (size_t i = 0; i < core.lista_tarefas.size(); ++i) {
        TCB &t = core.lista_tarefas[i];

        QString strEstado;
        switch (t.estado) {
            case Estado::NOVO:       strEstado = "Novo";       break;
            case Estado::PRONTA:     strEstado = "Pronta";     break;
            case Estado::EXECUTANDO: strEstado = "Executando"; break;
            case Estado::SUSPENSA:   strEstado = "Suspensa";   break;
            case Estado::TERMINADA:  strEstado = "Terminada";  break;
            default:                 strEstado = "?";          break;
        }

        tabelaTarefas->setItem(i, 0, new QTableWidgetItem(QString("T%1").arg(t.id)));
        tabelaTarefas->setItem(i, 1, new QTableWidgetItem(strEstado));
        tabelaTarefas->setItem(i, 2, new QTableWidgetItem(QString::number(t.prioridade)));
        tabelaTarefas->setItem(i, 3, new QTableWidgetItem(QString::number(t.tempo_ingresso)));
        tabelaTarefas->setItem(i, 4, new QTableWidgetItem(QString::number(t.tempo_restante)));
 
        // Coloriza o fundo da linha conforme o estado para facilitar leitura visual
        QColor bg = Qt::white;
        if (t.estado == Estado::EXECUTANDO) bg = QColor("#c8f0c8"); // verde claro
        else if (t.estado == Estado::TERMINADA) bg = QColor("#e0e0e0"); // cinza
        else if (t.estado == Estado::SUSPENSA)  bg = QColor("#f0c8c8"); // vermelho claro
        for (int col = 0; col < 5; col++) {
            if (tabelaTarefas->item(i, col))
                tabelaTarefas->item(i, col)->setBackground(bg);
        }
    }

    desenharGantt();
}

void MainWindow::desenharGantt() {
    cenaGantt->clear();
    int T_WIDTH = 30; // Largura visual de 1 tick (pixel)
    int CPU_HEIGHT = 50;
    const int LABEL_X    = -65;// Posição X dos rótulos de CPU

    for (int c = 0; c < core.qtde_cpus; ++c) {
        QGraphicsTextItem* cpuLabel = cenaGantt->addText(QString("CPU %1").arg(c + 1));
        cpuLabel->setPos(LABEL_X, c * CPU_HEIGHT + (CPU_HEIGHT / 4));
    }

    
    auto getCor = [](const std::vector<TCB>& tarefas, int tid) -> QString {
        for (const auto& t : tarefas) {
            if (t.id == tid) return QString("#") + QString::fromStdString(t.cor_hex);
        }
        return "#DDDDDD";
    };

    // Desenha o histórico passado (Graças a sua máquina do tempo!)
    for (const auto& snap : core.historico) {
        int x = snap.clock_global * T_WIDTH;
        for (int c = 0; c < core.qtde_cpus && c < (int)snap.ids_task_cpu.size(); ++c) {
            int tid = snap.ids_task_cpu[c];
            if (tid != -1) {
                QString cor = getCor(snap.lista_tarefas, tid);
                QGraphicsRectItem* rect = cenaGantt->addRect(x, c * CPU_HEIGHT, T_WIDTH, CPU_HEIGHT - 10);
                rect->setBrush(QColor(cor));
                rect->setPen(QPen(Qt::black));
                
                QGraphicsTextItem* texto = cenaGantt->addText(QString("T%1").arg(tid));
                texto->setPos(x + 2, c * CPU_HEIGHT + 2);
                texto->setDefaultTextColor(Qt::black);
            }
        }
    }
    
    {
        int x = core.clock_global * T_WIDTH;
        for (int c = 0; c < core.qtde_cpus; ++c) {
            if (core.cpus[c] != nullptr) {
                TCB* t = core.cpus[c];
                QString cor = QString("#") + QString::fromStdString(t->cor_hex);
 
                QGraphicsRectItem* rect = cenaGantt->addRect(x, c * CPU_HEIGHT, T_WIDTH, CPU_HEIGHT - 10);
                rect->setBrush(QColor(cor));
                rect->setPen(QPen(Qt::darkGreen, 2)); // Borda verde = tick atual em andamento
 
                QGraphicsTextItem* texto = cenaGantt->addText(QString("T%1").arg(t->id));
                texto->setPos(x + 2, c * CPU_HEIGHT + 2);
            }
        }
    }
    
    // Linha do Tempo Atual (Um ponteiro vermelho indicando onde estamos)
    cenaGantt->addLine(core.clock_global * T_WIDTH, 0, core.clock_global * T_WIDTH, core.qtde_cpus * CPU_HEIGHT, QPen(Qt::red, 2));
}

// Requisito 2.4 - Salvar imagem!
void MainWindow::btnExportarClicado() {
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
#include "moc_MainWindow.cpp"
