// ============================================================
// ARQUIVO: include/cgroup_manager.hpp
// DESCRIÇÃO: Headers do Control Group Manager (Componente 3)
// Gerencia criação, configuração e leitura de métricas de cgroups
// Suporta tanto CGroup v1 (legado) quanto v2 (unificado)
// ============================================================

#ifndef CGROUP_MANAGER_HPP
#define CGROUP_MANAGER_HPP

#include <string>           // Para std::string
#include <vector>           // Para std::vector
#include <iostream>         // Para std::cout, std::cerr

// PressureStats: métricas de contenção de recursos (CGroup v2 exclusively)
// Indica quanto tempo processos tiveram que esperar por um recurso
struct PressureStats {
    // Pressão média nos últimos 10 segundos (em %)
    double avg10;
    
    // Pressão média nos últimos 60 segundos (em %)
    double avg60;
    
    // Pressão média nos últimos 5 minutos (em %)
    double avg300;
    
    // Tempo total de pressão em microsegundos
    // Acumulado desde que cgroup foi criado
    // Útil para histograma de contenção
    unsigned long long total;
};

class CGroupManager {
private:
    // Caminho base dos cgroups no filesystem
    // Típicamente: /sys/fs/cgroup ou /sys/fs/cgroup/unified
    std::string base_path;

public:
    
    // Construtor: inicializa o gerenciador
    // Detecta automaticamente se é CGroup v1 ou v2
    // Define o base_path apropriado
    CGroupManager();
    
    // Destrutor: liberação de recursos (se houver)
    ~CGroupManager();


    // Define o caminho base dos cgroups (customização)
    // Util se cgroups estão em local não-padrão
    // Parâmetros:
    //   path: novo caminho base (ex: "/custom/cgroup/path")
    void set_base_path(const std::string& path);

    // Retorna o caminho base atual dos cgroups
    // Retorna: caminho base configurado
    std::string get_base_path() const;


    // Cria um novo cgroup (cria um diretório)
    // O cgroup é criado no filesystem mas sem processos inicialmente
    bool create_cgroup(const std::string& cgroup_path);

    // Deleta um cgroup (remove o diretório)
    // Antes de deletar, move todos os processos de volta para raiz
    // Diretório deve estar vazio para rmdir() funcionar
    bool delete_cgroup(const std::string& cgroup_path);

    // Verifica se um cgroup existe
    // Não tenta criar ou modificar, apenas consulta
    bool exists_cgroup(const std::string& cgroup_path);
    

    // Move um processo para um cgroup específico
    // Escreve PID no arquivo cgroup.procs (v2) ou tasks (v1)
    // O kernel moverá automaticamente o processo
    bool move_process_to_cgroup(int pid, const std::string& cgroup_path);

    // Move o processo atual (getpid()) para um cgroup
    // Versão conveniente para mover a si próprio
    bool move_current_process_to_cgroup(const std::string& cgroup_path);


    // Define um limite de CPU em número de cores
    bool set_cpu_limit(const std::string& cgroup_path, double cores);

    // Lê o uso acumulado de CPU do cgroup
    // Não é uso instantâneo, mas total desde criação do cgroup
    double read_cpu_usage(const std::string& cgroup_path);

    // Define quota de CPU em microsegundos por período
    // Versão granular (alternativa a set_cpu_limit)
    bool set_cpu_quota(const std::string& cgroup_path, int quota_us, int period_us = 100000);

    // Define um limite máximo de memória para o cgroup
    // Processo que tenta alocar acima do limite sofrerá:
    bool set_memory_limit(const std::string& cgroup_path, size_t limit_mb);

    // Define um limite para memória SWAP
    // SWAP é disco usado quando RAM está cheia
    // Limitar SWAP evita que processo use muito disco
    bool set_memory_swap_limit(const std::string& cgroup_path, size_t limit_mb);

    // Lê o uso atual de memória do cgroup
    // Memory actual em uso neste exato momento
    size_t read_memory_usage(const std::string& cgroup_path);

    // Lê o pico máximo de memória utilizada
    // Maior quantidade de memória que foi usada desde criação
    // Útil para dimensionar limite apropriado
    size_t read_memory_max_usage(const std::string& cgroup_path);

    // Lê o número de falhas de alocação de memória
    // Contador de quantas vezes malloc() falhou
    // Indica se limite foi atingido frequentemente
    int read_memory_failcnt(const std::string& cgroup_path);

    // Força uma situação de OOM (Out-of-Memory) para testes
    // Dispara OOM Killer artificialmente
    bool trigger_oom(const std::string& cgroup_path);

   

    // Define limites de I/O em bytes por segundo
    // Throttles (estrangula) velocidade de leitura/escrita em disco
    bool set_io_limit(const std::string& cgroup_path, const std::string& device, 
                     long read_bps, long write_bps);

    // Define peso relativo de I/O entre cgroups
    // Em competição, cgroup com maior peso recebe mais throughput
    bool set_io_weight(const std::string& cgroup_path, int weight);


    // Define um limite máximo de processos (PIDs) no cgroup
    // Quando limite é atingido, fork() falha
    // Útil para evitar fork bombs ou limitar paralelismo

    bool set_pids_limit(const std::string& cgroup_path, int max_pids);

    // Lê o número atual de processos no cgroup
    // Conta quantas tarefas (threads + processos) estão ativas

    int read_pids_current(const std::string& cgroup_path);

    // Lê o limite máximo de processos configurado
    // Compara com read_pids_current() para saber quão próximo do limite

    int read_pids_max(const std::string& cgroup_path);


    // Lê estatísticas de pressão de CPU
    // Quanto tempo processos esperaram para rodar (CPU indisponível)
    PressureStats read_cpu_pressure(const std::string& cgroup_path);

    // Lê estatísticas de pressão de memória
    // Quanto tempo processos esperaram para memória (direct/kswapd reclaim)
    PressureStats read_memory_pressure(const std::string& cgroup_path);

    // Lê estatísticas de pressão de I/O
    // Quanto tempo processos esperaram para I/O (bloqueio em disco)
    PressureStats read_io_pressure(const std::string& cgroup_path);

    // Imprime estatísticas de pressão de forma formatada
    // Mostra avg10, avg60, avg300 e total em console
    void print_pressure(const PressureStats& p, const std::string& type = "CPU");

    // Exibe estatísticas do cgroup no console
    // Mostra CPU, memória, PIDs e pressão (v2)
    void display_cgroup_stats(const std::string& cgroup_path);

    // Gera um relatório detalhado em arquivo de texto
    // Inclui todas as métricas disponíveis
    void generate_utilization_report(const std::string& cgroup_path, const std::string& filename = "cgroup_report.txt");

    // Lista todos os processos no cgroup
    // Lê PIDs de cgroup.procs ou tasks
    std::vector<std::string> list_processes_in_cgroup(const std::string& cgroup_path);

    // Detecta a versão de CGroup no sistema (ESTÁTICO)
    // Verifica existência de /sys/fs/cgroup/cgroup.controllers
    static bool is_cgroup_v2();

    // Obtém o cgroup atual de um processo (ESTÁTICO)
    // Lê de /proc/[pid]/cgroup
    static std::string get_current_cgroup(int pid = 0);
    // Executa um teste de limitação de PIDs
    // Cria cgroup, configura limite, tenta criar processos filhos
    // Limpa cgroup automaticamente após teste
    bool run_pid_limit_test(const std::string& test_cgroup = "/test_pid_limit", int max_pids = 5);
};

#endif