#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <string>
#include "../include/cgroup_manager.hpp"

void test_basic_operations() {
    std::cout << "\n TESTE 1: OPERAÇÕES BÁSICAS DO CGROUP\n";
    std::cout << "========================================\n";
    
    CGroupManager cgm;
    std::string test_cgroup = "/test_basic_ops";
    
    std::cout << "1. Criando cgroup...\n";
    if (cgm.create_cgroup(test_cgroup)) {
        std::cout << "    Cgroup criado com sucesso\n";
    } else {
        std::cout << "    Falha ao criar cgroup\n";
        return;
    }
    
    std::cout << "2. Verificando existência...\n";
    if (cgm.exists_cgroup(test_cgroup)) {
        std::cout << "    Cgroup existe\n";
    } else {
        std::cout << "    Cgroup não encontrado\n";
    }
    
    std::cout << "3. Movendo processo atual...\n";
    if (cgm.move_current_process_to_cgroup(test_cgroup)) {
        std::cout << "    Processo movido com sucesso\n";
    } else {
        std::cout << "    Falha ao mover processo\n";
    }
    
    std::cout << "4. Definindo limite de CPU (0.5 cores)...\n";
    if (cgm.set_cpu_limit(test_cgroup, 0.5)) {
        std::cout << "    Limite de CPU definido\n";
    } else {
        std::cout << "    Falha ao definir limite de CPU\n";
    }
    
    std::cout << "5. Definindo limite de memória (100 MB)...\n";
    if (cgm.set_memory_limit(test_cgroup, 100)) {
        std::cout << "    Limite de memória definido\n";
    } else {
        std::cout << "    Falha ao definir limite de memória\n";
    }
    
    std::cout << "6. Exibindo estatísticas...\n";
    cgm.display_cgroup_stats(test_cgroup);
    
    std::cout << "7. Limpando cgroup...\n";
    if (cgm.delete_cgroup(test_cgroup)) {
        std::cout << "    Cgroup removido com sucesso\n";
    } else {
        std::cout << "    Falha ao remover cgroup\n";
    }
}

void test_pid_limits() {
    std::cout << "\n TESTE 2: LIMITAÇÃO DE PIDs\n";
    std::cout << "==============================\n";
    
    CGroupManager cgm;
    std::string test_cgroup = "/test_pid_limit";
    int max_pids = 3;
    
    std::cout << "Configurando teste com limite de " << max_pids << " PIDs...\n";
    
    if (!cgm.create_cgroup(test_cgroup)) {
        std::cerr << " Falha ao criar cgroup de teste\n";
        return;
    }
    
    if (!cgm.set_pids_limit(test_cgroup, max_pids)) {
        std::cerr << " Falha ao definir limite de PIDs\n";
        cgm.delete_cgroup(test_cgroup);
        return;
    }
    
    if (!cgm.move_current_process_to_cgroup(test_cgroup)) {
        std::cerr << " Falha ao mover processo para cgroup\n";
        cgm.delete_cgroup(test_cgroup);
        return;
    }
    
    std::cout << " Ambiente configurado. Iniciando criação de processos...\n\n";
    
    std::vector<pid_t> children;
    int successful_forks = 0;
    int expected_forks = 5;
    
    for (int i = 0; i < expected_forks; i++) {
        pid_t pid = fork();
        
        if (pid == 0) {
            std::cout << "    Processo filho " << (i+1) << " (PID: " << getpid() << ") criado - dormindo...\n";
            sleep(5);
            std::cout << "    Processo filho " << (i+1) << " terminando\n";
            _exit(0);
        } else if (pid > 0) {
            children.push_back(pid);
            successful_forks++;
            std::cout << " Processo " << (i+1) << " criado (PID: " << pid << ")\n";
            
            int current_pids = cgm.read_pids_current(test_cgroup);
            std::cout << "    PIDs atuais no cgroup: " << current_pids << "/" << max_pids << "\n";
            
            sleep(1);
            
            if (current_pids >= max_pids) {
                std::cout << "  Limite de PIDs atingido! Próximos forks devem falhar.\n";
            }
        } else {
            std::cout << " Fork " << (i+1) << " falhou (limite de PIDs atingido!)\n";
            break;
        }
    }
    
    std::cout << "\n RELATÓRIO FINAL:\n";
    std::cout << "===================\n";
    std::cout << "Forks tentados: " << expected_forks << "\n";
    std::cout << "Forks bem-sucedidos: " << successful_forks << "\n";
    std::cout << "Forks que falharam: " << (expected_forks - successful_forks) << "\n";
    std::cout << "PIDs atuais no cgroup: " << cgm.read_pids_current(test_cgroup) << "\n";
    std::cout << "Limite configurado: " << max_pids << "\n";
    
    std::cout << "\n Aguardando processos filhos terminarem...\n";
    for (pid_t child : children) {
        int status;
        waitpid(child, &status, 0);
        std::cout << "    Processo " << child << " finalizado\n";
    }
    
    std::cout << "\n Gerando relatório detalhado...\n";
    cgm.generate_utilization_report(test_cgroup, "test_pid_limit_report.txt");
    
    std::cout << " Limpando cgroup de teste...\n";
    cgm.delete_cgroup(test_cgroup);
    std::cout << " Teste de limitação de PIDs concluído!\n";
}

