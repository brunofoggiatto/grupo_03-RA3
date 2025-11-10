#define _GNU_SOURCE
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

// Estrutura pra guardar resultados
struct BenchmarkResult {
    string ns_name;
    int ns_flag;
    double avg_us;
    double min_us;
    double max_us;
    bool supported;
};

// Mede tempo em microssegundos
double get_time_us(const timespec& start, const timespec& end) {
    return (end.tv_sec - start.tv_sec) * 1000000.0 +
           (end.tv_nsec - start.tv_nsec) / 1000.0;
}

// Testa criacao de namespace
double test_namespace_creation(int ns_flag) {
    timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    pid_t pid = fork();

    if (pid == 0) {
        // processo filho - tenta criar o namespace
        if (ns_flag != 0) {
            unshare(ns_flag);
        }
        _exit(0);
    } else if (pid > 0) {
        // processo pai - espera o filho terminar
        waitpid(pid, nullptr, 0);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    return get_time_us(start, end);
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

// Faz benchmark de um tipo de namespace
BenchmarkResult benchmark_namespace(const string& name, int ns_flag, int iterations) {
    BenchmarkResult result;
    result.ns_name = name;
    result.ns_flag = ns_flag;
    result.supported = true;

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

    cout << "OK" << endl;

    return result;
}

void print_results(const vector<BenchmarkResult>& results) {
    cout << "\n======================================================" << endl;
    cout << "  RESULTADOS DO BENCHMARK - OVERHEAD DE NAMESPACES" << endl;
    cout << "======================================================\n" << endl;

    // tabela formatada
    cout << left << setw(12) << "Namespace"
         << right << setw(15) << "Media (us)"
         << setw(15) << "Min (us)"
         << setw(15) << "Max (us)" << endl;

    cout << string(57, '-') << endl;

    for (const auto& result : results) {
        if (!result.supported) {
            cout << left << setw(12) << result.ns_name
                 << right << setw(45) << "NAO SUPORTADO" << endl;
            continue;
        }

        cout << left << setw(12) << result.ns_name
             << right << setw(15) << fixed << setprecision(2) << result.avg_us
             << setw(15) << fixed << setprecision(2) << result.min_us
             << setw(15) << fixed << setprecision(2) << result.max_us << endl;
    }

    cout << string(57, '-') << endl;

    // calcula overhead em relacao ao baseline
    cout << "\nOVERHEAD EM RELACAO AO BASELINE (sem namespace):\n" << endl;

    double baseline = results[0].avg_us;

    cout << left << setw(12) << "Namespace"
         << right << setw(20) << "Overhead (us)"
         << setw(20) << "Overhead (%)" << endl;

    cout << string(52, '-') << endl;

    for (size_t i = 1; i < results.size(); i++) {
        if (!results[i].supported) continue;

        double overhead_us = results[i].avg_us - baseline;
        double overhead_pct = (overhead_us / baseline) * 100.0;

        cout << left << setw(12) << results[i].ns_name
             << right << setw(20) << fixed << setprecision(2) << overhead_us
             << setw(20) << fixed << setprecision(2) << overhead_pct << "%" << endl;
    }
}

int main() {
    cout << "======================================================" << endl;
    cout << "  BENCHMARK DE NAMESPACES - ALUNO C" << endl;
    cout << "======================================================" << endl;
    cout << "\nEste programa mede o overhead de criacao de namespaces" << endl;
    cout << "Cada teste sera executado 50 vezes para precisao\n" << endl;

    const int ITERATIONS = 50;

    // verifica se esta rodando como root
    if (geteuid() != 0) {
        cout << "AVISO: Alguns namespaces podem precisar de root!\n" << endl;
    }

    vector<BenchmarkResult> results;

    // baseline - sem namespace
    cout << "Executando testes...\n" << endl;
    results.push_back(benchmark_namespace("BASELINE", 0, ITERATIONS));

    // testa cada tipo de namespace
    results.push_back(benchmark_namespace("CGROUP", CLONE_NEWCGROUP, ITERATIONS));
    results.push_back(benchmark_namespace("IPC", CLONE_NEWIPC, ITERATIONS));
    results.push_back(benchmark_namespace("NET", CLONE_NEWNET, ITERATIONS));
    results.push_back(benchmark_namespace("PID", CLONE_NEWPID, ITERATIONS));
    results.push_back(benchmark_namespace("USER", CLONE_NEWUSER, ITERATIONS));
    results.push_back(benchmark_namespace("UTS", CLONE_NEWUTS, ITERATIONS));

    // nao testa MOUNT namespace aqui pois requer mais setup

    print_results(results);

    // salva em arquivo CSV
    ofstream csv("benchmark_results.csv");
    csv << "Namespace,Avg_us,Min_us,Max_us,Supported\n";
    for (const auto& r : results) {
        csv << r.ns_name << ","
            << r.avg_us << ","
            << r.min_us << ","
            << r.max_us << ","
            << (r.supported ? "yes" : "no") << "\n";
    }
    csv.close();

    cout << "\n\nResultados salvos em: benchmark_results.csv" << endl;
    cout << "\n======================================================" << endl;

    return 0;
}
