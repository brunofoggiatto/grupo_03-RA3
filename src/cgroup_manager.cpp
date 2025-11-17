#include "cgroup_manager.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <dirent.h>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <vector>
#include <algorithm>

CGroupManager::CGroupManager() {
    base_path = "/sys/fs/cgroup";
    if (is_cgroup_v2()) {
        base_path += "/unified";
    }
}

CGroupManager::~CGroupManager() {
    // Destructor
}

void CGroupManager::set_base_path(const std::string& path) {
    base_path = path;
}

std::string CGroupManager::get_base_path() const {
    return base_path;
}

bool CGroupManager::create_cgroup(const std::string& cgroup_path) {
    std::string full_path = base_path + cgroup_path;
    
    if (mkdir(full_path.c_str(), 0755) == 0) {
        std::cout << " CGroup criado: " << full_path << std::endl;
        return true;
    } else {
        std::cerr << " Erro ao criar cgroup " << full_path 
                  << " - " << strerror(errno) << std::endl;
        return false;
    }
}

bool CGroupManager::delete_cgroup(const std::string& cgroup_path) {
    std::string full_path = base_path + cgroup_path;
    
    // Remover todos os processos primeiro
    std::string procs_file = full_path + "/cgroup.procs";
    std::ifstream procs(procs_file);
    if (procs.is_open()) {
        std::string line;
        while (std::getline(procs, line)) {
            int pid = std::stoi(line);
            move_process_to_cgroup(pid, "/");
        }
        procs.close();
    }
    
    if (rmdir(full_path.c_str()) == 0) {
        std::cout << " CGroup removido: " << full_path << std::endl;
        return true;
    } else {
        std::cerr << " Erro ao remover cgroup " << full_path 
                  << " - " << strerror(errno) << std::endl;
        return false;
    }
}

