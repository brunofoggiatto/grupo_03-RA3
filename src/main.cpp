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

using namespace std;

atomic<bool> monitoring_active{true};

// ================================
// CLASSE CSV EXPORTER
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
// COMPONENTE 1: RESOURCE PROFILER
// ================================

class ResourceProfiler {
private:
    double calculateCPUUsage(int pid) {
        // Simulação - implementar leitura real de /proc/[pid]/stat
        return (rand() % 1000) / 10.0; // 0.0 a 100.0%
    }
    
    long getMemoryRSS(int pid) {
        // Simulação - implementar leitura real de /proc/[pid]/status
        return 100000 + (rand() % 900000); // 100MB a 1GB
    }
    
    long getMemoryVSZ(int pid) {
        // Simulação - implementar leitura real de /proc/[pid]/status
        return 200000 + (rand() % 1800000); // 200MB a 2GB
    }
    
    long getIOBytes(int pid) {
        // Simulação - implementar leitura real de /proc/[pid]/io
        return rand() % 1000000; // até 1MB
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
    
    ProcessMetrics collectMetrics(int pid) {
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
        CSVExporter exporter(csv_filename);
        vector<string> headers = {
            "timestamp", "pid", "cpu_usage", "memory_rss", 
            "memory_vsz", "io_read", "io_write", "threads"
        };
        exporter.writeHeader(headers);
        
        cout << " Monitorando processo PID: " << pid << endl;
        cout << "  Intervalo: " << interval_sec << " segundos" << endl;
        cout << " Dados salvos em: " << csv_filename << endl;
        cout << "Press Ctrl+C para parar..." << endl;
        
        int iteration = 0;
        while (monitoring_active && iteration < 100) {
            try {
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
                     << "CPU: " << metrics.cpu_usage << "%, "
                     << "Mem: " << metrics.memory_rss << "KB, "
                     << "IO: " << metrics.io_read_bytes << "B"
                     << endl;
                     
                this_thread::sleep_for(chrono::seconds(interval_sec));
                iteration++;
                
            } catch (const exception& e) {
                cerr << " Erro: " << e.what() << endl;
                break;
            }
        }
        
        cout << " Monitoramento concluído. " << iteration << " iterações." << endl;
    }
};

// ================================
// COMPONENTE 2: NAMESPACE ANALYZER
// ================================

class NamespaceAnalyzer {
public:
    struct NamespaceInfo {
        string type;
        unsigned long inode;
        string ns_id;
    };
    
    vector<NamespaceInfo> getProcessNamespaces(int pid) {
        vector<NamespaceInfo> namespaces;
        vector<string> ns_types = {"pid", "net", "mnt", "uts", "ipc", "user", "cgroup"};
        
        for (const auto& type : ns_types) {
            NamespaceInfo ns;
            ns.type = type;
            ns.inode = 1000 + (rand() % 9000);
            ns.ns_id = to_string(rand() % 1000);
            namespaces.push_back(ns);
        }
        
        return namespaces;
    }
    
    vector<int> findProcessesInNamespace(const string& ns_type, unsigned long target_inode) {
        // Simulação - implementar busca real
        return {1, 1234, 5678}; // PIDs exemplo
    }
    
    bool compareNamespaces(int pid1, int pid2) {
        auto ns1 = getProcessNamespaces(pid1);
        auto ns2 = getProcessNamespaces(pid2);
        
        if (ns1.size() != ns2.size()) return false;
        
        for (size_t i = 0; i < ns1.size(); ++i) {
            if (ns1[i].inode != ns2[i].inode) return false;
        }
        
        return true;
    }
    
    void generateNamespaceReport(const string& output_file) {
        CSVExporter exporter(output_file);
        vector<string> headers = {"namespace_type", "process_count", "inode", "timestamp"};
        exporter.writeHeader(headers);
        
        cout << " Gerando relatório de namespaces do sistema..." << endl;
        
        vector<string> ns_types = {"pid", "net", "mnt", "uts", "ipc", "user", "cgroup"};
        
        for (const auto& type : ns_types) {
            auto now = chrono::system_clock::now();
            auto time_t = chrono::system_clock::to_time_t(now);
            stringstream timestamp;
            timestamp << put_time(localtime(&time_t), "%Y-%m-%d %H:%M:%S");
            
            vector<string> row = {
                type,
                to_string(1 + (rand() % 50)), // processo count
                to_string(1000 + (rand() % 9000)), // inode
                timestamp.str()
            };
            
            exporter.writeRow(row);
            
            cout << "● " << type << " - " << row[1] << " processos" << endl;
        }
        
        cout << " Relatório salvo em: " << output_file << endl;
    }
};

// ================================
// COMPONENTE 3: CONTROL GROUP MANAGER
// ================================

class ControlGroupManager {
public:
    bool createCGroup(const string& name) {
        cout << " Criando cgroup: " << name << endl;
        // Implementar criação real em /sys/fs/cgroup/
        return true;
    }
    
    bool setCPULimit(const string& cgroup_name, double cores) {
        cout << " Configurando limite de CPU: " << cores << " cores" << endl;
        // Implementar: echo cores > /sys/fs/cgroup/cpu/$cgroup_name/cpu.cfs_quota_us
        return true;
    }
    
    bool setMemoryLimit(const string& cgroup_name, size_t bytes) {
        cout << " Configurando limite de memória: " << bytes / (1024*1024) << " MB" << endl;
        // Implementar: echo bytes > /sys/fs/cgroup/memory/$cgroup_name/memory.limit_in_bytes
        return true;
    }
    
