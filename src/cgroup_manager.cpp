#include "cgroup_manager.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

CGroupMetrics::CGroupMetrics() 
    : cpu_usage_ns(0), memory_usage_bytes(0), 
      memory_limit_bytes(0), cgroup_version("unknown") {}

void CGroupMetrics::print() const {
    std::cout << "=== CGroup Metrics ===" << std::endl;
    std::cout << "Version: " << cgroup_version << std::endl;
    std::cout << "CPU Usage: " << cpu_usage_ns / 1e9 << " seconds" << std::endl;
    
    if (memory_usage_bytes > 0) {
        std::cout << "Memory Usage: " << memory_usage_bytes / (1024 * 1024) << " MB" << std::endl;
    } else {
        std::cout << "Memory Usage: N/A" << std::endl;
    }
    
    if (memory_limit_bytes > 0) {
        std::cout << "Memory Limit: " << memory_limit_bytes / (1024 * 1024) << " MB" << std::endl;
    } else {
        std::cout << "Memory Limit: N/A" << std::endl;
    }
}

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

long long CGroupManager::parseLong(const std::string& str) const {
    if (str.empty()) return 0;
    try {
        return std::stoll(str);
    } catch (...) {
        return 0;
    }
}


std::unique_ptr<CGroupMetrics> CGroupManager::getSystemMetrics() const {
    auto metrics = std::make_unique<CGroupMetrics>();
    metrics->cgroup_version = detectCGroupVersion();
    
    if (metrics->cgroup_version == "v2") {
        // BUSCAR CPU
        std::vector<std::string> cpu_paths = {
            "/sys/fs/cgroup/cpu.stat",
            "/sys/fs/cgroup/init.scope/cpu.stat",
            "/sys/fs/cgroup/user.slice/cpu.stat",
            "/sys/fs/cgroup/system.slice/cpu.stat"
        };
        
        for (const auto& path : cpu_paths) {
            if (fileExists(path)) {
                std::string cpu_stat = readFile(path);
                if (!cpu_stat.empty()) {
                    size_t pos = cpu_stat.find("usage_usec");
                    if (pos != std::string::npos) {
                        std::string usage_line = cpu_stat.substr(pos);
                        size_t start = usage_line.find(' ');
                        size_t end = usage_line.find('\n');
                        if (start != std::string::npos && end != std::string::npos) {
                            std::string usage_str = usage_line.substr(start + 1, end - start - 1);
                            metrics->cpu_usage_ns = parseLong(usage_str) * 1000;
                            break;
                        }
                    }
                }
            }
        }
        
        // BUSCAR MEMÓRIA
        std::vector<std::string> memory_paths = {
            "/sys/fs/cgroup/memory.current",
            "/sys/fs/cgroup/init.scope/memory.current",
            "/sys/fs/cgroup/user.slice/memory.current",
            "/sys/fs/cgroup/system.slice/memory.current"
        };
        
        for (const auto& path : memory_paths) {
            if (fileExists(path)) {
                std::string mem_usage = readFile(path);
                metrics->memory_usage_bytes = parseLong(mem_usage);
                if (metrics->memory_usage_bytes > 0) break;
            }
        }
        
        // BUSCAR LIMITE DE MEMÓRIA
        std::vector<std::string> memory_limit_paths = {
            "/sys/fs/cgroup/memory.max",
            "/sys/fs/cgroup/init.scope/memory.max",
            "/sys/fs/cgroup/user.slice/memory.max",
            "/sys/fs/cgroup/system.slice/memory.max"
        };
        
        for (const auto& path : memory_limit_paths) {
            if (fileExists(path)) {
                std::string mem_limit = readFile(path);
                if (mem_limit.find("max") != std::string::npos) {
                    metrics->memory_limit_bytes = 0;
                } else {
                    metrics->memory_limit_bytes = parseLong(mem_limit);
                }
                if (metrics->memory_limit_bytes > 0) break;
            }
        }
    }
    
    return metrics;
}
