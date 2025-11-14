#include "../include/monitor.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <thread>
#include <iomanip>
#include <cmath>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;
using namespace chrono;

// ================================
// EXPERIMENTO 1: OVERHEAD DE MONITORAMENTO
// ================================
// Este programa mede o impacto do próprio profiler no sistema
// utilizando as funções do Resource Profiler (Componente 1)
//
// METODOLOGIA:
// - Baseline: fork() + workload no filho + waitpid() SEM monitoramento
// - Com monitoramento: fork() + workload no filho + coleta de métricas no pai + waitpid()
// - Comparação justa: ambos usam fork/waitpid, diferença é apenas o monitoramento

struct BenchmarkResult {
    string test_name;
    double execution_time_ms;
    double cpu_overhead_percent;
    int sampling_latency_us;
    int iterations_completed;
};

// Workload de referência: cálculo intensivo de CPU
void cpu_intensive_workload(int iterations) {
    volatile double result = 0.0;
    for (int i = 0; i < iterations; i++) {
        result += sqrt(i) * sin(i) * cos(i);
    }
}

// Executa workload SEM monitoramento (mas com fork para comparação justa)
BenchmarkResult run_without_monitoring(int workload_iterations) {
    BenchmarkResult result;
    result.test_name = "SEM_MONITORAMENTO";
    result.cpu_overhead_percent = 0.0;
    result.sampling_latency_us = 0;
    result.iterations_completed = workload_iterations;

    auto start = high_resolution_clock::now();  // MEDIR ANTES DO FORK

    pid_t worker_pid = fork();

    if (worker_pid == 0) {
        // Processo filho: executa o workload
        cpu_intensive_workload(workload_iterations);
        _exit(0);
    } else if (worker_pid > 0) {
        // Processo pai: apenas espera (SEM monitorar)
        waitpid(worker_pid, nullptr, 0);

        auto end = high_resolution_clock::now();
        result.execution_time_ms = duration<double, milli>(end - start).count();
    }

    return result;
}

// Executa workload COM monitoramento usando o Resource Profiler
BenchmarkResult run_with_monitoring(int workload_iterations, int sampling_interval_ms) {
    BenchmarkResult result;
    result.test_name = "COM_MONITORAMENTO_" + to_string(sampling_interval_ms) + "ms";
    result.iterations_completed = workload_iterations;

    pid_t worker_pid = fork();

    if (worker_pid == 0) {
        // Processo filho: executa o workload
        cpu_intensive_workload(workload_iterations);
        _exit(0);
    } else if (worker_pid > 0) {
        // Processo pai: monitora usando o Resource Profiler
        auto start = high_resolution_clock::now();

        ProcStats prev_stats, curr_stats;
        vector<int> sampling_latencies;
        int sample_count = 0;

        // Coleta inicial de métricas usando o Componente 1
        get_cpu_usage(worker_pid, prev_stats);
        get_memory_usage(worker_pid, prev_stats);
        get_io_usage(worker_pid, prev_stats);

        while (true) {
            this_thread::sleep_for(milliseconds(sampling_interval_ms));

            auto sample_start = high_resolution_clock::now();

            // Coleta métricas de CPU, memória e I/O do processo filho
            int status = get_cpu_usage(worker_pid, curr_stats);
            if (status != 0) break; // processo terminou

            get_memory_usage(worker_pid, curr_stats);
            get_io_usage(worker_pid, curr_stats);

            auto sample_end = high_resolution_clock::now();
            int latency = duration_cast<microseconds>(sample_end - sample_start).count();
            sampling_latencies.push_back(latency);

            prev_stats = curr_stats;
            sample_count++;

            // Verifica se processo filho terminou
            int child_status;
            if (waitpid(worker_pid, &child_status, WNOHANG) > 0) {
                break;
            }
        }

        // Espera filho terminar
        waitpid(worker_pid, nullptr, 0);

        auto end = high_resolution_clock::now();
        result.execution_time_ms = duration<double, milli>(end - start).count();

        // Calcula latência média de sampling
        if (!sampling_latencies.empty()) {
            int sum = 0;
            for (int lat : sampling_latencies) sum += lat;
            result.sampling_latency_us = sum / sampling_latencies.size();
        } else {
            result.sampling_latency_us = 0;
        }
    }

    return result;
}

