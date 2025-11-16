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

// ================================
// COMPONENTE 1: RESOURCE PROFILER (CORRIGIDO)
// ================================

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
            // Validação inicial
            cout << "\n Validando acesso ao processo " << pid << "..." << endl;
            validateProcessAccess(pid);
            cout << "  Acesso validado com sucesso\n" << endl;
            
            // Abrir arquivo CSV
            ofstream csv(csv_file);
            if (!csv.is_open()) {
                throw runtime_error("Não foi possível criar arquivo: " + csv_file);
            }
            
            // Escrever cabeçalho CSV (memória em bytes, taxas em B/s)
            csv << "timestamp,pid,cpu_percent,memory_rss_bytes,memory_vsz_bytes,"
                << "memory_swap_bytes,io_read_bytes,io_write_bytes,io_read_rate_bps,"
                << "io_write_rate_bps,threads,minor_faults,major_faults\n";
            csv.flush();
            
            // Coleta inicial
            ProcStats initial_stats;
            if (get_cpu_usage(pid, initial_stats) != 0) {
                throw runtime_error("Falha ao coletar CPU do processo");
            }
            if (get_memory_usage(pid, initial_stats) != 0) {
                throw runtime_error("Falha ao coletar memória do processo");
            }
            if (get_io_usage(pid, initial_stats) != 0) {
                throw runtime_error("Falha ao coletar I/O do processo");
            }
            
            prev_stats = initial_stats;
            
            cout << "Monitorando processo PID: " << pid << endl;
            cout << "Duração: " << duration_sec << " segundos" << endl;
            cout << "Intervalo: " << interval_sec << " segundos" << endl;
            cout << "Arquivo: " << csv_file << endl;
            cout << "Pressione Ctrl+C para parar..." << endl;
            cout << string(70, '-') << endl;
            
            auto start = chrono::steady_clock::now();
            int iteration = 0;
            
            while (monitoring_active) {
                auto now = chrono::steady_clock::now();
                auto elapsed = chrono::duration_cast<chrono::seconds>(now - start).count();
                
                if (elapsed >= duration_sec) {
                    break;
                }
                
                try {
                    // Validar que processo ainda existe
                    if (!pidExists(pid)) {
                        throw runtime_error("Processo " + to_string(pid) + " não existe mais");
                    }
                    
                    // Coletar dados REAIS
                    ProcStats curr_stats;
                    
                    if (get_cpu_usage(pid, curr_stats) != 0) {
                        throw runtime_error("Processo terminou");
                    }
                    if (get_memory_usage(pid, curr_stats) != 0) {
                        throw runtime_error("Processo terminou");
                    }
                    if (get_io_usage(pid, curr_stats) != 0) {
                        throw runtime_error("Processo terminou");
                    }
                    
                    // Calcular CPU%
                    double cpu_pct = calculate_cpu_percent(prev_stats, curr_stats, interval_sec);
                    
                    // Calcular taxas de I/O
                    calculate_io_rate(prev_stats, curr_stats, interval_sec);
                    
                    // Timestamp (thread-safe)
                    auto time_now = chrono::system_clock::now();
                    time_t t = chrono::system_clock::to_time_t(time_now);
                    struct tm tm_buf;
                    localtime_r(&t, &tm_buf);
                    stringstream timestamp;
                    timestamp << put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
                    
                    // Escrever linha CSV
                    csv << timestamp.str() << ","
                        << pid << ","
                        << fixed << setprecision(2) << cpu_pct << ","
                        // memory_* values are bytes
                        << curr_stats.memory_rss << ","
                        << curr_stats.memory_vsz << ","
                        << curr_stats.memory_swap << ","
                        << curr_stats.io_read_bytes << ","
                        << curr_stats.io_write_bytes << ","
                        << fixed << setprecision(0) << curr_stats.io_read_rate << ","
                        << fixed << setprecision(0) << curr_stats.io_write_rate << ","
                        << curr_stats.threads << ","
                        << curr_stats.minor_faults << ","
                        << curr_stats.major_faults << "\n";
                    csv.flush();
                    
                    // Imprimir na tela
                    cout << "[" << timestamp.str() << "] "
                        << "CPU: " << setw(6) << fixed << setprecision(2) << cpu_pct << "% | "
                        << "RSS: " << setw(7) << (curr_stats.memory_rss / (1024*1024)) << "MB | "
                        << "IO: " << setw(7) << fixed << setprecision(2) << (double(curr_stats.io_read_rate) / (1024.0*1024.0)) << "MB/s"
                        << endl;
                    
                    prev_stats = curr_stats;
                    iteration++;
                    
                } catch (const exception& e) {
                    cerr << "\n  Erro na iteração " << iteration + 1 << ": " << e.what() << endl;
                    
                    if (string(e.what()).find("não existe") != string::npos) {
                        break;
                    }
                    
                    this_thread::sleep_for(chrono::seconds(2));
                }
                
                this_thread::sleep_for(chrono::seconds(interval_sec));
            }
            
            csv.close();
            
            cout << string(70, '-') << endl;
            cout << "Monitoramento concluído." << endl;
            cout << "   " << iteration << " iterações realizadas" << endl;
            cout << "   Dados salvos em: " << csv_file << endl;
            
            return true;
            
        } catch (const exception& e) {
            cerr << "\nErro: " << e.what() << endl;
            return false;
        }
    }
};

