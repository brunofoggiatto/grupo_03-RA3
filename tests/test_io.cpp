#include <fstream>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <chrono>

int main() {
    std::cout << "Iniciando teste de I/O..." << std::endl;
    const size_t size = 100 * 1024 * 1024; // 100 MB
    std::vector<char> data(1024, 'X');     // 1 KB de dados

    // Escrita
    auto start = std::chrono::steady_clock::now();
    std::ofstream out("teste_io.tmp", std::ios::binary);
    for (size_t i = 0; i < size / data.size(); ++i)
        out.write(data.data(), data.size());
    out.close();
    
    sync();

    // Leitura
    std::ifstream in("teste_io.tmp", std::ios::binary);
    char buffer[4096];
    while (in.read(buffer, sizeof(buffer))) { /* apenas lê */ }
    in.close();

    auto end = std::chrono::steady_clock::now();
    std::cout << "Tempo total: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " milissegundos.\n";
    std::cout << "Teste de I/O concluído." << std::endl;

    std::remove("teste_io.tmp");
    return 0;
}
