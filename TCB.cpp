#include "TCB.h"

// Def. tarefa
TCB::TCB(int id, std::string cor, int ingresso, int duracao, int prioridade) {
    this->id = id;
    this->cor_hex = cor;
    this->tempo_ingresso = ingresso;
    this->tempo_duracao = duracao;
    this->tempo_restante = duracao;
    this->prioridade = prioridade;
    
    this->estado = Estado::NOVO; 
}