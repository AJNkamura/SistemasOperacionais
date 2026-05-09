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
    
    
std::queue<TCB*> fila_prontos; 
    int clock_global = 0;
    int tarefas_concluidas = 0;
    int total_tarefas = lista_tarefas.size();

    // vetor de CPUs 
    std::vector<TCB*> cpus(qtde_cpus, nullptr);

    std::cout << "\nIniciando Simulacao com " << qtde_cpus << " CPUs...\n";
    
    while (tarefas_concluidas < total_tarefas) {
        
        //verifica novos ingressos 
        for (size_t i = 0; i < lista_tarefas.size(); i++) {
            if (lista_tarefas[i].tempo_ingresso == clock_global && lista_tarefas[i].estado == Estado::NOVO) {
                lista_tarefas[i].estado = Estado::PRONTA;
                fila_prontos.push(&lista_tarefas[i]);
                std::cout << "[Tick " << clock_global << "] Tarefa " << lista_tarefas[i].id << " ingressou.\n";
            }
        }

        // distribuir tarefas para CPUs ociosas
        for (int i = 0; i < qtde_cpus; i++) {
            if (cpus[i] == nullptr && !fila_prontos.empty()) {
                cpus[i] = fila_prontos.front();
                fila_prontos.pop();
                cpus[i]->estado = Estado::EXECUTANDO;
                std::cout << "[Tick " << clock_global << "] CPU " << i + 1 << " assumiu Tarefa " << cpus[i]->id << "\n";
            }
        }

        //processa o tempo em cada CPU
        bool alguma_cpu_trabalhando = false;
        for (int i = 0; i < qtde_cpus; i++) {
            if (cpus[i] != nullptr) {
                alguma_cpu_trabalhando = true;
                std::cout << "[Tick " << clock_global << "] CPU " << i + 1 << " executando T" << cpus[i]->id 
                          << " (Faltam: " << --cpus[i]->tempo_restante << ")\n";

                // Se a tarefa acabou nesta CPU
                if (cpus[i]->tempo_restante <= 0) {
                    cpus[i]->estado = Estado::TERMINADA;
                    std::cout << "[Tick " << clock_global << "] CPU " << i + 1 << " finalizou Tarefa " << cpus[i]->id << "\n";
                    cpus[i] = nullptr; // libera a CPU para o próximo tick
                    tarefas_concluidas++;
                }
            }
        }

        if (!alguma_cpu_trabalhando && fila_prontos.empty() && tarefas_concluidas < total_tarefas) {
            std::cout << "[Tick " << clock_global << "] Todas as CPUs ociosas...\n";
        }

        clock_global++;
        if (clock_global > 300) break; //limite
    }

    std::cout << "\nSimulacao concluida no tick " << clock_global << "!\n";
    return 0;
}