// ================================
// COMPONENTE 2: NAMESPACE ANALYZER (JÁ CORRETO)
// ================================

// Usa a implementação em namespace_analyzer.cpp

// ================================
// COMPONENTE 3: CONTROL GROUP MANAGER (CORRIGIDO)
// ================================

class ControlGroupManagerWrapper {
private:
    CGroupManager mgr;
    
public:
    bool runCPUThrottlingExperiment() {
        cout << "\n=== EXPERIMENTO 3: THROTTLING DE CPU ===" << endl;
        
        if (geteuid() != 0) {
            cerr << "Este experimento precisa de root!" << endl;
            return false;
        }
        
        string cgroup = "exp3_test_throttling";
        
        cout << "\nCriando cgroup experimental..." << endl;
        if (!mgr.createCGroup(cgroup)) {
            cerr << "Falha ao criar cgroup" << endl;
            return false;
        }
        
        vector<double> limits = {0.25, 0.5, 1.0, 2.0};
        
        cout << "\nTestando limites de CPU:\n" << endl;
        cout << left << setw(15) << "Limite (cores)"
             << right << setw(20) << "CPU Usage (ns)"
             << setw(20) << "Status" << endl;
        cout << string(55, '-') << endl;
        
        for (double limit : limits) {
            if (!mgr.setCPULimit(cgroup, limit)) {
                cerr << "Falha ao aplicar limite de " << limit << " cores" << endl;
                continue;
            }
            
            auto metrics = mgr.getCGroupMetrics("/sys/fs/cgroup/" + cgroup);
            if (metrics) {
                cout << fixed << setprecision(2) << setw(15) << limit
                     << right << setw(20) << metrics->cpu_usage_ns
                     << setw(20) << "OK" << endl;
            }
        }
        
        cout << "\nDeletando cgroup..." << endl;
        mgr.deleteCGroup(cgroup);
        cout << "Experimento 3 concluído!\n" << endl;
        
        return true;
    }
    
