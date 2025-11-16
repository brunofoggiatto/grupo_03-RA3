/*
 * Namespace Analyzer - CLI Tool
 *
 * Ferramenta de linha de comando para análise de namespaces do Linux.
 * Permite listar, comparar e gerar relatórios de namespaces do sistema.
 *
 * Uso:
 *   namespace_analyzer list <PID>              - Lista namespaces de um processo
 *   namespace_analyzer find <TYPE> <INODE>     - Encontra processos em um namespace
 *   namespace_analyzer compare <PID1> <PID2>   - Compara namespaces entre processos
 *   namespace_analyzer report <FORMAT>         - Gera relatório (csv/json)
 */

#include "../include/namespace.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>

void print_usage(const char* program_name) {
    std::cout << "Uso: " << program_name << " <comando> [argumentos]\n\n";
    std::cout << "Comandos:\n";
    std::cout << "  list <PID>              Lista namespaces de um processo\n";
    std::cout << "  find <TYPE> <INODE>     Encontra processos em um namespace específico\n";
    std::cout << "                          Tipos: cgroup, ipc, mnt, net, pid, user, uts\n";
    std::cout << "  compare <PID1> <PID2>   Compara namespaces entre dois processos\n";
    std::cout << "  report <FORMAT>         Gera relatório do sistema (csv ou json)\n";
    std::cout << "  help                    Mostra esta mensagem de ajuda\n";
    std::cout << "\nExemplos:\n";
    std::cout << "  " << program_name << " list 1234\n";
    std::cout << "  " << program_name << " find pid 4026531836\n";
    std::cout << "  " << program_name << " compare 1234 5678\n";
    std::cout << "  " << program_name << " report json\n";
}

int cmd_list(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Erro: PID não fornecido\n";
        std::cerr << "Uso: list <PID>\n";
        return 1;
    }

    pid_t pid = std::stoi(argv[2]);
    auto result = list_process_namespaces(pid);

    if (!result) {
        std::cerr << "Erro: Não foi possível ler namespaces do processo " << pid << "\n";
        std::cerr << "Verifique se o processo existe e se você tem permissões adequadas.\n";
        return 1;
    }

    print_process_namespaces(*result);
    return 0;
}

int cmd_find(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Erro: Argumentos insuficientes\n";
        std::cerr << "Uso: find <TYPE> <INODE>\n";
        std::cerr << "Tipos: cgroup, ipc, mnt, net, pid, user, uts\n";
        return 1;
    }

    auto ns_type = string_to_namespace_type(argv[2]);
    if (!ns_type) {
        std::cerr << "Erro: Tipo de namespace inválido: " << argv[2] << "\n";
        std::cerr << "Tipos válidos: cgroup, ipc, mnt, net, pid, user, uts\n";
        return 1;
    }

    ino_t inode = std::stoul(argv[3]);
    auto pids = find_processes_in_namespace(*ns_type, inode);

    std::cout << "\nProcessos no namespace " << argv[2] << " (inode " << inode << "):\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Total de processos encontrados: " << pids.size() << "\n\n";

    if (!pids.empty()) {
        std::cout << "PIDs:\n";
        for (size_t i = 0; i < pids.size(); i++) {
            std::cout << "  " << pids[i];
            if ((i + 1) % 10 == 0) {
                std::cout << "\n";
            }
        }
        std::cout << "\n";
    }

    return 0;
}

int cmd_compare(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Erro: Argumentos insuficientes\n";
        std::cerr << "Uso: compare <PID1> <PID2>\n";
        return 1;
    }

    pid_t pid1 = std::stoi(argv[2]);
    pid_t pid2 = std::stoi(argv[3]);

    auto result = compare_namespaces(pid1, pid2);

    if (!result) {
        std::cerr << "Erro: Não foi possível comparar processos\n";
        std::cerr << "Verifique se ambos os processos existem e se você tem permissões.\n";
        return 1;
    }

    print_namespace_comparison(*result);
    return 0;
}

int cmd_report(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Erro: Formato não especificado\n";
        std::cerr << "Uso: report <FORMAT>\n";
        std::cerr << "Formatos: csv, json\n";
        return 1;
    }

    std::string format = argv[2];
    if (format != "csv" && format != "json") {
        std::cerr << "Erro: Formato inválido: " << format << "\n";
        std::cerr << "Formatos válidos: csv, json\n";
        return 1;
    }

    std::string output_file = "namespace_report." + format;

    std::cout << "Gerando relatório de namespaces do sistema...\n";
    std::cout << "Isso pode levar alguns segundos...\n";

    if (!generate_namespace_report(output_file, format)) {
        std::cerr << "Erro: Falha ao gerar relatório\n";
        return 1;
    }

    std::cout << "Relatório gerado com sucesso: " << output_file << "\n";
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    std::string command = argv[1];

    if (command == "list") {
        return cmd_list(argc, argv);
    } else if (command == "find") {
        return cmd_find(argc, argv);
    } else if (command == "compare") {
        return cmd_compare(argc, argv);
    } else if (command == "report") {
        return cmd_report(argc, argv);
    } else if (command == "help" || command == "--help" || command == "-h") {
        print_usage(argv[0]);
        return 0;
    } else {
        std::cerr << "Erro: Comando desconhecido: " << command << "\n\n";
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}
