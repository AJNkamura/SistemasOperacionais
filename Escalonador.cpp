#include "Escalonador.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

// Remove espaços em branco
static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

// Converte string para maiuscula
static std::string toUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

//monta as tarefas do arquivo na memoria
bool Escalonador::carregarArquivo(const std::string& caminho) {
    std::ifstream arquivo(caminho);
    if (!arquivo.is_open()) return false;

    std::string linha;
    if (std::getline(arquivo, linha)) {
        std::stringstream ss(linha);
        std::string s_quantum, s_cpus;
        std::getline(ss, algoritmo, ';');
        std::getline(ss, s_quantum, ';');
        std::getline(ss, s_cpus, ';');
        algoritmo = toUpper(trim(algoritmo));
        quantum   = std::stoi(trim(s_quantum));
        qtde_cpus = std::stoi(trim(s_cpus));
    }
    lista_tarefas.clear();
    while (std::getline(arquivo, linha)) {
        if (linha.empty()) continue;
        std::stringstream ss(linha);
        std::string s_id, s_cor, s_in, s_dur, s_prio;
        std::getline(ss, s_id,   ';');
        std::getline(ss, s_cor,  ';');
        std::getline(ss, s_in,   ';');
        std::getline(ss, s_dur,  ';');
        std::getline(ss, s_prio, ';');
 
        //Cria TCB e coloca no vetor de tarefas
        lista_tarefas.emplace_back(
            std::stoi(trim(s_id)),
            trim(s_cor),
            std::stoi(trim(s_in)),
            std::stoi(trim(s_dur)),
            std::stoi(trim(s_prio))
        );
    }
    arquivo.close();

    //inicia ambiente - zera tudo
    cpus.assign(qtde_cpus, nullptr);
    fila_prontos.clear();
    historico.clear();
    clock_global = 0;
    tarefas_concluidas = 0;
    return true;
}

bool Escalonador::isFinalizado() {
    return !lista_tarefas.empty() && (tarefas_concluidas >= (int)lista_tarefas.size());
}

//foto do estado atual
Snapshot Escalonador::criarSnapshot() {
    Snapshot snap;
    snap.clock_global = clock_global;
    snap.tarefas_concluidas = tarefas_concluidas;
    snap.lista_tarefas = lista_tarefas; // Copia os valores exatos (tempo restante, estados)
    
    for (TCB* t : cpus)         snap.ids_task_cpu.push_back(t ? t->id : -1);
    for (TCB* t : fila_prontos) snap.ids_fila_prontos.push_back(t->id);
    return snap;
}

// Volta para estado salvo na foto
void Escalonador::restaurarSnapshot(const Snapshot& snap) {
    clock_global = snap.clock_global;
    tarefas_concluidas = snap.tarefas_concluidas;
    lista_tarefas = snap.lista_tarefas;

    // recontroi o estado da foto
    cpus.assign(qtde_cpus, nullptr);
    for (size_t i = 0; i < snap.ids_task_cpu.size(); i++) {
        if (snap.ids_task_cpu[i] != -1) {
            for (auto& t : lista_tarefas) {
                if (t.id == snap.ids_task_cpu[i]) 
                    cpus[i] = &t;
            }
        }
    }

    //recontroi fila de prontos
    fila_prontos.clear();
    for (int id : snap.ids_fila_prontos) {
        for (auto& t : lista_tarefas) {
            if (t.id == id) fila_prontos.push_back(&t);
        }
    }
}

//Tira o último estado da pilha e restaura
void Escalonador::retrocederTick() {
    if (historico.empty()) {
        std::cout << "[Aviso] Historico vazio, nao e possivel retroceder.\n";
        return;
    }
    restaurarSnapshot(historico.back());
    historico.pop_back(); // Remove passo anterior do historico para n duplicar
}

