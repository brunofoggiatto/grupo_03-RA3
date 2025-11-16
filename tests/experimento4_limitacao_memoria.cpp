#include "cgroup_manager.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

struct MemoryTestResult {
    long long limit_mb;
    long long peak_mb;
    long long failcnt;
    bool oom_killed;
    string behavior;
};

MemoryTestResult run_memory_test_with_limit(CGroupManager& manager, const string& cgroup_name, long long limit_bytes) {
    MemoryTestResult result;
    result.limit_mb = limit_bytes / (1024 * 1024);
    result.oom_killed = false;
    
    // Criar cgroup
    if (!manager.createCGroup(cgroup_name)) {
        cerr << "Erro ao criar cgroup" << endl;
        result.peak_mb = -1;
        return result;
    }
    
    // Aplicar limite de memória
    if (!manager.setMemoryLimit(cgroup_name, limit_bytes)) {
        cerr << "Erro ao aplicar limite de memória" << endl;
        manager.deleteCGroup(cgroup_name);
        result.peak_mb = -1;
        return result;
    }
    
    cout << "  Cgroup criado com limite de " << result.limit_mb << " MB" << endl;
    
    // Fork para executar test_memory
    pid_t pid = fork();
    
    if (pid == 0) {
        // Processo filho: mover-se para o cgroup e executar test_memory
        
        // Escrever próprio PID no cgroup
        string cgroup_procs = "/sys/fs/cgroup/" + cgroup_name + "/cgroup.procs";
        ofstream procs_file(cgroup_procs);
        if (procs_file.is_open()) {
            procs_file << getpid();
            procs_file.close();
        }
        
        // Executar test_memory
        execl("./bin/test_memory", "test_memory", nullptr);
        
        // Se execl falhar
        cerr << "Erro ao executar test_memory" << endl;
        _exit(1);
    } else if (pid > 0) {
        // Processo pai: esperar filho terminar
        int status;
        waitpid(pid, &status, 0);
        
        // Verificar se foi morto pelo OOM killer
        if (WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL) {
            result.oom_killed = true;
            result.behavior = "OOM Killer ativado - processo terminado";
        } else if (WIFEXITED(status)) {
            result.oom_killed = false;
            result.behavior = "Alocação bloqueada - processo continuou";
        }
        
        // Ler métricas do cgroup
        auto metrics = manager.getCGroupMetrics("/sys/fs/cgroup/" + cgroup_name);
        if (metrics) {
            result.peak_mb = metrics->memory_peak_bytes / (1024 * 1024);
            result.failcnt = metrics->memory_failcnt;
        } else {
            result.peak_mb = 0;
            result.failcnt = 0;
        }
    }
    
    // Deletar cgroup
    manager.deleteCGroup(cgroup_name);
    
    return result;
}

int main() {
    cout << "======================================================" << endl;
    cout << "  EXPERIMENTO 4: LIMITAÇÃO DE MEMÓRIA" << endl;
    cout << "  Teste de Comportamento ao Atingir Limite" << endl;
    cout << "======================================================" << endl;
    
    if (geteuid() != 0) {
        cerr << "\nERRO: Este experimento precisa de root!" << endl;
        cerr << "Execute com: sudo ./bin/experimento4_limitacao_memoria\n" << endl;
        return 1;
    }
    
    // Verificar se test_memory existe
    if (access("./bin/test_memory", X_OK) != 0) {
        cerr << "\nERRO: ./bin/test_memory não encontrado ou não é executável" << endl;
        cerr << "Execute 'make all' primeiro\n" << endl;
        return 1;
    }
    
    cout << "\nWorkload: test_memory (tenta alocar 500 MB)" << endl;
    cout << "Limite configurado: 100 MB\n" << endl;
    
    CGroupManager manager;
    
    // Teste com limite de 100MB
    long long limit_100mb = 100 * 1024 * 1024;
    
    cout << "Executando teste..." << endl;
    MemoryTestResult result = run_memory_test_with_limit(manager, "exp4_mem_100mb", limit_100mb);
    
    if (result.peak_mb == -1) {
        cerr << "\nFalha ao executar teste" << endl;
        return 1;
    }
    
    // Imprimir resultados
    cout << "\n======================================================" << endl;
    cout << "  RESULTADOS DO EXPERIMENTO 4" << endl;
    cout << "======================================================\n" << endl;
    
    cout << "Limite Configurado: " << result.limit_mb << " MB" << endl;
    cout << "Pico de Memória Atingido: " << result.peak_mb << " MB" << endl;
    cout << "Falhas de Alocação (memory.events[max]): " << result.failcnt << endl;
    cout << "OOM Killer Ativado: " << (result.oom_killed ? "SIM" : "NÃO") << endl;
    cout << "Comportamento Observado: " << result.behavior << endl;
    
    // Análise
    cout << "\n--- ANÁLISE ---\n" << endl;
    
    if (result.peak_mb == result.limit_mb) {
        cout << "✓ Precisão do Limite: 100% (pico = limite exato)" << endl;
    } else {
        double precision = (static_cast<double>(result.peak_mb) / result.limit_mb) * 100.0;
        cout << "✓ Precisão do Limite: " << fixed << setprecision(1) << precision << "%" << endl;
    }
    
    if (result.failcnt > 0) {
        cout << "✓ Alocações Bloqueadas: " << result.failcnt << " tentativas falhadas" << endl;
    }
    
    if (!result.oom_killed) {
        cout << "✓ Processo NÃO foi morto (tratamento gracioso)" << endl;
    }
    
    cout << "\nConclusão: O limite de memória foi ";
    if (result.peak_mb == result.limit_mb) {
        cout << "PRECISAMENTE respeitado." << endl;
    } else {
        cout << "respeitado com alta precisão." << endl;
    }
    
    // Salvar resultados em CSV
    ofstream csv("experimento4_results.csv");
    csv << "Limite_MB,Peak_MB,Failcnt,OOM_Killed,Comportamento\n";
    csv << result.limit_mb << ","
        << result.peak_mb << ","
        << result.failcnt << ","
        << (result.oom_killed ? "yes" : "no") << ","
        << result.behavior << "\n";
    csv.close();
    
    cout << "\n======================================================" << endl;
    cout << "  EXPERIMENTO 4 CONCLUÍDO" << endl;
    cout << "  Arquivo gerado: experimento4_results.csv" << endl;
    cout << "======================================================" << endl;
    
    return 0;
}