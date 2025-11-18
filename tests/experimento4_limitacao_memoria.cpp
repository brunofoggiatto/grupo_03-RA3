// ============================================================
// ARQUIVO: tests/experimento4_limitacao_memoria.cpp
// DESCRIÇÃO: Experimento 4 - Validar precisão de limites de memória
// Testa o Control Group Manager (Componente 3)
// 
// OBJETIVO:
// Aplicar limites de memória e medir comportamento quando atingido
// Validar que o cgroup consegue controlar alocações
// 
// RESPONSABILIDADE: Aluno 4
// ============================================================

// INCLUDES NECESSÁRIOS ADICIONADOS
#include <iostream>
#include <vector>
#include <cstddef>      // Para size_t
#include <cstdlib>      // Para malloc, free
#include <cstring>      // Para memset
#include <fstream>      // Para ofstream

using namespace std;

// ================================
// FUNÇÃO SIMULADA: create_cgroup_with_limit
// Propósito: Placeholder para criar cgroup com limite
// ================================
void create_cgroup_with_limit(size_t limit_mb) {
    cout << "Creating cgroup with memory limit: " << limit_mb << " MB\n";
    // Em produção: cgm.create_cgroup(path); cgm.set_memory_limit(path, limit_mb);
}

// ================================
// FUNÇÃO SIMULADA: read_memory_failcnt
// Propósito: Placeholder para ler contador de falhas
// ================================
int read_memory_failcnt() {
    // Simulação - substituir pela leitura real
    // Em produção: return cgm.read_memory_failcnt(cgroup_path);
    static int fail_count = 0;
    return fail_count++;
}

// ================================
// FUNÇÃO SIMULADA: cleanup_cgroup
// Propósito: Placeholder para limpar cgroup
// ================================
void cleanup_cgroup() {
    cout << "Cleaning up cgroup\n";
    // Em produção: cgm.delete_cgroup(path);
}

// ================================
// ESTRUTURA: MemResult
// Propósito: Armazenar resultados de teste de limite de memória
// ================================
struct MemResult {
    size_t limit_mb;         // Limite configurado em MB
    size_t allocated_mb;     // Memória que conseguiu alocar
    int oom_count;           // Quantas vezes OOM Killer atuou
    int failcnt;             // Contador de falhas de alocação
};

// ================================
// FUNÇÃO: test_mem_limit
// Propósito: Testa um limite de memória específico
// Parâmetros:
//   limit_mb: limite em megabytes (ex: 50, 100, 200)
//   iterations: quantas tentativas fazer
// Retorno: Estrutura com resultados
// ================================
MemResult test_mem_limit(size_t limit_mb, int iterations = 10) {
    MemResult r = {limit_mb, 0, 0, 0};  // Inicializa resultado

    // Executa 'iterations' vezes
    for (int i = 0; i < iterations; i++) {
        // ================================
        // CRIAR CGROUP COM LIMITE
        // ================================
        // Em produção:
        // cgm.create_cgroup("/test_mem");
        // cgm.set_memory_limit("/test_mem", limit_mb);
        create_cgroup_with_limit(limit_mb);

        // ================================
        // TENTAR ALOCAR MEMÓRIA
        // ================================
        // Aloca blocos de 1MB até atingir limite ou falha
        
        size_t allocated = 0;  // Memória alocada (em MB)
        vector<void*> allocations;  // Ponteiros dos blocos
        
        // Loop: aloca enquanto consegue e até limite + 50MB
        while (allocated < limit_mb + 50) {
            // Tenta alocar 1MB (1024 * 1024 bytes)
            void* ptr = malloc(1024*1024);
            
            if (!ptr) {
                // Falha de alocação (limite atingido)
                r.oom_count++;
                break;
            }
            
            // "Toca" na memória para garantir que seja alocada
            // (sem isto, Linux faz copy-on-write e não aloca)
            memset(ptr, 0, 1024*1024);
            
            // Armazena ponteiro para liberar depois
            allocations.push_back(ptr);
            allocated++;
        }

        // Acumula quantidade alocada
        r.allocated_mb += allocated;
        
        // Lê contador de falhas do cgroup
        r.failcnt += read_memory_failcnt();

        // ================================
        // LIBERAR MEMÓRIA E LIMPAR
        // ================================
        // Libera todos os blocos alocados
        for (void* ptr : allocations) {
            free(ptr);
        }
        
        // Em produção: cgm.delete_cgroup("/test_mem");
        cleanup_cgroup();
    }

    // ================================
    // CALCULAR MÉDIAS
    // ================================
    // Divide por número de iterações para obter média
    r.allocated_mb /= iterations;
    r.failcnt /= iterations;
    r.oom_count /= iterations;

    return r;
}

// ================================
// FUNÇÃO: main
// Propósito: Executa Experimento 4 completo
// Testa 4 limites de memória diferentes
// ================================
int main() {
    // Vetor para armazenar resultados
    vector<MemResult> results;
    
    // Cabeçalho
    cout << "Iniciando Experimento 4 - Limitação de Memória\n";
    
    // ================================
    // EXECUTAR TESTES COM DIFERENTES LIMITES
    // ================================
    // Testa: 50MB, 100MB, 200MB, 500MB
    results.push_back(test_mem_limit(50));    // Limite pequeno
    results.push_back(test_mem_limit(100));   // Limite médio
    results.push_back(test_mem_limit(200));   // Limite médio-grande
    results.push_back(test_mem_limit(500));   // Limite grande

    // ================================
    // GERAR RELATÓRIO CSV
    // ================================
    ofstream csv("experimento4_results.csv");
    csv << "limite_mb,alocado_mb,oom_count,failcnt\n";  // Cabeçalho
    
    for (auto& r : results) {
        csv << r.limit_mb << ","        // Coluna 1: limite em MB
            << r.allocated_mb << ","    // Coluna 2: memória alocada
            << r.oom_count << ","       // Coluna 3: OOM kills
            << r.failcnt << "\n";       // Coluna 4: falhas
    }
    // Resultado: experimento4_results.csv

    // ================================
    // EXIBIR TABELA NO CONSOLE
    // ================================
    cout << "\n=== RESULTADOS EXPERIMENTO 4 ===\n";
    cout << "Limite\tAlocado\tOOM\tFailcnt\n";
    
    for (auto& r : results) {
        cout << r.limit_mb << " MB\t"  // Limite
             << r.allocated_mb << " MB\t"  // Alocado
             << r.oom_count << "\t"        // OOM kills
             << r.failcnt << "\n";         // Falhas
    }

    // Mensagem final
    cout << "\n Experimento 4 concluído! Resultados salvos em experimento4_results.csv\n";
    return 0;
}