//Logica
void Escalonador::avancarTick() {
    if (isFinalizado()) return;

    //Salva o estado da pilha antes e depois
    historico.push_back(criarSnapshot());

    int total_tarefas = lista_tarefas.size();

    //Verifica novos ingressos
    for (size_t i = 0; i < lista_tarefas.size(); i++) {
        if (lista_tarefas[i].tempo_ingresso == clock_global && lista_tarefas[i].estado == Estado::NOVO) {
            lista_tarefas[i].estado = Estado::PRONTA;
            fila_prontos.push_back(&lista_tarefas[i]);
            std::cout << "[Tick " << clock_global << "] Tarefa " << lista_tarefas[i].id << " ingressou.\n";
        }
    }

    //Escalonamento e Preemp
    bool mudanca_na_fila = true;
    while (!fila_prontos.empty() && mudanca_na_fila) {
        mudanca_na_fila = false;

        // Achar a melhor tarefa dependendo do algoritmo
        int melhor_idx = 0;
        for (size_t j = 1; j < fila_prontos.size(); j++) {
            TCB* candidata = fila_prontos[j];
            TCB* atual_melhor = fila_prontos[melhor_idx];
            bool trocar = false;
            bool empate = false;

            //Se PRIOP numero maior = maior prioridade
            if (algoritmo == "PRIOP") {
                if (candidata->prioridade > atual_melhor->prioridade) trocar = true; 
                else if (candidata->prioridade == atual_melhor->prioridade) empate = true;
            }
            //se SRTF menor tempo restante = melhor
            else if (algoritmo == "SRTF") {
                if (candidata->tempo_restante < atual_melhor->tempo_restante) trocar = true;
                else if (candidata->tempo_restante == atual_melhor->tempo_restante) empate = true;
            }

            //desempate - criterio 1: se uma das tarefas empatadas já estiver executando, ela ganha 
            if (empate) {
                bool candidata_executando    = (candidata->estado == Estado::EXECUTANDO);
                bool atual_melhor_executando = (atual_melhor->estado == Estado::EXECUTANDO);
                if (candidata_executando && !atual_melhor_executando) {
                    trocar = true;
                    empate = false;
                } else if (!candidata_executando && atual_melhor_executando) {
                    empate = false;
                }
            }
        
            //Se empate persistir - cascata de critérios
            if (empate) {
                //criterio 2: FIFO - menor tempo de ingresso
                if (candidata->tempo_ingresso < atual_melhor->tempo_ingresso) {
                    trocar = true;
                    empate = false;
                } else if (candidata->tempo_ingresso == atual_melhor->tempo_ingresso) {
                    // criterio 3: menor duração original
                    if (candidata->duracao_original < atual_melhor->duracao_original) {
                        trocar = true;
                        empate = false;
                    } else if (candidata->duracao_original == atual_melhor->duracao_original) {
                        // criterio 4: sorteio
                        if (rand() % 2 == 0) trocar = true;
                        empate = false;
                    }
                }
            }
            if (trocar) melhor_idx = j; //att melhor candidato
        }

        TCB* melhor_da_fila = fila_prontos[melhor_idx];

        // Tenta alocar a tarefa em uma CPU vazia
        bool alocou = false;
        for (int i = 0; i < qtde_cpus; i++) {
            if (cpus[i] == nullptr) {
                cpus[i] = melhor_da_fila;
                cpus[i]->estado = Estado::EXECUTANDO;
                fila_prontos.erase(fila_prontos.begin() + melhor_idx);
                std::cout << "[Tick " << clock_global << "] CPU " << i + 1 << " assumiu T" << cpus[i]->id << "\n";
                alocou = true;
                mudanca_na_fila = true; 
                break;
            }
        }

        // Sem CPU vazia - Preemp
        if (!alocou) {
            int pior_cpu_idx = -1;
            if (algoritmo == "PRIOP") {
                int menor_prio_na_cpu = melhor_da_fila->prioridade;
                // Se PRIOP - procura a CPU que está rodando a tarefa com o MENOR prioridade
                for (int i = 0; i < qtde_cpus; i++) {
                    if (cpus[i]->prioridade < menor_prio_na_cpu) {
                        menor_prio_na_cpu = cpus[i]->prioridade;
                        pior_cpu_idx = i;
                    }
                }
            } 
            // Se SRTF - procura a CPU que está rodando a tarefa com o MAIOR tempo restante
            else if (algoritmo == "SRTF") {
                int maior_tempo_na_cpu = melhor_da_fila->tempo_restante;
                
                for (int i = 0; i < qtde_cpus; i++) {
                    // MAIOR tempo restante - pior 
                    if (cpus[i]->tempo_restante > maior_tempo_na_cpu) {
                        maior_tempo_na_cpu = cpus[i]->tempo_restante;
                        pior_cpu_idx = i;
                    }
                }
            }
            // Se achamos alguém pior na CPU -> Troca de contexto
            if (pior_cpu_idx != -1) {
                std::cout << "[Tick " << clock_global << "] PREEMPCAO (" << algoritmo << "): T" << melhor_da_fila->id 
                          << " expulsou T" << cpus[pior_cpu_idx]->id << " da CPU " << pior_cpu_idx + 1 << "\n";
                
                TCB* expulsa = cpus[pior_cpu_idx];
                expulsa->estado = Estado::PRONTA; //att estado da tarefa expulsa
                
                // Remove a melhor da fila e insere a expulsa
                fila_prontos.erase(fila_prontos.begin() + melhor_idx);
                fila_prontos.push_back(expulsa);
                
                cpus[pior_cpu_idx] = melhor_da_fila;
                cpus[pior_cpu_idx]->estado = Estado::EXECUTANDO; //att estado da tarefa que entrou na CPU
                mudanca_na_fila = true;
            }
        }
    }

    //Passagem de tempo - decrementa 1 tick de execução de quem está ativo nas CPUs
    bool alguma_cpu_trabalhando = false;
    for (int i = 0; i < qtde_cpus; i++) {
        if (cpus[i] != nullptr) {
            alguma_cpu_trabalhando = true;
            std::cout << "[Tick " << clock_global << "] CPU " << i + 1 << " executando T" << cpus[i]->id 
                      << " (Faltam: " << --cpus[i]->tempo_restante << ")\n";

            // Se o tempo restante zerou -> libera a CPU
            if (cpus[i]->tempo_restante <= 0) {
                cpus[i]->estado = Estado::TERMINADA;
                std::cout << "[Tick " << clock_global << "] CPU " << i + 1 << " finalizou T" << cpus[i]->id << "\n";
                cpus[i] = nullptr; 
                tarefas_concluidas++;
            }
        }
    }
    // Se n tem ngm rodando e nem na fila mas ainda faltam tarefas -> sistema fica ocioso
    if (!alguma_cpu_trabalhando && fila_prontos.empty() && tarefas_concluidas < total_tarefas) {
        std::cout << "[Tick " << clock_global << "] Sistema Ocioso...\n";
    }


    clock_global++;

    if (isFinalizado()) {
        std::cout << "\nSimulacao concluida no tick " << clock_global << "!\n";
    }
}