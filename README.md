# ATR — Sistema de Comunicação Inter-Processos

## 📋 Visão Geral

**ATR (Aquisição, Transformação, Roteamento)** é um sistema modular Windows completo que implementa um pipeline de processamento de mensagens usando múltiplos processos comunicando-se através de **buffers circulares compartilhados**, **semáforos**, **mutexes** e **pipes nomeados**.

O sistema demonstra padrões avançados de sincronização e IPC em Windows, com dois níveis de buffers circulares (L1 e L2) que roteiam mensagens entre módulos especializados.

---

## 🏗️ Arquitetura

O sistema consiste em 6 processos independentes que se comunicam através de memória compartilhada e IPC:

```
                    ┌──────────────┐
                    │   LAUNCHER   │ (Orquestrador)
                    └──────┬───────┘
            ┌──────────────┼──────────────┐
            │              │              │
    ┌───────▼──────┐  ┌───▼────┐  ┌─────▼──────┐
    │  IO_ENTRADA  │  │TECLADO │  │  CAPTURA   │
    │(Prod Msg11/44)│  │(Ctrl)  │  │(Roteador)  │
    └───────┬──────┘  └────────┘  └─────┬──────┘
            │                            │
            ▼                            ▼
      ┌─────────┐                  ┌─────────┐
      │Buffer L1├─────────────────►│ CAPTURA │
      └─────────┘                  └────┬────┘
                                        │
                    ┌───────────────────┴────────────┐
                    │                                │
                    ▼                                ▼
            ┌──────────────┐                  ┌──────────┐
            │  NAMED PIPE  │                  │Buffer L2 │
            └──────┬───────┘                  └────┬─────┘
                   │                               │
                   ▼                               ▼
            ┌──────────┐                    ┌──────────┐
            │ EXIBIÇÃO │                    │ ANÁLISE  │
            └──────────┘                    └──────────┘
```

**Fluxo de dados:**
1. **IO_ENTRADA** gera mensagens tipo 11 (granulometria) e 44 (processo)
2. Empilha em **Buffer L1** (capacidade 50, tamanho 100 bytes)
3. **CAPTURA** consome de L1 e roteia:
   - MSG 44 → **Named Pipe** → **EXIBIÇÃO**
   - MSG 11 → **Buffer L2** → **ANÁLISE**
4. **TECLADO** controla RUN/PAUSE/QUIT de todos os módulos

---

## 📦 Módulos

| Módulo | Função | Características |
|--------|--------|-----------------|
| **launcher.exe** | Orquestrador | Cria objetos kernel, spawna processos filhos, aguarda finalização |
| **io_entrada.exe** | Gerador | 2 threads: Msg11 (1-5s aleatório), Msg44 (500ms fixo) |
| **captura.exe** | Roteador | Consome L1, roteia 44→Pipe, 11→L2 |
| **exibicao.exe** | Display | Server Named Pipe, exibe Msg44 |
| **analise.exe** | Processador | Consome L2, processa Msg11 |
| **teclado.exe** | Controle | Interface RUN/PAUSE/QUIT via teclas |

### Formatos de Mensagem
```
Tipo 11: 11/NSEQ/TIMESTAMP/ID/GR_MED/GR_MAX/GR_MIN/SIGMA
Tipo 44: 44/NSEQ/ID/TIMESTAMP/VEL/INCL/POT/VZ_ENT/VZ_SAIDA
```

---

## 🔄 Buffer Circular (Shared Ring)

Implementação de fila FIFO em memória compartilhada usando índices lógicos infinitos:

```cpp
struct SharedRing {
    LONG capacity;      // Número de slots
    LONG msg_size;      // Tamanho de cada mensagem
    LONG head;          // Índice de escrita (infinito)
    LONG tail;          // Índice de leitura (infinito)
    BYTE data[];        // Slots compactados
};
```

**Índices:**
- **Lógico**: Nunca reseta (0, 1, 2, 3, ..., ∞)
- **Físico**: `lógico % capacity` (acesso circular ao array)

**Configuração:**
- **L1**: 50 slots × 100 bytes (mensagens mistas)
- **L2**: 30 slots × 128 bytes (apenas Msg11)

---

## 🔐 Sincronização

Cada buffer usa 3 primitivas de sincronização:

```
┌─────────────────────┬──────────────┬─────────────────────┐
│   Objeto Kernel     │ Inicial      │ Uso                 │
├─────────────────────┼──────────────┼─────────────────────┤
│ g_semItems_L1       │ 0            │ Conta itens cheios  │
│ g_semSpaces_L1      │ B1_CAP (50)  │ Conta vagas livres  │
│ mutexL1             │ Unlocked     │ Protege head/tail   │
└─────────────────────┴──────────────┴─────────────────────┘
```

