#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <thread>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <ctime>
#include <csignal>
#include <atomic>
#include <filesystem>
#include <cstdlib>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include "monitor.hpp"
#include "cgroup_manager.hpp"
#include "namespace.hpp"

using namespace std;

atomic<bool> monitoring_active{true};

class ResourceProfiler {
private:
    ProcStats prev_stats;
    
    bool pidExists(int pid) {
        string proc_path = "/proc/" + to_string(pid);
        struct stat info;
        return (stat(proc_path.c_str(), &info) == 0);
    }
    
    bool canAccessProcess(int pid) {
        string stat_path = "/proc/" + to_string(pid) + "/stat";
        ifstream file(stat_path);
        if (!file.is_open()) {
            return false;
        }
        file.close();
        return true;
    }

    string getErrorDescription(int error_code) {
        switch (error_code) {
            case ERR_PROCESS_NOT_FOUND:
                return "Processo não encontrado";
            case ERR_PERMISSION_DENIED:
                return "Permissão negada";
            case ERR_UNKNOWN:
                return "Erro desconhecido";
            default:
                return "Erro código: " + to_string(error_code);
        }
    }

public:
    bool validateProcessAccess(int pid) {
        if (pid <= 0) {
            throw invalid_argument("PID inválido: " + to_string(pid));
        }
        
        if (!pidExists(pid)) {
            throw runtime_error("Processo com PID " + to_string(pid) + " não existe");
        }
        
        if (!canAccessProcess(pid)) {
            throw runtime_error("Sem permissão para acessar o processo " + to_string(pid) + 
                              " (execute como root ou com permissões adequadas)");
        }
        
        return true;
    }
    
