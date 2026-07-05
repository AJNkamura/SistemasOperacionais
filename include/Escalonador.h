#ifndef ESCALONADOR_H
#define ESCALONADOR_H

#include <vector>
#include <string>
#include "TCB.h"
#include <map>

// Estrutura para salvar o estado de um momento no tempo
struct Snapshot {
    int clock_global;                               // tick global do snap
    int tarefas_concluidas;                         // qtde de tarefas concluidas
    int tempo_ocioso;                               // somatorio de ticks ociosos
    std::vector<TCB> lista_tarefas;                 // vetor de todas as tarefas
    std::vector<int> ids_task_cpu;                  // vetor das tarefas na cpu
    std::vector<int> ids_fila_prontos;              // vetor das tarefas prontas
    std::vector<int> ids_sorteio;                   // vetor para sorteio
    std::map<int, int> mutexes_estado;              // mapa de mutexes (ver abaixo)
    std::vector<int> ids_quem_rodou;                // vetor das tarefas que estavam executando no tick
    std::map<int, std::string> eventos_ocorridos;   // mapa de eventos
};

class Escalonador {
public:
    //atributos
    int clock_global = 0;
    int tarefas_concluidas = 0;
    int qtde_cpus = 0;
    int quantum = 0;
    int tempo_ocioso = 0;
    int alpha = 0; 
    std::string algoritmo;

    std::map<int, int> mutexes;     // chave: id do mutex, valor: id da tarefa usando mutex (-1 se livre)

    std::vector<TCB> lista_tarefas;
    std::vector<TCB*> cpus;
    std::vector<TCB*> fila_prontos;
    std::vector<Snapshot> historico; // Histórico para o retrocesso

    //métodos
    bool carregarArquivo(const std::string& caminho);
    bool isFinalizado();
    
    void avancarTick();
    void retrocederTick();

private:
    Snapshot criarSnapshot(std::vector<int> sorteios = {}); 
    void restaurarSnapshot(const Snapshot& snap);
};

#endif