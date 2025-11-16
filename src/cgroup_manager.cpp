#include "cgroup_manager.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>
#include <cstring>

namespace fs = std::filesystem;

// ============================================================
// CONSTRUTOR E MÉTODOS DA CLASSE CGroupMetrics
// ============================================================

CGroupMetrics::CGroupMetrics() 
    : cpu_usage_ns(0), memory_usage_bytes(0), 
      memory_limit_bytes(0), memory_peak_bytes(0),
      memory_failcnt(0), blkio_read_bytes(0), blkio_write_bytes(0),
      cgroup_version("unknown") {}

void CGroupMetrics::print() const {
    std::cout << "=== CGroup Metrics ===" << std::endl;
    std::cout << "Version: " << cgroup_version << std::endl;
    
    // CPU
    std::cout << "CPU Usage: " << cpu_usage_ns / 1e9 << " seconds" << std::endl;
    
    // Memory - Current
    if (memory_usage_bytes > 0) {
        std::cout << "Memory Usage (current): " << memory_usage_bytes / (1024 * 1024) << " MB" << std::endl;
    } else {
        std::cout << "Memory Usage (current): N/A" << std::endl;
    }
    
    // Memory - Peak
    if (memory_peak_bytes > 0) {
        std::cout << "Memory Usage (peak): " << memory_peak_bytes / (1024 * 1024) << " MB" << std::endl;
    } else {
        std::cout << "Memory Usage (peak): N/A" << std::endl;
    }
    
    // Memory - Limit
    if (memory_limit_bytes > 0) {
        std::cout << "Memory Limit: " << memory_limit_bytes / (1024 * 1024) << " MB" << std::endl;
    } else {
        std::cout << "Memory Limit: N/A (unlimited)" << std::endl;
    }
    
    // Memory - Failures
    std::cout << "Memory Allocation Failures: " << memory_failcnt << std::endl;
    
    // BlkIO
    if (blkio_read_bytes > 0 || blkio_write_bytes > 0) {
        std::cout << "BlkIO Read: " << blkio_read_bytes / (1024 * 1024) << " MB" << std::endl;
        std::cout << "BlkIO Write: " << blkio_write_bytes / (1024 * 1024) << " MB" << std::endl;
    } else {
        std::cout << "BlkIO: N/A" << std::endl;
    }
}

// ============================================================
// CONSTRUTOR E MÉTODOS AUXILIARES DA CLASSE CGroupManager
// ============================================================

CGroupManager::CGroupManager() {}

bool CGroupManager::fileExists(const std::string& path) const {
    return fs::exists(path);
}

std::string CGroupManager::detectCGroupVersion() const {
    if (fileExists("/sys/fs/cgroup/cgroup.controllers")) {
        return "v2";
    }
    return "unknown";
}

std::string CGroupManager::readFile(const std::string& path) const {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool CGroupManager::writeFile(const std::string& path, const std::string& content) const {
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }
    file << content;
    file.close();
    return true;
}

long long CGroupManager::parseLong(const std::string& str) const {
    if (str.empty()) return 0;
    try {
        return std::stoll(str);
    } catch (...) {
        return 0;
    }
}

// ============================================================
// LEITURA DE MÉTRICAS DO SISTEMA
// ============================================================

