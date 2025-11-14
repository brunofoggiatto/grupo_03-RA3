#include "../include/namespace.hpp"
#include <iostream>
#include <unistd.h>

using namespace std;

void test_list_namespaces() {
    cout << "\n=== TESTE 1: Listando namespaces do proprio processo ===" << endl;

    pid_t my_pid = getpid();
    auto result = list_process_namespaces(my_pid);

    if (result) {
        print_process_namespaces(*result);
    } else {
        cout << "Erro ao listar namespaces" << endl;
    }
}

void test_find_processes() {
    cout << "\n=== TESTE 2: Encontrando processos em diferentes namespaces ===" << endl;

    pid_t my_pid = getpid();
    auto result = list_process_namespaces(my_pid);

    if (result) {
        // testa varios tipos de namespaces para verificar visibilidade
        vector<NamespaceType> types_to_test = {
            NamespaceType::PID,
            NamespaceType::NET,
            NamespaceType::IPC,
            NamespaceType::UTS
        };

        for (auto ns_type : types_to_test) {
            for (const auto& ns : result->namespaces) {
                if (ns.type == ns_type && ns.exists) {
                    cout << "\n" << ns.name << " namespace (inode: " << ns.inode << ")" << endl;

                    auto pids = find_processes_in_namespace(ns_type, ns.inode);

                    cout << "  Encontrados " << pids.size() << " processos" << endl;
                    cout << "  Primeiros 10 PIDs: ";
                    for (size_t i = 0; i < min(pids.size(), size_t(10)); i++) {
                        cout << pids[i] << " ";
                    }
                    cout << endl;
                    break;
                }
            }
        }
    }
}

void test_compare_namespaces() {
    cout << "\n=== TESTE 3: Comparando namespaces ===" << endl;

    pid_t pid1 = getpid();
    pid_t pid2 = getppid();  // processo pai

    auto result = compare_namespaces(pid1, pid2);

    if (result) {
        print_namespace_comparison(*result);
    } else {
        cout << "Erro ao comparar namespaces" << endl;
    }
}

void test_generate_report() {
    cout << "\n=== TESTE 4: Gerando relatorio ===" << endl;

    // gera CSV
    if (generate_namespace_report("experimento2_namespace_report.csv", "csv")) {
        cout << "Relatorio CSV gerado com sucesso: experimento2_namespace_report.csv" << endl;
    } else {
        cout << "Erro ao gerar relatorio CSV" << endl;
    }

    // gera JSON
    if (generate_namespace_report("experimento2_namespace_report.json", "json")) {
        cout << "Relatorio JSON gerado com sucesso: experimento2_namespace_report.json" << endl;
    } else {
        cout << "Erro ao gerar relatorio JSON" << endl;
    }
}

int main() {
    cout << "==================================================" << endl;
    cout << "  TESTES DO NAMESPACE ANALYZER - ALUNO C" << endl;
    cout << "==================================================" << endl;

    test_list_namespaces();
    test_find_processes();
    test_compare_namespaces();
    test_generate_report();

    cout << "\n==================================================" << endl;
    cout << "  TESTES FINALIZADOS" << endl;
    cout << "==================================================" << endl;

    return 0;
}
