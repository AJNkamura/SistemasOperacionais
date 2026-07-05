#ifndef TCB_H
#define TCB_H

#include <vector>
#include <string>

// Estados possiveis
enum class Estado { NOVO, PRONTA, EXECUTANDO, SUSPENSA, TERMINADA };

struct Evento {
    std::string tipo;           // "ML", "MU" ou "IO"
    int recurso_id;             // ID p/ Mutex (campo 'xx')
    int instante;               // tempo de inicio do evento
    int duracao;                // Apenas para I/O (campo 'yy')
    bool concluido = false;
};

class TCB {
public:
    int id;                     //id da tarefa
    std::string cor_hex;        //hexadecimal da cor
    
    int tempo_ingresso;
    int tempo_duracao;
    int duracao_original;
    int tempo_restante;
    
    int tempo_executado;        
    int io_restante = 0;        // ticks restantes para io terminar
    int mutex_esperado = -1;    // id do mutex que a task espera (-1 é nenhum)

    int prioridade_estatica;    // PRIOPEnv
    int prioridade_dinamica;    // PRIOPEnv

    Estado estado;
    int quantum_usado = 0;

    std::vector<Evento> eventos;

    TCB(int id, std::string cor, int ingresso, int duracao, int prio);
};

#endif