#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdio>
#include <errno.h>
#include <cstring>
#include "../include/monitor.hpp"

// Códigos de erro semânticos
#define ERR_PROCESS_NOT_FOUND -2
#define ERR_PERMISSION_DENIED -3
#define ERR_UNKNOWN -4

int get_memory_usage(int pid, ProcStats& stats) {
    std::string path = "/proc/" + std::to_string(pid) + "/status";
    std::ifstream file(path);
    if (!file.is_open()) {
        if (errno == ENOENT) {
            std::cerr << "ERRO: Processo com PID " << pid << " não existe" << std::endl;
            return ERR_PROCESS_NOT_FOUND;
        } else if (errno == EACCES) {
            std::cerr << "ERRO: Permissão negada para acessar processo " << pid << std::endl;
            return ERR_PERMISSION_DENIED;
        } else {
            std::cerr << "ERRO: Não foi possível abrir " << path << " - " << strerror(errno) << std::endl;
            return ERR_UNKNOWN;
        }
    }

    std::string line;
    long rss_kb = 0, vsz_kb = 0, swap_kb = 0;

    while (std::getline(file, line)) {
        if (line.rfind("VmRSS:", 0) == 0)
            std::sscanf(line.c_str(), "VmRSS: %ld", &rss_kb);
        else if (line.rfind("VmSize:", 0) == 0)
            std::sscanf(line.c_str(), "VmSize: %ld", &vsz_kb);
        else if (line.rfind("VmSwap:", 0) == 0)
            std::sscanf(line.c_str(), "VmSwap: %ld", &swap_kb);
    }
    file.close();

    // Lê memória total do sistema
    std::ifstream meminfo("/proc/meminfo");
    if (!meminfo.is_open()) {
        std::cerr << "ERRO: Não foi possível abrir /proc/meminfo - " << strerror(errno) << std::endl;
        return ERR_UNKNOWN;
    }

    long mem_total_kb = 0;
    while (std::getline(meminfo, line)) {
        if (line.rfind("MemTotal:", 0) == 0)
            std::sscanf(line.c_str(), "MemTotal: %ld", &mem_total_kb);
    }
    meminfo.close();

    // Lê page faults de /proc/[pid]/stat
    path = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream stat_file(path);
    if (!stat_file.is_open()) {
        if (errno == ENOENT) {
            std::cerr << "ERRO: Processo com PID " << pid << " não existe" << std::endl;
            return ERR_PROCESS_NOT_FOUND;
        } else if (errno == EACCES) {
            std::cerr << "ERRO: Permissão negada para acessar processo " << pid << std::endl;
            return ERR_PERMISSION_DENIED;
        } else {
            std::cerr << "ERRO: Não foi possível abrir " << path << " - " << strerror(errno) << std::endl;
            return ERR_UNKNOWN;
        }
    }

    long minor_faults = 0, major_faults = 0;
    std::string token;
    for (int i = 0; i < 9; ++i) stat_file >> token;
    stat_file >> minor_faults >> token >> major_faults;
    stat_file.close();

    // Calcula percentual de uso da memória
    double mem_percent = mem_total_kb > 0 ? (rss_kb * 100.0) / mem_total_kb : 0.0;

    stats.memory_rss = rss_kb;
    stats.memory_vsz = vsz_kb;
    stats.memory_swap = swap_kb;
    stats.minor_faults = minor_faults;
    stats.major_faults = major_faults;
    stats.memory_percent = mem_percent;

    return 0;
}