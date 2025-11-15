/*
 * -----------------------------------------------------------------------------
 * ARQUIVO: test_memory.cpp
 *
 * PROPÓSITO:
 * Este arquivo é um programa de workload (carga de trabalho) "Memory-intensive".
 * É usado no Experimento 4 para testar o comportamento do sistema ao
 * atingir um limite de memória imposto por um cgroup.
 *
 * RESPONSABILIDADE:
 * Aluno 2 (conforme divisão de tarefas).
 *
 * LÓGICA DE FUNCIONAMENTO:
 * 1. Aloca lentamente 500 MB de memória RAM em blocos de 10 MB.
 * 2. Pausa por 200ms entre cada alocação para permitir que o monitoramento
 * (e o OOM Killer do cgroup) reaja.
 * 3. Preenche cada bloco para garantir que a memória seja fisicamente
 * alocada (RAM real), não apenas virtual.
 * 4. Mantém a memória alocada por 10 segundos.
 * 5. Libera a memória e termina.
 *
 * COMO COMPILAR (Exemplo):
 * g++ -std=c++23 -Wall -Wextra -O2 -o test_memory test_memory.cpp
 * -----------------------------------------------------------------------------
 */

#include <iostream>   // Para std::cout (impressão no console)
#include <vector>     // Para std::vector (armazenar os ponteiros dos blocos)
#include <thread>     // Para std::this_thread::sleep_for (pausar)
#include <chrono>     // Para std::chrono::milliseconds

int main() {
    std::cout << "Iniciando teste de memória (alocando 500 MB)..." << std::endl;

    // Vetor para guardar os ponteiros de cada bloco de memória alocado,
    // para que possamos liberá-los (delete[]) no final.
    std::vector<char*> blocos;

    /*
     * -----------------------------------------------------------------------------
     * LOOP DE ALOCAÇÃO INCREMENTAL
     *
     * Este loop simula um "vazamento" ou um programa que consome
     * memória aos poucos, que é o cenário de teste do Experimento 4.
     * -----------------------------------------------------------------------------
     */
    for (int i = 0; i < 50; ++i) { // 50 blocos = 50 * 10MB = 500MB
        // 1. Aloca 10MB de memória no heap
        char* b = new char[10 * 1024 * 1024]; // 10 Megabytes

        /*
         * 2. "Toca" na memória (PONTO CRÍTICO)
         *
         * Apenas 'new' aloca memória *virtual*. O Kernel Linux só aloca
         * memória *física* (RAM) quando ela é usada pela primeira vez.
         *
         * O 'std::fill' força o programa a escrever em todo o bloco,
         * garantindo que o Kernel realmente nos dê a RAM. É isso que
         * faz o contador do cgroup (memory.usage_in_bytes) subir.
         */
        std::fill(b, b + (10 * 1024 * 1024), 0xAA); // Preenche com o byte 0xAA

        // Guarda o ponteiro para liberar depois
        blocos.push_back(b);

        // 3. Pausa por 200ms
        // Isso dá tempo ao monitor e ao cgroup para "perceberem"
        // a alocação. Se alocássemos tudo de uma vez (em 1ms),
        // o OOM Killer poderia não ter tempo de agir.
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "Memória máxima (500MB) alocada." << std::endl;
    std::cout << "Mantendo memória alocada por 10 segundos..." << std::endl;

    // Pausa por 10 segundos com toda a memória alocada.
    // É durante este tempo que o professor (ou o monitor)
    // pode observar o estado "estável" de 500MB de uso.
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // --- LIMPEZA ---
    // Libera cada bloco de 10MB que foi alocado
    for (auto b : blocos) {
        delete[] b;
    }

    std::cout << "Memória liberada, teste concluído." << std::endl;
    return 0;
}