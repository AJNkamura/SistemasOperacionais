#include "SimCore.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

// Remove espaços em branco do início e fim de uma string (trim).
// Necessário porque o arquivo de configuração usa "; " com espaços, ex: "SRTF; 0; 1".
// Sem isso, algoritmo == "SRTF" sempre falhava (string continha " SRTF" ou "SRTF ").
static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

// Converte uma string para maiúsculas (para cumprir req. 3.3.2: "priop", "PRIOP" e "PrioP"
// devem ser tratados da mesma forma).
static std::string toUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}



bool CoreSimulacao::carregarArquivo(const std::string& caminho) {
    std::ifstream arquivo(caminho);
    if (!arquivo.is_open()) return false;

    //parse the system settings
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

    //parse the tasks
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
 
        lista_tarefas.emplace_back(
            std::stoi(trim(s_id)),
            trim(s_cor),
            std::stoi(trim(s_in)),
            std::stoi(trim(s_dur)),
            std::stoi(trim(s_prio))
        );
    }
    arquivo.close();

    //prepare for run
    cpus.assign(qtde_cpus, nullptr);
    fila_prontos.clear();
    historico.clear();
    clock_global = 0;
    tarefas_concluidas = 0;
    return true;
}

bool CoreSimulacao::isFinalizado() {
    return !lista_tarefas.empty() && (tarefas_concluidas >= (int)lista_tarefas.size());
}

// Cria um snapshot (fotografia) completo do estado atual para viagem no tempo.
Snapshot CoreSimulacao::criarSnapshot() {
    Snapshot snap;
    snap.clock_global = clock_global;
    snap.tarefas_concluidas = tarefas_concluidas;
    snap.lista_tarefas = lista_tarefas; // Copia os valores exatos (tempo restante, estados)
    
    for (TCB* t : cpus)         snap.ids_task_cpu.push_back(t ? t->id : -1);
    for (TCB* t : fila_prontos) snap.ids_fila_prontos.push_back(t->id);
    return snap;
}

