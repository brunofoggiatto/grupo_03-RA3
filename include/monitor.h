#ifndef MONITOR_H
#define MONITOR_H

// Estrutura que vai armazenar as informações de um processo
typedef struct {
    double cpu_usage;     // Porcentagem de CPU usada
    long memory_rss;      // Memória física usada (em kB)
    long io_read;         // Bytes lidos pelo processo
    long io_write;        // Bytes escritos pelo processo
} ProcStats;

// Funções que seu módulo vai disponibilizar
int get_cpu_usage(int pid, ProcStats *stats);
int get_memory_usage(int pid, ProcStats *stats);

#endif
