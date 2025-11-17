#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <cstring>

// Simulações - integrar com sua CGroupManager
void create_cgroup_with_limit(size_t limit_mb) {
    std::cout << "Creating cgroup with memory limit: " << limit_mb << " MB\n";
}

int read_memory_failcnt() {
    // Simulação - substituir pela leitura real
    static int fail_count = 0;
    return fail_count++;
}

void cleanup_cgroup() {
    std::cout << "Cleaning up cgroup\n";
}

struct MemResult {
    size_t limit_mb;
    size_t allocated_mb;
    int oom_count;
    int failcnt;
};

MemResult test_mem_limit(size_t limit_mb, int iterations = 10) {
    MemResult r = {limit_mb, 0, 0, 0};

    for (int i = 0; i < iterations; i++) {
        // Criar cgroup, aplicar limite
        create_cgroup_with_limit(limit_mb);

        // Tentar alocar
        size_t allocated = 0;
        std::vector<void*> allocations;
        
        while (allocated < limit_mb + 50) {
            void* ptr = malloc(1024*1024); // 1MB
            if (!ptr) {
                r.oom_count++;
                break;
            }
            memset(ptr, 0, 1024*1024); // Touch
            allocations.push_back(ptr);
            allocated++;
        }

        r.allocated_mb += allocated;
        r.failcnt += read_memory_failcnt();

        // Cleanup
        for (void* ptr : allocations) {
            free(ptr);
        }
        cleanup_cgroup();
    }

    r.allocated_mb /= iterations;
    r.failcnt /= iterations;
    r.oom_count /= iterations;

    return r;
}

int main() {
    std::vector<MemResult> results;
    
    std::cout << "Iniciando Experimento 4 - Limitação de Memória\n";
    results.push_back(test_mem_limit(50));
    results.push_back(test_mem_limit(100));
    results.push_back(test_mem_limit(200));
    results.push_back(test_mem_limit(500));

    // CSV
    std::ofstream csv("experimento4_results.csv");
    csv << "limite_mb,alocado_mb,oom_count,failcnt\n";
    for (auto& r : results) {
        csv << r.limit_mb << "," << r.allocated_mb << "," 
            << r.oom_count << "," << r.failcnt << "\n";
    }

    // Tabela
    std::cout << "\n=== RESULTADOS EXPERIMENTO 4 ===\n";
    std::cout << "Limite\tAlocado\tOOM\tFailcnt\n";
    for (auto& r : results) {
        std::cout << r.limit_mb << " MB\t" << r.allocated_mb << " MB\t" 
                  << r.oom_count << "\t" << r.failcnt << "\n";
    }

    std::cout << "\n Experimento 4 concluído! Resultados salvos em experimento4_results.csv\n";
    return 0;
}