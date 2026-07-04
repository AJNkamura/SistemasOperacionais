#include "Escalonador.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cstdlib> 
#include <ctime>   

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

    std::srand(std::time(nullptr)); // Inicializa a semente do sorteio

    std::string linha;
    if (std::getline(arquivo, linha)) {
        std::stringstream ss(linha);
        std::string s_quantum, s_cpus, s_alpha;
        std::getline(ss, algoritmo, ';');
        std::getline(ss, s_quantum, ';');
        std::getline(ss, s_cpus, ';');
        if (std::getline(ss, s_alpha, ';')) { // Se tiver o 4º parâmetro
            alpha = std::stoi(trim(s_alpha));
        }
        algoritmo = toUpper(trim(algoritmo));
        quantum   = std::stoi(trim(s_quantum));
        qtde_cpus = std::stoi(trim(s_cpus));
    }
    lista_tarefas.clear();
    while (std::getline(arquivo, linha)) {
        if (linha.empty()) continue;
        std::stringstream ss(linha);
        std::string s_id, s_cor, s_in, s_dur, s_prio, evento_str;
        std::getline(ss, s_id,   ';');
        std::getline(ss, s_cor,  ';');
        std::getline(ss, s_in,   ';');
        std::getline(ss, s_dur,  ';');
        std::getline(ss, s_prio, ';');

        std::string s_id_limpo = "";
        for (char c : s_id) {
            if (isdigit(c)) s_id_limpo += c;
        }
        int id_final = s_id_limpo.empty() ? 0 : std::stoi(s_id_limpo);

        //criar tarefa
        TCB nova_tarefa(
            id_final, trim(s_cor), std::stoi(trim(s_in)),
            std::stoi(trim(s_dur)), std::stoi(trim(s_prio))
        );

        //ler eventos
        while (std::getline(ss, evento_str, ';')) {
            evento_str = trim(evento_str);
            if(evento_str.empty()) continue;

            Evento ev;
            if (evento_str.substr(0, 2) == "ML" || evento_str.substr(0, 2) == "MU") {
                ev.tipo = evento_str.substr(0, 2);
                ev.recurso_id = std::stoi(evento_str.substr(2, 2)); // Pega o 'xx'
                ev.instante = std::stoi(evento_str.substr(5, 2));   // Pega o '00'
                ev.duracao = 0;
            } else if (evento_str.substr(0, 2) == "IO") {
                ev.tipo = "IO";
                ev.recurso_id = 0;
                size_t traco = evento_str.find('-');
                ev.instante = std::stoi(evento_str.substr(3, traco - 3));
                ev.duracao = std::stoi(evento_str.substr(traco + 1));
            }
            //inserir eventos na tarefa
            nova_tarefa.eventos.push_back(ev);
        }
 
        lista_tarefas.push_back(nova_tarefa);
    }
    arquivo.close();

    cpus.assign(qtde_cpus, nullptr);
    fila_prontos.clear();
    historico.clear();
    clock_global = 0;
    tarefas_concluidas = 0;
    tempo_ocioso = 0;
    //historico.push_back(criarSnapshot());
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

Snapshot Escalonador::criarSnapshot(std::vector<int> sorteios) {
    Snapshot snap;
    snap.clock_global = clock_global;
    snap.tarefas_concluidas = tarefas_concluidas;
    snap.tempo_ocioso = tempo_ocioso;
    snap.lista_tarefas = lista_tarefas; 
    snap.ids_sorteio = sorteios;
    
    for (TCB* t : cpus)         snap.ids_task_cpu.push_back(t ? t->id : -1);
    for (TCB* t : fila_prontos) snap.ids_fila_prontos.push_back(t->id);
    return snap;
}

