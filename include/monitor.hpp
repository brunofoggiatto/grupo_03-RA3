#ifndef MONITOR_HPP
#define MONITOR_HPP

#include <string>

// ================================
// CÓDIGOS DE ERRO SEMÂNTICOS (ADICIONADOS)
// ================================

#define ERR_PROCESS_NOT_FOUND -2
#define ERR_PERMISSION_DENIED -3
#define ERR_UNKNOWN -4

// ================================
// ESTRUTURAS DE DADOS
// ================================

struct ProcStats {
    // CPU
    long utime, stime;
    int threads;
    long voluntary_ctxt, nonvoluntary_ctxt;
    
    // Memória
    long memory_rss, memory_vsz, memory_swap;
    long minor_faults, major_faults;
    double memory_percent;
    
    // I/O
    long io_read_bytes, io_write_bytes;
    double io_read_rate, io_write_rate;
    
    // Rede
    long net_rx_bytes, net_tx_bytes;
    double net_rx_rate, net_tx_rate;
    int tcp_connections;
};

struct NetworkStats {
    int tcp_connections;
};

// ================================
// DECLARAÇÕES DE FUNÇÕES
// ================================

int get_cpu_usage(int pid, ProcStats& stats);
double calculate_cpu_percent(const ProcStats& prev, const ProcStats& curr, double interval);
int get_memory_usage(int pid, ProcStats& stats);
int get_io_usage(int pid, ProcStats& stats);
void calculate_io_rate(const ProcStats& prev, ProcStats& curr, double interval);
int get_network_usage(ProcStats& stats);
int get_network_usage(int pid, ProcStats& stats);
int get_network_usage(int pid, NetworkStats& stats);
void calculate_network_rate(const ProcStats& prev, ProcStats& curr, double interval);

#endif