    bool runMemoryLimitExperiment() {
        cout << "\n=== EXPERIMENTO 4: LIMITAÇÃO DE MEMÓRIA ===" << endl;
        
        if (geteuid() != 0) {
            cerr << "Este experimento precisa de root!" << endl;
            return false;
        }
        
        string cgroup = "exp4_test_memory";
        long long limit_100mb = 100 * 1024 * 1024;
        
        cout << "\nCriando cgroup experimental..." << endl;
        if (!mgr.createCGroup(cgroup)) {
            cerr << "Falha ao criar cgroup" << endl;
            return false;
        }
        
        cout << "Aplicando limite de 100MB..." << endl;
        if (!mgr.setMemoryLimit(cgroup, limit_100mb)) {
            cerr << "Falha ao aplicar limite" << endl;
            mgr.deleteCGroup(cgroup);
            return false;
        }
        
        auto metrics = mgr.getCGroupMetrics("/sys/fs/cgroup/" + cgroup);
        if (metrics) {
            cout << "\nLimites Aplicados:" << endl;
            cout << left << setw(25) << "  Limite configurado:"
                 << right << setw(15) << limit_100mb / (1024*1024) << " MB" << endl;
            cout << left << setw(25) << "  Uso atual:"
                 << right << setw(15) << metrics->memory_usage_bytes / (1024*1024) << " MB" << endl;
            cout << left << setw(25) << "  Pico:"
                 << right << setw(15) << metrics->memory_peak_bytes / (1024*1024) << " MB" << endl;
            cout << left << setw(25) << "  Falhas de alocação:"
                 << right << setw(15) << metrics->memory_failcnt << endl;
        }
        
        cout << "\nDeletando cgroup..." << endl;
        mgr.deleteCGroup(cgroup);
        cout << "Experimento 4 concluído!\n" << endl;
        
        return true;
    }
    
    bool runIOLimitExperiment() {
        cout << "\n=== EXPERIMENTO 5: LIMITAÇÃO DE I/O ===" << endl;
        
        if (geteuid() != 0) {
            cerr << "Este experimento precisa de root!" << endl;
            return false;
        }
        
        cout << "\nExperimento 5 requer execução de script externo" << endl;
        cout << "Execute manualmente: sudo bash tests/experimento5_run.sh\n" << endl;
        
        return true;
    }
};

// ================================
// MENUS INTERATIVOS
// ================================

void resourceProfilerMenu() {
    ResourceProfiler profiler;
    int pid, duration, interval;
    
    cout << "\n=== RESOURCE PROFILER ===" << endl;
    cout << "Digite o PID para monitorar: ";
    
    if (!(cin >> pid)) {
        cout << "  Erro: PID deve ser um número inteiro!" << endl;
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

    cout << "\n=== NAMESPACE ANALYZER ===" << endl;
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
                    cout << "  Erro: Não foi possível acessar o processo " << pid << endl;
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
                    cout << "  Erro: Não foi possível comparar os processos" << endl;
                }
            }
            break;

        case 3:
            {
                string filename = "namespace_report.csv";
                if (generate_namespace_report(filename, "csv")) {
                    cout << " Relatório CSV gerado: " << filename << endl;
                } else {
                    cout << "  Erro ao gerar relatório CSV" << endl;
                }
            }
            break;

        case 4:
            {
                string filename = "namespace_report.json";
                if (generate_namespace_report(filename, "json")) {
                    cout << " Relatório JSON gerado: " << filename << endl;
                } else {
                    cout << "  Erro ao gerar relatório JSON" << endl;
                }
            }
            break;

        default:
            cout << "  Opção inválida!" << endl;
    }
}

void controlGroupManagerMenu() {
    ControlGroupManagerWrapper cgroup_mgr;
    int choice;
    
    cout << "\n=== CONTROL GROUP MANAGER ===" << endl;
    cout << "1. Executar Experimento 3 - Throttling de CPU" << endl;
    cout << "2. Executar Experimento 4 - Limitação de Memória" << endl;
    cout << "3. Informações Experimento 5 - Limitação de I/O" << endl;
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
            cout << "  Opção inválida!" << endl;
    }
}

