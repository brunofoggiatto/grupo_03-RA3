#include "../include/namespace.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <ctime>
#include <sched.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <cstring>
#include <iomanip>

using namespace std;

// ================================
// EXPERIMENTO 2: BENCHMARK DE OVERHEAD DE NAMESPACES
// ================================
// Este programa mede o overhead de criação de namespaces
// utilizando as funções do Namespace Analyzer (Componente 2)
// implementadas em namespace_analyzer.cpp
//
// METODOLOGIA:
// - Mede apenas a syscall unshare(), sem incluir fork/waitpid no tempo
// - Usa pipe para comunicar o tempo medido do processo filho ao pai
// - 200 iterações para reduzir variância estatística
// - Cada namespace é testado de forma independente

// Estrutura para armazenar resultados do benchmark
struct BenchmarkResult {
    string ns_name;
    int ns_flag;
    double avg_us;
    double min_us;
    double max_us;
    bool supported;
    int processes_found; // usando a funcao find_processes_in_namespace
};

// Mede tempo em microssegundos
double get_time_us(const timespec& start, const timespec& end) {
    return (end.tv_sec - start.tv_sec) * 1000000.0 +
           (end.tv_nsec - start.tv_nsec) / 1000.0;
}

// Testa criacao de namespace medindo APENAS unshare() (sem fork/waitpid)
// Usa pipe para comunicar o tempo do filho ao pai
double test_namespace_creation(int ns_flag) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        return -1;
    }

    pid_t pid = fork();

    if (pid == 0) {
        // Processo filho - mede APENAS unshare()
        close(pipefd[0]); // fecha leitura

        timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        if (ns_flag != 0) {
            unshare(ns_flag);
        }

        clock_gettime(CLOCK_MONOTONIC, &end);

        double time_us = get_time_us(start, end);
        ssize_t bytes_written = write(pipefd[1], &time_us, sizeof(time_us));
        (void)bytes_written;
        close(pipefd[1]);
        _exit(0);
    } else if (pid > 0) {
        // Processo pai - lê tempo do filho
        close(pipefd[1]); // fecha escrita

        double time_us;
        ssize_t bytes_read = read(pipefd[0], &time_us, sizeof(time_us));
        (void)bytes_read;
        close(pipefd[0]);

        waitpid(pid, nullptr, 0);
        return time_us;
    }

    return -1;
}

// Verifica se o namespace é suportado
bool is_namespace_supported(int ns_flag) {
    pid_t pid = fork();

    if (pid == 0) {
        if (unshare(ns_flag) == 0) {
            _exit(0);  // sucesso
        } else {
            _exit(1);  // falha
        }
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }

    return false;
}

// Conta processos no namespace usando a API do projeto
int count_processes_in_namespace(NamespaceType ns_type) {
    pid_t my_pid = getpid();
    auto result = list_process_namespaces(my_pid);

    if (!result) return 0;

    // encontra o inode do namespace do tipo especificado
    for (const auto& ns : result->namespaces) {
        if (ns.type == ns_type && ns.exists) {
            // usa a funcao do projeto para contar processos
            auto pids = find_processes_in_namespace(ns_type, ns.inode);
            return pids.size();
        }
    }

    return 0;
}

// Faz benchmark de um tipo de namespace
// AGORA INTEGRADO COM A ESTRUTURA DO PROJETO
BenchmarkResult benchmark_namespace(const string& name, int ns_flag,
                                   NamespaceType ns_type, int iterations) {
    BenchmarkResult result;
    result.ns_name = name;
    result.ns_flag = ns_flag;
    result.supported = true;
    result.processes_found = 0;

    // verifica se é suportado
    if (ns_flag != 0 && !is_namespace_supported(ns_flag)) {
        result.supported = false;
        result.avg_us = 0;
        result.min_us = 0;
        result.max_us = 0;
        return result;
    }

    vector<double> times;

    cout << "  Testando " << name << "... " << flush;

    for (int i = 0; i < iterations; i++) {
        double time_us = test_namespace_creation(ns_flag);
        times.push_back(time_us);
    }

    // calcula estatisticas
    sort(times.begin(), times.end());

    double sum = 0;
    for (double t : times) {
        sum += t;
    }

    result.avg_us = sum / times.size();
    result.min_us = times.front();
    result.max_us = times.back();

    // Usa a API do Namespace Analyzer para contar processos no namespace
    if (ns_flag != 0) {
        result.processes_found = count_processes_in_namespace(ns_type);
    }

    cout << "OK (" << result.processes_found << " processos)" << endl;

    return result;
}

