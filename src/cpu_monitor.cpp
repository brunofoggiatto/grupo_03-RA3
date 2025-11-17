#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <thread>
#include "../include/monitor.hpp"

// Códigos de erro semânticos
#define ERR_PROCESS_NOT_FOUND -2
#define ERR_PERMISSION_DENIED -3
#define ERR_UNKNOWN -4

// Lê dados de CPU de um PID específico
int get_cpu_usage(int pid, ProcStats& stats) {
    std::string path = "/proc/" + std::to_string(pid) + "/stat";
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

    std::string token;
    long utime = 0, stime = 0, cutime = 0, cstime = 0;

    // Pula até o campo 14
    for (int i = 0; i < 13; ++i)
        file >> token;

    file >> utime >> stime >> cutime >> cstime;
    file.close();

    // Lê contexto e threads
    path = "/proc/" + std::to_string(pid) + "/status";
    std::ifstream status_file(path);
    if (!status_file.is_open()) {
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
    int threads = 0;
    long voluntary_ctxt = 0, nonvoluntary_ctxt = 0;

    while (std::getline(status_file, line)) {
        if (line.rfind("Threads:", 0) == 0) {
            std::sscanf(line.c_str(), "Threads: %d", &threads);
        } else if (line.rfind("voluntary_ctxt_switches:", 0) == 0) {
            std::sscanf(line.c_str(), "voluntary_ctxt_switches: %ld", &voluntary_ctxt);
        } else if (line.rfind("nonvoluntary_ctxt_switches:", 0) == 0) {
            std::sscanf(line.c_str(), "nonvoluntary_ctxt_switches: %ld", &nonvoluntary_ctxt);
        }
    }
    status_file.close();

    stats.utime = utime;
    stats.stime = stime;
    stats.threads = threads;
    stats.voluntary_ctxt = voluntary_ctxt;
    stats.nonvoluntary_ctxt = nonvoluntary_ctxt;

    return 0;
}

// Obtém número de núcleos do sistema para normalização
unsigned int get_num_cores() {
    unsigned int cores = std::thread::hardware_concurrency();
    return (cores > 0) ? cores : 1; // Fallback para 1 se não detectar
}

// Calcula o percentual de uso da CPU entre duas leituras (normalizado por núcleos)
double calculate_cpu_percent(const ProcStats& prev, const ProcStats& curr, double interval) {
    const long ticks_per_sec = sysconf(_SC_CLK_TCK);
    double delta_time = ((curr.utime + curr.stime) - (prev.utime + prev.stime)) / static_cast<double>(ticks_per_sec);
    double percent = (delta_time / interval) * 100.0;
    
    // Normalização por número de núcleos
    unsigned int num_cores = get_num_cores();
    double normalized_percent = percent / num_cores;
    
    return normalized_percent < 0 ? 0 : normalized_percent;
}