void experimentsMenu() {
    int choice;
    
    cout << "\n=== MENU DE EXPERIMENTOS ===" << endl;
    cout << "1. Experimento 1 - Overhead de Monitoramento" << endl;
    cout << "2. Experimento 2 - Isolamento de Namespaces" << endl;
    cout << "3. Experimento 3 - Throttling de CPU (via Control Group Manager)" << endl;
    cout << "4. Experimento 4 - Limitação de Memória (via Control Group Manager)" << endl;
    cout << "5. Experimento 5 - Limitação de I/O" << endl;
    cout << "0. Voltar" << endl;
    cout << "Escolha: ";
    cin >> choice;
    cin.clear();
    cin.ignore(10000, '\n');
    
    switch (choice) {
        case 1:
            cout << "\nExecutando Experimento 1..." << endl;
            cout << "Este experimento mede o overhead do Resource Profiler" << endl;
            cout << "Requer sudo e leva alguns minutos..." << endl;
            {
                int ret = system("sudo ./bin/experimento1_overhead_monitoring");
                if (ret != 0) {
                    cerr << "  O comando do experimento 1 retornou código: " << ret << "\n";
                    cerr << "     (se for falha por permissão, execute o programa como root)" << endl;
                }
            }
            break;
            
        case 2:
            cout << "\nExecutando Experimento 2..." << endl;
            cout << "Este experimento analisa namespaces do sistema" << endl;
            {
                int ret = system("./bin/experimento2_benchmark_namespaces");
                if (ret != 0) {
                    cerr << "  O comando do experimento 2 retornou código: " << ret << "\n";
                }
            }
            break;
            
        case 3:
            cout << "\nVeja 'Control Group Manager' no menu principal" << endl;
            break;
            
        case 4:
            cout << "\nVeja 'Control Group Manager' no menu principal" << endl;
            break;
            
        case 5:
            if (geteuid() == 0) {
                cout << "\nExecutando Experimento 5..." << endl;
                int ret = system("bash tests/experimento5_run.sh");
                if (ret != 0) {
                    cerr << "  O script experimento5_run.sh retornou código: " << ret << "\n";
                    cerr << "     Execute manualmente: sudo bash tests/experimento5_run.sh" << endl;
                }
            } else {
                cout << "\nExperimento 5 requer root. Execute com: sudo bash tests/experimento5_run.sh" << endl;
            }
            break;
            
        case 0:
            break;
            
        default:
            cout << "  Opção inválida!" << endl;
    }
}

void showMainMenu() {
    cout << "\n╔════════════════════════════════════════╗" << endl;
    cout << "║   SISTEMA DE MONITORAMENTO - RA3      ║" << endl;
    cout << "║   Recursos, Namespaces e CGroups      ║" << endl;
    cout << "╚════════════════════════════════════════╝" << endl;
    cout << "\n1. Resource Profiler (Componente 1)" << endl;
    cout << "2. Namespace Analyzer (Componente 2)" << endl;
    cout << "3. Control Group Manager (Componente 3)" << endl;
    cout << "4. Menu de Experimentos (1-5)" << endl;
    cout << "0. Sair" << endl;
    cout << "\nEscolha uma opção: ";
}

void signal_handler(int sig) {
    (void)sig;  // Silencia warning de parâmetro não usado
    monitoring_active = false;
    cout << "\n\n   Recebido sinal de interrupção. Parando monitoramento..." << endl;
}


// ================================
// MAIN
// ================================

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    cout << "\n╔════════════════════════════════════════╗" << endl;
    cout << "║    SISTEMA DE MONITORAMENTO - RA3     ║" << endl;
    cout << "║  Recursos, Namespaces e Control Groups║" << endl;
    cout << "║  Grupo 03 - Trabalho Avaliativo       ║" << endl;
    cout << "╚════════════════════════════════════════╝" << endl;
    cout << "\nComponente 1: Resource Profiler - ATIVO" << endl;
    cout << "Componente 2: Namespace Analyzer - ATIVO" << endl;
    cout << "Componente 3: Control Group Manager - ATIVO" << endl;
    cout << "\nTodas as APIs estão integradas e funcionais!" << endl;
    
    int choice;
    
    do {
        showMainMenu();
        cin >> choice;
        cin.clear();
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
                cout << "\nEncerrando sistema...\n" << endl;
                break;
                
            default:
                cout << "   Opção inválida!\n" << endl;
                break;
        }
        
        if (choice != 0 && choice >= 1 && choice <= 4) {
            cout << "\nPressione Enter para continuar...";
            cin.get();
        }
        
    } while (choice != 0);
    
    return 0;
}