#ifndef MONITOR_HPP
#define MONITOR_HPP

#include <cstddef>   // para std::size_t
#include <cstdint>   // para tipos inteiros fixos

// Estrutura de estatísticas do processo
struct ProcStats {
    long utime = 0;                     // Tempo de usuário (CPU)
    long stime = 0;                     // Tempo de sistema (CPU)
    int threads = 0;                    // Número de threads
    long voluntary_ctxt = 0;            // Trocas de contexto voluntárias
    long nonvoluntary_ctxt = 0;         // Trocas de contexto não voluntárias

    long memory_rss = 0;                // Resident Set Size (memória física)
    long memory_vsz = 0;                // Virtual Memory Size (memória virtual)
    long memory_swap = 0;               // Memória em swap
    long minor_faults = 0;              // Page faults menores
    long major_faults = 0;              // Page faults maiores
    double memory_percent = 0.0;        // Percentual de uso de memória
};

// Declarações de função
int get_cpu_usage(int pid, ProcStats& stats);
int get_memory_usage(int pid, ProcStats& stats);
double calculate_cpu_percent(const ProcStats& prev, const ProcStats& curr, double interval);

#endif // MONITOR_HPP
    