#ifndef CGROUP_MANAGER_HPP
#define CGROUP_MANAGER_HPP

#include <string>
#include <vector>
#include <iostream>

struct PressureStats {
    double avg10;
    double avg60;
    double avg300;
    unsigned long long total;
};

class CGroupManager {
private:
    std::string base_path;

public:
    CGroupManager();
    ~CGroupManager();

    // Configuração base
    void set_base_path(const std::string& path);
    std::string get_base_path() const;

    // Gerenciamento básico de cgroups
    bool create_cgroup(const std::string& cgroup_path);
    bool delete_cgroup(const std::string& cgroup_path);
    bool exists_cgroup(const std::string& cgroup_path);
    
    // Gerenciamento de processos
    bool move_process_to_cgroup(int pid, const std::string& cgroup_path);
    bool move_current_process_to_cgroup(const std::string& cgroup_path);

    // Controles de CPU
    bool set_cpu_limit(const std::string& cgroup_path, double cores);
    double read_cpu_usage(const std::string& cgroup_path);
    bool set_cpu_quota(const std::string& cgroup_path, int quota_us, int period_us = 100000);

    // Controles de Memória
    bool set_memory_limit(const std::string& cgroup_path, size_t limit_mb);
    bool set_memory_swap_limit(const std::string& cgroup_path, size_t limit_mb);
    size_t read_memory_usage(const std::string& cgroup_path);
    size_t read_memory_max_usage(const std::string& cgroup_path);
    int read_memory_failcnt(const std::string& cgroup_path);
    bool trigger_oom(const std::string& cgroup_path); // Para testes

    // Controles de I/O
    bool set_io_limit(const std::string& cgroup_path, const std::string& device, 
                     long read_bps, long write_bps);
    bool set_io_weight(const std::string& cgroup_path, int weight);

    // **NOVO: Controles de PIDs**
    bool set_pids_limit(const std::string& cgroup_path, int max_pids);
    int read_pids_current(const std::string& cgroup_path);
    int read_pids_max(const std::string& cgroup_path);

    // **NOVO: CGroup v2 Pressure Stall Information**
    PressureStats read_cpu_pressure(const std::string& cgroup_path);
    PressureStats read_memory_pressure(const std::string& cgroup_path);
    PressureStats read_io_pressure(const std::string& cgroup_path);
    void print_pressure(const PressureStats& p, const std::string& type = "CPU");

    // Monitoramento e relatórios
    void display_cgroup_stats(const std::string& cgroup_path);
    void generate_utilization_report(const std::string& cgroup_path, const std::string& filename = "cgroup_report.txt");
    std::vector<std::string> list_processes_in_cgroup(const std::string& cgroup_path);

    // Utilitários
    static bool is_cgroup_v2();
    static std::string get_current_cgroup(int pid = 0);
    
    // Testes e debugging
    bool run_pid_limit_test(const std::string& test_cgroup = "/test_pid_limit", int max_pids = 5);
};

#endif