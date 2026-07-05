# Simulador de Escalonamento de Processos

## Instalação/Execução

```bash
# Clone o repositório
git clone https://github.com/AJNkamura/SistemasOperacionais.git
cd SistemasOperacionais

### No Linux (Ubuntu/Debian)

Certifique-se de ter o Qt e os compiladores básicos instalados:
```bash

# 1. Instale as dependências (Qt5 e ferramentas de build)
sudo apt update
sudo apt install build-essential qtbase5-dev qt5-qmake qtchooser qt5-default

# 2. Compile o projeto
mkdir build && cd build 
cmake ..
make

# 3. Execute o simulador
./SimuladorSO
```
### No Windows (Standalone / Sem instalação extra)
Para cumprir o requisito de rodar o programa sem necessidade de bibliotecas extras instaladas no PC destino:
1. Abra o projeto no **Qt Creator**.
2. Selecione o modo de compilação para **Release** e compile o projeto (botão de martelo/Build).
3. Abra o **Qt Command Prompt** (MinGW) no menu Iniciar.
4. Navegue até a pasta de *release* gerada pelo Qt Creator e rode o comando para empacotar as DLLs junto ao `.exe`:
```cmd
windeployqt Escalonador.exe
```
5. Agora você pode zipar a pasta inteira e executá-la em qualquer máquina Windows clicando em `Escalonador.exe`.

---

## 📄 Formato do Arquivo de Configuração (.txt)

A simulação é alimentada por arquivos de texto simples (separados por ponto e vírgula `;`). 

**Primeira Linha (Configuração do Sistema):**
```text
ALGORITMO;QUANTUM;QTD_CPUS;[ALPHA]
```
* *Exemplo:* `PRIOPEnv;3;2;1` (Algoritmo PRIOPEnv, Quantum 3, 2 CPUs, Fator de Envelhecimento 1).

**Demais Linhas (Configuração das Tarefas):**
```text
ID;COR_HEX;INGRESSO;DURACAO;PRIORIDADE_ESTATICA;EVENTO_1;EVENTO_2;...
```
* *Exemplo:* `t01;FF0000;0;10;5;ML01:03;IO:04-02;MU01:08`

### ⏰ Dicionário de Eventos:
Os instantes de tempo dos eventos são relativos ao **início da execução da tarefa** (tempo de CPU efetivo da tarefa).
- `MLxx:00` -> **Mutex Lock:** Solicita o Mutex `xx` no instante `00`.
- `MUxx:00` -> **Mutex Unlock:** Libera o Mutex `xx` no instante `00`.
- `IO:xx-yy` -> **Input/Output:** Inicia I/O no instante `xx` com duração de `yy` ticks.

*(Exemplo de linha completa: `t02;00FF00;2;15;3;ML02:05;IO:08-04;MU02:14`)*

---

## 🎮 Como Usar a Interface
1. Clique em **"Carregar TXT"** e selecione seu arquivo de configuração.
2. Use **"Avançar Passo >>"** para depurar o comportamento do escalonador tick a tick.
3. Use **"<< Retroceder"** caso precise voltar no tempo para entender um comportamento (deadlock, preempção, etc).
4. Clique em **"Play Automático"** para rodar a simulação continuamente até o fim.
5. Ao final, clique em **"Exportar Imagem"** para salvar o Gráfico de Gantt gerado em PNG.
```