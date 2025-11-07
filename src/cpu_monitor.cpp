#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include "monitor.hpp"

// Lê dados de CPU de um PID específico
int get_cpu_usage(int pid, ProcStats& stats) {
    std::string path = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream file(path);
    if (!file.is_open()) return -1;

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
    if (!status_file.is_open()) return -1;

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

// Calcula o percentual de uso da CPU entre duas leituras
double calculate_cpu_percent(const ProcStats& prev, const ProcStats& curr, double interval) {
    const long ticks_per_sec = sysconf(_SC_CLK_TCK);
    double delta_time = ((curr.utime + curr.stime) - (prev.utime + prev.stime)) / static_cast<double>(ticks_per_sec);
    double percent = (delta_time / interval) * 100.0;
    return percent < 0 ? 0 : percent;
}