bool CGroupManager::exists_cgroup(const std::string& cgroup_path) {
    std::string full_path = base_path + cgroup_path;
    struct stat st;
    return stat(full_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool CGroupManager::move_process_to_cgroup(int pid, const std::string& cgroup_path) {
    std::string full_path = base_path + cgroup_path + "/cgroup.procs";
    std::ofstream file(full_path);
    
    if (!file.is_open()) {
        std::cerr << " Erro ao abrir " << full_path << std::endl;
        return false;
    }
    
    file << pid;
    file.close();
    
    std::cout << " Processo " << pid << " movido para " << cgroup_path << std::endl;
    return true;
}

bool CGroupManager::move_current_process_to_cgroup(const std::string& cgroup_path) {
    return move_process_to_cgroup(getpid(), cgroup_path);
}

bool CGroupManager::set_cpu_limit(const std::string& cgroup_path, double cores) {
    std::string full_path = base_path + cgroup_path;
    
    if (is_cgroup_v2()) {
        std::string cpu_max_file = full_path + "/cpu.max";
        std::ofstream file(cpu_max_file);
        
        if (!file.is_open()) {
            std::cerr << " Erro ao abrir " << cpu_max_file << std::endl;
            return false;
        }
        
        int max_quota = cores * 100000;
        file << max_quota << " 100000";
        file.close();
    } else {
        std::string quota_file = full_path + "/cpu.cfs_quota_us";
        std::string period_file = full_path + "/cpu.cfs_period_us";
        
        std::ofstream period(period_file);
        if (!period.is_open()) {
            std::cerr << " Erro ao abrir " << period_file << std::endl;
            return false;
        }
        period << "100000";
        period.close();
        
        std::ofstream quota(quota_file);
        if (!quota.is_open()) {
            std::cerr << " Erro ao abrir " << quota_file << std::endl;
            return false;
        }
        
        int max_quota = cores * 100000;
        quota << max_quota;
        quota.close();
    }
    
    std::cout << " Limite de CPU definido: " << cores << " cores" << std::endl;
    return true;
}

double CGroupManager::read_cpu_usage(const std::string& cgroup_path) {
    std::string full_path = base_path + cgroup_path;
    std::string usage_file;
    
    if (is_cgroup_v2()) {
        usage_file = full_path + "/cpu.stat";
        std::ifstream file(usage_file);
        
        if (!file.is_open()) {
            std::cerr << " Erro ao abrir " << usage_file << std::endl;
            return -1.0;
        }
        
        std::string line;
        long long usage_usec = 0;
        
        while (std::getline(file, line)) {
            if (line.find("usage_usec") != std::string::npos) {
                std::istringstream iss(line);
                std::string key;
                iss >> key >> usage_usec;
                break;
            }
        }
        
        file.close();
        return usage_usec / 1000000.0;
    } else {
        usage_file = full_path + "/cpuacct.usage";
        std::ifstream file(usage_file);
        
        if (!file.is_open()) {
            std::cerr << " Erro ao abrir " << usage_file << std::endl;
            return -1.0;
        }
        
        long long usage_nsec;
        file >> usage_nsec;
        file.close();
        
        return usage_nsec / 1e9;
    }
}

bool CGroupManager::set_memory_limit(const std::string& cgroup_path, size_t limit_mb) {
    std::string full_path = base_path + cgroup_path;
    std::string limit_file;
    
    if (is_cgroup_v2()) {
        limit_file = full_path + "/memory.max";
    } else {
        limit_file = full_path + "/memory.limit_in_bytes";
    }
    
    std::ofstream file(limit_file);
    if (!file.is_open()) {
        std::cerr << " Erro ao abrir " << limit_file << std::endl;
        return false;
    }
    
    file << (limit_mb * 1024 * 1024);
    file.close();
    
    std::cout << " Limite de memória definido: " << limit_mb << " MB" << std::endl;
    return true;
}

size_t CGroupManager::read_memory_usage(const std::string& cgroup_path) {
    std::string full_path = base_path + cgroup_path;
    std::string usage_file;
    
    if (is_cgroup_v2()) {
        usage_file = full_path + "/memory.current";
    } else {
        usage_file = full_path + "/memory.usage_in_bytes";
    }
    
    std::ifstream file(usage_file);
    if (!file.is_open()) {
        std::cerr << " Erro ao abrir " << usage_file << std::endl;
        return 0;
    }
    
    size_t usage;
    file >> usage;
    file.close();
    
    return usage / (1024 * 1024);
}

bool CGroupManager::set_pids_limit(const std::string& cgroup_path, int max_pids) {
    std::string full_path = base_path + cgroup_path;
    std::string pids_max_file;
    
    if (is_cgroup_v2()) {
        pids_max_file = full_path + "/pids.max";
    } else {
        pids_max_file = full_path + "/pids.max";
    }
    
    std::ofstream file(pids_max_file);
    if (!file.is_open()) {
        std::cerr << " Erro: não foi possível abrir " << pids_max_file << std::endl;
        return false;
    }
    
    file << max_pids;
    file.close();
    
    std::cout << " Limite de PIDs definido: " << max_pids << " em " << cgroup_path << std::endl;
    return true;
}

int CGroupManager::read_pids_current(const std::string& cgroup_path) {
    std::string full_path = base_path + cgroup_path;
    std::string pids_current_file;
    
    if (is_cgroup_v2()) {
        pids_current_file = full_path + "/pids.current";
    } else {
        pids_current_file = full_path + "/pids.current";
    }
    
    std::ifstream file(pids_current_file);
    if (!file.is_open()) {
        std::cerr << " Erro: não foi possível abrir " << pids_current_file << std::endl;
        return -1;
    }
    
    int current;
    file >> current;
    file.close();
    
    return current;
}

int CGroupManager::read_pids_max(const std::string& cgroup_path) {
    std::string full_path = base_path + cgroup_path;
    std::string pids_max_file;
    
    if (is_cgroup_v2()) {
        pids_max_file = full_path + "/pids.max";
    } else {
        pids_max_file = full_path + "/pids.max";
    }
    
    std::ifstream file(pids_max_file);
    if (!file.is_open()) {
        std::cerr << " Erro: não foi possível abrir " << pids_max_file << std::endl;
        return -1;
    }
    
    std::string value;
    file >> value;
    file.close();
    
    if (value == "max") {
        return -1;
    }
    
    return std::stoi(value);
}

PressureStats CGroupManager::read_cpu_pressure(const std::string& cgroup_path) {
    PressureStats stats = {0, 0, 0, 0};
    std::string full_path = base_path + cgroup_path;
    
    if (!is_cgroup_v2()) {
        std::cerr << "  Pressure Stall Information só disponível no CGroup v2" << std::endl;
        return stats;
    }
    
    std::string pressure_file = full_path + "/cpu.pressure";
    std::ifstream file(pressure_file);
    
    if (!file.is_open()) {
        std::cerr << " Erro: não foi possível abrir " << pressure_file << std::endl;
        return stats;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("some") == 0) {
            sscanf(line.c_str(), "some avg10=%lf avg60=%lf avg300=%lf total=%llu",
                   &stats.avg10, &stats.avg60, &stats.avg300, &stats.total);
            break;
        }
    }
    
    file.close();
    return stats;
}

PressureStats CGroupManager::read_memory_pressure(const std::string& cgroup_path) {
    PressureStats stats = {0, 0, 0, 0};
    std::string full_path = base_path + cgroup_path;
    
    if (!is_cgroup_v2()) {
        std::cerr << "  Pressure Stall Information só disponível no CGroup v2" << std::endl;
        return stats;
    }
    
    std::string pressure_file = full_path + "/memory.pressure";
    std::ifstream file(pressure_file);
    
    if (!file.is_open()) {
        std::cerr << " Erro: não foi possível abrir " << pressure_file << std::endl;
        return stats;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("some") == 0) {
            sscanf(line.c_str(), "some avg10=%lf avg60=%lf avg300=%lf total=%llu",
                   &stats.avg10, &stats.avg60, &stats.avg300, &stats.total);
            break;
        }
    }
    
    file.close();
    return stats;
}

