#ifndef SIMCORE_H
#define SIMCORE_H

#include <vector>
#include <string>
#include "TCB.h"

// Estrutura para salvar o estado exato de um momento no tempo (Máquina do tempo)
struct Snapshot {
    int clock_global;
    int tarefas_concluidas;
    std::vector<TCB> lista_tarefas;     // Cópia profunda das tarefas
    std::vector<int> ids_task_cpu;      // Quais IDs estavam nas CPUs
    std::vector<int> ids_fila_prontos;  // Quais IDs estavam na fila
};

class CoreSimulacao {
public:
    int clock_global = 0;
    int tarefas_concluidas = 0;
    int qtde_cpus = 0;
    int quantum = 0;
    std::string algoritmo;

    std::vector<TCB> lista_tarefas;
    std::vector<TCB*> cpus;
    std::vector<TCB*> fila_prontos;
    std::vector<Snapshot> historico; // Histórico para o retrocesso

    bool carregarArquivo(const std::string& caminho);
    bool isFinalizado();
    
    void avancarTick();
    void retrocederTick();

private:
    Snapshot criarSnapshot();
    void restaurarSnapshot(const Snapshot& snap);
};

#endif