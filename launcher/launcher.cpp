// launcher.cpp — Etapa 1 (ATR)
// Compilar com MSVC (VS 2022). Lança: io_entrada, captura, exibicao, teclado (+ analise opcional).

#include <windows.h>
#include "globals.hpp"
#include <iostream>
#include <string>
#include <vector>

// Retorna o diretório onde está o launcher.exe (sem a barra final)
std::wstring exe_dir() {
    wchar_t buf[MAX_PATH];
    DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH); // obtém caminho completo do executável
    std::wstring full(buf, n);
    size_t p = full.find_last_of(L"\\/"); // encontra última barra
    return (p == std::wstring::npos) ? L"." : full.substr(0, p); // retorna diretório
}

// Junta dois caminhos, garantindo uma única barra entre eles
std::wstring join(const std::wstring& a, const std::wstring& b) {
    if (a.empty()) return b; // se a for vazio, retorna b (caminho absoluto)
    if (!a.empty() && (a.back() == L'\\' || a.back() == L'/')) return a + b; // evita barra duplicada
    return a + L"\\" + b; // adiciona barra normal entre a e b
}

// Cria um processo filho com CREATE_NEW_CONSOLE e retorna PROCESS_INFORMATION (handles devem ser fechados depois)
bool spawn(const std::wstring& exe_path, PROCESS_INFORMATION& pi_out, const std::wstring& args = L"") {
    STARTUPINFOW si{};
    si.cb = sizeof(si);

    // Monta a linha de comando (CreateProcess pode modificar o buffer)
    std::wstring cmd = L"\"" + exe_path + L"\"";
    if (!args.empty()) cmd += L" " + args;

    // Define diretório de trabalho como o mesmo do executável principal
    std::wstring cwd = exe_dir();
    BOOL ok = CreateProcessW(
        nullptr,               // nome do executável (usando command line)
        cmd.data(),            // linha de comando mutável
        nullptr, nullptr,      // atributos de segurança padrão
        FALSE,                 // não herda handles
        CREATE_NEW_CONSOLE,    // cria nova janela de console
        nullptr,               // ambiente herdado
        cwd.c_str(),           // diretório atual
        &si, &pi_out           // estruturas de inicialização e saída
    );

    if (!ok) {
        DWORD e = GetLastError();
        std::wcerr << L"[launcher] Falha ao criar: " << exe_path << L" (erro " << e << L")\n";
        return false;
    }

    std::wcout << L"[launcher] OK: " << exe_path << L" (PID " << pi_out.dwProcessId << L")\n";
    return true;
}

int wmain() {
    std::wcout << L"[launcher] Iniciando processos...\n";

    atr::init_globals(); // inicializa objetos kernel globais (eventos, semáforos etc.)
    const std::wstring base = exe_dir();

    // Nomes dos executáveis a serem lançados
    const std::vector<std::wstring> programas = {
        L"io_entrada.exe",
        L"captura.exe",
        L"exibicao.exe",
        L"teclado.exe",
        L"analise.exe",
    };

    std::vector<PROCESS_INFORMATION> children;
    children.reserve(programas.size()); // reserva espaço para os processos filhos

    // Cria cada processo com spawn() e armazena seus dados
    for (const auto& name : programas) {
        PROCESS_INFORMATION pi{};
        const std::wstring path = join(base, name);
        if (spawn(path, pi)) {
            children.push_back(pi);
        }
        else {
            std::wcerr << L"[launcher] Aviso: não foi possível iniciar " << name << L"\n";
        }
    }

    if (children.empty()) {
        std::wcerr << L"[launcher] Nenhum processo filho foi iniciado.\n";
        return 1;
    }

    std::wcout << L"[launcher] Todos iniciados. Aguardando finalização dos filhos...\n";

    // Coleta os handles dos processos filhos
    std::vector<HANDLE> hs;
    hs.reserve(children.size());
    for (auto& pi : children) hs.push_back(pi.hProcess);

    // Espera até que todos os processos finalizem
    WaitForMultipleObjects(static_cast<DWORD>(hs.size()), hs.data(), TRUE, INFINITE);

    atr::cleanup_globals(); // limpa e fecha objetos kernel globais

    // Fecha os handles dos processos e threads filhos
    for (auto& pi : children) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }

    std::wcout << L"[launcher] Fim. Todos os filhos encerraram.\n";
    return 0;
}