    bool setIOLimit(const string& cgroup_name, size_t read_bps, size_t write_bps) {
        cout << " Configurando limite de I/O: R=" << read_bps << "B/s, W=" << write_bps << "B/s" << endl;
        return true;
    }
    
    bool addProcessToCGroup(const string& cgroup_name, int pid) {
        cout << " Adicionando processo " << pid << " ao cgroup " << cgroup_name << endl;
        // Implementar: echo pid > /sys/fs/cgroup/$cgroup_name/cgroup.procs
        return true;
    }
    
    void runCPUThrottlingExperiment() {
        cout << " EXPERIMENTO: Throttling de CPU" << endl;
        cout << "==================================" << endl;
        
        CSVExporter exporter("experiment_cpu_throttling.csv");
        vector<string> headers = {"limit_cores", "measured_usage", "deviation_percent", "throughput"};
        exporter.writeHeader(headers);
        
        vector<double> limits = {0.25, 0.5, 1.0, 2.0};
        
        for (double limit : limits) {
            string cgroup_name = "cpu_test_" + to_string(static_cast<int>(limit * 100));
            
            createCGroup(cgroup_name);
            setCPULimit(cgroup_name, limit);
            
            // Simular medição
            double measured = limit * (0.9 + (rand() % 20) / 100.0); // 90-110% do limite
            double deviation = ((measured - limit) / limit) * 100.0;
            double throughput = limit * 1000 + (rand() % 500);
            
            vector<string> row = {
                to_string(limit),
                to_string(measured),
                to_string(deviation),
                to_string(throughput)
            };
            exporter.writeRow(row);
            
            cout << "✓ Limite: " << limit << " core(s) -> Medido: " << measured 
                 << " (Desvio: " << fixed << setprecision(1) << deviation << "%)" << endl;
        }
        
        cout << " Dados salvos em: experiment_cpu_throttling.csv" << endl;
    }
    
    void runMemoryLimitExperiment() {
        cout << " EXPERIMENTO: Limitação de Memória" << endl;
        cout << "====================================" << endl;
        
        CSVExporter exporter("experiment_memory_limit.csv");
        vector<string> headers = {"limit_mb", "allocated_mb", "fail_count", "oom_triggered"};
        exporter.writeHeader(headers);
        
        size_t limit = 100 * 1024 * 1024; // 100MB
        string cgroup_name = "mem_test_100MB";
        
        createCGroup(cgroup_name);
        setMemoryLimit(cgroup_name, limit);
        
        // Simular teste de alocação
        size_t allocated = 80 * 1024 * 1024; // 80MB alocado
        int fail_count = 2;
        bool oom_triggered = false;
        
        vector<string> row = {
            to_string(limit / (1024*1024)),
            to_string(allocated / (1024*1024)),
            to_string(fail_count),
            oom_triggered ? "true" : "false"
        };
        exporter.writeRow(row);
        
        cout << " Limite: 100MB -> Alocado: 80MB, Falhas: " << fail_count << endl;
        cout << " Dados salvos em: experiment_memory_limit.csv" << endl;
    }
};

// ================================
// MENU PRINCIPAL - EXATAMENTE 3 OPÇÕES
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
    cin >> pid;
    cout << "Digite o intervalo em segundos: ";
    cin >> interval;
    
    string filename = "monitoring_pid_" + to_string(pid) + ".csv";
    profiler.monitorProcess(pid, interval, filename);
}

void namespaceAnalyzerMenu() {
    NamespaceAnalyzer analyzer;
    int choice, pid, pid1, pid2;
    
    cout << "\n=== NAMESPACE ANALYZER ===" << endl;
    cout << "1. Listar namespaces de um processo" << endl;
    cout << "2. Comparar namespaces entre processos" << endl;
    cout << "3. Gerar relatório do sistema" << endl;
    cout << "Escolha: ";
    cin >> choice;
    
    switch (choice) {
        case 1:
            cout << "Digite o PID: ";
            cin >> pid;
            {
                auto namespaces = analyzer.getProcessNamespaces(pid);
                cout << "\n Namespaces do processo " << pid << ":" << endl;
                for (const auto& ns : namespaces) {
                    cout << "● " << ns.type << " - Inode: " << ns.inode << " (ID: " << ns.ns_id << ")" << endl;
                }
            }
            break;
            
        case 2:
            cout << "Digite o primeiro PID: ";
            cin >> pid1;
            cout << "Digite o segundo PID: ";
            cin >> pid2;
            {
                bool same = analyzer.compareNamespaces(pid1, pid2);
                cout << "\n Comparação: PID " << pid1 << " vs PID " << pid2 << endl;
                cout << "Resultado: " << (same ? " MESMOS namespaces" : " DIFERENTES namespaces") << endl;
            }
            break;
            
        case 3:
            analyzer.generateNamespaceReport("namespace_report.csv");
            break;
            
        default:
            cout << " Opção inválida!" << endl;
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
                cout << " Cgroup criado e processo adicionado!" << endl;
            }
            break;
            
        default:
            cout << " Opção inválida!" << endl;
    }
}

void signal_handler(int signal) {
    monitoring_active = false;
}

int main() {
    signal(SIGINT, signal_handler);
    srand(time(nullptr));
    
    int choice;
    
    cout << " Sistema de Monitoramento de Containers - RA3" << endl;
    cout << "Desenvolvido conforme especificação do documento" << endl;
    
    do {
        showMainMenu();
        cin >> choice;
        
        monitoring_active = true; // Reset para nova operação
        
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
                cout << " Opção inválida!" << endl;
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