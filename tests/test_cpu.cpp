#include <cmath>
#include <chrono>
#include <thread>
#include <iostream>

int main() {
    std::cout << "Iniciando teste de CPU (10 segundos)..." << std::endl;
    auto start = std::chrono::steady_clock::now();

    // Loop pesado de cálculo
    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(10)) {
        double x = 0;
        for (int i = 0; i < 100000; ++i)
            x += std::sin(i) * std::cos(i);
    }

    std::cout << "Teste de CPU finalizado." << std::endl;
    return 0;
}
