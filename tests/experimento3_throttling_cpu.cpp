// ============================================================
// ARQUIVO: tests/experimento3_throttling_cpu.cpp
// DESCRIÇÃO: Experimento 3 - Validar precisão de limites de CPU
// Testa o Control Group Manager (Componente 3)
// 
// OBJETIVO:
// Aplicar limites de CPU em diferentes níveis e medir a precisão
// Validar que o cgroup consegue controlar corretamente o uso
// 
// RESPONSABILIDADE: Aluno 4
// ============================================================

#include <iostream>
#include <fstream>
#include <vector>
#include <numeric>
#include <cmath>
#include <chrono>
#include <thread>
#include <random>

// ================================
// FUNÇÃO SIMULADA: set_cpu_limit
// Propósito: Placeholder para demonstrar como usaria CGroupManager
// Em produção, chamaria cgm.set_cpu_limit(cores)
// ================================
void set_cpu_limit(double cores) {
    std::cout << "Setting CPU limit: " << cores << " cores\n";
    // Implementar usando sua CGroupManager::set_cpu_limit()
}

// ================================
// FUNÇÃO SIMULADA: read_cpu_from_cgroup
// Propósito: Placeholder para ler métrica de cgroup
// Em produção, chamaria cgm.read_cpu_usage()
// ================================
double read_cpu_from_cgroup() {
    // Simulação - substituir pela leitura real do cgroup
    // Em produção: return cgm.read_cpu_usage(cgroup_path);
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.95, 1.05);  // ±5% de variação
    return dis(gen) * 25.0;  // Simula ~25% para 0.25 cores
}

// ================================
// ESTRUTURA: Result
// Propósito: Armazenar resultados de um teste de limite
// ================================
struct Result {
    double limit_cores;       // Limite configurado em cores
    double mean_cpu_percent;  // Percentual médio observado
    double std_dev;           // Desvio padrão (variância)
    double mean_throughput;   // Throughput médio de operações
};

// ================================
// FUNÇÃO: test_limit
// Propósito: Testa um limite de CPU específico
// Parâmetros:
//   cores: limite em cores (ex: 0.25, 0.5, 1.0)
//   iterations: quantas medições fazer (padrão 10)
// Retorno: Estrutura com resultados do teste
// ================================
Result test_limit(double cores, int iterations = 10) {
    Result r;
    r.limit_cores = cores;  // Armazena limite testado

    // Vetores para armazenar medições individuais
    std::vector<double> cpu_percents;  // Cada medição de CPU%
    std::vector<double> throughputs;   // Cada medição de throughput

    // Executa o teste 'iterations' vezes
    for (int i = 0; i < iterations; i++) {
        // ================================
        // APLICAR LIMITE
        // ================================
        // Em produção: cgm.set_cpu_limit("/test_cgroup", cores);
        set_cpu_limit(cores);

        // ================================
        // EXECUTAR WORKLOAD
        // ================================
        // Roda por 5 segundos contando quantas operações consegue fazer
        // Isto simula um processo realizando cálculos
        
        auto start = std::chrono::high_resolution_clock::now();
        unsigned long long counter = 0;  // Contador de operações
        auto deadline = start + std::chrono::seconds(5);  // 5 segundos
        
        // Loop: faz operações enquanto não passou 5 segundos
        while (std::chrono::high_resolution_clock::now() < deadline) {
            counter++;  // Cada iteração = 1 operação fictícia
        }

        // ================================
        // MEDIR CPU%
        // ================================
        // Em produção: double cpu = cgm.read_cpu_usage(cgroup_path);
        double cpu = read_cpu_from_cgroup();
        cpu_percents.push_back(cpu);  // Armazena leitura
        
        // Calcula throughput: quantas operações por segundo
        // Esperado: mais operações = menos throttling
        throughputs.push_back(counter / 5.0);

        // Pausa entre iterações para deixar sistema se recuperar
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // ================================
    // CALCULAR ESTATÍSTICAS
    // ================================
    
    // Média de CPU%
    // Soma todos os percentuais e divide pela quantidade
    r.mean_cpu_percent = std::accumulate(cpu_percents.begin(), cpu_percents.end(), 0.0) / iterations;

    // Desvio padrão
    // Medida de variância: quanto as medições diferem da média
    double var = 0;
    for (double x : cpu_percents) {
        var += (x - r.mean_cpu_percent) * (x - r.mean_cpu_percent);
    }
    r.std_dev = std::sqrt(var / iterations);  // Raiz quadrada da variância

    // Média de throughput
    r.mean_throughput = std::accumulate(throughputs.begin(), throughputs.end(), 0.0) / iterations;

    return r;
}

// ================================
// FUNÇÃO: main
// Propósito: Executa Experimento 3 completo
// Testa 4 limites de CPU diferentes
// ================================
int main() {
    // Vetor para armazenar resultados de todos os testes
    std::vector<Result> results;
    
    // Cabeçalho
    std::cout << "Iniciando Experimento 3 - Throttling de CPU\n";
    
    // ================================
    // EXECUTAR TESTES COM DIFERENTES LIMITES
    // ================================
    // Testa: 0.25 cores (25%), 0.5 cores (50%), 1.0 core (100%), 2.0 cores (200%)
    results.push_back(test_limit(0.25));  // Limite agressivo
    results.push_back(test_limit(0.5));   // Limite moderado
    results.push_back(test_limit(1.0));   // Limite de 1 core
    results.push_back(test_limit(2.0));   // Limite de 2 cores

    // ================================
    // GERAR RELATÓRIO CSV
    // ================================
    // Formato tabular para abrir em Excel ou processar
    std::ofstream csv("experimento3_results.csv");
    // Cabeçalho
    csv << "limite_cores,media_cpu,desvio_padrao,throughput,precisao_pct\n";
    
    // Dados
    for (auto& r : results) {
        // Calcula CPU% esperado (limite_cores * 100)
        double expected = r.limit_cores * 100.0;
        
        // Calcula precisão: |observado - esperado| / esperado * 100
        // Quanto menor, mais preciso (ideal = 0%)
        double precisao = std::abs((expected - r.mean_cpu_percent) / expected) * 100.0;
        
        // Escreve linha do CSV
        csv << r.limit_cores << ","               // Coluna 1: limite em cores
            << r.mean_cpu_percent << ","          // Coluna 2: CPU% médio
            << r.std_dev << ","                   // Coluna 3: desvio padrão
            << r.mean_throughput << ","           // Coluna 4: throughput
            << precisao << "\n";                  // Coluna 5: precisão %
    }
    // Fecha arquivo CSV
    // Resultado: experimento3_results.csv

    // ================================
    // EXIBIR TABELA NO CONSOLE
    // ================================
    // Torna os dados visíveis imediatamente
    std::cout << "\n=== RESULTADOS EXPERIMENTO 3 ===\n";
    std::cout << "Limite\tCPU% (média±desvio)\tThroughput\tPrecisão\n";
    
    for (auto& r : results) {
        double expected = r.limit_cores * 100.0;
        double precisao = std::abs((expected - r.mean_cpu_percent) / expected) * 100.0;
        
        // Imprime cada resultado em formato tabular
        std::cout << r.limit_cores << "\t"              // Limite em cores
                  << r.mean_cpu_percent << "±"          // CPU% ±
                  << r.std_dev                          // Desvio padrão
                  << "\t" << r.mean_throughput          // Throughput
                  << "\t" << precisao << "%\n";         // Precisão %
    }

    // Mensagem final
    std::cout << "\n Experimento 3 concluído! Resultados salvos em experimento3_results.csv\n";
    return 0;
}