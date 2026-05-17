#include "Escalonador.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

// Remove espacos em branco
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

 // -- Avanço de Tick -- // 
void Escalonador::avancarTick() {
    // Verifica se ainda tem tarefas para processar ou se as CPUs estao vazias
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
    // Verificar e incluir novas tarefas que ingressaram neste tick
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

    // ----- Escalonamento e Preempção ------ //
    // Controle para repetir a busca se houver alteração na fila de prontos -> garantir que as mudanças sejam refletidas imediatamente
    bool mudanca_na_fila = true;
    while (!fila_prontos.empty() && mudanca_na_fila) {
        mudanca_na_fila = false;

    //Escolher a melhor tarefa da fila de prontos de acordo com o algoritmo e critérios de desempate
        int melhor_idx = 0;
        for (size_t j = 1; j < fila_prontos.size(); j++) {
            TCB* candidata = fila_prontos[j];
            TCB* atual_melhor = fila_prontos[melhor_idx];
            bool trocar = false;
            bool empate = false;

            //criterio PRIOP: maior prioridade 
            if (algoritmo == "PRIOP") {
                if (candidata->prioridade > atual_melhor->prioridade) trocar = true; 
                else if (candidata->prioridade == atual_melhor->prioridade) empate = true;
            }
            //criterio SRTF: menor tempo restante
            else if (algoritmo == "SRTF") {
                if (candidata->tempo_restante < atual_melhor->tempo_restante) trocar = true;
                else if (candidata->tempo_restante == atual_melhor->tempo_restante) empate = true;
            }

            // ---- Casos de empate ----- //
            //criterio de desempate 01: quem já está executando ganha 
            if (empate) {
                bool cand_exec = (candidata->estado == Estado::EXECUTANDO);
                bool atual_melhor_exec = (atual_melhor->estado == Estado::EXECUTANDO); 
                
                if (cand_exec && !atual_melhor_exec) trocar = true;
                else if (!cand_exec && atual_melhor_exec) empate = false;
            }

            if (empate) {
                // criterio de desempate 02: quem entrou primeiro ganha 
                if (candidata->tempo_ingresso < atual_melhor->tempo_ingresso) trocar = true;
                else if (candidata->tempo_ingresso == atual_melhor->tempo_ingresso) {
                    
                    // criterio de desempate 03: quem tem menor duracao original ganha 
                    if (candidata->duracao_original < atual_melhor->duracao_original) trocar = true;
                    else if (candidata->duracao_original == atual_melhor->duracao_original) {
                        
                        //criterio de desempate 04: desempate aleatório 
                        if (std::rand() % 2 == 1) {
                            trocar = true;
                        }
                    }
                }
            }
            
            if (trocar) melhor_idx = j;
        }

        TCB* melhor_da_fila = fila_prontos[melhor_idx];

        // Resolver o problema de clonar a mesma tarefa em duas CPUs distintas
        bool ja_alocada = false;
        for (int i = 0; i < qtde_cpus; i++) {
            if (cpus[i] != nullptr && cpus[i]->id == melhor_da_fila->id) {
                ja_alocada = true; break;
            }
        }

        // ---- Alocacao -----//
        bool alocou = false;
        // Tenta alocar em CPU livre
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

        // Se as CPUs estão cheias, tenta a preempção
        if (!alocou) {
            int pior_cpu_idx = -1;
            // Cenario PRIOP
            if (algoritmo == "PRIOP") {
                int menor_prio = melhor_da_fila->prioridade;
                // Varre as CPUs para encontrar quem está rodando com prioridade menor que a da nova tarefa
                for (int i = 0; i < qtde_cpus; i++) {
                    if (cpus[i] != nullptr && cpus[i]->prioridade < menor_prio) {
                        menor_prio = cpus[i]->prioridade; pior_cpu_idx = i;
                    }
                }
            }
            // Cenário SRTF
            else if (algoritmo == "SRTF") {
                int maior_restante = melhor_da_fila->tempo_restante;
                // Varre as CPUs para encontrar quem tem um tempo de execução restante maior que o tempo total da nova tarefa
                for (int i = 0; i < qtde_cpus; i++) {
                    if (cpus[i] != nullptr && cpus[i]->tempo_restante > maior_restante) {
                        maior_restante = cpus[i]->tempo_restante; pior_cpu_idx = i;
                    }
                }
            }

            // Se uma CPU passível de preempção foi encontrada, executa a troca de contexto
            if (pior_cpu_idx != -1) {
                TCB* expulsa = cpus[pior_cpu_idx];
                expulsa->estado = Estado::PRONTA;
                
                fila_prontos.erase(fila_prontos.begin() + melhor_idx); // Remove a tarefa eleita da sua posição original na fila de prontos
                if (std::find(fila_prontos.begin(), fila_prontos.end(), expulsa) == fila_prontos.end()) { // Remove a tarefa eleita da sua posição original na fila de prontos
                    fila_prontos.push_back(expulsa);
                }
                
                // Aloca a nova tarefa na CPU liberada e atualiza seu bloco de controle
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

    // ----  Controle de ticks ---- //
    bool alguma_cpu_trabalhando = false;
    std::vector<int> tarefas_processadas_neste_tick;

    //Varredura para verificar o andamento das tarefas alocadas nas CPUs e verificar tempo ocioso
    for (int i = 0; i < qtde_cpus; i++) {
        // Caso 1 - A CPU possui uma tarefa alocada e está trabalhando
        if (cpus[i] != nullptr) {
            alguma_cpu_trabalhando = true;

            // Resolver erros: Verifica se a tarefa atual já foi processada por outra CPU neste mesmo tick
            if (std::find(tarefas_processadas_neste_tick.begin(), tarefas_processadas_neste_tick.end(), cpus[i]->id) != tarefas_processadas_neste_tick.end()) {
                continue; 
            }
            tarefas_processadas_neste_tick.push_back(cpus[i]->id);

            // Consome 1 tick da tarefa alocada na CPU
            if (cpus[i]->tempo_restante > 0) {
                cpus[i]->tempo_restante--;
                std::cout << "[Tick " << clock_global << "] CPU " << i + 1 << " processando T" << cpus[i]->id 
                          << " (Restam: " << cpus[i]->tempo_restante << ")\n";
            }
        } else {
            // Caso 2 - CPU livre
            //verificar se o sistema está ocioso ou se há tarefas pendentes (Incrementa o tempo ocioso acumulado só se o sistema ainda possuir tarefas pendentes que não puderam ser escalonadas)
            if (tarefas_concluidas < total_tarefas) {
                tempo_ocioso++;
            }
        }
    }
    //Todas as CPUs estejam vazias e ngm aguardando na fila de prontos
    if (!alguma_cpu_trabalhando && fila_prontos.empty() && tarefas_concluidas < total_tarefas) {
        std::cout << "[Tick " << clock_global << "] Sistema Ocioso...\n";
    }

    // Registrar foto do estado atual antes de avançar para o próximo tick
    historico.push_back(criarSnapshot());


    // Verificar se alguma tarefa foi concluída neste tick e liberar a CPU correspondente
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