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

// Monta as tarefas do arquivo na memoria
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
 
        // Cria TCB e coloca no vetor de tarefas
        lista_tarefas.emplace_back(
            std::stoi(trim(s_id)),
            trim(s_cor),
            std::stoi(trim(s_in)),
            std::stoi(trim(s_dur)),
            std::stoi(trim(s_prio))
        );
    }
    arquivo.close();

    // Inicia ambiente - zera tudo
    cpus.assign(qtde_cpus, nullptr);
    fila_prontos.clear();
    historico.clear();
    clock_global = 0;
    tarefas_concluidas = 0;
    tempo_ocioso = 0;
    return true;
}

bool Escalonador::isFinalizado() {
    int total_tarefas = lista_tarefas.size();
    if (total_tarefas == 0) return true;

    bool cpus_vazias = true;
    for (int i = 0; i < qtde_cpus; i++) {
        if (cpus[i] != nullptr) {
            cpus_vazias = false;
            break;
        }
    }
    return (tarefas_concluidas >= total_tarefas && cpus_vazias);
}

// Foto do estado atual
Snapshot Escalonador::criarSnapshot() {
    Snapshot snap;
    snap.clock_global = clock_global;
    snap.tarefas_concluidas = tarefas_concluidas;
    snap.lista_tarefas = lista_tarefas; 
    
    for (TCB* t : cpus)         snap.ids_task_cpu.push_back(t ? t->id : -1);
    for (TCB* t : fila_prontos) snap.ids_fila_prontos.push_back(t->id);
    return snap;
}

// Volta para estado salvo na foto
void Escalonador::restaurarSnapshot(const Snapshot& snap) {
    clock_global = snap.clock_global;
    tarefas_concluidas = snap.tarefas_concluidas;
    lista_tarefas = snap.lista_tarefas;

    cpus.assign(qtde_cpus, nullptr);
    for (size_t i = 0; i < snap.ids_task_cpu.size(); i++) {
        if (snap.ids_task_cpu[i] != -1) {
            for (auto& t : lista_tarefas) {
                if (t.id == snap.ids_task_cpu[i]) 
                    cpus[i] = &t;
            }
        }
    }

    fila_prontos.clear();
    for (int id : snap.ids_fila_prontos) {
        for (auto& t : lista_tarefas) {
            if (t.id == id) fila_prontos.push_back(&t);
        }
    }
}

// Tira o último estado da pilha e restaura
void Escalonador::retrocederTick() {
    if (historico.empty()) {
        std::cout << "[Aviso] Historico vazio, nao e possivel retroceder.\n";
        return;
    }
    restaurarSnapshot(historico.back());
    historico.pop_back();
}