**Protocolo Produtor:**
```cpp
WaitForSingleObject(g_semSpaces_L1)    // Espera vaga
WaitForSingleObject(mutexL1)           // Entra RC
  memcpy(buffer[head % cap], msg)      // Escreve
  head++                                // Avança
ReleaseMutex(mutexL1)                   // Sai RC
ReleaseSemaphore(g_semItems_L1)         // Sinaliza item
```

**Protocolo Consumidor:**
```cpp
WaitForSingleObject(g_semItems_L1)      // Espera item
WaitForSingleObject(mutexL1)            // Entra RC
  memcpy(msg, buffer[tail % cap])       // Lê
  tail++                                 // Avança
ReleaseMutex(mutexL1)                    // Sai RC
ReleaseSemaphore(g_semSpaces_L1)         // Sinaliza vaga
```

---

## 🎛️ Controle: RUN/PAUSE/QUIT

Todos os módulos respondem a eventos manuais de controle:

| Tecla (Teclado) | Evento | Ação |
|-----------------|--------|------|
| M | `g_evtRunMedicao` | Liga/Desliga geração Msg11 |
| P | `g_evtRunCLP` | Liga/Desliga geração Msg44 |
| R | `g_evtRunCaptura` | Liga/Desliga roteamento |
| E | `g_evtRunExibicao` | Liga/Desliga display |
| A | `g_evtRunAnalise` | Liga/Desliga processamento |
| ESC | `g_evtQuitAll` | Encerra todos os processos |

**Implementação:**
```cpp
// Loop típico de um módulo
for (;;) {
    if (WaitForSingleObject(g_evtQuitAll, 0) == WAIT_OBJECT_0) break;
    
    if (WaitForSingleObject(evtRun, 0) != WAIT_OBJECT_0) {
        // PAUSE: aguarda RUN ou QUIT
        HANDLE hs[2] = { g_evtQuitAll, evtRun };
        WaitForMultipleObjects(2, hs, FALSE, INFINITE);
    }
    
    // Trabalho...
}
```

---

## 🔧 Compilação e Execução

### Pré-requisitos
- Windows 10/11
- Visual Studio 2022 (MSVC C++17)
- CMake 3.20+

### Build
```bash
# Usando o script fornecido
.\build.bat

# Ou manualmente
cmake -S . -B out\build -G "Visual Studio 17 2022" -A x64
cmake --build out\build --config Release
cd out\build\bin\Release
```

### Executar
```bash
.\launcher.exe
```

Isso abrirá **6 janelas de console** (uma para cada processo). Use a janela do **TECLADO** para controlar o sistema.

### Exemplo de Saída

```
[produtor prod_11] L1 push idx=0 (fis=0), bytes=85, msg="11/0/14:32:15/2/45.23/..."
[consumidor captura] L1 pull idx=0, roteado: MSG11 para L2
[consumidor analise] L2 pull idx=0, processando granulometria

[produtor prod_44] L1 push idx=1 (fis=1), bytes=92, msg="44/0/5/14:32:15/450.50/..."
[consumidor captura] L1 pull idx=1, enviado ao pipe
[exibicao] MENSAGEM RECEBIDA: 44/0/5/14:32:15/450.50/22.30/1.80/600.00/580.00
```

---

## 📚 Estrutura de Código

```
ATR/
├── launcher/        # Processo orquestrador
├── io_entrada/      # Gerador de mensagens
├── captura/         # Roteador L1→L2/Pipe
├── exibicao/        # Display via Named Pipe
├── teclado/         # Controle via console
├── analise/         # Processador de granulometria
└── common/          # Biblioteca compartilhada
    ├── globals.cpp      # Objetos kernel globais
    ├── utils.cpp        # RNG, logging, time
    ├── messages.cpp     # Serialização Msg11/44
    └── shared_layout.hpp # Estrutura SharedRing
```

---

## 🎯 Conceitos Implementados

- **Producer-Consumer Pattern** com buffers circulares
- **Memory-Mapped Files** para IPC
- **Semaphore-Mutex Synchronization**
- **Named Pipes** para comunicação duplex
- **Event-Driven Control** (RUN/PAUSE/QUIT)
- **Multi-Process Architecture** com processos independentes

---

## 📝 Notas Técnicas

### Prevenção de Deadlock
O código implementa verificação dupla de eventos PAUSE:
1. Antes de aguardar recursos (semáforos)
2. Depois de obter recursos, antes de entrar na região crítica

Isso garante que um módulo pausado não bloqueie recursos críticos.

### Índices Infinitos
Os buffers usam índices lógicos que nunca resetam (`LONG` de 32 bits → ~2 bilhões), evitando condições de corrida em wraparound. O mapeamento físico é feito via módulo (`idx % capacity`).

---

## 📄 Licença

Projeto educacional para estudo de IPC e sincronização em Windows.

---

**Desenvolvido com:** C++17 • Windows API • CMake  
**Compilado com:** MSVC 2022
