#ifndef CGROUP_MANAGER_HPP
#define CGROUP_MANAGER_HPP

#include <string>
#include <memory>
#include <iostream>

class CGroupMetrics {
public:
    // CPU
    long long cpu_usage_ns;
    
    // Memory - Current Usage
    long long memory_usage_bytes;
    long long memory_limit_bytes;
    
    // Memory - Peak Usage
    long long memory_peak_bytes;
    
    // Memory - Failures
    long long memory_failcnt;
    
    // BlkIO - NOVO
    long long blkio_read_bytes;
    long long blkio_write_bytes;
    
    // Version
    std::string cgroup_version;
    
    CGroupMetrics();
    void print() const;
};

class CGroupManager {
private:
    std::string detectCGroupVersion() const;
    std::string readFile(const std::string& path) const;
    bool writeFile(const std::string& path, const std::string& content) const;
    long long parseLong(const std::string& str) const;
    bool fileExists(const std::string& path) const;

public:
    CGroupManager();
    
    // === LEITURA DE MÉTRICAS ===
    std::unique_ptr<CGroupMetrics> getSystemMetrics() const;
    std::unique_ptr<CGroupMetrics> getCGroupMetrics(const std::string& cgroup_path) const;
    
    // === CRIAÇÃO E CONFIGURAÇÃO (NOVAS FUNÇÕES) ===
    bool createCGroup(const std::string& cgroup_name) const;
    bool deleteCGroup(const std::string& cgroup_name) const;
    bool moveProcessToCGroup(const std::string& cgroup_name, int pid) const;
    
    // === APLICAR LIMITES ===
    bool setCPULimit(const std::string& cgroup_name, double cores) const;
    bool setMemoryLimit(const std::string& cgroup_name, long long bytes) const;
    bool setIOLimit(const std::string& cgroup_name, const std::string& device, long long bps) const;
    
    // === RELATÓRIOS ===
    bool generateUtilizationReport(const std::string& cgroup_name, const std::string& output_file) const;
};

#endif