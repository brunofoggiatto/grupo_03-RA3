#include "cgroup_manager.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <unistd.h>
#include <sys/wait.h>
#include <iomanip>

using namespace std;
using namespace chrono;

struct CPUTestResult {
    double limit_cores;
    double real_time_seconds;
    double user_time_seconds;
    double deviation_percent;
};

// Executa test_cpu dentro de um cgroup com limite
CPUTestResult run_cpu_test_with_limit(CGroupManager& manager, const string& cgroup_name, double limit_cores) {
    CPUTestResult result;
    result.limit_cores = limit_cores;
    
    // Criar cgroup
    if (!manager.createCGroup(cgroup_name)) {
        cerr << "Erro ao criar cgroup" << endl;
        result.real_time_seconds = -1;
        return result;
    }
    
    // Aplicar limite de CPU
    if (limit_cores > 0) {
        if (!manager.setCPULimit(cgroup_name, limit_cores)) {
            cerr << "Erro ao aplicar limite de CPU" << endl;
            manager.deleteCGroup(cgroup_name);
            result.real_time_seconds = -1;
            return result;
        }
    }
    
    // Fork para executar test_cpu
    auto start = high_resolution_clock::now();
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // Processo filho: mover-se para o cgroup e executar test_cpu
        
        // Escrever próprio PID no cgroup
        string cgroup_procs = "/sys/fs/cgroup/" + cgroup_name + "/cgroup.procs";
        ofstream procs_file(cgroup_procs);
        if (procs_file.is_open()) {
            procs_file << getpid();
            procs_file.close();
        }
        
        // Executar test_cpu
        execl("./bin/test_cpu", "test_cpu", nullptr);
        
        // Se execl falhar
        cerr << "Erro ao executar test_cpu" << endl;
        _exit(1);
    } else if (pid > 0) {
        // Processo pai: esperar filho terminar
        int status;
        waitpid(pid, &status, 0);
        
        auto end = high_resolution_clock::now();
        result.real_time_seconds = duration<double>(end - start).count();
        
        // Ler CPU usage do cgroup
        auto metrics = manager.getCGroupMetrics("/sys/fs/cgroup/" + cgroup_name);
        if (metrics) {
            result.user_time_seconds = metrics->cpu_usage_ns / 1e9;
        } else {
            result.user_time_seconds = 0;
        }
        
        // Calcular desvio (real time vs esperado)
        if (limit_cores > 0) {
            double expected_time = 10.0; // test_cpu roda por 10 segundos
            result.deviation_percent = ((result.real_time_seconds - expected_time) / expected_time) * 100.0;
        } else {
            result.deviation_percent = 0.0;
        }
    }
    
    // Deletar cgroup
    manager.deleteCGroup(cgroup_name);
    
    return result;
}

void print_results(const vector<CPUTestResult>& results) {
    cout << "\n======================================================" << endl;
    cout << "  EXPERIMENTO 3 - THROTTLING DE CPU" << endl;
    cout << "  Usando CGroup Manager v2" << endl;
    cout << "======================================================\n" << endl;
    
    cout << left << setw(20) << "Limite Configurado"
         << right << setw(15) << "Real (s)"
         << setw(15) << "User (s)"
         << setw(18) << "Desvio (%)" << endl;
    
    cout << string(68, '-') << endl;
    
    for (const auto& r : results) {
        string limit_str;
        if (r.limit_cores == 0) {
            limit_str = "Sem Limite";
        } else {
            limit_str = to_string(r.limit_cores) + " cores";
        }
        
        cout << left << setw(20) << limit_str
             << right << setw(15) << fixed << setprecision(3) << r.real_time_seconds
             << setw(15) << fixed << setprecision(3) << r.user_time_seconds
             << setw(18) << fixed << setprecision(1) << r.deviation_percent << "%" << endl;
    }
    
    cout << string(68, '-') << endl;
}

int main() {
    cout << "======================================================" << endl;
    cout << "  EXPERIMENTO 3: THROTTLING DE CPU" << endl;
    cout << "  Avaliação de Precisão de Limitação via CGroups v2" << endl;
    cout << "======================================================" << endl;
    
    if (geteuid() != 0) {
        cerr << "\nERRO: Este experimento precisa de root!" << endl;
        cerr << "Execute com: sudo ./bin/experimento3_throttling_cpu\n" << endl;
        return 1;
    }
    
    // Verificar se test_cpu existe
    if (access("./bin/test_cpu", X_OK) != 0) {
        cerr << "\nERRO: ./bin/test_cpu não encontrado ou não é executável" << endl;
        cerr << "Execute 'make all' primeiro\n" << endl;
        return 1;
    }
    
    cout << "\nWorkload: test_cpu (10 segundos de cálculos intensivos)" << endl;
    cout << "Limites testados: Sem limite, 0.25, 0.5, 1.0 cores\n" << endl;
    
    CGroupManager manager;
    vector<CPUTestResult> results;
    
    // TESTE 1: Baseline (sem limite)
    cout << "Executando baseline (sem limite de CPU)... " << flush;
    results.push_back(run_cpu_test_with_limit(manager, "exp3_baseline", 0));
    cout << "OK (" << fixed << setprecision(2) << results.back().real_time_seconds << "s)" << endl;
    
    // TESTE 2: Limite de 0.25 cores
    cout << "Executando com limite de 0.25 cores... " << flush;
    results.push_back(run_cpu_test_with_limit(manager, "exp3_025cores", 0.25));
    cout << "OK (" << fixed << setprecision(2) << results.back().real_time_seconds << "s)" << endl;
    
    // TESTE 3: Limite de 0.5 cores
    cout << "Executando com limite de 0.5 cores... " << flush;
    results.push_back(run_cpu_test_with_limit(manager, "exp3_050cores", 0.5));
    cout << "OK (" << fixed << setprecision(2) << results.back().real_time_seconds << "s)" << endl;
    
    // TESTE 4: Limite de 1.0 cores
    cout << "Executando com limite de 1.0 cores... " << flush;
    results.push_back(run_cpu_test_with_limit(manager, "exp3_100cores", 1.0));
    cout << "OK (" << fixed << setprecision(2) << results.back().real_time_seconds << "s)" << endl;
    
    print_results(results);
    
    // Salvar resultados em CSV
    ofstream csv("experimento3_results.csv");
    csv << "Limite_Cores,Real_Time_s,User_Time_s,Desvio_Percent\n";
    for (const auto& r : results) {
        csv << r.limit_cores << ","
            << r.real_time_seconds << ","
            << r.user_time_seconds << ","
            << r.deviation_percent << "\n";
    }
    csv.close();
    
    cout << "\n======================================================" << endl;
    cout << "  EXPERIMENTO 3 CONCLUÍDO" << endl;
    cout << "  Arquivo gerado: experimento3_results.csv" << endl;
    cout << "======================================================" << endl;
    
    return 0;
}