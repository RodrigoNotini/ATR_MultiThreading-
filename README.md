
# ATR — Fase A (Skeleton)

## Como compilar (CMake)
```bash
cmake -S . -B build
cmake --build build --config Release
```

No Visual Studio 2022: **File > Open > Folder...** (selecione esta pasta). O VS detecta o CMake e cria targets.
Alvos: `launcher`, `teclado`, `io_entrada`, `captura`, `exibicao`.

## O que tem pronto
- Biblioteca `atr_common` com:
  - `utils.hpp/.cpp`: horário, random, padding, log.
  - `messages.hpp/.cpp`: estruturas Msg11/Msg44 e serialização ASCII no formato especificado.
- Cinco executáveis stub (cada um loga sua função).
- Demostração em `io_entrada` imprimindo 1 Msg11 e 1 Msg44.

## Próximos passos (Fase B em diante)
- BufferCircular<T> com semáforos + mutex.
- Eventos Win32 nomeados para toggles/quit.
- Produtores reais com temporização (Sleep provisório na Etapa 1).
- Captura/demux e IPC (pipes/mailslots).
- Projetores formatados.