    bool monitorProcess(int pid, int duration_sec, int interval_sec, const string& csv_file) {
        try {
            cout << "\nValidando acesso ao processo " << pid << "..." << endl;
            validateProcessAccess(pid);
            cout << "Acesso validado com sucesso\n" << endl;
            
            ofstream csv(csv_file);
            if (!csv.is_open()) {
                throw runtime_error("Não foi possível criar arquivo: " + csv_file);
            }
            
            csv << "timestamp,pid,cpu_percent,memory_rss_bytes,memory_vsz_bytes,"
                << "memory_swap_bytes,io_read_bytes,io_write_bytes,io_read_rate_bps,"
                << "io_write_rate_bps,threads,minor_faults,major_faults,tcp_connections\n";
            csv.flush();
            
            ProcStats initial_stats;
            int cpu_result = get_cpu_usage(pid, initial_stats);
            if (cpu_result < 0) {
                throw runtime_error("Falha ao coletar CPU: " + getErrorDescription(cpu_result));
            }
            
            int memory_result = get_memory_usage(pid, initial_stats);
            if (memory_result < 0) {
                throw runtime_error("Falha ao coletar memória: " + getErrorDescription(memory_result));
            }
            
            int io_result = get_io_usage(pid, initial_stats);
            if (io_result < 0) {
                throw runtime_error("Falha ao coletar I/O: " + getErrorDescription(io_result));
            }
            
            prev_stats = initial_stats;
            
            cout << "Monitorando processo PID: " << pid << endl;
            cout << "Duração: " << duration_sec << " segundos" << endl;
            cout << "Intervalo: " << interval_sec << " segundos" << endl;
            cout << "Arquivo: " << csv_file << endl;
            cout << "Pressione Ctrl+C para parar..." << endl;
            cout << "----------------------------------------" << endl;
            
            auto start = chrono::steady_clock::now();
            int iteration = 0;
            int error_count = 0;
            const int MAX_ERRORS = 3;
            
            while (monitoring_active && error_count < MAX_ERRORS) {
                auto now = chrono::steady_clock::now();
                auto elapsed = chrono::duration_cast<chrono::seconds>(now - start).count();
                
                if (elapsed >= duration_sec) {
                    break;
                }
                
                try {
                    if (!pidExists(pid)) {
                        throw runtime_error("Processo " + to_string(pid) + " não existe mais");
                    }
                    
                    ProcStats curr_stats;
                    
                    cpu_result = get_cpu_usage(pid, curr_stats);
                    if (cpu_result < 0) {
                        throw runtime_error("Erro CPU: " + getErrorDescription(cpu_result));
                    }
                    
                    memory_result = get_memory_usage(pid, curr_stats);
                    if (memory_result < 0) {
                        throw runtime_error("Erro memória: " + getErrorDescription(memory_result));
                    }
                    
                    io_result = get_io_usage(pid, curr_stats);
                    if (io_result < 0) {
                        throw runtime_error("Erro I/O: " + getErrorDescription(io_result));
                    }
                    
                    int network_result = get_network_usage(pid, curr_stats);
                    if (network_result < 0) {
                        curr_stats.tcp_connections = 0;
                    }
                    
                    double cpu_pct = calculate_cpu_percent(prev_stats, curr_stats, interval_sec);
                    calculate_io_rate(prev_stats, curr_stats, interval_sec);
                    
                    auto time_now = chrono::system_clock::now();
                    time_t t = chrono::system_clock::to_time_t(time_now);
                    struct tm tm_buf;
                    localtime_r(&t, &tm_buf);
                    stringstream timestamp;
                    timestamp << put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
                    
                    csv << timestamp.str() << ","
                        << pid << ","
                        << fixed << setprecision(2) << cpu_pct << ","
                        << curr_stats.memory_rss << ","
                        << curr_stats.memory_vsz << ","
                        << curr_stats.memory_swap << ","
                        << curr_stats.io_read_bytes << ","
                        << curr_stats.io_write_bytes << ","
                        << fixed << setprecision(0) << curr_stats.io_read_rate << ","
                        << fixed << setprecision(0) << curr_stats.io_write_rate << ","
                        << curr_stats.threads << ","
                        << curr_stats.minor_faults << ","
                        << curr_stats.major_faults << ","
                        << curr_stats.tcp_connections << "\n";
                    csv.flush();
                    
                    cout << "[" << timestamp.str() << "] "
                         << "CPU: " << setw(6) << fixed << setprecision(2) << cpu_pct << "% | "
                         << "RSS: " << setw(6) << (curr_stats.memory_rss / 1024) << "MB | "
                         << "IO_R: " << setw(7) << fixed << setprecision(1) << (curr_stats.io_read_rate / (1024.0*1024.0)) << "MB/s | "
                         << "TCP: " << setw(2) << curr_stats.tcp_connections
                         << endl;
                    
                    prev_stats = curr_stats;
                    iteration++;
                    error_count = 0;
                    
                } catch (const exception& e) {
                    error_count++;
                    cerr << "Erro iteração " << iteration + 1 << " (" << error_count << "/" << MAX_ERRORS << "): " << e.what() << endl;
                    
                    if (string(e.what()).find("não existe") != string::npos) {
                        break;
                    }
                    
                    if (error_count >= MAX_ERRORS) {
                        cerr << "Muitos erros consecutivos. Parando monitoramento." << endl;
                        break;
                    }
                    
                    this_thread::sleep_for(chrono::seconds(2));
                }
                
                this_thread::sleep_for(chrono::seconds(interval_sec));
            }
            
            csv.close();
            
            cout << "----------------------------------------" << endl;
            if (error_count >= MAX_ERRORS) {
                cout << "Monitoramento interrompido devido a múltiplos erros" << endl;
            } else {
                cout << "Monitoramento concluído" << endl;
            }
            cout << "Iterações: " << iteration << endl;
            cout << "Arquivo: " << csv_file << endl;
            
            return (error_count < MAX_ERRORS);
            
        } catch (const exception& e) {
            cerr << "\nErro: " << e.what() << endl;
            return false;
        }
    }
};

class ControlGroupManagerWrapper {
private:
    CGroupManager mgr;
    
