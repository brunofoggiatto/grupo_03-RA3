// ============================================================
// ARQUIVO: include/monitor.hpp
// DESCRIÇÃO: Headers do Resource Profiler (Componente 1)
// Contém definições de estruturas de dados e funções para
// monitoramento de CPU, memória, I/O e rede de processos
// ============================================================

#ifndef MONITOR_HPP
#define MONITOR_HPP

#include <string>

// Processo com PID não existe em /proc
#define ERR_PROCESS_NOT_FOUND -2

// Sem permissão para acessar o processo (outro usuário)
#define ERR_PERMISSION_DENIED -3

// Outro erro (arquivo inacessível, permissões, etc)
#define ERR_UNKNOWN -4


// ProcStats: captura todas as métricas de um processo
// Usada para armazenar leituras de CPU, memória, I/O e rede
struct ProcStats {
    
    // Tempo em modo usuário (ticks do kernel)
    // Incrementa quando processo executa em espaço do usuário
    long utime;
    
    // Tempo em modo sistema (ticks do kernel)
    // Incrementa quando processo executa chamadas de sistema (syscalls)
    long stime;
    
    // Número de threads ativas do processo
    // Conta threads leves (lightweight processes)
    int threads;
    
    // Context switches voluntários (processo dormiu/esperou)
    // Processo cedeu CPU voluntariamente
    long voluntary_ctxt;
    
    // Context switches involuntários (escalonador trocou)
    // Kernel forçou troca de contexto (timeout de timeslice)
    long nonvoluntary_ctxt;
    
    // Resident Set Size (memória física em KB)
    // Quantidade de RAM que o processo está realmente usando
    // Páginas que estão na RAM (não em SWAP)
    long memory_rss;
    
    // Virtual Set Size (memória virtual em KB)
    // Total de endereçamento de memória que o processo alocou
    // Inclui memória não-residente (em SWAP ou não-alocada)
    long memory_vsz;
    
    // Memória SWAP utilizada em KB
    // Dados que foram movidos do RAM para o disco (SWAP)
    // Indica swapping (desempenho ruim)
    long memory_swap;
    
    // Page faults menores (sem I/O de disco)
    long minor_faults;
    
    // Page faults maiores (com I/O de disco)
    long major_faults;
    
    // Percentual de uso em relação à RAM total do sistema
    double memory_percent;

    // Total acumulado de bytes lidos do disco
    // Contabiliza I/O real de disco, não dados em cache
    long io_read_bytes;
    
    // Total acumulado de bytes escritos no disco
    // Contabiliza I/O real de disco, não dados em cache
    long io_write_bytes;
    
    // Taxa de leitura calculada (B/s)
    double io_read_rate;
    
    // Taxa de escrita calculada (B/s)
    double io_write_rate;

    // Bytes recebidos pelo sistema (todas as interfaces)
    // Inclui tráfego de todas as placas de rede exceto loopback
    long net_rx_bytes;
    
    // Bytes transmitidos pelo sistema (todas as interfaces)
    // Inclui tráfego de todas as placas de rede exceto loopback
    long net_tx_bytes;
    
    // Taxa de recepção calculada (B/s)
    double net_rx_rate;
    
    // Taxa de transmissão calculada (B/s)
    double net_tx_rate;
    
    // Número de conexões TCP ativas do processo
    // Contabiliza portas abertas
    int tcp_connections;
};

// NetworkStats: apenas estatísticas de rede de um processo
// Estrutura mais leve quando só se quer rede
struct NetworkStats {
    // Número de conexões TCP ativas do processo
    int tcp_connections;
};
// Coleta dados de CPU do arquivo /proc/[pid]/stat
int get_cpu_usage(int pid, ProcStats& stats);

// Calcula o percentual de CPU entre duas leituras consecutivas
// Normaliza automaticamente pelo número de núcleos do sistema
double calculate_cpu_percent(const ProcStats& prev, const ProcStats& curr, double interval);

// Coleta dados de memória do arquivo /proc/[pid]/status
int get_memory_usage(int pid, ProcStats& stats);

// Coleta dados de I/O do arquivo /proc/[pid]/io
int get_io_usage(int pid, ProcStats& stats);

// Calcula as taxas de I/O entre duas leituras
// Atualiza io_read_rate e io_write_rate em stats
void calculate_io_rate(const ProcStats& prev, ProcStats& curr, double interval);

// Coleta dados de rede do sistema todo (todas as interfaces)
// Lê de /proc/net/dev
int get_network_usage(ProcStats& stats);

// Coleta dados de rede de um processo específico (conexões TCP)
// Lê de /proc/net/tcp e /proc/[pid]/fd/
int get_network_usage(int pid, ProcStats& stats);

// Coleta dados de rede em estrutura separada
int get_network_usage(int pid, NetworkStats& stats);

// Calcula as taxas de rede entre duas leituras
// Atualiza net_rx_rate e net_tx_rate em stats
void calculate_network_rate(const ProcStats& prev, ProcStats& curr, double interval);

#endif