std::unique_ptr<CGroupMetrics> CGroupManager::getSystemMetrics() const {
    auto metrics = std::make_unique<CGroupMetrics>();
    metrics->cgroup_version = detectCGroupVersion();
    
    if (metrics->cgroup_version == "v2") {
        // ========== LEITURA DE CPU ==========
        if (fileExists("/sys/fs/cgroup/cpu.stat")) {
            std::string cpu_stat = readFile("/sys/fs/cgroup/cpu.stat");
            if (!cpu_stat.empty()) {
                size_t pos = cpu_stat.find("usage_usec");
                if (pos != std::string::npos) {
                    std::string usage_line = cpu_stat.substr(pos);
                    size_t start = usage_line.find(' ');
                    size_t end = usage_line.find('\n');
                    if (start != std::string::npos && end != std::string::npos) {
                        std::string usage_str = usage_line.substr(start + 1, end - start - 1);
                        metrics->cpu_usage_ns = parseLong(usage_str) * 1000;
                    }
                }
            }
        }
        
        // ========== LEITURA DE MEMÓRIA ==========
        if (fileExists("/sys/fs/cgroup/memory.stat")) {
            std::string mem_stat = readFile("/sys/fs/cgroup/memory.stat");
            if (!mem_stat.empty()) {
                size_t pos = mem_stat.find("anon");
                if (pos != std::string::npos) {
                    std::string anon_line = mem_stat.substr(pos);
                    size_t start = anon_line.find(' ');
                    size_t end = anon_line.find('\n');
                    if (start != std::string::npos && end != std::string::npos) {
                        std::string anon_str = anon_line.substr(start + 1, end - start - 1);
                        metrics->memory_usage_bytes = parseLong(anon_str);
                    }
                }
            }
        }
        
        // ========== LEITURA DE LIMITE DE MEMÓRIA ==========
        if (fileExists("/sys/fs/cgroup/memory.max")) {
            std::string mem_limit = readFile("/sys/fs/cgroup/memory.max");
            if (!mem_limit.empty() && mem_limit.find("max") == std::string::npos) {
                metrics->memory_limit_bytes = parseLong(mem_limit);
            }
        }
        
        // ========== LEITURA DE PICO DE MEMÓRIA ==========
        if (fileExists("/sys/fs/cgroup/memory.peak")) {
            std::string mem_peak = readFile("/sys/fs/cgroup/memory.peak");
            metrics->memory_peak_bytes = parseLong(mem_peak);
        }
        
        // ========== LEITURA DE FALHAS DE ALOCAÇÃO ==========
        if (fileExists("/sys/fs/cgroup/memory.events")) {
            std::string mem_events = readFile("/sys/fs/cgroup/memory.events");
            size_t pos = mem_events.find("max");
            if (pos != std::string::npos) {
                std::string max_line = mem_events.substr(pos);
                size_t start = max_line.find(' ');
                size_t end = max_line.find('\n');
                if (start != std::string::npos && end != std::string::npos) {
                    std::string max_str = max_line.substr(start + 1, end - start - 1);
                    metrics->memory_failcnt = parseLong(max_str);
                }
            }
        }
        
        // ========== LEITURA DE BlkIO ==========
        if (fileExists("/sys/fs/cgroup/io.stat")) {
            std::string blkio_stat = readFile("/sys/fs/cgroup/io.stat");
            if (!blkio_stat.empty()) {
                std::istringstream iss(blkio_stat);
                std::string line;
                while (std::getline(iss, line)) {
                    size_t rbytes_pos = line.find("rbytes=");
                    size_t wbytes_pos = line.find("wbytes=");
                    
                    if (rbytes_pos != std::string::npos) {
                        std::string rbytes_str = line.substr(rbytes_pos + 7);
                        size_t space = rbytes_str.find(' ');
                        if (space != std::string::npos) {
                            rbytes_str = rbytes_str.substr(0, space);
                        }
                        metrics->blkio_read_bytes += parseLong(rbytes_str);
                    }
                    
                    if (wbytes_pos != std::string::npos) {
                        std::string wbytes_str = line.substr(wbytes_pos + 7);
                        size_t space = wbytes_str.find(' ');
                        if (space != std::string::npos) {
                            wbytes_str = wbytes_str.substr(0, space);
                        }
                        metrics->blkio_write_bytes += parseLong(wbytes_str);
                    }
                }
            }
        }
    }
    
    return metrics;
}

// ============================================================
// LEITURA DE MÉTRICAS DE UM CGROUP ESPECÍFICO
// ============================================================