    void checkRootPermissions() {
        if (geteuid() != 0) {
            throw runtime_error("Este experimento requer privilégios de root. Execute com 'sudo ./bin/resource-monitor'");
        }
    }
    
public:
    bool runCPUThrottlingExperiment() {
        try {
            cout << "\nEXPERIMENTO 3: THROTTLING DE CPU" << endl;
            
            checkRootPermissions();
            
            string cgroup = "exp3_test_throttling";
            
            cout << "\nCriando cgroup experimental..." << endl;
            if (!mgr.create_cgroup(cgroup)) {
                throw runtime_error("Falha ao criar cgroup " + cgroup);
            }
            
            vector<double> limits = {0.25, 0.5, 1.0, 2.0};
            
            cout << "\nTestando limites de CPU:" << endl;
            cout << "-------------------------" << endl;
            cout << left << setw(15) << "Limite (cores)"
                 << right << setw(20) << "CPU Usage (ns)"
                 << setw(20) << "Status" << endl;
            cout << "-------------------------" << endl;
            
            for (double limit : limits) {
                cout << fixed << setprecision(2) << setw(15) << limit;
                
                if (!mgr.set_cpu_limit(cgroup, limit)) {
                    cout << right << setw(20) << "N/A" << setw(20) << "FALHA" << endl;
                    continue;
                }
                
                double cpu_usage = mgr.read_cpu_usage(cgroup);
                cout << right << setw(20) << cpu_usage << setw(20) << "OK" << endl;
                
                this_thread::sleep_for(chrono::seconds(1));
            }
            
            cout << "\nDeletando cgroup..." << endl;
            mgr.delete_cgroup(cgroup);
            cout << "Experimento 3 concluído!\n" << endl;
            
            return true;
            
        } catch (const exception& e) {
            cerr << "Erro no Experimento 3: " << e.what() << endl;
            return false;
        }
    }
    
    bool runMemoryLimitExperiment() {
        try {
            cout << "\nEXPERIMENTO 4: LIMITAÇÃO DE MEMÓRIA" << endl;
            
            checkRootPermissions();
            
            string cgroup = "exp4_test_memory";
            size_t limit_mb = 100;
            
            cout << "\nCriando cgroup experimental..." << endl;
            if (!mgr.create_cgroup(cgroup)) {
                throw runtime_error("Falha ao criar cgroup " + cgroup);
            }
            
            cout << "Aplicando limite de " << limit_mb << "MB..." << endl;
            if (!mgr.set_memory_limit(cgroup, limit_mb)) {
                mgr.delete_cgroup(cgroup);
                throw runtime_error("Falha ao aplicar limite de memória");
            }
            
            size_t memory_usage = mgr.read_memory_usage(cgroup);
            int pids_current = mgr.read_pids_current(cgroup);
            
            cout << "\nResultados:" << endl;
            cout << "-----------" << endl;
            cout << left << setw(25) << "Limite configurado:"
                 << right << setw(10) << limit_mb << " MB" << endl;
            cout << left << setw(25) << "Uso atual:"
                 << right << setw(10) << memory_usage << " MB" << endl;
            cout << left << setw(25) << "PIDs ativos:"
                 << right << setw(10) << pids_current << endl;
            
            cout << "\nDeletando cgroup..." << endl;
            mgr.delete_cgroup(cgroup);
            cout << "Experimento 4 concluído!\n" << endl;
            
            return true;
            
        } catch (const exception& e) {
            cerr << "Erro no Experimento 4: " << e.what() << endl;
            return false;
        }
    }
    
    bool runIOLimitExperiment() {
        try {
            cout << "\nEXPERIMENTO 5: LIMITAÇÃO DE I/O" << endl;
            
            checkRootPermissions();
            
            cout << "\nExecutando benchmark de I/O em C++..." << endl;
            cout << "Isso pode levar alguns minutos..." << endl;
            
            ifstream bin_file("./bin/experimento5_limitacao_io");
            if (!bin_file.good()) {
                throw runtime_error("Binário do experimento 5 não encontrado. Compile com 'make all'");
            }
            
            int ret = system("sudo ./bin/experimento5_limitacao_io");
            if (ret != 0) {
                throw runtime_error("Experimento 5 retornou código: " + to_string(ret));
            }
            
            cout << "Experimento 5 concluído!\n" << endl;
            return true;
            
        } catch (const exception& e) {
            cerr << "Erro no Experimento 5: " << e.what() << endl;
            return false;
        }
    }
};

