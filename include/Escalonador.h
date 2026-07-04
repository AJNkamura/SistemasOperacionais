#ifndef ESCALONADOR_H
#define ESCALONADOR_H

#include <vector>
#include <string>
#include "TCB.h"
#include <map>

// Estrutura para salvar o estado de um momento no tempo
struct Snapshot {
    int clock_global;
    int tarefas_concluidas;
    int tempo_ocioso;
    std::vector<TCB> lista_tarefas;
    std::vector<int> ids_task_cpu; 
    std::vector<int> ids_fila_prontos; 
    std::vector<int> ids_sorteio; 
    std::map<int, int> mutexes_estado;
    std::vector<int> ids_quem_rodou;
    std::map<int, std::string> eventos_ocorridos;
};

class Escalonador {
public:
    int clock_global = 0;
    int tarefas_concluidas = 0;
    int qtde_cpus = 0;
    int quantum = 0;
    int tempo_ocioso = 0;
    int alpha = 0; //(envelhecimento)
    std::string algoritmo;

    std::map<int, int> mutexes;     // chave: id do mutex, valor: id da tarefa usando mutex (-1 se livre)

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