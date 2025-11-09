#include <iostream>
#include <iomanip>
#include <string>
#include <thread>
#include <chrono>
#include <ctime>
#include "monitor.hpp"

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Uso: " << argv[0] << " profile <PID> <intervalo_segundos>\n";
        return 1;
    }

    std::string command = argv[1];
    if (command != "profile") {
        std::cerr << "Comando inválido! Use: profile\n";
        return 1;
    }

    int pid = std::stoi(argv[2]);
    int intervalo = std::stoi(argv[3]);

    std::cout << "Monitorando processo PID " << pid 
              << " a cada " << intervalo << " segundos...\n";
    std::cout << "Pressione Ctrl+C para parar.\n\n";

    ProcStats prev{}, curr{};
    get_cpu_usage(pid, prev);
    std::this_thread::sleep_for(std::chrono::seconds(intervalo));

    while (true) {
        get_cpu_usage(pid, curr);
        get_memory_usage(pid, curr);

        double cpu_percent = calculate_cpu_percent(prev, curr, intervalo);

        // Obter hora atual
        std::time_t t = std::time(nullptr);
        std::tm* tm_info = std::localtime(&t);
        char hora[9];
        std::strftime(hora, sizeof(hora), "%H:%M:%S", tm_info);

        // Saída formatada
        std::cout << "[" << hora << "] "
                  << std::fixed << std::setprecision(2)
                  << "CPU: " << cpu_percent << "% | "
                  << "Threads: " << curr.threads << " | "
                  << "Ctx: " << curr.voluntary_ctxt << "/" << curr.nonvoluntary_ctxt << " | "
                  << "Mem: " << curr.memory_percent << "% "
                  << "(RSS: " << curr.memory_rss << " kB, "
                  << "VSZ: " << curr.memory_vsz << " kB, "
                  << "Swap: " << curr.memory_swap << " kB) | "
                  << "Faults: " << curr.minor_faults << "/" << curr.major_faults
                  << std::endl;

        prev = curr;
        std::this_thread::sleep_for(std::chrono::seconds(intervalo));
    }

    return 0;
}
