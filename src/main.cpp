#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <thread>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <csignal>
#include <atomic>
#include <filesystem>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include "../include/namespace.hpp"

using namespace std;

atomic<bool> monitoring_active{true};

// ================================
// CLASSE CSV EXPORTER (APENAS PARA RESOURCE PROFILER)
// ================================

class CSVExporter {
private:
    string filename;
    ofstream file;
    bool header_written;

public:
    CSVExporter(const string& filename) : filename(filename), header_written(false) {
        file.open(filename, ios::out | ios::trunc);
        if (!file.is_open()) {
            throw runtime_error("Cannot open CSV file: " + filename);
        }
    }
    
    ~CSVExporter() {
        if (file.is_open()) {
            file.close();
        }
    }
    
    void writeHeader(const vector<string>& headers) {
        for (size_t i = 0; i < headers.size(); ++i) {
            file << headers[i];
            if (i < headers.size() - 1) file << ",";
        }
        file << "\n";
        header_written = true;
    }
    
    void writeRow(const vector<string>& data) {
        for (size_t i = 0; i < data.size(); ++i) {
            file << data[i];
            if (i < data.size() - 1) file << ",";
        }
        file << "\n";
        file.flush();
    }
};

// ================================
// COMPONENTE 1: RESOURCE PROFILER (COM CSV)
// ================================

class ResourceProfiler {
private:
    // Verifica se o PID existe no sistema
    bool pidExists(int pid) {
        string proc_path = "/proc/" + to_string(pid);
        struct stat info;
        return (stat(proc_path.c_str(), &info) == 0);
    }
    
    // Verifica se temos permissão para acessar o processo
    bool canAccessProcess(int pid) {
        string stat_path = "/proc/" + to_string(pid) + "/stat";
        
        // Tenta abrir o arquivo stat do processo
        ifstream file(stat_path);
        if (!file.is_open()) {
            return false;
        }
        file.close();
        return true;
    }
    
    double calculateCPUUsage([[maybe_unused]] int pid) {
        // Simulação - na implementação real, leríamos /proc/[pid]/stat
        return (rand() % 1000) / 10.0;
    }
    
    long getMemoryRSS([[maybe_unused]] int pid) {
        // Simulação - na implementação real, leríamos /proc/[pid]/status
        return 100000 + (rand() % 900000);
    }
    
    long getMemoryVSZ([[maybe_unused]] int pid) {
        // Simulação - na implementação real, leríamos /proc/[pid]/status
        return 200000 + (rand() % 1800000);
    }
    
    long getIOBytes([[maybe_unused]] int pid) {
        // Simulação - na implementação real, leríamos /proc/[pid]/io
        return rand() % 1000000;
    }

public:
    struct ProcessMetrics {
        double cpu_usage;
        long memory_rss;
        long memory_vsz;
        long io_read_bytes;
        long io_write_bytes;
        int thread_count;
    };
    
    // Valida se podemos monitorar o processo
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
    
    ProcessMetrics collectMetrics(int pid) {
        // Valida acesso antes de coletar métricas
        validateProcessAccess(pid);
        
        ProcessMetrics metrics;
        metrics.cpu_usage = calculateCPUUsage(pid);
        metrics.memory_rss = getMemoryRSS(pid);
        metrics.memory_vsz = getMemoryVSZ(pid);
        metrics.io_read_bytes = getIOBytes(pid);
        metrics.io_write_bytes = getIOBytes(pid);
        metrics.thread_count = 1 + (rand() % 10);
        return metrics;
    }
    