void print_results(const vector<BenchmarkResult>& results) {
    cout << "\n======================================================" << endl;
    cout << "  EXPERIMENTO 2 - OVERHEAD DE NAMESPACES" << endl;
    cout << "  Usando Namespace Analyzer (Componente 2)" << endl;
    cout << "======================================================\n" << endl;

    // tabela formatada
    cout << left << setw(12) << "Namespace"
         << right << setw(15) << "Media (us)"
         << setw(15) << "Min (us)"
         << setw(15) << "Max (us)"
         << setw(12) << "Processos" << endl;

    cout << string(69, '-') << endl;

    for (const auto& result : results) {
        if (!result.supported) {
            cout << left << setw(12) << result.ns_name
                 << right << setw(57) << "NAO SUPORTADO" << endl;
            continue;
        }

        cout << left << setw(12) << result.ns_name
             << right << setw(15) << fixed << setprecision(2) << result.avg_us
             << setw(15) << fixed << setprecision(2) << result.min_us
             << setw(15) << fixed << setprecision(2) << result.max_us
             << setw(12) << result.processes_found << endl;
    }

    cout << string(69, '-') << endl;

    // Ranking de overhead (do mais rápido ao mais lento)
    cout << "\nRANKING DE OVERHEAD (mais rapido -> mais lento):\n" << endl;

    // Copia resultados e ordena por tempo médio
    vector<BenchmarkResult> sorted_results;
    for (const auto& r : results) {
        if (r.supported) {
            sorted_results.push_back(r);
        }
    }
    sort(sorted_results.begin(), sorted_results.end(),
         [](const BenchmarkResult& a, const BenchmarkResult& b) {
             return a.avg_us < b.avg_us;
         });

    for (size_t i = 0; i < sorted_results.size(); i++) {
        cout << (i + 1) << ". " << sorted_results[i].ns_name
             << " (" << fixed << setprecision(2) << sorted_results[i].avg_us << " us)" << endl;
    }
}

int main() {
    cout << "======================================================" << endl;
    cout << "  EXPERIMENTO 2: ISOLAMENTO VIA NAMESPACES" << endl;
    cout << "  Benchmark de Overhead - Aluno C" << endl;
    cout << "======================================================" << endl;
    cout << "\nEste experimento mede o overhead de criacao de namespaces" << endl;
    cout << "usando as funcoes do Namespace Analyzer (Componente 2)" << endl;
    cout << "Cada teste sera executado 200 vezes para precisao\n" << endl;

    const int ITERATIONS = 200;

    // verifica se esta rodando como root
    if (geteuid() != 0) {
        cout << "AVISO: Alguns namespaces podem precisar de root!\n" << endl;
    }

    // DEMONSTRA USO DA API DO PROJETO
    cout << "Verificando namespace do processo atual...\n" << endl;
    pid_t my_pid = getpid();
    auto my_namespaces = list_process_namespaces(my_pid);

    if (my_namespaces) {
        cout << "Processo " << my_pid << " tem "
             << my_namespaces->ns_count << " namespaces ativos\n" << endl;
    }

    vector<BenchmarkResult> results;

    cout << "Executando testes de overhead (medindo apenas unshare)...\n" << endl;

    // Testa cada tipo de namespace usando a API do Namespace Analyzer
    results.push_back(benchmark_namespace("CGROUP", CLONE_NEWCGROUP, NamespaceType::CGROUP, ITERATIONS));
    results.push_back(benchmark_namespace("IPC", CLONE_NEWIPC, NamespaceType::IPC, ITERATIONS));
    results.push_back(benchmark_namespace("MNT", CLONE_NEWNS, NamespaceType::MNT, ITERATIONS));
    results.push_back(benchmark_namespace("NET", CLONE_NEWNET, NamespaceType::NET, ITERATIONS));
    results.push_back(benchmark_namespace("PID", CLONE_NEWPID, NamespaceType::PID, ITERATIONS));
    results.push_back(benchmark_namespace("USER", CLONE_NEWUSER, NamespaceType::USER, ITERATIONS));
    results.push_back(benchmark_namespace("UTS", CLONE_NEWUTS, NamespaceType::UTS, ITERATIONS));

    print_results(results);

    // salva em arquivo CSV
    ofstream csv("experimento2_benchmark_results.csv");
    csv << "Namespace,Avg_us,Min_us,Max_us,Processes_Found,Supported\n";
    for (const auto& r : results) {
        csv << r.ns_name << ","
            << r.avg_us << ","
            << r.min_us << ","
            << r.max_us << ","
            << r.processes_found << ","
            << (r.supported ? "yes" : "no") << "\n";
    }
    csv.close();

    cout << "\n\nResultados salvos em: experimento2_benchmark_results.csv" << endl;

    // DEMONSTRA USO DA API: gera relatorio completo do sistema
    cout << "\nGerando relatorio completo do sistema usando generate_namespace_report()..." << endl;
    if (generate_namespace_report("experimento2_system_namespaces.csv", "csv")) {
        cout << "Relatorio do sistema salvo em: experimento2_system_namespaces.csv" << endl;
    }

    cout << "\n======================================================" << endl;
    cout << "  EXPERIMENTO 2 CONCLUIDO" << endl;
    cout << "  Arquivos gerados:" << endl;
    cout << "  - experimento2_benchmark_results.csv" << endl;
    cout << "  - experimento2_system_namespaces.csv" << endl;
    cout << "======================================================" << endl;

    return 0;
}