void print_results(const vector<BenchmarkResult>& results) {
    cout << "\n======================================================" << endl;
    cout << "  EXPERIMENTO 1 - OVERHEAD DE MONITORAMENTO" << endl;
    cout << "  Usando Resource Profiler (Componente 1)" << endl;
    cout << "======================================================\n" << endl;

    double baseline_time = results[0].execution_time_ms;

    cout << left << setw(30) << "Teste"
         << right << setw(15) << "Tempo (ms)"
         << setw(18) << "Overhead (%)"
         << setw(20) << "Latencia Amost(us)" << endl;

    cout << string(83, '-') << endl;

    for (const auto& r : results) {
        double overhead = ((r.execution_time_ms - baseline_time) / baseline_time) * 100.0;

        cout << left << setw(30) << r.test_name
             << right << setw(15) << fixed << setprecision(2) << r.execution_time_ms
             << setw(18) << fixed << setprecision(2) << overhead << "%"
             << setw(20) << r.sampling_latency_us << endl;
    }

    cout << string(83, '-') << endl;
}

int main() {
    cout << "======================================================" << endl;
    cout << "  EXPERIMENTO 1: OVERHEAD DE MONITORAMENTO" << endl;
    cout << "  Benchmark do Resource Profiler - Aluno 1/2" << endl;
    cout << "======================================================" << endl;
    cout << "\nEste experimento mede o impacto do profiler no sistema" << endl;
    cout << "usando as funcoes do Resource Profiler (Componente 1)\n" << endl;

    const int WORKLOAD_ITERATIONS = 5000000; // 5 milhões de iterações

    vector<BenchmarkResult> results;

    cout << "Configuracao:" << endl;
    cout << "  Workload: " << WORKLOAD_ITERATIONS << " iteracoes" << endl;
    cout << "  Metricas coletadas: CPU, Memoria, I/O" << endl;
    cout << "  Intervalos testados: Sem, 10ms, 50ms, 100ms, 500ms\n" << endl;

    // TESTE 1: Baseline - SEM monitoramento
    cout << "Executando baseline (sem monitoramento)... " << flush;
    results.push_back(run_without_monitoring(WORKLOAD_ITERATIONS));
    cout << "OK (" << fixed << setprecision(2) << results.back().execution_time_ms << "ms)" << endl;

    // TESTE 2-5: COM monitoramento em diferentes intervalos
    vector<int> intervals = {10, 50, 100, 500};

    for (int interval : intervals) {
        cout << "Executando com monitoramento (" << interval << "ms)... " << flush;
        results.push_back(run_with_monitoring(WORKLOAD_ITERATIONS, interval));
        cout << "OK (" << fixed << setprecision(2) << results.back().execution_time_ms << "ms)" << endl;
    }

    print_results(results);

    // Salva resultados em CSV
    ofstream csv("experimento1_overhead_results.csv");
    csv << "Teste,Tempo_ms,Overhead_percent,Latencia_amostragem_us,Iteracoes\n";

    double baseline_time = results[0].execution_time_ms;
    for (const auto& r : results) {
        double overhead = ((r.execution_time_ms - baseline_time) / baseline_time) * 100.0;
        csv << r.test_name << ","
            << r.execution_time_ms << ","
            << overhead << ","
            << r.sampling_latency_us << ","
            << r.iterations_completed << "\n";
    }
    csv.close();

    // DEMONSTRA USO ADICIONAL DA API
    cout << "\n\nDemonstracao adicional - Monitorando processo atual:" << endl;
    ProcStats my_stats;
    pid_t my_pid = getpid();

    if (get_cpu_usage(my_pid, my_stats) == 0) {
        cout << "  PID: " << my_pid << endl;
        cout << "  Threads: " << my_stats.threads << endl;
        cout << "  Context switches (voluntarios): " << my_stats.voluntary_ctxt << endl;
    }

    if (get_memory_usage(my_pid, my_stats) == 0) {
        cout << "  RSS: " << my_stats.memory_rss / 1024 << " KB" << endl;
        cout << "  VSZ: " << my_stats.memory_vsz / 1024 << " KB" << endl;
    }

    cout << "\n======================================================" << endl;
    cout << "  EXPERIMENTO 1 CONCLUIDO" << endl;
    cout << "  Arquivo gerado: experimento1_overhead_results.csv" << endl;
    cout << "======================================================" << endl;

    return 0;
}