void resourceProfilerMenu() {
    ResourceProfiler profiler;
    int pid, duration, interval;
    
    cout << "\nRESOURCE PROFILER" << endl;
    cout << "-----------------" << endl;
    cout << "Digite o PID para monitorar: ";
    
    if (!(cin >> pid)) {
        cout << "Erro: PID deve ser um número inteiro!" << endl;
        cin.clear();
        cin.ignore(10000, '\n');
        return;
    }
    
    cout << "Digite a duração em segundos (padrão 30): ";
    if (!(cin >> duration) || duration <= 0) {
        duration = 30;
    }
    
    cout << "Digite o intervalo em segundos (padrão 2): ";
    if (!(cin >> interval) || interval <= 0) {
        interval = 2;
    }
    
    cin.clear();
    cin.ignore(10000, '\n');
    
    string filename = "monitoring_pid_" + to_string(pid) + ".csv";
    profiler.monitorProcess(pid, duration, interval, filename);
}

void namespaceAnalyzerMenu() {
    int choice, pid, pid1, pid2;

    cout << "\nNAMESPACE ANALYZER" << endl;
    cout << "------------------" << endl;
    cout << "1. Listar namespaces de um processo" << endl;
    cout << "2. Comparar namespaces entre processos" << endl;
    cout << "3. Gerar relatório do sistema (CSV)" << endl;
    cout << "4. Gerar relatório do sistema (JSON)" << endl;
    cout << "Escolha: ";
    cin >> choice;
    cin.clear();
    cin.ignore(10000, '\n');

    switch (choice) {
        case 1:
            cout << "Digite o PID: ";
            cin >> pid;
            {
                auto proc_ns = list_process_namespaces(pid);
                if (proc_ns) {
                    print_process_namespaces(*proc_ns);
                } else {
                    cout << "Erro: Não foi possível acessar o processo " << pid << endl;
                }
            }
            break;

        case 2:
            cout << "Digite o primeiro PID: ";
            cin >> pid1;
            cout << "Digite o segundo PID: ";
            cin >> pid2;
            {
                auto comparison = compare_namespaces(pid1, pid2);
                if (comparison) {
                    print_namespace_comparison(*comparison);
                } else {
                    cout << "Erro: Não foi possível comparar os processos" << endl;
                }
            }
            break;

        case 3:
            {
                string filename = "namespace_report.csv";
                if (generate_namespace_report(filename, "csv")) {
                    cout << "Relatório CSV gerado: " << filename << endl;
                } else {
                    cout << "Erro ao gerar relatório CSV" << endl;
                }
            }
            break;

        case 4:
            {
                string filename = "namespace_report.json";
                if (generate_namespace_report(filename, "json")) {
                    cout << "Relatório JSON gerado: " << filename << endl;
                } else {
                    cout << "Erro ao gerar relatório JSON" << endl;
                }
            }
            break;

        default:
            cout << "Opção inválida!" << endl;
    }
}

void controlGroupManagerMenu() {
    ControlGroupManagerWrapper cgroup_mgr;
    int choice;
    
    cout << "\nCONTROL GROUP MANAGER" << endl;
    cout << "---------------------" << endl;
    cout << "1. Executar Experimento 3 - Throttling de CPU" << endl;
    cout << "2. Executar Experimento 4 - Limitação de Memória" << endl;
    cout << "3. Executar Experimento 5 - Limitação de I/O" << endl;
    cout << "0. Voltar" << endl;
    cout << "Escolha: ";
    cin >> choice;
    cin.clear();
    cin.ignore(10000, '\n');
    
    switch (choice) {
        case 1:
            cgroup_mgr.runCPUThrottlingExperiment();
            break;
            
        case 2:
            cgroup_mgr.runMemoryLimitExperiment();
            break;
            
        case 3:
            cgroup_mgr.runIOLimitExperiment();
            break;
            
        case 0:
            break;
            
        default:
            cout << "Opção inválida!" << endl;
    }
}