std::unique_ptr<CGroupMetrics> CGroupManager::getCGroupMetrics(const std::string& cgroup_path) const {
    auto metrics = std::make_unique<CGroupMetrics>();
    metrics->cgroup_version = detectCGroupVersion();
    
    if (metrics->cgroup_version == "v2") {
        // ========== CPU ==========
        std::string cpu_file = cgroup_path + "/cpu.stat";
        if (fileExists(cpu_file)) {
            std::string cpu_stat = readFile(cpu_file);
            if (!cpu_stat.empty()) {
                size_t pos = cpu_stat.find("usage_usec");
                if (pos != std::string::npos) {
                    std::string usage_line = cpu_stat.substr(pos);
                    size_t start = usage_line.find(' ');
                    size_t end = usage_line.find('\n');
                    if (start != std::string::npos && end != std::string::npos) {
                        std::string usage_str = usage_line.substr(start + 1, end - start - 1);
                        metrics->cpu_usage_ns = parseLong(usage_str) * 1000;
                    }
                }
            }
        }
        
        // ========== MEMÓRIA ==========
        std::string mem_stat_file = cgroup_path + "/memory.stat";
        if (fileExists(mem_stat_file)) {
            std::string mem_stat = readFile(mem_stat_file);
            if (!mem_stat.empty()) {
                size_t pos = mem_stat.find("anon");
                if (pos != std::string::npos) {
                    std::string anon_line = mem_stat.substr(pos);
                    size_t start = anon_line.find(' ');
                    size_t end = anon_line.find('\n');
                    if (start != std::string::npos && end != std::string::npos) {
                        std::string anon_str = anon_line.substr(start + 1, end - start - 1);
                        metrics->memory_usage_bytes = parseLong(anon_str);
                    }
                }
            }
        }
        
        // ========== LIMITE DE MEMÓRIA ==========
        std::string mem_limit_file = cgroup_path + "/memory.max";
        if (fileExists(mem_limit_file)) {
            std::string mem_limit = readFile(mem_limit_file);
            if (!mem_limit.empty() && mem_limit.find("max") == std::string::npos) {
                metrics->memory_limit_bytes = parseLong(mem_limit);
            }
        }
        
        // ========== PICO DE MEMÓRIA ==========
        std::string mem_peak_file = cgroup_path + "/memory.peak";
        if (fileExists(mem_peak_file)) {
            std::string mem_peak = readFile(mem_peak_file);
            metrics->memory_peak_bytes = parseLong(mem_peak);
        }
        
        // ========== FALHAS DE ALOCAÇÃO ==========
        std::string mem_events_file = cgroup_path + "/memory.events";
        if (fileExists(mem_events_file)) {
            std::string mem_events = readFile(mem_events_file);
            size_t pos = mem_events.find("max");
            if (pos != std::string::npos) {
                std::string max_line = mem_events.substr(pos);
                size_t start = max_line.find(' ');
                size_t end = max_line.find('\n');
                if (start != std::string::npos && end != std::string::npos) {
                    std::string max_str = max_line.substr(start + 1, end - start - 1);
                    metrics->memory_failcnt = parseLong(max_str);
                }
            }
        }
        
        // ========== BlkIO ==========
        std::string blkio_stat_file = cgroup_path + "/io.stat";
        if (fileExists(blkio_stat_file)) {
            std::string blkio_stat = readFile(blkio_stat_file);
            if (!blkio_stat.empty()) {
                std::istringstream iss(blkio_stat);
                std::string line;
                while (std::getline(iss, line)) {
                    size_t rbytes_pos = line.find("rbytes=");
                    size_t wbytes_pos = line.find("wbytes=");
                    
                    if (rbytes_pos != std::string::npos) {
                        std::string rbytes_str = line.substr(rbytes_pos + 7);
                        size_t space = rbytes_str.find(' ');
                        if (space != std::string::npos) {
                            rbytes_str = rbytes_str.substr(0, space);
                        }
                        metrics->blkio_read_bytes += parseLong(rbytes_str);
                    }
                    
                    if (wbytes_pos != std::string::npos) {
                        std::string wbytes_str = line.substr(wbytes_pos + 7);
                        size_t space = wbytes_str.find(' ');
                        if (space != std::string::npos) {
                            wbytes_str = wbytes_str.substr(0, space);
                        }
                        metrics->blkio_write_bytes += parseLong(wbytes_str);
                    }
                }
            }
        }
    }
    
    return metrics;
}

// ============================================================
// CRIAÇÃO E CONFIGURAÇÃO DE CGROUPS
// ============================================================

bool CGroupManager::createCGroup(const std::string& cgroup_name) const {
    std::string cgroup_path = "/sys/fs/cgroup/" + cgroup_name;
    
    if (fileExists(cgroup_path)) {
        std::cout << "CGroup '" << cgroup_name << "' já existe" << std::endl;
        return true;
    }
    
    if (mkdir(cgroup_path.c_str(), 0755) != 0) {
        std::cerr << "Erro ao criar cgroup: " << strerror(errno) << std::endl;
        return false;
    }
    
    std::cout << "CGroup '" << cgroup_name << "' criado com sucesso em " << cgroup_path << std::endl;
    return true;
}

bool CGroupManager::deleteCGroup(const std::string& cgroup_name) const {
    std::string cgroup_path = "/sys/fs/cgroup/" + cgroup_name;
    
    if (!fileExists(cgroup_path)) {
        std::cerr << "CGroup '" << cgroup_name << "' não existe" << std::endl;
        return false;
    }
    
    if (rmdir(cgroup_path.c_str()) != 0) {
        std::cerr << "Erro ao deletar cgroup: " << strerror(errno) << std::endl;
        return false;
    }
    
    std::cout << "CGroup '" << cgroup_name << "' deletado" << std::endl;
    return true;
}

bool CGroupManager::moveProcessToCGroup(const std::string& cgroup_name, int pid) const {
    std::string cgroup_procs_path = "/sys/fs/cgroup/" + cgroup_name + "/cgroup.procs";
    
    if (!fileExists(cgroup_procs_path)) {
        std::cerr << "CGroup '" << cgroup_name << "' não existe" << std::endl;
        return false;
    }
    
    if (!writeFile(cgroup_procs_path, std::to_string(pid))) {
        std::cerr << "Erro ao mover processo " << pid << " para cgroup '" << cgroup_name << "'" << std::endl;
        return false;
    }
    
    std::cout << "Processo " << pid << " movido para cgroup '" << cgroup_name << "'" << std::endl;
    return true;
}

