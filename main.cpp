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

    std::string arquivo_tarefas = "tarefas_SRTF.txt";
    
    //abrir arquivo txt
    std::ifstream arquivo(arquivo_tarefas);
    if (!arquivo.is_open()) {
        std::cerr << "Erro ao abrir tarefas" << std::endl;
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
    
    
// --- Implement PRIOP pu SRTF ---
    std::vector<TCB*> fila_prontos; //vetor de prontos
    int clock_global = 0;
    int tarefas_concluidas = 0;
    int total_tarefas = lista_tarefas.size();
    std::vector<TCB*> cpus(qtde_cpus, nullptr); //vetor de CPU
    
    //Loop principal
    while (tarefas_concluidas < total_tarefas) {
        //Verifica novos ingressos: mover tarefas do vetor para a fila de prontos
        for (size_t i = 0; i < lista_tarefas.size(); i++) {
            if (lista_tarefas[i].tempo_ingresso == clock_global && lista_tarefas[i].estado == Estado::NOVO) {
                lista_tarefas[i].estado = Estado::PRONTA;
                fila_prontos.push_back(&lista_tarefas[i]);
                std::cout << "[Tick " << clock_global << "] Tarefa " << lista_tarefas[i].id << " ingressou.\n";
            }
        }

            /* teste PRIOP - menor prioridade
            int melhor_idx = 0;
            for (size_t j = 1; j < fila_prontos.size(); j++) {
                if (fila_prontos[j]->prioridade < fila_prontos[melhor_idx]->prioridade) {
                    melhor_idx = j;
                }
            */
        
            bool mudanca_na_fila = true;
            while (!fila_prontos.empty() && mudanca_na_fila) {
                 mudanca_na_fila = false;

           // Achar a melhor tarefa baseado no algoritmo escolhido
            int melhor_idx = 0;
            for (size_t j = 1; j < fila_prontos.size(); j++) {
                if (algoritmo == "PRIOP") {
                    if (fila_prontos[j]->prioridade < fila_prontos[melhor_idx]->prioridade) melhor_idx = j;
                } else if (algoritmo == "SRTF") {
                    if (fila_prontos[j]->tempo_restante < fila_prontos[melhor_idx]->tempo_restante) melhor_idx = j;
                }
            }

            TCB* melhor_da_fila = fila_prontos[melhor_idx];

            // Otimização - Tentar alguma CPU vazia e aloca se tiver
            bool alocou = false;
            for (int i = 0; i < qtde_cpus; i++) {
                if (cpus[i] == nullptr) {
                    cpus[i] = melhor_da_fila;
                    cpus[i]->estado = Estado::EXECUTANDO;
                    fila_prontos.erase(fila_prontos.begin() + melhor_idx);
                    std::cout << "[Tick " << clock_global << "] CPU " << i + 1 << " assumiu T" << cpus[i]->id << "\n";
                    alocou = true;
                    mudanca_na_fila = true; //reavaliar a fila no mesmo tick
                    break;
                }
            }

            // Sem CPU vazia - Preempção
            if (!alocou) {
                int pior_cpu_idx = -1;
                int valor_comparacao_melhor;
                if (algoritmo == "PRIOP") {
                    valor_comparacao_melhor = melhor_da_fila->prioridade;
                }
                else 
                    valor_comparacao_melhor = melhor_da_fila->tempo_restante;

                int pior_valor_na_cpu = valor_comparacao_melhor;

                for (int i = 0; i < qtde_cpus; i++) {
                    int valor_atual_cpu = (algoritmo == "PRIOP") ? cpus[i]->prioridade : cpus[i]->tempo_restante;
                    
                    //expulsa o pior para entrada da nova tarefa
                    if (valor_atual_cpu > pior_valor_na_cpu) {
                        pior_valor_na_cpu = valor_atual_cpu;
                        pior_cpu_idx = i;
                    }
                }

                if (pior_cpu_idx != -1) {
                    std::cout << "[Tick " << clock_global << "] PREEMPCAO (" << algoritmo << "): T" << melhor_da_fila->id 
                              << " expulsou T" << cpus[pior_cpu_idx]->id << " da CPU " << pior_cpu_idx + 1 << "\n";
                    
                    //troca de contexto - salva fila e carrega nova
                    cpus[pior_cpu_idx]->estado = Estado::PRONTA;
                    fila_prontos.push_back(cpus[pior_cpu_idx]);

                    cpus[pior_cpu_idx] = melhor_da_fila;
                    cpus[pior_cpu_idx]->estado = Estado::EXECUTANDO;
                    fila_prontos.erase(fila_prontos.begin() + melhor_idx);
                    mudanca_na_fila = true;
                }
            }
        }

        //Processa o tempo nas CPUs
        bool alguma_cpu_trabalhando = false;
        for (int i = 0; i < qtde_cpus; i++) {
            if (cpus[i] != nullptr) {
                alguma_cpu_trabalhando = true;
                std::cout << "[Tick " << clock_global << "] CPU " << i + 1 << " executando T" << cpus[i]->id 
                          << " (Faltam: " << --cpus[i]->tempo_restante << ")\n";

                //verifica se o processo terminou
                if (cpus[i]->tempo_restante <= 0) {
                    cpus[i]->estado = Estado::TERMINADA;
                    std::cout << "[Tick " << clock_global << "] CPU " << i + 1 << " finalizou T" << cpus[i]->id << "\n";
                    cpus[i] = nullptr; 
                    tarefas_concluidas++;
                }
            }
        }

        //verifica se o sistema esta ocioso
        if (!alguma_cpu_trabalhando && fila_prontos.empty() && tarefas_concluidas < total_tarefas) {
            std::cout << "[Tick " << clock_global << "] Sistema Ocioso...\n";
        }

        clock_global++;
        if (clock_global > 500) break; 
    }

    std::cout << "\nSimulacao concluida no tick " << clock_global << "!\n";
    return 0;
}