void experimentsMenu() {
    int choice;
    
    cout << "\nMENU DE EXPERIMENTOS" << endl;
    cout << "--------------------" << endl;
    cout << "1. Experimento 1 - Overhead de Monitoramento" << endl;
    cout << "2. Experimento 2 - Isolamento de Namespaces" << endl;
    cout << "3. Experimento 3 - Throttling de CPU" << endl;
    cout << "4. Experimento 4 - Limitação de Memória" << endl;
    cout << "5. Experimento 5 - Limitação de I/O" << endl;
    cout << "0. Voltar" << endl;
    cout << "Escolha: ";
    cin >> choice;
    cin.clear();
    cin.ignore(10000, '\n');
    
    switch (choice) {
        case 1:
            cout << "\nExecutando Experimento 1..." << endl;
            cout << "Medindo overhead do Resource Profiler" << endl;
            {
                int ret = system("sudo ./bin/experimento1_overhead_monitoring");
                if (ret != 0) {
                    cerr << "Experimento 1 retornou código: " << ret << endl;
                }
            }
            break;
            
        case 2:
            cout << "\nExecutando Experimento 2..." << endl;
            cout << "Analisando namespaces do sistema" << endl;
            {
                int ret = system("./bin/experimento2_benchmark_namespaces");
                if (ret != 0) {
                    cerr << "Experimento 2 retornou código: " << ret << endl;
                }
            }
            break;
            
        case 3:
            cout << "\nExecutando Experimento 3..." << endl;
            cout << "Testando throttling de CPU" << endl;
            {
                ControlGroupManagerWrapper mgr;
                mgr.runCPUThrottlingExperiment();
            }
            break;
            
        case 4:
            cout << "\nExecutando Experimento 4..." << endl;
            cout << "Testando limite de memória" << endl;
            {
                ControlGroupManagerWrapper mgr;
                mgr.runMemoryLimitExperiment();
            }
            break;
            
        case 5:
            cout << "\nExecutando Experimento 5..." << endl;
            cout << "Testando limite de I/O" << endl;
            {
                ControlGroupManagerWrapper mgr;
                mgr.runIOLimitExperiment();
            }
            break;
            
        case 0:
            break;
            
        default:
            cout << "Opção inválida!" << endl;
    }
}

void showMainMenu() {
    cout << "\nSISTEMA DE MONITORAMENTO - RA3" << endl;
    cout << "------------------------------" << endl;
    cout << "1. Resource Profiler (Componente 1)" << endl;
    cout << "2. Namespace Analyzer (Componente 2)" << endl;
    cout << "3. Control Group Manager (Componente 3)" << endl;
    cout << "4. Menu de Experimentos (1-5)" << endl;
    cout << "0. Sair" << endl;
    cout << "\nEscolha uma opção: ";
}

void signal_handler(int sig) {
    (void)sig;
    monitoring_active = false;
    cout << "\n\nRecebido sinal de interrupção. Parando monitoramento..." << endl;
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    cout << "\nSISTEMA DE MONITORAMENTO - RA3" << endl;
    cout << "Recursos, Namespaces e Control Groups" << endl;
    cout << "Grupo 03 - Trabalho Avaliativo" << endl;
    
    int choice;
    
    do {
        showMainMenu();
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(10000, '\n');
            choice = -1;
        }
        cin.ignore(10000, '\n');
        
        monitoring_active = true;
        
        switch (choice) {
            case 1:
                resourceProfilerMenu();
                break;
                
            case 2:
                namespaceAnalyzerMenu();
                break;
                
            case 3:
                controlGroupManagerMenu();
                break;
                
            case 4:
                experimentsMenu();
                break;
                
            case 0:
                cout << "\nEncerrando sistema..." << endl;
                break;
                
            default:
                cout << "Opção inválida!\n" << endl;
                break;
        }
        
        if (choice != 0 && choice >= 1 && choice <= 4) {
            cout << "\nPressione Enter para continuar...";
            cin.get();
        }
        
    } while (choice != 0);
    
    return 0;
}