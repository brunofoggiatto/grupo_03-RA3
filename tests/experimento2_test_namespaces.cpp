// ============================================================
// ARQUIVO: tests/experimento2_test_namespaces.cpp
// DESCRIÇÃO: Testes funcionais do Namespace Analyzer (Componente 2)
// Valida as 4 funções principais do analisador de namespaces
// 
// TESTES INCLUSOS:
// 1. Listar namespaces de um processo
// 2. Encontrar processos em um namespace específico
// 3. Comparar namespaces entre dois processos
// 4. Gerar relatórios em CSV e JSON
// 
// RESPONSABILIDADE: Aluno 3
// ============================================================

#include "../include/namespace.hpp"
#include <iostream>
#include <unistd.h>

using namespace std;

// ================================
// FUNÇÃO: test_list_namespaces
// Propósito: Testa a função list_process_namespaces()
// Obtém todos os 7 namespaces do processo atual
// ================================
void test_list_namespaces() {
    // Cabeçalho do teste
    cout << "\n=== TESTE 1: Listando namespaces do proprio processo ===" << endl;

    // Obtém o PID do processo atual
    pid_t my_pid = getpid();
    
    // Chama a função do Namespace Analyzer
    // Retorna std::optional, que pode estar vazio se erro
    auto result = list_process_namespaces(my_pid);

    // Verifica se conseguiu ler os namespaces
    if (result) {
        // result tem dados, imprime de forma formatada
        print_process_namespaces(*result);
    } else {
        // Falha ao ler (permissão ou outro erro)
        cout << "Erro ao listar namespaces" << endl;
    }
}

// ================================
// FUNÇÃO: test_find_processes
// Propósito: Testa a função find_processes_in_namespace()
// Encontra todos os processos em cada namespace do processo atual
// ================================
void test_find_processes() {
    // Cabeçalho do teste
    cout << "\n=== TESTE 2: Encontrando processos em diferentes namespaces ===" << endl;

    // Obtém PID do processo atual
    pid_t my_pid = getpid();
    
    // Lista namespaces do processo atual
    auto result = list_process_namespaces(my_pid);

    if (result) {
        // Define quais tipos de namespaces testar
        // Teste com 4 tipos principais
        vector<NamespaceType> types_to_test = {
            NamespaceType::PID,  // Isolamento de processos
            NamespaceType::NET,  // Isolamento de rede
            NamespaceType::IPC,  // Isolamento de IPC
            NamespaceType::UTS   // Isolamento de hostname
        };

        // Para cada tipo de namespace
        for (auto ns_type : types_to_test) {
            // Procura este tipo nos namespaces do processo
            for (const auto& ns : result->namespaces) {
                // Se encontrou e o namespace existe
                if (ns.type == ns_type && ns.exists) {
                    // Mostra o inode deste namespace
                    cout << "\n" << ns.name << " namespace (inode: " << ns.inode << ")" << endl;

                    // Usa a função do Analyzer para encontrar TODOS os processos
                    // que compartilham este namespace
                    auto pids = find_processes_in_namespace(ns_type, ns.inode);

                    // Mostra quantos processos encontrou
                    cout << "  Encontrados " << pids.size() << " processos" << endl;
                    
                    // Mostra os primeiros 10 PIDs (se houver mais)
                    cout << "  Primeiros 10 PIDs: ";
                    for (size_t i = 0; i < min(pids.size(), size_t(10)); i++) {
                        cout << pids[i] << " ";
                    }
                    cout << endl;
                    
                    // Passa para próximo tipo de namespace
                    break;
                }
            }
        }
    }
}

// ================================
// FUNÇÃO: test_compare_namespaces
// Propósito: Testa a função compare_namespaces()
// Compara namespaces de dois processos: processo atual e seu pai
// ================================
void test_compare_namespaces() {
    // Cabeçalho do teste
    cout << "\n=== TESTE 3: Comparando namespaces ===" << endl;

    // PID do processo atual
    pid_t pid1 = getpid();
    
    // PID do processo pai (geralmente bash ou shell)
    // getppid() retorna o PID do processo que criou este processo
    pid_t pid2 = getppid();

    // Chama a função de comparação
    // Retorna std::optional com resultado da comparação
    auto result = compare_namespaces(pid1, pid2);

    if (result) {
        // result tem dados, imprime resultado
        print_namespace_comparison(*result);
    } else {
        // Falha na comparação (permissão ou outro erro)
        cout << "Erro ao comparar namespaces" << endl;
    }
}

// ================================
// FUNÇÃO: test_generate_report
// Propósito: Testa a função generate_namespace_report()
// Gera relatórios do sistema em dois formatos: CSV e JSON
// ================================
void test_generate_report() {
    // Cabeçalho do teste
    cout << "\n=== TESTE 4: Gerando relatorio ===" << endl;

    // ================================
    // GERAR RELATÓRIO EM CSV
    // ================================
    // CSV (Comma-Separated Values) é formato tabular simples
    // Pode ser aberto em Excel, LibreOffice, ou processado por scripts
    if (generate_namespace_report("experimento2_namespace_report.csv", "csv")) {
        cout << "Relatorio CSV gerado com sucesso: experimento2_namespace_report.csv" << endl;
    } else {
        cout << "Erro ao gerar relatorio CSV" << endl;
    }

    // ================================
    // GERAR RELATÓRIO EM JSON
    // ================================
    // JSON (JavaScript Object Notation) é formato estruturado e hierárquico
    // Melhor para programas que precisam parsear dados estruturados
    if (generate_namespace_report("experimento2_namespace_report.json", "json")) {
        cout << "Relatorio JSON gerado com sucesso: experimento2_namespace_report.json" << endl;
    } else {
        cout << "Erro ao gerar relatorio JSON" << endl;
    }
}

// ================================
// FUNÇÃO: main
// Propósito: Executa todos os 4 testes do Namespace Analyzer
// ================================
int main() {
    // Cabeçalho do programa
    cout << "==================================================" << endl;
    cout << "  TESTES DO NAMESPACE ANALYZER - ALUNO C" << endl;
    cout << "==================================================" << endl;

    // ================================
    // EXECUTAR TESTES
    // ================================
    // Cada teste valida uma função principal do Analyzer
    
    // Teste 1: Listar namespaces
    test_list_namespaces();
    
    // Teste 2: Encontrar processos em namespace
    test_find_processes();
    
    // Teste 3: Comparar namespaces entre processos
    test_compare_namespaces();
    
    // Teste 4: Gerar relatórios
    test_generate_report();

    // Resumo final
    cout << "\n==================================================" << endl;
    cout << "  TESTES FINALIZADOS" << endl;
    cout << "==================================================" << endl;

    return 0;
}

// ============================================================
// NOTAS DE EXECUÇÃO
// ============================================================
//
// REQUISITOS:
// - Compilar com: g++ -std=c++23 -o experimento2_test_namespaces \
//                      tests/experimento2_test_namespaces.cpp \
//                      src/namespace_analyzer.cpp
// - Executar: ./experimento2_test_namespaces
// - Não requer root (apenas leitura de /proc)
//
// SAÍDA ESPERADA:
// 1. Lista dos 7 namespaces do processo
// 2. Processos que compartilham cada namespace
// 3. Comparação com processo pai (geralmente bash)
// 4. Dois arquivos: .csv e .json
//
// INTERPRETAÇÃO DOS RESULTADOS:
// - Se processo e pai compartilham namespaces = rodando no mesmo container
// - Se diferem em alguns namespaces = isolamento parcial (ex: docker)
// - Se diferem em todos = alta isolamento (ex: VM ou lxc)