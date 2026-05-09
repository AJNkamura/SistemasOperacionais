#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include "TCB.h" 
#include <queue>

int main() {

    /* teste - cria tarefa e clock (1 tarefa)
    int clock_global = 0;
    TCB tarefaTeste(1, "FF0000", 0, 10, 3);
    teste criacao ok
    std::cout << "Tarefa ID: " << tarefaTeste.id << std::endl;
    std::cout << "Tempo restante: " << tarefaTeste.tempo_restante << std::endl;
   
    Teste clock
    while (tarefaTeste.tempo_restante >0) {
        tarefaTeste.estado = Estado::EXECUTANDO;

        std::cout << "Tick " << clock_global << " - ";
        std::cout << "Tarefa " << tarefaTeste.id << ": executando - ";

        tarefaTeste.tempo_restante--;
        std::cout << "Tempo restante: " << tarefaTeste.tempo_restante << " ticks" << std::endl;

        clock_global++;
    }
    */

    //abrir arquivo txt
    std::ifstream arquivo("tarefasTeste.txt");
    if (!arquivo.is_open()) {
        std::cerr << "Erro ao abrir tarefasTeste.txt" << std::endl;
        return 1;
    }

    std::string linha;
    std::vector<TCB> lista_tarefas;

    std::string algoritmo;
    int quantum = 0;
    int qtde_cpus = 0;

    //ler txt
    if (std::getline(arquivo, linha)) {
        std::stringstream ss_config(linha);
        std::string s_quantum, s_cpus;

        std::getline(ss_config, algoritmo, ';');
        std::getline(ss_config, s_quantum, ';');
        std::getline(ss_config, s_cpus, ';');

        quantum = std::stoi(s_quantum);
        qtde_cpus = std::stoi(s_cpus);

        std::cout << "Leitura ok: " << algoritmo << " | Quantum: " << quantum << " | CPUs: " << qtde_cpus << std::endl;
    }


    // Cria tarefa e coloca no vetor
    while (std::getline(arquivo, linha)) {
        if (linha.empty()) continue; 

        std::stringstream ss(linha);
        std::string s_id, s_cor, s_ingresso, s_duracao, s_prio;

        // toca de linha apos ;
        std::getline(ss, s_id, ';');
        std::getline(ss, s_cor, ';');
        std::getline(ss, s_ingresso, ';');
        std::getline(ss, s_duracao, ';');
        std::getline(ss, s_prio, ';');  

        lista_tarefas.emplace_back(
            std::stoi(s_id), 
            s_cor, 
            std::stoi(s_ingresso), 
            std::stoi(s_duracao), 
            std::stoi(s_prio)
        );
    }
    arquivo.close();   
    
    

// 3. O CORAÇÃO DA SIMULAÇÃO (O NOVO TRECHO)
    std::queue<TCB*> fila_prontos; 
    int clock_global = 0;
    int tarefas_concluidas = 0;
    int total_tarefas = lista_tarefas.size();

    std::cout << "\nIniciando Simulacao de Escalonamento...\n";
    
    //Processando tarefas
    while (tarefas_concluidas < total_tarefas) {
        
        // Verifica novo ingresso
        for (size_t i = 0; i < lista_tarefas.size(); i++) {
            if (lista_tarefas[i].tempo_ingresso == clock_global && lista_tarefas[i].estado == Estado::NOVO) {
                lista_tarefas[i].estado = Estado::PRONTA;
                fila_prontos.push(&lista_tarefas[i]);
                std::cout << "[Tick " << clock_global << "] Tarefa " << lista_tarefas[i].id << " ingressou no sistema.\n";
            }
        }

        // Processa a tarefa que estiver na frente 
        if (!fila_prontos.empty()) {
            TCB* atual = fila_prontos.front();
            atual->estado = Estado::EXECUTANDO;

            std::cout << "[Tick " << clock_global << "] CPU 1 executando Tarefa " << atual->id 
                      << " (Restam: " << --atual->tempo_restante << ")\n";

            if (atual->tempo_restante <= 0) {
                atual->estado = Estado::TERMINADA;
                fila_prontos.pop();
                tarefas_concluidas++;
                std::cout << "[Tick " << clock_global << "] Tarefa " << atual->id << " FINALIZADA.\n";
            }
        } else {
            std::cout << "[Tick " << clock_global << "] Sistema Ocioso...\n";
        }

        clock_global++;
        if (clock_global > 200) break; // Segurança contra loops infinitos
    }

    std::cout << "\nSimulacao concluida no tick " << clock_global << "!\n";
    return 0;
}

