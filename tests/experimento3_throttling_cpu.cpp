#include <iostream>
#include <fstream>
#include <vector>
#include <numeric>
#include <cmath>
#include <chrono>
#include <thread>
#include <random>

// Simulações das funções do cgroup (você precisará integrar com sua CGroupManager)
void set_cpu_limit(double cores) {
    std::cout << "Setting CPU limit: " << cores << " cores\n";
    // Implementar usando sua CGroupManager
}

double read_cpu_from_cgroup() {
    // Simulação - substituir pela leitura real do cgroup
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.95, 1.05);
    return dis(gen) * 25.0; // Simula ~25% para 0.25 cores
}

struct Result {
    double limit_cores;
    double mean_cpu_percent;
    double std_dev;
    double mean_throughput;
};

Result test_limit(double cores, int iterations = 10) {
    Result r;
    r.limit_cores = cores;

    std::vector<double> cpu_percents;
    std::vector<double> throughputs;

    for (int i = 0; i < iterations; i++) {
        // Aplicar limite
        set_cpu_limit(cores);

        // Workload 5s
        auto start = std::chrono::high_resolution_clock::now();
        unsigned long long counter = 0;
        auto deadline = start + std::chrono::seconds(5);
        while (std::chrono::high_resolution_clock::now() < deadline) {
            counter++;
        }

        // Medir CPU%
        double cpu = read_cpu_from_cgroup();
        cpu_percents.push_back(cpu);
        throughputs.push_back(counter / 5.0);

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Estatísticas
    r.mean_cpu_percent = std::accumulate(cpu_percents.begin(), cpu_percents.end(), 0.0) / iterations;

    double var = 0;
    for (double x : cpu_percents) {
        var += (x - r.mean_cpu_percent) * (x - r.mean_cpu_percent);
    }
    r.std_dev = std::sqrt(var / iterations);

    r.mean_throughput = std::accumulate(throughputs.begin(), throughputs.end(), 0.0) / iterations;

    return r;
}

int main() {
    std::vector<Result> results;
    
    std::cout << "Iniciando Experimento 3 - Throttling de CPU\n";
    results.push_back(test_limit(0.25));
    results.push_back(test_limit(0.5));
    results.push_back(test_limit(1.0));
    results.push_back(test_limit(2.0));

    // CSV
    std::ofstream csv("experimento3_results.csv");
    csv << "limite_cores,media_cpu,desvio_padrao,throughput,precisao_pct\n";
    for (auto& r : results) {
        double expected = r.limit_cores * 100.0;
        double precisao = std::abs((expected - r.mean_cpu_percent) / expected) * 100.0;
        csv << r.limit_cores << "," << r.mean_cpu_percent << ","
            << r.std_dev << "," << r.mean_throughput << "," << precisao << "\n";
    }

    // Tabela
    std::cout << "\n=== RESULTADOS EXPERIMENTO 3 ===\n";
    std::cout << "Limite\tCPU% (média±desvio)\tThroughput\tPrecisão\n";
    for (auto& r : results) {
        double expected = r.limit_cores * 100.0;
        double precisao = std::abs((expected - r.mean_cpu_percent) / expected) * 100.0;
        std::cout << r.limit_cores << "\t" << r.mean_cpu_percent << "±" << r.std_dev 
                  << "\t" << r.mean_throughput << "\t" << precisao << "%\n";
    }

    std::cout << "\n Experimento 3 concluído! Resultados salvos em experimento3_results.csv\n";
    return 0;
}