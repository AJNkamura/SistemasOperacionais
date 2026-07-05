#include "TCB.h"

// Implementação do construtor
TCB::TCB(int id, std::string cor, int ingresso, int duracao, int prio) {
    this->id = id;
    this->cor_hex = cor;
    this->tempo_ingresso = ingresso;
    this->tempo_duracao = duracao;
    this->duracao_original = duracao;
    this->tempo_restante = duracao;
    this->tempo_executado = 0;

    this->prioridade_estatica = prio;   // suporte a PRIOd: prioridade inicial é igual em ambas
    this->prioridade_dinamica = prio;
    
    this->estado = Estado::NOVO;
    this->quantum_usado = 0;
}