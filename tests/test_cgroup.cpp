#include "cgroup_manager.hpp"
#include <iostream>

int main() {
    std::cout << "Testando CGroup Manager (v2)..." << std::endl;
    
    CGroupManager manager;
    auto metrics = manager.getSystemMetrics();
    
    if (metrics) {
        metrics->print();
        std::cout << "\nPrimeiro teste completo!" << std::endl;
    } else {
        std::cout << "Falha ao ler métricas" << std::endl;
    }
    
    return 0;
}
