#include "TCB.h"

// Implementação do construtor
TCB::TCB(int id, std::string cor, int ingresso, int duracao, int prio) {
    this->id = id;
    this->cor_hex = cor;
    this->tempo_ingresso = ingresso;
    this->tempo_duracao = duracao;
    this->duracao_original = duracao;
    this->tempo_restante = duracao;
    this->prioridade = prio;
    this->estado = Estado::NOVO;
}