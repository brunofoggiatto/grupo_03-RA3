#include "cgroup_manager.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

CGroupMetrics::CGroupMetrics() 
    : cpu_usage_ns(0), memory_usage_bytes(0), 
      memory_limit_bytes(0), memory_peak_bytes(0),
      memory_failcnt(0), cgroup_version("unknown") {}

void CGroupMetrics::print() const {
    std::cout << "=== CGroup Metrics ===" << std::endl;
    std::cout << "Version: " << cgroup_version << std::endl;
    
    std::cout << "CPU Usage: " << cpu_usage_ns / 1e9 << " seconds" << std::endl;
    
    if (memory_usage_bytes > 0) {
        std::cout << "Memory Usage (current): " << memory_usage_bytes / (1024 * 1024) << " MB" << std::endl;
    } else {
        std::cout << "Memory Usage (current): N/A" << std::endl;
    }
    
    if (memory_peak_bytes > 0) {
        std::cout << "Memory Usage (peak): " << memory_peak_bytes / (1024 * 1024) << " MB" << std::endl;
    } else {
        std::cout << "Memory Usage (peak): N/A" << std::endl;
    }
    
    if (memory_limit_bytes > 0) {
        std::cout << "Memory Limit: " << memory_limit_bytes / (1024 * 1024) << " MB" << std::endl;
    } else {
        std::cout << "Memory Limit: N/A (unlimited)" << std::endl;
    }
    
    std::cout << "Memory Allocation Failures: " << memory_failcnt << std::endl;
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
        
        if (fileExists("/sys/fs/cgroup/memory.max")) {
            std::string mem_limit = readFile("/sys/fs/cgroup/memory.max");
            if (!mem_limit.empty() && mem_limit.find("max") == std::string::npos) {
                metrics->memory_limit_bytes = parseLong(mem_limit);
            }
        }
    }
    
    return metrics;
}

std::unique_ptr<CGroupMetrics> CGroupManager::getCGroupMetrics(const std::string& cgroup_path) const {
    auto metrics = std::make_unique<CGroupMetrics>();
    metrics->cgroup_version = detectCGroupVersion();
    
    if (metrics->cgroup_version == "v2") {
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
        
        std::string mem_limit_file = cgroup_path + "/memory.max";
        if (fileExists(mem_limit_file)) {
            std::string mem_limit = readFile(mem_limit_file);
            if (!mem_limit.empty() && mem_limit.find("max") == std::string::npos) {
                metrics->memory_limit_bytes = parseLong(mem_limit);
            }
        }
        
        std::string mem_peak_file = cgroup_path + "/memory.peak";
        if (fileExists(mem_peak_file)) {
            std::string mem_peak = readFile(mem_peak_file);
            metrics->memory_peak_bytes = parseLong(mem_peak);
        }
        
        std::string mem_failcnt_file = cgroup_path + "/memory.failcnt";
        if (fileExists(mem_failcnt_file)) {
            std::string mem_failcnt = readFile(mem_failcnt_file);
            metrics->memory_failcnt = parseLong(mem_failcnt);
        }
    }
    
    return metrics;
}