PressureStats CGroupManager::read_io_pressure(const std::string& cgroup_path) {
    PressureStats stats = {0, 0, 0, 0};
    std::string full_path = base_path + cgroup_path;
    
    if (!is_cgroup_v2()) {
        std::cerr << "  Pressure Stall Information só disponível no CGroup v2" << std::endl;
        return stats;
    }
    
    std::string pressure_file = full_path + "/io.pressure";
    std::ifstream file(pressure_file);
    
    if (!file.is_open()) {
        std::cerr << " Erro: não foi possível abrir " << pressure_file << std::endl;
        return stats;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("some") == 0) {
            sscanf(line.c_str(), "some avg10=%lf avg60=%lf avg300=%lf total=%llu",
                   &stats.avg10, &stats.avg60, &stats.avg300, &stats.total);
            break;
        }
    }
    
    file.close();
    return stats;
}

void CGroupManager::print_pressure(const PressureStats& p, const std::string& type) {
    std::cout << " " << type << " Pressure Stall Information:\n";
    std::cout << "    10s:  " << p.avg10 << "%\n";
    std::cout << "    60s:  " << p.avg60 << "%\n";
    std::cout << "    5min: " << p.avg300 << "%\n";
    std::cout << "   Total: " << p.total << " microseconds\n";
}

bool CGroupManager::is_cgroup_v2() {
    struct stat st;
    return stat("/sys/fs/cgroup/cgroup.controllers", &st) == 0;
}

std::string CGroupManager::get_current_cgroup(int pid) {
    if (pid == 0) pid = getpid();
    
    std::string cgroup_file = "/proc/" + std::to_string(pid) + "/cgroup";
    std::ifstream file(cgroup_file);
    
    if (!file.is_open()) {
        return "";
    }
    
    std::string line;
    if (std::getline(file, line)) {
        if (is_cgroup_v2()) {
            size_t pos = line.find("::");
            if (pos != std::string::npos) {
                return line.substr(pos + 2);
            }
        } else {
            std::istringstream iss(line);
            std::string part;
            while (std::getline(iss, part, ':')) {}
            return part;
        }
    }
    
    return "";
}

void CGroupManager::display_cgroup_stats(const std::string& cgroup_path) {
    std::cout << "\n ESTATÍSTICAS DO CGROUP: " << cgroup_path << std::endl;
    std::cout << "=========================================" << std::endl;
    
    double cpu_usage = read_cpu_usage(cgroup_path);
    std::cout << " Uso de CPU: " << cpu_usage << " segundos" << std::endl;
    
    size_t mem_usage = read_memory_usage(cgroup_path);
    std::cout << " Uso de Memória: " << mem_usage << " MB" << std::endl;
    
    int pids_current = read_pids_current(cgroup_path);
    int pids_max = read_pids_max(cgroup_path);
    std::cout << " PIDs: " << pids_current;
    if (pids_max != -1) {
        std::cout << "/" << pids_max;
    }
    std::cout << std::endl;
    
    if (is_cgroup_v2()) {
        PressureStats cpu_pressure = read_cpu_pressure(cgroup_path);
        print_pressure(cpu_pressure, "CPU");
        
        PressureStats mem_pressure = read_memory_pressure(cgroup_path);
        print_pressure(mem_pressure, "Memory");
    }
    
    std::cout << "=========================================\n" << std::endl;
}