void Escalonador::restaurarSnapshot(const Snapshot& snap) {
    clock_global = snap.clock_global;
    tarefas_concluidas = snap.tarefas_concluidas;
    tempo_ocioso = snap.tempo_ocioso;
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

void Escalonador::retrocederTick() {
    if (historico.empty()) {
        std::cout << "[Aviso] Historico vazio.\n";
        return;
    }
    restaurarSnapshot(historico.back());
    historico.pop_back();
}

void Escalonador::avancarTick() {
    if (tarefas_concluidas >= (int)lista_tarefas.size()) {
        bool cpus_vazias = true;
        for (int i = 0; i < qtde_cpus; i++) if (cpus[i] != nullptr) cpus_vazias = false;
        if (cpus_vazias) return;
    }

    std::srand(clock_global); 

    int total_tarefas = lista_tarefas.size();
    std::vector<int> sorteados_neste_tick; 

    for (size_t i = 0; i < lista_tarefas.size(); i++) {
        if (lista_tarefas[i].tempo_ingresso == clock_global && lista_tarefas[i].estado == Estado::NOVO) {
            lista_tarefas[i].estado = Estado::PRONTA;
            fila_prontos.push_back(&lista_tarefas[i]);
        }
    }

    for (int i = 0; i < qtde_cpus; i++) {
        if (cpus[i] != nullptr && cpus[i]->estado == Estado::EXECUTANDO) {
            if (std::find(fila_prontos.begin(), fila_prontos.end(), cpus[i]) == fila_prontos.end()) {
                fila_prontos.push_back(cpus[i]);
            }
        }
    }

    bool mudanca_na_fila = true;
    while (!fila_prontos.empty() && mudanca_na_fila) {
        mudanca_na_fila = false;

        //Escolher a melhor tarefa da fila de prontos de acordo com o algoritmo
        int melhor_idx = 0;
        int id_ganhou_sorteio = -1;

        for (size_t j = 1; j < fila_prontos.size(); j++) {
            TCB* candidata = fila_prontos[j];
            TCB* atual_melhor = fila_prontos[melhor_idx];
            bool trocar = false;
            bool empate = false;

            //criterio PRIOP: maior prioridade 
            if (algoritmo == "PRIOP" || algoritmo == "PRIOPENV") {
                if (candidata->prioridade_dinamica > atual_melhor->prioridade_dinamica) { trocar = true; id_ganhou_sorteio = -1; }
                else if (candidata->prioridade_dinamica == atual_melhor->prioridade_dinamica) empate = true;
            }
            //criterio SRTF: menor tempo restante
            else if (algoritmo == "SRTF") {
                if (candidata->tempo_restante < atual_melhor->tempo_restante) { trocar = true; id_ganhou_sorteio = -1; }
                else if (candidata->tempo_restante == atual_melhor->tempo_restante) empate = true;
            }

            // -- Casos de empate -- //
            //criterio de desempate 01: quem já está executando ganha
            if (empate) {
                bool cand_exec = (candidata->estado == Estado::EXECUTANDO);
                bool atual_melhor_exec = (atual_melhor->estado == Estado::EXECUTANDO); 
                
                if (cand_exec && !atual_melhor_exec) { trocar = true; id_ganhou_sorteio = -1; }
                else if (!cand_exec && atual_melhor_exec) { empate = false; }
                else if (cand_exec && atual_melhor_exec) { empate = false; } // Ignora o sorteio se ambas já rodam
            }
        
            if (empate) {
                // criterio de desempate 02: quem entrou primeiro ganha
                if (candidata->tempo_ingresso < atual_melhor->tempo_ingresso) { trocar = true; id_ganhou_sorteio = -1; }
                else if (candidata->tempo_ingresso == atual_melhor->tempo_ingresso) {
                    
                    // criterio de desempate 03: quem tem menor duracao original ganha
                    if (candidata->duracao_original < atual_melhor->duracao_original) { trocar = true; id_ganhou_sorteio = -1; }
                    else if (candidata->duracao_original == atual_melhor->duracao_original) {
                        
                        //criterio de desempate 04: desempate aleatório
                        if (std::rand() % 2 == 1) {
                            trocar = true;
                            id_ganhou_sorteio = candidata->id;
                        }
                    }
                }
            }
            if (trocar) melhor_idx = j;
        }

        TCB* melhor_da_fila = fila_prontos[melhor_idx];
        
        if (id_ganhou_sorteio == melhor_da_fila->id) {
            sorteados_neste_tick.push_back(id_ganhou_sorteio);
        }

        bool ja_alocada = false;
        for (int i = 0; i < qtde_cpus; i++) {
            if (cpus[i] != nullptr && cpus[i]->id == melhor_da_fila->id) {
                ja_alocada = true; break;
            }
        }

        bool alocou = false;
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
            if (algoritmo == "PRIOP" || algoritmo == "PRIOPENV") {
                int menor_prio = melhor_da_fila->prioridade_dinamica;
                for (int i = 0; i < qtde_cpus; i++) {
                    if (cpus[i] != nullptr && cpus[i]->prioridade_dinamica < menor_prio) {
                        menor_prio = cpus[i]->prioridade_dinamica; 
                        pior_cpu_idx = i;
                    }
                }
            }
            else if (algoritmo == "SRTF") {
                int maior_restante = melhor_da_fila->tempo_restante;
                for (int i = 0; i < qtde_cpus; i++) {
                    if (cpus[i] != nullptr && cpus[i]->tempo_restante > maior_restante) {
                        maior_restante = cpus[i]->tempo_restante; 
                        pior_cpu_idx = i;
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

    for (auto* t : fila_prontos) {
        if (t->estado == Estado::EXECUTANDO) t->estado = Estado::PRONTA;
    }

    historico.push_back(criarSnapshot(sorteados_neste_tick));

    // ----  Controle de ticks ---- //
    bool alguma_cpu_trabalhando = false;
    std::vector<int> tarefas_processadas_neste_tick;

    for (int i = 0; i < qtde_cpus; i++) {
        if (cpus[i] != nullptr) {
            alguma_cpu_trabalhando = true;
            if (std::find(tarefas_processadas_neste_tick.begin(), tarefas_processadas_neste_tick.end(), cpus[i]->id) != tarefas_processadas_neste_tick.end()) {
                continue; 
            }
            tarefas_processadas_neste_tick.push_back(cpus[i]->id);

            if (cpus[i]->tempo_restante > 0){
                cpus[i]->tempo_restante--;
                cpus[i]->quantum_usado++;
                cpus[i]->tempo_executado++;
            } 
            if (quantum > 0 && cpus[i]->quantum_usado >= quantum && cpus[i]->tempo_restante > 0) {
                cpus[i]->estado = Estado::PRONTA; // Retorna pra fila
                cpus[i]->quantum_usado = 0;       // Reseta o quantum
            }
        } else {
            if (tarefas_concluidas < total_tarefas) tempo_ocioso++;
        }
    }

    if (algoritmo == "PRIOPENV") {
        for (auto* t : fila_prontos) {
            // Aumenta a prioridade dinâmica de quem tá na fila
            t->prioridade_dinamica += alpha; 
        }
        // Reseta a prioridade de quem acabou de entrar na CPU
        for (int i = 0; i < qtde_cpus; i++) {
            if (cpus[i] != nullptr) {
                cpus[i]->prioridade_dinamica = cpus[i]->prioridade_estatica;
            }
        }
    }

    for (int i = 0; i < qtde_cpus; i++) {
        if (cpus[i] != nullptr && cpus[i]->tempo_restante <= 0) {
            cpus[i]->tempo_restante = 0;
            if (cpus[i]->estado != Estado::TERMINADA) {
                cpus[i]->estado = Estado::TERMINADA;
                tarefas_concluidas++;
            }
            cpus[i] = nullptr;
        }
    }
    clock_global++;
}