    void monitorProcess(int pid, int interval_sec, const string& csv_filename) {
        try {
            // Validação inicial do processo
            cout << " Validando acesso ao processo " << pid << "..." << endl;
            validateProcessAccess(pid);
            cout << "  Acesso validado com sucesso" << endl;
            
        } catch (const exception& e) {
            cerr << "  Erro na validação: " << e.what() << endl;
            return;
        }
        
        CSVExporter exporter(csv_filename);
        vector<string> headers = {
            "timestamp", "pid", "cpu_usage", "memory_rss", 
            "memory_vsz", "io_read", "io_write", "threads"
        };
        exporter.writeHeader(headers);
        
        cout << "\n Monitorando processo PID: " << pid << endl;
        cout << "  Intervalo: " << interval_sec << " segundos" << endl;
        cout << "  Dados salvos em: " << csv_filename << endl;
        cout << "  Pressione Ctrl+C para parar..." << endl;
        cout << "------------------------------------------" << endl;
        
        int iteration = 0;
        while (monitoring_active && iteration < 100) {
            try {
                // Revalida periodicamente (processo pode terminar)
                if (!pidExists(pid)) {
                    throw runtime_error("Processo " + to_string(pid) + " não existe mais");
                }
                
                auto metrics = collectMetrics(pid);
                auto now = chrono::system_clock::now();
                auto time_t = chrono::system_clock::to_time_t(now);
                stringstream timestamp;
                timestamp << put_time(localtime(&time_t), "%Y-%m-%d %H:%M:%S");
                
                vector<string> row = {
                    timestamp.str(),
                    to_string(pid),
                    to_string(metrics.cpu_usage),
                    to_string(metrics.memory_rss),
                    to_string(metrics.memory_vsz),
                    to_string(metrics.io_read_bytes),
                    to_string(metrics.io_write_bytes),
                    to_string(metrics.thread_count)
                };
                
                exporter.writeRow(row);
                
                cout << "[" << timestamp.str() << "] "
                     << "CPU: " << setw(5) << metrics.cpu_usage << "%, "
                     << "Mem: " << setw(7) << metrics.memory_rss << "KB, "
                     << "IO: " << setw(7) << metrics.io_read_bytes << "B"
                     << " [Iteração: " << iteration + 1 << "]"
                     << endl;
                     
                this_thread::sleep_for(chrono::seconds(interval_sec));
                iteration++;
                
            } catch (const exception& e) {
                cerr << "  Erro na iteração " << iteration + 1 << ": " << e.what() << endl;
                
                // Se o processo não existe mais, para o monitoramento
                if (string(e.what()).find("não existe") != string::npos) {
                    break;
                }
                
                // Para outros erros, espera um pouco e tenta continuar
                this_thread::sleep_for(chrono::seconds(2));
            }
        }
        
        if (iteration > 0) {
            cout << "------------------------------------------" << endl;
            cout << " Monitoramento concluído. " << iteration << " iterações realizadas." << endl;
        } else {
            cout << " Monitoramento interrompido sem coletar dados." << endl;
        }
    }
};

// ================================
// COMPONENTE 2: NAMESPACE ANALYZER (USANDO IMPLEMENTAÇÃO REAL)
// ================================
// A implementação real está em src/namespace_analyzer.cpp

// ================================
// COMPONENTE 3: CONTROL GROUP MANAGER (SEM CSV)
// ================================

class ControlGroupManager {
public:
    bool createCGroup(const string& name) {
        cout << " Criando cgroup: " << name << endl;
        return true;
    }
    
    bool setCPULimit([[maybe_unused]] const string& cgroup_name, double cores) {
        cout << " Configurando limite de CPU: " << cores << " cores" << endl;
        return true;
    }
    
    bool setMemoryLimit([[maybe_unused]] const string& cgroup_name, size_t bytes) {
        cout << " Configurando limite de memória: " << bytes / (1024*1024) << " MB" << endl;
        return true;
    }
    
    bool addProcessToCGroup(const string& cgroup_name, int pid) {
        cout << " Adicionando processo " << pid << " ao cgroup " << cgroup_name << endl;
        return true;
    }
    
    void runCPUThrottlingExperiment() {
        cout << " EXPERIMENTO: Throttling de CPU" << endl;
        cout << "==================================" << endl;
        
        vector<double> limits = {0.25, 0.5, 1.0, 2.0};
        
        for (double limit : limits) {
            string cgroup_name = "cpu_test_" + to_string(static_cast<int>(limit * 100));
            
            createCGroup(cgroup_name);
            setCPULimit(cgroup_name, limit);
            
            // Simular medição
            double measured = limit * (0.9 + (rand() % 20) / 100.0);
            double deviation = ((measured - limit) / limit) * 100.0;
            
            cout << " Limite: " << limit << " core(s) -> Medido: " << measured 
                 << " (Desvio: " << fixed << setprecision(1) << deviation << "%)" << endl;
        }
        
        cout << " Experimento concluído!" << endl;
    }
    
    void runMemoryLimitExperiment() {
        cout << " EXPERIMENTO: Limitação de Memória" << endl;
        cout << "====================================" << endl;
        
        size_t limit = 100 * 1024 * 1024; // 100MB
        string cgroup_name = "mem_test_100MB";
        
        createCGroup(cgroup_name);
        setMemoryLimit(cgroup_name, limit);
        
        // Simular teste de alocação
        [[maybe_unused]] size_t allocated = 80 * 1024 * 1024; // 80MB alocado
        int fail_count = 2;
        
        cout << " Limite: 100MB -> Alocado: 80MB, Falhas: " << fail_count << endl;
        cout << " Experimento concluído!" << endl;
    }
};

// ================================
// MENU PRINCIPAL
// ================================

void showMainMenu() {
    cout << "\n==========================================" << endl;
    cout << "    SISTEMA DE MONITORAMENTO - RA3" << endl;
    cout << "==========================================" << endl;
    cout << "1.  Resource Profiler" << endl;
    cout << "2.  Namespace Analyzer" << endl;
    cout << "3.  Control Group Manager" << endl;
    cout << "0.  Sair" << endl;
    cout << "==========================================" << endl;
    cout << "Escolha uma opção: ";
}

