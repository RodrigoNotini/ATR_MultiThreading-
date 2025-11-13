// shared_layout.hpp
#pragma once
#include <windows.h>
#include <cstdint>

namespace atr {
#define ATR_B1_NAME L"Local\\ATR.MAP.B1"   // nome do mapeamento de L1
    constexpr LONG B1_CAP = 200;               // capacidade da L1 
    constexpr LONG MSG_SZ = 56;                // TAMANHO maximo, considerando o \0

#define ATR_B2_NAME L"Local\\ATR.MAP.B2"   // nome do mapeamento de L2
    constexpr LONG B2_CAP = 100;                // capacidade da L2
    constexpr LONG MSG_SZ_11 = 48;             // Mensagens de granulometria tem tamanho menor, tambêm deve considerar o \0


#pragma pack(push, 1)

    struct MsgSlot {
        char text[MSG_SZ];        // linha ASCII completa "11/.../..." ou "44/.../..."
    };

    struct RingHeader {
        LONG capacity;      // nº de slots (ex.: 200)
        LONG msg_size;      // bytes por mensagem (ex.: 64)
        volatile LONG head; // próximo índice para ESCREVER
        volatile LONG tail; // próximo índice para LER
    };

#pragma pack(pop)

    // Região mapeada: [RingHeader][dados: capacity * msg_size]
    struct SharedRing {
        RingHeader hdr;
        BYTE data[1]; // truque: o “vetor real” vem depois do header
    };

    inline size_t ring_total_size(LONG cap, LONG msg) {
        // sizeof(RingHeader) é necessário pois guarda dados para a gestão do buffer circular 
        return sizeof(RingHeader) + size_t(cap) * msg;
    }
}

