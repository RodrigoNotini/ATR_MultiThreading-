// launcher.cpp — Etapa 1 (ATR)
// Compilar com MSVC (VS 2022). Lança: io_entrada, captura, exibicao, teclado (+ analise opcional).
#include <windows.h>
#include "globals.hpp"
#include <iostream>
#include <string>
#include <vector>

// Retorna a pasta onde está o launcher.exe (sem a barra final)
std::wstring exe_dir() {
    wchar_t buf[MAX_PATH];
    DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring full(buf, n);
    size_t p = full.find_last_of(L"\\/");
    return (p == std::wstring::npos) ? L"." : full.substr(0, p);
}

std::wstring join(const std::wstring& a, const std::wstring& b) {
	if (a.empty()) return b; // se a for vazio, retorne b, diretorio absoluto
	if (!a.empty() && (a.back() == L'\\' || a.back() == L'/')) return a + b; // se a terminar com barra, não adicione outra e concatene b
	return a + L"\\" + b; // caso normal, adicione barra entre a e b
}

// Cria um processo com CREATE_NEW_CONSOLE e retorna PROCESS_INFORMATION (handles precisam ser fechados depois)
bool spawn(const std::wstring& exe_path, PROCESS_INFORMATION& pi_out, const std::wstring& args = L"") {
    STARTUPINFOW si{};
    si.cb = sizeof(si);

    // Monta a command line (CreateProcess pode modificar o buffer; use algo mutável)
    std::wstring cmd = L"\"" + exe_path + L"\"";
    if (!args.empty()) cmd += L" " + args;

    // Use diretório do próprio exe como CWD do filho (facilita acesso a arquivos relativos)
    std::wstring cwd = exe_dir();
    BOOL ok = CreateProcessW(
        /*lpApplicationName*/ nullptr,     // deixe null e use a command line acima
        /*lpCommandLine*/     cmd.data(),  // buffer mutável
        /*lpProcessAttributes*/ nullptr,
        /*lpThreadAttributes*/  nullptr,
        /*bInheritHandles*/     FALSE,
        /*dwCreationFlags*/     0,
        /*lpEnvironment*/       nullptr,
        /*lpCurrentDirectory*/  cwd.c_str(),
        /*lpStartupInfo*/       &si,
        /*lpProcessInformation*/&pi_out
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

    atr::init_globals();
    const std::wstring base = exe_dir();
    // Se os .exe estiverem no MESMO diretório do launcher (padrão mais comum):
    // Vetor com os nomes dos programas a iniciar
    const std::vector<std::wstring> programas = {
        L"io_entrada.exe",
        L"captura.exe",
        L"exibicao.exe",
        L"teclado.exe",
        // L"analise.exe", // descomente se já existir
    };

    std::vector<PROCESS_INFORMATION> children;
    children.reserve(programas.size());
	// Inicia cada programa com spawn() e armazena o PROCESS_INFORMATION em children
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
    // Aguarda todos os processos finalizarem (cada módulo deve encerrar pelo seu próprio fluxo)
    std::vector<HANDLE> hs;
	// Reserva espaço para os handles
    hs.reserve(children.size());
	// Guarda os handles de processo (hProcess) em hs
    for (auto& pi : children) hs.push_back(pi.hProcess);

    // WaitForMultipleObjects aceita no máx. 64 handles — estamos bem abaixo disso
    WaitForMultipleObjects(static_cast<DWORD>(hs.size()), hs.data(), TRUE, INFINITE);

    atr::cleanup_globals();

    // Limpeza de handles dos processos filhos
    for (auto& pi : children) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }

    std::wcout << L"[launcher] Fim. Todos os filhos encerraram.\n";
    return 0;
}
