#include "cgroup_manager.hpp"
#include <iostream>
#include <unistd.h>

using namespace std;

void test_read_metrics() {
    cout << "\n========================================" << endl;
    cout << "TESTE 1: Leitura de Métricas do Sistema" << endl;
    cout << "========================================" << endl;
    
    CGroupManager manager;
    auto metrics = manager.getSystemMetrics();
    
    if (metrics) {
        metrics->print();
        cout << "\nLeitura de métricas OK (incluindo BlkIO)" << endl;
    } else {
        cout << "Falha ao ler métricas" << endl;
    }
}

void test_create_and_delete_cgroup() {
    cout << "\n========================================" << endl;
    cout << "TESTE 2: Criar e Deletar CGroup" << endl;
    cout << "========================================" << endl;
    
    if (geteuid() != 0) {
        cout << "\nPulando Teste 2: criar/deletar cgroup (requer root)" << endl;
        return;
    }

    CGroupManager manager;
    string test_group = "test_cgroup_ra3";
    
    // Criar
    cout << "\nCriando cgroup '" << test_group << "'..." << endl;
    if (manager.createCGroup(test_group)) {
        cout << "Criação OK" << endl;
    } else {
        cout << "Falha na criação" << endl;
        return;
    }
    
    // Tentar criar de novo (deve informar que já existe)
    cout << "\nTentando criar novamente (deve dizer que já existe)..." << endl;
    manager.createCGroup(test_group);
    
    // Deletar
    cout << "\nDeletando cgroup..." << endl;
    if (manager.deleteCGroup(test_group)) {
        cout << "Deleção OK" << endl;
    } else {
        cout << "Falha na deleção" << endl;
    }
}

void test_set_cpu_limit() {
    cout << "\n========================================" << endl;
    cout << "TESTE 3: Aplicar Limite de CPU" << endl;
    cout << "========================================" << endl;
    
    if (geteuid() != 0) {
        cout << "\nPulando Teste 3: aplicar limite de CPU (requer root)" << endl;
        return;
    }

    CGroupManager manager;
    string test_group = "test_cpu_limit";
    
    if (!manager.createCGroup(test_group)) {
        cout << "Falha ao criar cgroup" << endl;
        return;
    }
    
    cout << "\nAplicando limite de 0.5 cores..." << endl;
    if (manager.setCPULimit(test_group, 0.5)) {
        cout << "Limite de CPU aplicado com sucesso" << endl;
    } else {
        cout << "Falha ao aplicar limite" << endl;
    }
    
    manager.deleteCGroup(test_group);
}

void test_set_memory_limit() {
    cout << "\n========================================" << endl;
    cout << "TESTE 4: Aplicar Limite de Memória" << endl;
    cout << "========================================" << endl;
    
    if (geteuid() != 0) {
        cout << "\nPulando Teste 4: aplicar limite de memória (requer root)" << endl;
        return;
    }

    CGroupManager manager;
    string test_group = "test_memory_limit";
    
    if (!manager.createCGroup(test_group)) {
        cout << "Falha ao criar cgroup" << endl;
        return;
    }
    
    long long limit_100mb = 100 * 1024 * 1024; // 100MB
    cout << "\nAplicando limite de 100MB..." << endl;
    if (manager.setMemoryLimit(test_group, limit_100mb)) {
        cout << "Limite de memória aplicado com sucesso" << endl;
    } else {
        cout << "Falha ao aplicar limite" << endl;
    }
    
    manager.deleteCGroup(test_group);
}

void test_move_process() {
    cout << "\n========================================" << endl;
    cout << "TESTE 5: Mover Processo para CGroup" << endl;
    cout << "========================================" << endl;
    
    if (geteuid() != 0) {
        cout << "\nPulando Teste 5: mover processo para cgroup (requer root)" << endl;
        return;
    }

    CGroupManager manager;
    string test_group = "test_move_process";
    
    if (!manager.createCGroup(test_group)) {
        cout << "Falha ao criar cgroup" << endl;
        return;
    }
    
    pid_t my_pid = getpid();
    cout << "\nMovendo processo atual (PID " << my_pid << ") para o cgroup..." << endl;
    if (manager.moveProcessToCGroup(test_group, my_pid)) {
        cout << "Processo movido com sucesso" << endl;
        
        // Ler métricas do cgroup
        cout << "\nLendo métricas do cgroup após mover processo..." << endl;
        auto metrics = manager.getCGroupMetrics("/sys/fs/cgroup/" + test_group);
        if (metrics) {
            metrics->print();
        }
        
        // Voltar para o cgroup raiz
        cout << "\nMovendo processo de volta para o cgroup raiz..." << endl;
        manager.moveProcessToCGroup(".", my_pid);
        } else {
        cout << "Falha ao mover processo" << endl;
    }
    
    manager.deleteCGroup(test_group);
}

void test_generate_report() {
    cout << "\n========================================" << endl;
    cout << "TESTE 6: Gerar Relatório de Utilização" << endl;
    cout << "========================================" << endl;
    
    if (geteuid() != 0) {
        cout << "\nPulando Teste 6: gerar relatório (requer root)" << endl;
        return;
    }

    CGroupManager manager;
    string test_group = "test_report";
    
    if (!manager.createCGroup(test_group)) {
        cout << "Falha ao criar cgroup" << endl;
        return;
    }
    
    // Aplicar alguns limites
    manager.setCPULimit(test_group, 1.0);
    manager.setMemoryLimit(test_group, 200 * 1024 * 1024); // 200MB
    
    // Mover processo
    pid_t my_pid = getpid();
    manager.moveProcessToCGroup(test_group, my_pid);
    
    // Gerar relatório
    string report_file = "test_cgroup_report.txt";
    cout << "\nGerando relatório em '" << report_file << "'..." << endl;
    if (manager.generateUtilizationReport(test_group, report_file)) {
        cout << "Relatório gerado com sucesso" << endl;
        cout << "\nConteúdo do relatório:" << endl;
        cout << "-----------------------------------" << endl;
        int ret = system(("cat " + report_file).c_str());
        if (ret != 0) {
            cerr << "Aviso: Falha ao exibir conteúdo do arquivo" << endl;
        }
        cout << "-----------------------------------" << endl;
        } else {
        cout << "Falha ao gerar relatório" << endl;
    }
    
    // Voltar para o cgroup raiz
    manager.moveProcessToCGroup(".", my_pid);
    manager.deleteCGroup(test_group);
}

int main() {
    cout << "========================================" << endl;
    cout << "   TESTES DO CGROUP MANAGER - RA3" << endl;
    cout << "========================================" << endl;
    cout << "\nTeste completo de todas as funcionalidades" << endl;
    cout << "incluindo criação, limites e relatórios" << endl;
    
    // Verificar se está rodando como root
    if (geteuid() != 0) {
        cout << "\nAVISO: Alguns testes precisam de root!" << endl;
        cout << "Execute com: sudo ./bin/test_cgroup\n" << endl;
    }
    
    test_read_metrics();
    test_create_and_delete_cgroup();
    test_set_cpu_limit();
    test_set_memory_limit();
    test_move_process();
    test_generate_report();
    
    cout << "\n========================================" << endl;
    cout << "   TODOS OS TESTES CONCLUÍDOS" << endl;
    cout << "========================================" << endl;
    
    return 0;
}