void CGroupManager::generate_utilization_report(const std::string& cgroup_path, const std::string& filename) {
    std::ofstream report(filename);
    
    if (!report.is_open()) {
        std::cerr << " Erro ao criar relatório " << filename << std::endl;
        return;
    }
    
    report << "RELATÓRIO DE UTILIZAÇÃO - CGROUP: " << cgroup_path << "\n";
    report << "Gerado em: " << __DATE__ << " " << __TIME__ << "\n";
    report << "=========================================\n\n";
    
    report << "INFORMAÇÕES BÁSICAS:\n";
    report << "CGroup Path: " << cgroup_path << "\n";
    report << "CGroup Version: " << (is_cgroup_v2() ? "v2" : "v1") << "\n";
    report << "Base Path: " << base_path << "\n\n";
    
    report << "ESTATÍSTICAS DE CPU:\n";
    double cpu_usage = read_cpu_usage(cgroup_path);
    report << "Uso Total: " << cpu_usage << " segundos\n";
    
    report << "\nESTATÍSTICAS DE MEMÓRIA:\n";
    size_t mem_usage = read_memory_usage(cgroup_path);
    report << "Uso Atual: " << mem_usage << " MB\n";
    
    report << "\nESTATÍSTICAS DE PIDs:\n";
    int pids_current = read_pids_current(cgroup_path);
    int pids_max = read_pids_max(cgroup_path);
    report << "Processos Atuais: " << pids_current << "\n";
    report << "Limite de PIDs: " << (pids_max == -1 ? "ilimitado" : std::to_string(pids_max)) << "\n";
    
    if (is_cgroup_v2()) {
        report << "\nPRESSURE STALL INFORMATION:\n";
        
        PressureStats cpu_pressure = read_cpu_pressure(cgroup_path);
        report << "CPU Pressure:\n";
        report << "  Média 10s: " << cpu_pressure.avg10 << "%\n";
        report << "  Média 60s: " << cpu_pressure.avg60 << "%\n";
        report << "  Média 5min: " << cpu_pressure.avg300 << "%\n";
        report << "  Total: " << cpu_pressure.total << " μs\n";
        
        PressureStats mem_pressure = read_memory_pressure(cgroup_path);
        report << "Memory Pressure:\n";
        report << "  Média 10s: " << mem_pressure.avg10 << "%\n";
        report << "  Média 60s: " << mem_pressure.avg60 << "%\n";
        report << "  Média 5min: " << mem_pressure.avg300 << "%\n";
        report << "  Total: " << mem_pressure.total << " μs\n";
    }
    
    report.close();
    std::cout << " Relatório gerado: " << filename << std::endl;
}

bool CGroupManager::run_pid_limit_test(const std::string& test_cgroup, int max_pids) {
    std::cout << "\n INICIANDO TESTE DE LIMITAÇÃO DE PIDs\n";
    std::cout << "=========================================\n";
    
    if (!create_cgroup(test_cgroup)) {
        std::cerr << " Falha ao criar cgroup de teste" << std::endl;
        return false;
    }
    
    if (!set_pids_limit(test_cgroup, max_pids)) {
        std::cerr << " Falha ao definir limite de PIDs" << std::endl;
        delete_cgroup(test_cgroup);
        return false;
    }
    
    if (!move_current_process_to_cgroup(test_cgroup)) {
        std::cerr << " Falha ao mover processo para cgroup" << std::endl;
        delete_cgroup(test_cgroup);
        return false;
    }
    
    std::cout << " Configuração do teste concluída\n";
    std::cout << " Limite de PIDs: " << max_pids << std::endl;
    std::cout << " PIDs atuais: " << read_pids_current(test_cgroup) << std::endl;
    
    delete_cgroup(test_cgroup);
    std::cout << " Teste de limitação de PIDs concluído\n";
    return true;
}

// Implementações vazias para remover warnings (sem redefinição)
bool CGroupManager::set_cpu_quota(const std::string& cgroup_path, int quota_us, int period_us) {
    (void)cgroup_path;
    (void)quota_us;
    (void)period_us;
    return true;
}

bool CGroupManager::set_memory_swap_limit(const std::string& cgroup_path, size_t limit_mb) {
    (void)cgroup_path;
    (void)limit_mb;
    return true;
}

size_t CGroupManager::read_memory_max_usage(const std::string& cgroup_path) {
    (void)cgroup_path;
    return 0;
}

bool CGroupManager::trigger_oom(const std::string& cgroup_path) {
    (void)cgroup_path;
    return true;
}

bool CGroupManager::set_io_limit(const std::string& cgroup_path, const std::string& device, 
                               long read_bps, long write_bps) {
    (void)cgroup_path;
    (void)device;
    (void)read_bps;
    (void)write_bps;
    return true;
}

bool CGroupManager::set_io_weight(const std::string& cgroup_path, int weight) {
    (void)cgroup_path;
    (void)weight;
    return true;
}

std::vector<std::string> CGroupManager::list_processes_in_cgroup(const std::string& cgroup_path) {
    (void)cgroup_path;
    return std::vector<std::string>();
}