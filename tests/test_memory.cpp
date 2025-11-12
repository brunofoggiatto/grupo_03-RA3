#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

int main() {
    std::cout << "Iniciando teste de memória (alocando 500 MB)..." << std::endl;

    std::vector<char*> blocos;
    for (int i = 0; i < 50; ++i) {            // 50 blocos de 10 MB
        char* b = new char[10 * 1024 * 1024];
        std::fill(b, b + 10 * 1024 * 1024, 0xAA);
        blocos.push_back(b);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "Manter memória alocada por 10 segundos..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));

    for (auto b : blocos) delete[] b;
    std::cout << "Memória liberada, teste concluído." << std::endl;
    return 0;
}