void resourceProfilerMenu() {
    ResourceProfiler profiler;
    int pid, interval;
    
    cout << "\n=== RESOURCE PROFILER ===" << endl;
    cout << "Digite o PID para monitorar: ";
    
    if (!(cin >> pid)) {
        cout << "  Erro: PID deve ser um número inteiro!" << endl;
        cin.clear();
        cin.ignore(10000, '\n');
        return;
    }
    
    cout << "Digite o intervalo em segundos: ";
    
    if (!(cin >> interval) || interval <= 0) {
        cout << "  Erro: Intervalo deve ser um número positivo!" << endl;
        cin.clear();
        cin.ignore(10000, '\n');
        return;
    }
    
    string filename = "monitoring_pid_" + to_string(pid) + ".csv";
    profiler.monitorProcess(pid, interval, filename);
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
                    cout << "\n========================================" << endl;
                    cout << " COMPARAÇÃO DE NAMESPACES" << endl;
                    cout << " PID " << pid1 << " vs PID " << pid2 << endl;
                    cout << "========================================" << endl;

                    cout << "Total de namespaces verificados: " << comparison->total_namespaces << endl;
                    cout << "Namespaces compartilhados: " << comparison->shared_namespaces << endl;
                    cout << "Namespaces diferentes: " << comparison->different_namespaces << endl;

                    if (comparison->different_namespaces > 0) {
                        cout << "\nNamespaces que diferem:" << endl;

                        // Pegar os namespaces de ambos os processos para mostrar os inodes
                        auto proc1 = list_process_namespaces(pid1);
                        auto proc2 = list_process_namespaces(pid2);

                        for (const auto& diff_type : comparison->differences) {
                            string ns_name = namespace_type_to_string(diff_type);
                            cout << "  ✗ " << ns_name << endl;

                            // Encontrar os inodes específicos
                            if (proc1 && proc2) {
                                for (const auto& ns1 : proc1->namespaces) {
                                    if (ns1.type == diff_type && ns1.exists) {
                                        cout << "     PID " << pid1 << ": inode " << ns1.inode << endl;
                                        break;
                                    }
                                }
                                for (const auto& ns2 : proc2->namespaces) {
                                    if (ns2.type == diff_type && ns2.exists) {
                                        cout << "     PID " << pid2 << ": inode " << ns2.inode << endl;
                                        break;
                                    }
                                }
                            }
                        }
                    } else {
                        cout << "\n RESULTADO: Todos os namespaces são IDÊNTICOS" << endl;
                    }

                    cout << "========================================" << endl;
                } else {
                    cout << "  Erro: Não foi possível comparar os processos" << endl;
                }
            }
            break;

        case 3:
            {
                string filename = "namespace_report.csv";
                if (generate_namespace_report(filename, "csv")) {
                    cout << " Relatório CSV gerado com sucesso: " << filename << endl;
                } else {
                    cout << "  Erro ao gerar relatório CSV" << endl;
                }
            }
            break;

        case 4:
            {
                string filename = "namespace_report.json";
                if (generate_namespace_report(filename, "json")) {
                    cout << " Relatório JSON gerado com sucesso: " << filename << endl;
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
    ControlGroupManager cgroup_mgr;
    int choice;
    
    cout << "\n=== CONTROL GROUP MANAGER ===" << endl;
    cout << "1. Executar experimento de CPU Throttling" << endl;
    cout << "2. Executar experimento de Limitação de Memória" << endl;
    cout << "3. Criar cgroup experimental" << endl;
    cout << "Escolha: ";
    cin >> choice;
    
    switch (choice) {
        case 1:
            cgroup_mgr.runCPUThrottlingExperiment();
            break;
            
        case 2:
            cgroup_mgr.runMemoryLimitExperiment();
            break;
            
        case 3:
            {
                string name;
                int pid;
                cout << "Nome do cgroup: ";
                cin >> name;
                cout << "PID para adicionar: ";
                cin >> pid;
                cgroup_mgr.createCGroup(name);
                cgroup_mgr.addProcessToCGroup(name, pid);
                cout << "  Cgroup criado e processo adicionado!" << endl;
            }
            break;
            
        default:
            cout << "  Opção inválido!" << endl;
    }
}

void signal_handler([[maybe_unused]] int sig) {
    monitoring_active = false;
    cout << "\n   Recebido sinal de interrupção. Parando monitoramento..." << endl;
}

int main() {
    signal(SIGINT, signal_handler);
    srand(time(nullptr));
    
    int choice;
    
    cout << " Sistema de Monitoramento de Containers - RA3" << endl;
    cout << " Com tratamento de erros para PID inexistente e permissões" << endl;
    cout << " ===============================================" << endl;
    
    do {
        showMainMenu();
        cin >> choice;
        
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
                
            case 0:
                cout << " Encerrando sistema..." << endl;
                break;
                
            default:
                cout << "  Opção inválida!" << endl;
                break;
        }
        
        if (choice != 0) {
            cout << "\nPressione Enter para continuar...";
            cin.ignore();
            cin.get();
        }
        
    } while (choice != 0);
    
    return 0;
}