// ============================================================
// APLICAR LIMITES
// ============================================================

bool CGroupManager::setCPULimit(const std::string& cgroup_name, double cores) const {
    std::string cpu_max_path = "/sys/fs/cgroup/" + cgroup_name + "/cpu.max";
    
    if (!fileExists(cpu_max_path)) {
        std::cerr << "CGroup '" << cgroup_name << "' não existe ou não suporta CPU controller" << std::endl;
        return false;
    }
    
    long long period = 100000;
    long long quota = static_cast<long long>(cores * period);
    
    std::string limit = std::to_string(quota) + " " + std::to_string(period);
    
    if (!writeFile(cpu_max_path, limit)) {
        std::cerr << "Erro ao aplicar limite de CPU" << std::endl;
        return false;
    }
    
    std::cout << "Limite de CPU aplicado: " << cores << " cores (" << quota << "/" << period << ")" << std::endl;
    return true;
}

bool CGroupManager::setMemoryLimit(const std::string& cgroup_name, long long bytes) const {
    std::string memory_max_path = "/sys/fs/cgroup/" + cgroup_name + "/memory.max";
    
    if (!fileExists(memory_max_path)) {
        std::cerr << "CGroup '" << cgroup_name << "' não existe ou não suporta Memory controller" << std::endl;
        return false;
    }
    
    if (!writeFile(memory_max_path, std::to_string(bytes))) {
        std::cerr << "Erro ao aplicar limite de memória" << std::endl;
        return false;
    }
    
    std::cout << "Limite de memória aplicado: " << bytes / (1024*1024) << " MB" << std::endl;
    return true;
}

bool CGroupManager::setIOLimit(const std::string& cgroup_name, const std::string& device, long long bps) const {
    std::string io_max_path = "/sys/fs/cgroup/" + cgroup_name + "/io.max";
    
    if (!fileExists(io_max_path)) {
        std::cerr << "CGroup '" << cgroup_name << "' não existe ou não suporta IO controller" << std::endl;
        return false;
    }
    
    std::string limit = device + " wbps=" + std::to_string(bps);
    
    if (!writeFile(io_max_path, limit)) {
        std::cerr << "Erro ao aplicar limite de I/O" << std::endl;
        return false;
    }
    
    std::cout << "Limite de I/O aplicado: " << device << " -> " << bps / (1024*1024) << " MB/s" << std::endl;
    return true;
}

// ============================================================
// RELATÓRIO DE UTILIZAÇÃO
// ============================================================

bool CGroupManager::generateUtilizationReport(const std::string& cgroup_name, const std::string& output_file) const {
    auto metrics = getCGroupMetrics("/sys/fs/cgroup/" + cgroup_name);
    
    if (!metrics) {
        std::cerr << "Erro ao ler métricas do cgroup '" << cgroup_name << "'" << std::endl;
        return false;
    }
    
    std::ofstream report(output_file);
    if (!report.is_open()) {
        std::cerr << "Erro ao criar arquivo de relatório: " << output_file << std::endl;
        return false;
    }
    
    report << "=== RELATÓRIO DE UTILIZAÇÃO - CGroup: " << cgroup_name << " ===" << std::endl;
    report << "Versão do CGroup: " << metrics->cgroup_version << std::endl;
    report << std::endl;
    
    report << "CPU Usage: " << metrics->cpu_usage_ns / 1e9 << " seconds" << std::endl;
    report << std::endl;
    
    report << "Memory Usage (current): " << metrics->memory_usage_bytes / (1024*1024) << " MB" << std::endl;
    report << "Memory Usage (peak): " << metrics->memory_peak_bytes / (1024*1024) << " MB" << std::endl;
    report << "Memory Limit: ";
    if (metrics->memory_limit_bytes > 0) {
        report << metrics->memory_limit_bytes / (1024*1024) << " MB" << std::endl;
    } else {
        report << "Unlimited" << std::endl;
    }
    report << "Memory Failures: " << metrics->memory_failcnt << std::endl;
    report << std::endl;
    
    report << "BlkIO Read: " << metrics->blkio_read_bytes / (1024*1024) << " MB" << std::endl;
    report << "BlkIO Write: " << metrics->blkio_write_bytes / (1024*1024) << " MB" << std::endl;
    
    report.close();
    
    std::cout << "Relatório salvo em: " << output_file << std::endl;
    return true;
}