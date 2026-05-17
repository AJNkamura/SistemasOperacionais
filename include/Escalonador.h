#ifndef ESCALONADOR_H
#define ESCALONADOR_H

#include <vector>
#include <string>
#include "TCB.h"

// Estrutura para salvar o estado de um momento no tempo
struct Snapshot {
    int clock_global;
    int tarefas_concluidas;
    std::vector<TCB> lista_tarefas;
    std::vector<int> ids_task_cpu; 
    std::vector<int> ids_fila_prontos; 
    std::vector<int> ids_sorteio; 
};

class Escalonador {
public:
    int clock_global = 0;
    int tarefas_concluidas = 0;
    int qtde_cpus = 0;
    int quantum = 0;
    int tempo_ocioso = 0;
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
    Snapshot criarSnapshot(std::vector<int> sorteios = {}); 
    void restaurarSnapshot(const Snapshot& snap);
};

#endif