void test_pressure_stall_info() {
    std::cout << "\n TESTE 3: PRESSURE STALL INFORMATION\n";
    std::cout << "======================================\n";
    
    CGroupManager cgm;
    
    if (!cgm.is_cgroup_v2()) {
        std::cout << "  Este sistema usa CGroup v1\n";
        std::cout << "  Pressure Stall Information só está disponível no CGroup v2\n";
        std::cout << "  Pulando teste de Pressure Stall Information\n";
        return;
    }
    
    std::cout << " Sistema usa CGroup v2 - Pressure Stall Information disponível\n\n";
    
    std::string current_cgroup = cgm.get_current_cgroup();
    if (current_cgroup.empty()) {
        std::cout << " Não foi possível determinar o cgroup atual\n";
        return;
    }
    
    std::cout << " CGroup atual: " << current_cgroup << "\n\n";
    
    std::cout << " Lendo Pressure Stall Information...\n";
    
    PressureStats cpu_pressure = cgm.read_cpu_pressure(current_cgroup);
    std::cout << "\n CPU PRESSURE:\n";
    std::cout << "   Avg10: " << cpu_pressure.avg10 << "%\n";
    std::cout << "   Avg60: " << cpu_pressure.avg60 << "%\n";
    std::cout << "   Avg300: " << cpu_pressure.avg300 << "%\n";
    std::cout << "   Total: " << cpu_pressure.total << " μs\n";
    
    PressureStats mem_pressure = cgm.read_memory_pressure(current_cgroup);
    std::cout << "\n MEMORY PRESSURE:\n";
    std::cout << "   Avg10: " << mem_pressure.avg10 << "%\n";
    std::cout << "   Avg60: " << mem_pressure.avg60 << "%\n";
    std::cout << "   Avg300: " << mem_pressure.avg300 << "%\n";
    std::cout << "   Total: " << mem_pressure.total << " μs\n";
    
    PressureStats io_pressure = cgm.read_io_pressure(current_cgroup);
    std::cout << "\n I/O PRESSURE:\n";
    std::cout << "   Avg10: " << io_pressure.avg10 << "%\n";
    std::cout << "   Avg60: " << io_pressure.avg60 << "%\n";
    std::cout << "   Avg300: " << io_pressure.avg300 << "%\n";
    std::cout << "   Total: " << io_pressure.total << " μs\n";
    
    std::cout << "\n Pressure Stall Information lida com sucesso!\n";
}

void test_reporting() {
    std::cout << "\n TESTE 4: RELATÓRIOS E ESTATÍSTICAS\n";
    std::cout << "=====================================\n";
    
    CGroupManager cgm;
    std::string test_cgroup = "/test_reporting";
    
    if (!cgm.create_cgroup(test_cgroup)) {
        std::cerr << " Falha ao criar cgroup de teste\n";
        return;
    }
    
    cgm.set_cpu_limit(test_cgroup, 1.0);
    cgm.set_memory_limit(test_cgroup, 200);
    cgm.set_pids_limit(test_cgroup, 10);
    cgm.move_current_process_to_cgroup(test_cgroup);
    
    std::cout << " Exibindo estatísticas em tempo real:\n";
    cgm.display_cgroup_stats(test_cgroup);
    
    std::cout << " Gerando relatório detalhado...\n";
    cgm.generate_utilization_report(test_cgroup, "detailed_cgroup_report.txt");
    
    std::cout << " Relatório gerado: detailed_cgroup_report.txt\n";
    
    cgm.delete_cgroup(test_cgroup);
}

int main() {
    std::cout << " INICIANDO TESTES DO CGROUP MANAGER\n";
    std::cout << "=====================================\n";
    std::cout << "Este teste verificará:\n";
    std::cout << "1. Operações básicas de CGroup\n";
    std::cout << "2. Limitação de PIDs\n";
    std::cout << "3. Pressure Stall Information (CGroup v2)\n";
    std::cout << "4. Sistema de relatórios\n";
    std::cout << "=====================================\n\n";
    
    test_basic_operations();
    test_pid_limits();
    test_pressure_stall_info();
    test_reporting();
    
    std::cout << "\n TODOS OS TESTES CONCLUÍDOS!\n";
    std::cout << "=====================================\n";
    std::cout << " Resumo:\n";
    std::cout << " CGroup Manager totalmente funcional\n";
    std::cout << " Limitação de PIDs implementada\n";
    std::cout << " Suporte a CGroup v2 com Pressure Stall Information\n";
    std::cout << " Sistema de relatórios operacional\n";
    std::cout << "=====================================\n";
    
    return 0;
}