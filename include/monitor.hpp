#ifndef MONITOR_HPP
#define MONITOR_HPP

#include <cstddef>   // std::size_t
#include <cstdint>   // tipos fixos (int64_t etc.)
#include <vector>
#include <string>

// Estrutura principal com todas as métricas do processo
struct ProcStats {
    // --- CPU ---
    long utime = 0;
    long stime = 0;
    int threads = 0;
    long voluntary_ctxt = 0;
    long nonvoluntary_ctxt = 0;

    // --- Memória ---
    long memory_rss = 0;
    long memory_vsz = 0;
    long memory_swap = 0;
    long minor_faults = 0;
    long major_faults = 0;
    double memory_percent = 0.0;

    // --- I/O ---  (ADICIONADO PELO ALUNO 2)
    long io_read_bytes = 0;
    long io_write_bytes = 0;
    double io_read_rate = 0.0;
    double io_write_rate = 0.0;

    // --- Network ---  (ADICIONADO PELO ALUNO 2)
    long net_rx_bytes = 0;
    long net_tx_bytes = 0;
    double net_rx_rate = 0.0;
    double net_tx_rate = 0.0;
};

// --- CPU ---
int get_cpu_usage(int pid, ProcStats& stats);
double calculate_cpu_percent(const ProcStats& prev, const ProcStats& curr, double interval);

// --- Memória ---
int get_memory_usage(int pid, ProcStats& stats);

// --- I/O ---  (ADICIONADO PELO ALUNO 2)
int get_io_usage(int pid, ProcStats& stats);
void calculate_io_rate(const ProcStats& prev, ProcStats& curr, double interval);

// --- Network ---  (ADICIONADO PELO ALUNO 2)
int get_network_usage(ProcStats& stats);
void calculate_network_rate(const ProcStats& prev, ProcStats& curr, double interval);

struct MetricsSnapshot {
    std::string timestamp;
    int pid;
    double cpu_percent;
    long memory_rss;
    long memory_vsz;
    long io_read_bytes;
    long io_write_bytes;
    double io_read_rate;
    double io_write_rate;
    long net_rx_bytes;
    long net_tx_bytes;
    int threads;
};

bool export_to_csv(const std::vector<MetricsSnapshot>& snapshots, const std::string& filename);
bool export_to_json(const std::vector<MetricsSnapshot>& snapshots, const std::string& filename);


#endif