// Restaura o sistema ao estado salvo em um snapshot.
void CoreSimulacao::restaurarSnapshot(const Snapshot& snap) {
    clock_global = snap.clock_global;
    tarefas_concluidas = snap.tarefas_concluidas;
    lista_tarefas = snap.lista_tarefas;

    // Reconstrói os ponteiros com segurança baseados na cópia nova
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

void CoreSimulacao::retrocederTick() {
    if (historico.empty()) {
        std::cout << "[Aviso] Historico vazio, nao e possivel retroceder.\n";
        return;
    }
    restaurarSnapshot(historico.back()); // Pega o passo anterior
    historico.pop_back(); // Remove ele do histórico (voltamos no tempo)
}

void CoreSimulacao::avancarTick() {
    if (isFinalizado()) return;

    // 1. Salva o estado ANTES de alterar (Para a viagem no tempo funcionar)
    historico.push_back(criarSnapshot());

    int total_tarefas = lista_tarefas.size();

    // 2. Verifica novos ingressos
    for (size_t i = 0; i < lista_tarefas.size(); i++) {
        if (lista_tarefas[i].tempo_ingresso == clock_global && lista_tarefas[i].estado == Estado::NOVO) {
            lista_tarefas[i].estado = Estado::PRONTA;
            fila_prontos.push_back(&lista_tarefas[i]);
            std::cout << "[Tick " << clock_global << "] Tarefa " << lista_tarefas[i].id << " ingressou.\n";
        }
    }

    // 3. Escalonamento e Preempção
    bool mudanca_na_fila = true;

    while (!fila_prontos.empty() && mudanca_na_fila) {
        mudanca_na_fila = false;

        // Achar a melhor tarefa baseado no algoritmo escolhido
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
                bool candidata_executando    = (candidata->estado == Estado::EXECUTANDO);
                bool atual_melhor_executando = (atual_melhor->estado == Estado::EXECUTANDO);
                if (candidata_executando && !atual_melhor_executando) {
                    trocar = true;
                    empate = false;
                } else if (!candidata_executando && atual_melhor_executando) {
                    // Mantém atual_melhor; nenhum trocar.
                    empate = false;
                }
            }
            
            // 2º critério de desempate: quem chegou primeiro (menor tempo_ingresso).
            if (empate) {
                if (candidata->tempo_ingresso < atual_melhor->tempo_ingresso) {
                    trocar = true;
                    empate = false;
                } else if (candidata->tempo_ingresso == atual_melhor->tempo_ingresso) {
                    // 3º critério: menor duração original.
                    if (candidata->duracao_original < atual_melhor->duracao_original) {
                        trocar = true;
                        empate = false;
                    } else if (candidata->duracao_original == atual_melhor->duracao_original) {
                        // 4º critério: sorteio (req. 4.3 item 4).
                        if (rand() % 2 == 0) trocar = true;
                        empate = false;
                    }
                }
            }
            if (trocar) melhor_idx = j;
        }

        TCB* melhor_da_fila = fila_prontos[melhor_idx];

        // Tentar CPU vazia
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
        // Sem CPU vazia - Preempção
        if (!alocou) {
            int pior_cpu_idx = -1;
            int valor_comparacao_melhor = (algoritmo == "PRIOP") ? melhor_da_fila->prioridade : melhor_da_fila->tempo_restante;
            int pior_valor_na_cpu = valor_comparacao_melhor;

            for (int i = 0; i < qtde_cpus; i++) {
                int valor_atual_cpu = (algoritmo == "PRIOP") ? cpus[i]->prioridade : cpus[i]->tempo_restante;
                if (valor_atual_cpu > pior_valor_na_cpu) {
                    pior_valor_na_cpu = valor_atual_cpu;
                    pior_cpu_idx = i;
                }
            }

            if (pior_cpu_idx != -1) {
                std::cout << "[Tick " << clock_global << "] PREEMPCAO (" << algoritmo << "): T" << melhor_da_fila->id 
                            << " expulsou T" << cpus[pior_cpu_idx]->id << " da CPU " << pior_cpu_idx + 1 << "\n";
                
                TCB* expulsa = cpus[pior_cpu_idx];
                expulsa->estado = Estado::PRONTA;
                fila_prontos.erase(fila_prontos.begin() + melhor_idx);
                fila_prontos.push_back(expulsa);
                cpus[pior_cpu_idx] = melhor_da_fila;
                cpus[pior_cpu_idx]->estado = Estado::EXECUTANDO;
                fila_prontos.erase(fila_prontos.begin() + melhor_idx);
                mudanca_na_fila = true;
            }
        }
    }

    // 4. Processa o tempo nas CPUs
    bool alguma_cpu_trabalhando = false;
    for (int i = 0; i < qtde_cpus; i++) {
        if (cpus[i] != nullptr) {
            alguma_cpu_trabalhando = true;
            std::cout << "[Tick " << clock_global << "] CPU " << i + 1 << " executando T" << cpus[i]->id 
                        << " (Faltam: " << --cpus[i]->tempo_restante << ")\n";

            if (cpus[i]->tempo_restante <= 0) {
                cpus[i]->estado = Estado::TERMINADA;
                std::cout << "[Tick " << clock_global << "] CPU " << i + 1 << " finalizou T" << cpus[i]->id << "\n";
                cpus[i] = nullptr; 
                tarefas_concluidas++;
            }
        }
    }

    if (!alguma_cpu_trabalhando && fila_prontos.empty() && tarefas_concluidas < total_tarefas) {
        std::cout << "[Tick " << clock_global << "] Sistema Ocioso...\n";
    }

    // 5. Avança o relógio!
    clock_global++;

    if (isFinalizado()) {
        std::cout << "\nSimulacao concluida no tick " << clock_global << "!\n";
    }
}