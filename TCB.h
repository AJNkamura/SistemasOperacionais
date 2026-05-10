#ifndef TCB_H
#define TCB_H

#include <string>

// Estados possiveis
enum class Estado { NOVO, PRONTA, EXECUTANDO, SUSPENSA, TERMINADA };

class TCB {
public:
    int id;
    std::string cor_hex;
    int tempo_ingresso;
    int tempo_duracao;
    int duracao_original;
    int tempo_restante;
    int prioridade;
    Estado estado;

    TCB(int id, std::string cor, int ingresso, int duracao, int prio);
};

#endif