// Logica de Avanço de Tick
void Escalonador::avancarTick() {
    // ------------------------------------------------------------------------
    // PASSO 0: Validação de Fim Real
    // ------------------------------------------------------------------------
    if (tarefas_concluidas >= (int)lista_tarefas.size()) {
        bool cpus_realmente_vazias = true;
        for (int i = 0; i < qtde_cpus; i++) {
            if (cpus[i] != nullptr) cpus_realmente_vazias = false;
        }
        if (cpus_realmente_vazias) {
            std::cout << "[Simulação] Já finalizada completamente.\n";
            return;
        }
    }

    int total_tarefas = lista_tarefas.size();

    // ------------------------------------------------------------------------
    // PASSO 1: Verificar e incluir novas tarefas que ingressaram neste tick
    // ------------------------------------------------------------------------
    for (size_t i = 0; i < lista_tarefas.size(); i++) {
        if (lista_tarefas[i].tempo_ingresso == clock_global && lista_tarefas[i].estado == Estado::NOVO) {
            lista_tarefas[i].estado = Estado::PRONTA;
            fila_prontos.push_back(&lista_tarefas[i]);
            std::cout << "[Tick " << clock_global << "] Tarefa " << lista_tarefas[i].id << " ingressou.\n";
        }
    }

    // Mantém quem está executando sob avaliação de preempção
    for (int i = 0; i < qtde_cpus; i++) {
        if (cpus[i] != nullptr && cpus[i]->estado == Estado::EXECUTANDO) {
            if (std::find(fila_prontos.begin(), fila_prontos.end(), cpus[i]) == fila_prontos.end()) {
                fila_prontos.push_back(cpus[i]);
            }
        }
    }

    // ------------------------------------------------------------------------
    // PASSO 2: Escalonamento e Preempção (Tomada de Decisão)
    // ------------------------------------------------------------------------
    bool mudanca_na_fila = true;
    while (!fila_prontos.empty() && mudanca_na_fila) {
        mudanca_na_fila = false;

        int melhor_idx = 0;
        for (size_t j = 1; j < fila_prontos.size(); j++) {
            TCB* candidata = fila_prontos[j];
            TCB* atual_melhor = fila_prontos[melhor_idx];
            bool trocar = false;
            bool empate = false;

            if (algoritmo == "PRIOP") {
                if (candidata->prioridade > atual_melhor->prioridade) trocar = true; 
                else if (candidata->prioridade == atual_melhor->prioridade) empate = true;
            }
            else if (algoritmo == "SRTF") {
                if (candidata->tempo_restante < atual_melhor->tempo_restante) trocar = true;
                else if (candidata->tempo_restante == atual_melhor->tempo_restante) empate = true;
            }

            if (empate) {
                bool cand_exec = (candidata->estado == Estado::EXECUTANDO);
                bool atual_melhor_exec = (atual_melhor->estado == Estado::EXECUTANDO); 
                
                if (cand_exec && !atual_melhor_exec) trocar = true;
                else if (!cand_exec && atual_melhor_exec) empate = false;
            }
        
            if (empate) {
                if (candidata->tempo_ingresso < atual_melhor->tempo_ingresso) trocar = true;
                else if (candidata->tempo_ingresso == atual_melhor->tempo_ingresso) {
                    if (candidata->duracao_original < atual_melhor->duracao_original) trocar = true;
                    else if (candidata->duracao_original == atual_melhor->duracao_original) {
                        if (candidata->id < atual_melhor->id) trocar = true;
                    }
                }
            }
            if (trocar) melhor_idx = j;
        }

        TCB* melhor_da_fila = fila_prontos[melhor_idx];

        // Evita clonar a mesma tarefa em duas CPUs distintas
        bool ja_alocada = false;
        for (int i = 0; i < qtde_cpus; i++) {
            if (cpus[i] != nullptr && cpus[i]->id == melhor_da_fila->id) {
                ja_alocada = true; break;
            }
        }

        bool alocou = false;
        // 1. Tenta alocar em CPU livre
        for (int i = 0; i < qtde_cpus; i++) {
            if (cpus[i] == nullptr) {
                if (!ja_alocada) {
                    cpus[i] = melhor_da_fila;
                    cpus[i]->estado = Estado::EXECUTANDO;
                }
                fila_prontos.erase(fila_prontos.begin() + melhor_idx);
                alocou = true; mudanca_na_fila = true; break;
            } else if (cpus[i]->id == melhor_da_fila->id) {
                fila_prontos.erase(fila_prontos.begin() + melhor_idx);
                alocou = true; mudanca_na_fila = true; break;
            }
        }

        // 2. Se as CPUs estão cheias, tenta a preempção
        if (!alocou) {
            int pior_cpu_idx = -1;
            if (algoritmo == "PRIOP") {
                int menor_prio = melhor_da_fila->prioridade;
                for (int i = 0; i < qtde_cpus; i++) {
                    if (cpus[i] != nullptr && cpus[i]->prioridade < menor_prio) {
                        menor_prio = cpus[i]->prioridade; pior_cpu_idx = i;
                    }
                }
            }
            else if (algoritmo == "SRTF") {
                int maior_restante = melhor_da_fila->tempo_restante;
                for (int i = 0; i < qtde_cpus; i++) {
                    if (cpus[i] != nullptr && cpus[i]->tempo_restante > maior_restante) {
                        maior_restante = cpus[i]->tempo_restante; pior_cpu_idx = i;
                    }
                }
            }

            if (pior_cpu_idx != -1) {
                TCB* expulsa = cpus[pior_cpu_idx];
                expulsa->estado = Estado::PRONTA;
                
                fila_prontos.erase(fila_prontos.begin() + melhor_idx);
                if (std::find(fila_prontos.begin(), fila_prontos.end(), expulsa) == fila_prontos.end()) {
                    fila_prontos.push_back(expulsa);
                }
                
                cpus[pior_cpu_idx] = melhor_da_fila;
                cpus[pior_cpu_idx]->estado = Estado::EXECUTANDO;
                mudanca_na_fila = true;
            }
        }
    }

    // Garante que quem não conseguiu CPU volte a ficar com o estado de PRONTA
    for (auto* t : fila_prontos) {
        if (t->estado == Estado::EXECUTANDO) {
            t->estado = Estado::PRONTA;
        }
    }

    // ------------------------------------------------------------------------
    // PASSO 3: Consumo de Tempo (Controle de Ticks de Processamento e Ociosidade)
    // ------------------------------------------------------------------------
    bool alguma_cpu_trabalhando = false;
    std::vector<int> tarefas_processadas_neste_tick;

    for (int i = 0; i < qtde_cpus; i++) {
        if (cpus[i] != nullptr) {
            alguma_cpu_trabalhando = true;
            
            if (std::find(tarefas_processadas_neste_tick.begin(), tarefas_processadas_neste_tick.end(), cpus[i]->id) != tarefas_processadas_neste_tick.end()) {
                continue; 
            }
            tarefas_processadas_neste_tick.push_back(cpus[i]->id);

            if (cpus[i]->tempo_restante > 0) {
                cpus[i]->tempo_restante--;
                std::cout << "[Tick " << clock_global << "] CPU " << i + 1 << " processando T" << cpus[i]->id 
                          << " (Restam: " << cpus[i]->tempo_restante << ")\n";
            }
        } else {
            // Computa individualmente por CPU livre
            if (tarefas_concluidas < total_tarefas) {
                tempo_ocioso++;
            }
        }
    }

    if (!alguma_cpu_trabalhando && fila_prontos.empty() && tarefas_concluidas < total_tarefas) {
        std::cout << "[Tick " << clock_global << "] Sistema Ocioso...\n";
    }

    // ------------------------------------------------------------------------
    // PASSO 4: Registrar Snapshot imediatamente no histórico gráfico
    // ------------------------------------------------------------------------
    historico.push_back(criarSnapshot());

    // ------------------------------------------------------------------------
    // PASSO 5: Atualizar estados e limpar CPUs concluídas para a próxima rodada
    // ------------------------------------------------------------------------
    for (int i = 0; i < qtde_cpus; i++) {
        if (cpus[i] != nullptr && cpus[i]->tempo_restante <= 0) {
            cpus[i]->tempo_restante = 0;
            if (cpus[i]->estado != Estado::TERMINADA) {
                cpus[i]->estado = Estado::TERMINADA;
                tarefas_concluidas++;
            }
            std::cout << "[Tick " << clock_global << "] CPU " << i + 1 << " liberou T" << cpus[i]->id << " (Concluida)\n";
            cpus[i] = nullptr;
        }
    }

    // Avança o relógio geral do sistema
    clock_global++;
}